//
//  IOSDCard.cpp
//  RealtekCardReader
//
//  Created by FireWolf on 6/3/21.
//

#include "IOSDCard.hpp"
#include "IOSDHostDriver.hpp"
#include "IOSDHostDriverUserConfigs.hpp"

//
// MARK: - Meta Class Definitions
//

OSDefineMetaClassAndStructors(IOSDCard, IOService);

///
/// Initialize the card with the given OCR register value
///
/// @param driver The host driver
/// @param ocr The current operating condition register value
/// @return `true` on success, `false` otherwise.
/// @note Port: This function replaces `mmc_sd_init_card()` defined in `sd.c`.
///
bool IOSDCard::init(IOSDHostDriver* driver, UInt32 ocr)
{
    if (!super::init())
    {
        return false;
    }
    
    // ===============================
    // | BEGIN PORTED mmc_sd_get_cid |
    // ===============================
    // The initialization routine may change the OCR value
    UInt32 pocr = ocr;
    
    this->driver = driver;
    
    pinfo("Initializing the SD card with OCR = 0x%08x.", ocr);
    
    // Tell the card to go to the idle state
    if (this->driver->CMD0() != kIOReturnSuccess)
    {
        perr("Failed to tell the card to go back to the idle state.");
        
        return false;
    }
    
    pinfo("The card is now in the idle state.");
    
    // Check whether the card supports SD 2.0
    if (this->driver->CMD8(ocr) == kIOReturnSuccess)
    {
        pinfo("Found a SD 2.0 compliant card.");
        
        ocr |= OCR::kCardCapacityStatus;
    }
    
    // Check whether the host supports UHS-I mode
    // If so, we assume that the card also supports UHS-I mode
    // and request the card to switch to 1.8V signal voltage
    // Later the card will return a OCR value indicating whether it supports UHS-I mode
    if (this->driver->hostSupportsUltraHighSpeedMode())
    {
        pinfo("The host supports UHS-I mode and will try to request the card to switch to 1.8V signal voltage.");
        
        ocr |= OCR::kRequest1d8V;
    }
    
    // Check whether users request to initialize the card at 3.3V
    if (UserConfigs::Card::InitAt3v3)
    {
        pinfo("User requests to initialize the card at 3.3V. Will not request the card to switch to 1.8V.");
        
        ocr &= (~OCR::kRequest1d8V);
    }
    
    // Check whether the host can supply more than 150mA at the current voltage level
    // If so, we must follow the specification to set the XPC bit
    if (this->driver->getHostMaxCurrent() > 150)
    {
        pinfo("The host can supply more than 150mA.");
        
        ocr |= OCR::kSDXCPowerControl;
    }
    
    // Send the new OCR value
    UInt32 rocr;
    
    pinfo("Sending the new OCR value 0x%08x...", ocr);
    
    if (this->driver->ACMD41(ocr, rocr) != kIOReturnSuccess)
    {
        perr("Failed to send the new OCR value.");
        
        return false;
    }
    
    pinfo("OCR value returned from the card is 0x%08x.", rocr);
    
    // Check whether the card accepts the 1.8V signal voltage
    if (BitOptions(rocr).contains(OCR::kCardCapacityStatus | OCR::kAccepted1d8V))
    {
        pinfo("The card has accepted the 1.8V signal voltage.");
        
        // TODO: Retry on failure not implemented; Just fails?
        // Tell the card and the host to switch to 1.8V
        if (!this->enableUltraHighSpeedSignalVoltage(pocr))
        {
            perr("Failed to switch the host and the card to 1.8V.");
            
            return false;
        }
        
        pinfo("Both the host and the card has switched to the 1.8V signal voltage.");
    }
    
    // Fetch and parse the card identification data
    pinfo("Fetching the card identification data...");
    
    if (this->driver->CMD2(this->cid) != kIOReturnSuccess)
    {
        perr("Failed to fetch the card identification data.");
        
        return false;
    }
    
    pinfo("The card identification data has been fetched and decoded.");
    
    // ===============================
    // |  END  PORTED mmc_sd_get_cid |
    // ===============================
    // =================================
    // | BEGIN PORTED mmc_sd_init_card |
    // =================================
    
    // Ask the card to publish its relative address
    pinfo("Requesting the card relative address...");
    
    if (this->driver->CMD3(this->rca) != kIOReturnSuccess)
    {
        perr("Failed to fetch the card relative address.");
        
        return false;
    }
    
    pinfo("The card relative address is 0x%08x.", this->rca);
    
    // Fetch the card specific data
    pinfo("Fetching the card specific data...");
    
    if (this->driver->CMD9(this->rca, this->csd) != kIOReturnSuccess)
    {
        perr("Failed to fetch the card specific data.");
        
        return false;
    }
    
    pinfo("The card specific data has been fetched and decoded.");
    
    // Select the card: Will enter into the transfer state
    pinfo("Asking the card to enter into the transfer state...");
    
    if (this->driver->CMD7(this->rca) != kIOReturnSuccess)
    {
        perr("Failed to select the card.");
        
        return false;
    }
    
    pinfo("The card is now in the transfer state.");
    
    // ==================================
    // | BEGIN PORTED mmc_sd_setup_card |
    // ==================================
    
    // Fetch and parse the SD configuration data from the card
    pinfo("Fetching the card configuration data...");
    
    if (this->driver->ACMD51(this->rca, this->scr) != kIOReturnSuccess)
    {
        perr("Failed to fetch the SD configuration data.");
        
        return false;
    }
    
    pinfo("The card configuration data has been fetched and decoded.");
    
    // Fetch the SD status register value from the card
    pinfo("Fetching the SD status register value...");
    
    if (this->driver->ACMD13(this->rca, this->ssr) != kIOReturnSuccess)
    {
        perr("Failed to fetch the SD status data.");
        
        return false;
    }
    
    pinfo("The SD status register value has been fetched.");
    
    // TODO: INIT ERASE??? How does macOS handle erase
    
    // Fetch the switch information from the card
    pinfo("Fetching the switch capabilities from the card...");
    
    if (!this->readSwitchCapabilities())
    {
        perr("Failed to fetch the switch information from the card.");
        
        return false;
    }
    
    pinfo("The switch capabilities have been fetched.");
    
    // The Linux driver checks whether the card is already running under 1.8V.
    // Our driver detaches and powers off the card when the machine goes to sleep.
    // When the machines wakes up, the driver powers on the card and initializes it.
    // So the card is always power cycled, so it should never be running under 1.8V at this moment.
    // Please submit an issue if this is not true when you are testing this driver.
    BitOptions currentBusMode = this->switchCaps.sd3BusMode;
    
    if (currentBusMode.containsOneOf(SwitchCaps::BusMode::kModeUHSSDR50, SwitchCaps::BusMode::kModeUHSSDR104, SwitchCaps::BusMode::kModeUHSDDR50) &&
        this->driver->getHostDevice()->getHostBusConfig().signalVoltage != IOSDBusConfig::SignalVoltage::k1d8V)
    {
        pinfo("The card is already operating at 1.8V but the host does not.");
        
        pinfo("Abort the initialization. Should never happen. Not implemented.");
        
        return false;
    }
    
    // TODO: Consider fallback mechanism?

    // Guard: Check whether the card supports the high speed mode
    if (this->scr.spec < SCR::Spec::kVersion1d1x)
    {
        pinfo("The card does not support SD 1.10+. High Speed mode is not supported.");
        
        return this->initDefaultSpeedMode();
    }
    
    // Guard: Check whether the card supports switch functions
    if (!BitOptions(this->csd.cardCommandClasses).contains(CSD::CommandClass::kSwitch))
    {
        pinfo("The card does not support switch functions. High Speed mode is not supported.");
        
        return this->initDefaultSpeedMode();
    }
    
    // Guard: Check whether the host supports the high speed mode
    if (!this->driver->hostSupportsHighSpeedMode())
    {
        pinfo("The host does not support the high speed mode.");
        
        return this->initDefaultSpeedMode();
    }
    
    // Guard: Check the card switch functionality
    if (this->switchCaps.maxClockFrequencies.highSpeedMode == 0)
    {
        pinfo("The maximum clock frequency value for the high speed mode is zero.");
        
        return this->initDefaultSpeedMode();
    }
    
    // Guard: Check whether the user requests to initialize the card at the default speed mode
    if (UserConfigs::Card::InitAtDefaultSpeed)
    {
        pinfo("User requests to initialize the card at the default speed mode.");
        
        return this->initDefaultSpeedMode();
    }
    
    // Guard: Check whether the card supports the UHS-I mode
    if (BitOptions(rocr).contains(OCR::kAccepted1d8V) && this->driver->hostSupportsUltraHighSpeedMode())
    {
        pinfo("Both the host and the card support the ultra high speed mode.");

        // Guard: Check whether the user requests to initialize the card at the high speed mode
        if (!UserConfigs::Card::InitAtHighSpeed)
        {
            return this->initUltraHighSpeedMode();
        }
        else
        {
            pinfo("User requests to initialize the card at the high speed mode.");
        }
    }
    
    // Guard: Tell the card to switch to the high speed mode
    pinfo("Asking the card to switch to the high speed mode...");
    
    UInt8 status[64] = {};
    
    if (this->driver->CMD6(1, 0, SwitchCaps::BusSpeed::kSpeedHighSpeed, status) != kIOReturnSuccess)
    {
        perr("Failed to issue the CMD6 while the card supports switch functions.");
        
        return false;
    }
    
    // Guard: Check the card status
    if ((status[16] & 0xF) == SwitchCaps::BusSpeed::kSpeedHighSpeed)
    {
        pinfo("The card has switched to the high speed mode.");
        
        return this->initHighSpeedMode();;
    }
    else
    {
        perr("The card fails to switch to the high speed mode. Will use the default speed mode.");
        
        return this->initDefaultSpeedMode();
    }
}

///
/// [Helper] Initialize the card with Default Speed Mode enabled
///
/// @return `true` on success, `false` otherwise.
/// @note Port: This function replaces the default speed portion of `mmc_sd_init_card()` defined in `sd.c`.
///
bool IOSDCard::initDefaultSpeedMode()
{
    // Finish the card initialization sequence with the default speed enabled
    pinfo("Initializing the card at the default speed mode...");
    
    // Guard: Set the bus speed
    UInt32 clock = min(UINT32_MAX, this->csd.maxDataTransferRate);
    
    pinfo("Setting the bus clock to %u Hz.", clock);
    
    if (this->driver->setBusClock(clock) != kIOReturnSuccess)
    {
        perr("Failed to set the bus clock to %u Hz.", clock);
        
        return false;
    }
    
    // Switch to wider bus if possible
    // Guard: Check whether the card supports the 4-bit bus
    if (!BitOptions(this->scr.busWidths).contains(SCR::BusWidth::k4Bit))
    {
        pinfo("The card does not support the 4-bit bus.");
        
        return true;
    }
    
    // Guard: Check whether the host supports the 4-bit bus
    if (!this->driver->hostSupports4BitBusWidth())
    {
        pinfo("The host does not support the 4-bit bus.");
        
        return true;
    }
    
    // Enable the 4-bit wide bus
    if (!this->enable4BitWideBus())
    {
        perr("Failed to enable the 4-bit wide bus.");
        
        return false;
    }
    else
    {
        pinfo("The 4-bit bus has been enabled.")
    }
    
    pinfo("The card has been initialized with the default speed mode enabled.");
    
    return true;
}

///
/// [Helper] Initialize the card with High Speed Mode enabled
///
/// @return `true` on success, `false` otherwise.
/// @note Port: This function replaces the high speed portion of `mmc_sd_init_card()` defined in `sd.c`.
///
bool IOSDCard::initHighSpeedMode()
{
    // Finish the card initialization sequence with the high speed enabled
    pinfo("Initializing the card at the high speed mode...");
    
    // Guard: Set the bus timing
    pinfo("Setting the bus timing to the high speed mode...");
    
    if (this->driver->setBusTiming(IOSDBusConfig::BusTiming::kSDHighSpeed) != kIOReturnSuccess)
    {
        perr("Failed to set the bus timing to High Speed.");
        
        return false;
    }
    
    pinfo("The bus timing has been set to the high speed mode.");
    
    // Guard: Set the bus speed
    UInt32 clock = this->switchCaps.maxClockFrequencies.highSpeedMode;
    
    pinfo("Setting the bus clock to %u Hz.", clock);
    
    if (this->driver->setBusClock(clock) != kIOReturnSuccess)
    {
        perr("Failed to set the bus clock to %u Hz.", clock);
        
        return false;
    }
    
    pinfo("The bus clock has been set to %u Hz.", clock);
    
    // Switch to wider bus if possible
    // Guard: Check whether the card supports the 4-bit bus
    if (!BitOptions(this->scr.busWidths).contains(SCR::BusWidth::k4Bit))
    {
        pinfo("The card does not support the 4-bit bus.");
        
        return true;
    }
    
    // Guard: Check whether the host supports the 4-bit bus
    if (!this->driver->hostSupports4BitBusWidth())
    {
        pinfo("The host does not support the 4-bit bus.");
        
        return true;
    }
    
    // Enable the 4-bit wide bus
    pinfo("Enabling the 4-bit bus...");
    
    if (!this->enable4BitWideBus())
    {
        perr("Failed to enable the 4-bit wide bus.");
        
        return false;
    }
    else
    {
        pinfo("The 4-bit bus has been enabled.");
    }
    
    // The card initialization sequence finishes
    pinfo("The card has been initialized with the high speed mode enabled.");
    
    return true;
}

///
/// [Helper] Initialize the card with UHS-I Mode enabled
///
/// @return `true` on success, `false` otherwise.
/// @note Port: This function replaces `mmc_sd_init_uhs_card()` defined in `sd.c`.
///
bool IOSDCard::initUltraHighSpeedMode()
{
    // Finish the card initialization sequence with the ultra high speed enabled
    pinfo("Initializing the card at the ultra high speed mode...");
    
    // Enable the 4-bit wide bus
    pinfo("Enabling the 4-bit bus...");
    
    if (!this->enable4BitWideBus())
    {
        perr("Failed to enable the 4-bit wide bus required by the UHS-I mode.");
        
        return false;
    }
    
    pinfo("The 4-bit bus has been enabled.")
    
    // Select the bus speed mode based on the host and card capability
    SwitchCaps::BusSpeed speedMode = this->selectUltraHighSpeedBusSpeed();
    
    pinfo("Selected bus speed mode is %d.", speedMode);
    
    // Set the driver strength for the card
    pinfo("Setting the driver strength for the card...");
    
    if (!this->setUHSDriverType(speedMode))
    {
        perr("Failed to set the driver type for the card.");
        
        return false;
    }
    
    pinfo("The driver strength has been set for the card.");
    
    // Set the current limit for the card
    pinfo("Setting the current limit for the card...");
    
    if (!this->setUHSCurrentLimit(speedMode))
    {
        perr("Failed to set the current limit for the card.");
        
        return false;
    }
    
    pinfo("The current limit has been set for the card.");
    
    // Set the bus speed mode of the card
    pinfo("Setting the bus speed mode for the card...");
    
    if (!this->setUHSBusSpeedMode(speedMode))
    {
        perr("Failed to set the bus speed mode of the card.");
        
        return false;
    }
    
    pinfo("The bus speed mode has been set for the card.");
    
    // Check whether the host needs tuning
    if (speedMode < SwitchCaps::BusSpeed::kSpeedUHSSDR50)
    {
        pinfo("The current bus speed mode does not require tuning.");
        
        return true;
    }
    
    // Execute the tuning
    pinfo("The current bus speed mode requires tuning.");
    
    if (this->driver->executeTuning() != kIOReturnSuccess)
    {
        perr("Failed to execute tuning for the current bus speed mode %d.", speedMode);
        
        return speedMode == SwitchCaps::BusSpeed::kSpeedUHSDDR50;
    }
    
    pinfo("Tuning has finished.");
    
    // The card initialization sequence finishes
    pinfo("The card has been initialized with the ultra high speed mode enabled.");
    
    return true;
}

///
/// [Helper] Read the card switch capabilities
///
/// @return `true` on success, `false` otherwise.
/// @note Port: This function replaces `mmc_read_switch()` defined in `sd.c`.
///
bool IOSDCard::readSwitchCapabilities()
{
    // Check the SD specification supported by the card
    if (this->scr.spec < SCR::Spec::kVersion1d1x)
    {
        pinfo("The card does not support SD 1.10+. Will skip the switch capabilities.");
        
        return true;
    }
    
    // Check whether the card supports switch functions
    if (!BitOptions(this->csd.cardCommandClasses).contains(CSD::CommandClass::kSwitch))
    {
        pinfo("The card conforms to SD 1.10+ but does not support mandatory switch functions.");
        
        return false;
    }

    // Fetch the switch function status
    UInt8 status[64] = {};
    
    if (this->driver->CMD6(0, 0, 0, status) != kIOReturnSuccess)
    {
        perr("Failed to issue the CMD6.");
        
        return false;
    }
    
    if (BitOptions(status[13]).contains(SwitchCaps::BusMode::kModeHighSpeed))
    {
        this->switchCaps.maxClockFrequencies.highSpeedMode = SwitchCaps::MaxClockFrequency::kClockHighSpeed;
    }
    
    if (this->scr.spec3 != 0)
    {
        this->switchCaps.sd3BusMode = status[13];
        
        this->switchCaps.sd3DriverType = status[9];
        
        this->switchCaps.sd3MaxCurrent = status[7] | status[6] << 8;
    }
    
    return true;
}

///
/// [Helper] Enable the 4-bit wide bus for data transfer
///
/// @return `true` on success, `false` otherwise.
///
bool IOSDCard::enable4BitWideBus()
{
    // Switch to the 4-bit bus (card)
    pinfo("Asking the card to use the 4-bit bus for data transfer.");
    
    if (this->driver->ACMD6(this->rca, IOSDBusConfig::BusWidth::k4Bit) != kIOReturnSuccess)
    {
        perr("Failed to issue the ACMD6 to switch to the 4-bit bus.");
        
        return false;
    }
    
    pinfo("The card will use the 4-bit bus for data transfer.");
    
    // Switch to the 4-bit bus (host)
    pinfo("Asking the host device to use the 4-bit bus for data transfer.");
    
    if (this->driver->setBusWidth(IOSDBusConfig::BusWidth::k4Bit) != kIOReturnSuccess)
    {
        perr("Failed to ask the host to switch to the 4-bit bus.");
        
        return false;
    }
    
    pinfo("The host device will use the 4-bit bus for data transfer.");
    
    return true;
}

///
/// [Helper] Enable the signal voltage for the UHS-I mode
///
/// @param ocr The current operating condition register value
/// @return `true` on success, `false` otherwise.
/// @note Port: This function replaces `mmc_set_uhs_voltage()` defined in `core.c`.
///
bool IOSDCard::enableUltraHighSpeedSignalVoltage(UInt32 ocr)
{
    pinfo("Enabling the signal voltage for the UHS-I mode...");
    
    // Tell the card to switch to 1.8V
    if (this->driver->CMD11() != kIOReturnSuccess)
    {
        perr("Failed to tell the card to switch to 1.8V.");
        
        return false;
    }
    
    // The card should put the CMD and DATA lines low
    // immediately after sending the response of CMD11
    // Wait for 1ms and then check the line status
    IOSleep(1);
    
    // Since the driver has issued the CMD11,
    // if the driver fails to switch to 1.8V,
    // it must restart the power of the card.
    bool retVal = false;
    
    // Verify the line status
    // TODO: DATA LOW == DATA IDLE???
//    bool status = false;
//
//    if (this->driver->getHostDevice()->isCardDataLineBusy(status) != kIOReturnSuccess)
//    {
//        perr("Failed to retrieve the data line status.");
//
//        goto cycle;
//    }
//
//    if (status)
//    {
//        perr("The card should put the CMD and DATA lines low at this moment.");
//
//        goto cycle;
//    }
    
    // Tell the host the switch to 1.8V
    if (this->driver->setUltraHighSpeedSignalVoltage() != kIOReturnSuccess)
    {
        perr("Failed to tell the host to switch to 1.8V.");
        
        goto cycle;
    }
    
    // Wait at least 1ms
    IOSleep(1);
    
    retVal = true;
    
    // If failed to switch to 1.8V, the card data lines are low
    // TODO: DATA LOW == DATA IDLE???
//    if (this->driver->getHostDevice()->isCardDataLineBusy(status) != kIOReturnSuccess)
//    {
//        perr("Failed to retrieve the data line status.");
//
//        goto cycle;
//    }
//
//    if (status)
//    {
//        pinfo("The signal voltage has been enabled for the UHS-I mode.");
//
//        retVal = true;
//    }
//    else
//    {
//        perr("Failed to enable the signal voltage for the UHS-I mode.");
//    }
    
cycle:
    if (!retVal)
    {
        psoftassert(this->driver->powerCycle(ocr) == kIOReturnSuccess,
                    "Failed to restart the power of the card.");
    }
    
    return retVal;
}

///
/// [Helper] Select the bus speed for the UHS-I mode
///
/// @return The bus speed supported by both the card and the host.
/// @note Port: This function replaces `sd_update_bus_speed_mode()` defined in `sd.c`.
///
SwitchCaps::BusSpeed IOSDCard::selectUltraHighSpeedBusSpeed()
{
    BitOptions hostCaps = this->driver->getHostDevice()->getCapabilities();
    
    BitOptions cardCaps = this->switchCaps.sd3BusMode;
    
    if (hostCaps.contains(IOSDHostDevice::Capability::kUHSSDR104) && cardCaps.contains(SwitchCaps::BusMode::kModeUHSSDR104))
    {
        pinfo("Will use the speed mode UHS-I SDR104.");
        
        return SwitchCaps::BusSpeed::kSpeedUHSSDR104;
    }
    
    if (hostCaps.contains(IOSDHostDevice::Capability::kUHSDDR50) && cardCaps.contains(SwitchCaps::BusMode::kModeUHSDDR50))
    {
        pinfo("Will use the speed mode UHS-I DDR50.");
        
        return SwitchCaps::BusSpeed::kSpeedUHSDDR50;
    }
    
    if (hostCaps.contains(IOSDHostDevice::Capability::kUHSSDR50) && cardCaps.contains(SwitchCaps::BusMode::kModeUHSSDR50))
    {
        pinfo("Will use the speed mode UHS-I SDR50.");
        
        return SwitchCaps::BusSpeed::kSpeedUHSSDR50;
    }
    
    if (hostCaps.contains(IOSDHostDevice::Capability::kUHSSDR25) && cardCaps.contains(SwitchCaps::BusMode::kModeUHSSDR25))
    {
        pinfo("Will use the speed mode UHS-I SDR25.");
        
        return SwitchCaps::BusSpeed::kSpeedUHSSDR25;
    }
    
    if (hostCaps.contains(IOSDHostDevice::Capability::kUHSSDR12) && cardCaps.contains(SwitchCaps::BusMode::kModeUHSSDR12))
    {
        pinfo("Will use the speed mode UHS-I SDR12.");
        
        return SwitchCaps::BusSpeed::kSpeedUHSSDR12;
    }
    
    pfatal("Should never reach at here. The host and/or card capabilities are invalid.");
}

bool IOSDCard::setUHSDriverType(SwitchCaps::BusSpeed busSpeed)
{
    return true;
}

///
/// [Helper] Set the current limit for the UHS-I card
///
/// @param busSpeed The bus speed
/// @return `true` on success, `false` otherwise.
/// @note Port: This function replaces `sd_set_current_limit()` defined in `sd.c`.
/// @note This function is a part of the UHS-I card initialization routine `initUltraHighSpeedMode()`.
///
bool IOSDCard::setUHSCurrentLimit(SwitchCaps::BusSpeed busSpeed)
{
    // Current limit switch is only defined for SDR50, SDR104 and DDR50 modes
    // For other bus speed modes, we do not change the current limit
    if (Value::of(busSpeed).isNotOneOf(SwitchCaps::BusSpeed::kSpeedUHSSDR50, SwitchCaps::BusSpeed::kSpeedUHSSDR104, SwitchCaps::BusSpeed::kSpeedUHSDDR50))
    {
        pinfo("No need to change the current limit.");
        
        return true;
    }
    
    // The host has different current capabilities when operating at different voltages
    // Get the maximum current supported by the host device
    UInt32 hostMaxCurrent = this->driver->getHostMaxCurrent();
    
    UInt32 cardMaxCurrent = this->switchCaps.sd3MaxCurrent;
    
    // The new current limit
    UInt8 limit;
    
    if (hostMaxCurrent >= 800 && BitOptions(cardMaxCurrent).contains(SwitchCaps::MaxCurrent::kMaxCurrent800mA))
    {
        limit = SwitchCaps::SetCurrentLimit::kCurrentLimit800mA;
    }
    else if (hostMaxCurrent >= 600 && BitOptions(cardMaxCurrent).contains(SwitchCaps::MaxCurrent::kMaxCurrent600mA))
    {
        limit = SwitchCaps::SetCurrentLimit::kCurrentLimit600mA;
    }
    else if (hostMaxCurrent >= 400 && BitOptions(cardMaxCurrent).contains(SwitchCaps::MaxCurrent::kMaxCurrent400mA))
    {
        limit = SwitchCaps::SetCurrentLimit::kCurrentLimit400mA;
    }
    else if (hostMaxCurrent >= 200 && BitOptions(cardMaxCurrent).contains(SwitchCaps::MaxCurrent::kMaxCurrent200mA))
    {
        limit = SwitchCaps::SetCurrentLimit::kCurrentLimit200mA;
    }
    else
    {
        return true;
    }
    
    // Switch the current limit
    UInt8 status[64] = {};
    
    if (this->driver->CMD6(1, 3, limit, status) != kIOReturnSuccess)
    {
        perr("Failed to switch the current limit.");
        
        return false;
    }
    
    // Verify the current limit
    if (((status[15] >> 4) & 0x0F) != limit)
    {
        perr("The new current limit is not the requested one.");
        
        return false;
    }
    
    return true;
}

///
/// [Helper] Set the bus speed for the UHS-I card
///
/// @param busSpeed The bus speed
/// @return `true` on success, `false` otherwise.
/// @note Port: This function replaces `sd_set_bus_speed_mode()` defined in `sd.c`.
/// @note This function is a part of the UHS-I card initialization routine `initUltraHighSpeedMode()`.
///
bool IOSDCard::setUHSBusSpeedMode(SwitchCaps::BusSpeed busSpeed)
{
    IOSDBusConfig::BusTiming timing;
    
    switch (busSpeed)
    {
        case SwitchCaps::BusSpeed::kSpeedUHSSDR104:
        {
            timing = IOSDBusConfig::BusTiming::kUHSSDR104;
            
            this->switchCaps.maxClockFrequencies.ultraHighSpeedMode = SwitchCaps::MaxClockFrequency::kClockUHSSDR104;
            
            break;
        }
            
        case SwitchCaps::BusSpeed::kSpeedUHSDDR50:
        {
            timing = IOSDBusConfig::BusTiming::kUHSDDR50;
            
            this->switchCaps.maxClockFrequencies.ultraHighSpeedMode = SwitchCaps::MaxClockFrequency::kClockUHSDDR50;
            
            break;
        }
            
        case SwitchCaps::BusSpeed::kSpeedUHSSDR50:
        {
            timing = IOSDBusConfig::BusTiming::kUHSSDR50;
            
            this->switchCaps.maxClockFrequencies.ultraHighSpeedMode = SwitchCaps::MaxClockFrequency::kClockUHSSDR50;
            
            break;
        }
            
        case SwitchCaps::BusSpeed::kSpeedUHSSDR25:
        {
            timing = IOSDBusConfig::BusTiming::kUHSSDR25;
            
            this->switchCaps.maxClockFrequencies.ultraHighSpeedMode = SwitchCaps::MaxClockFrequency::kClockUHSSDR25;
            
            break;
        }
            
        case SwitchCaps::BusSpeed::kSpeedUHSSDR12:
        {
            timing = IOSDBusConfig::BusTiming::kUHSSDR12;
            
            this->switchCaps.maxClockFrequencies.ultraHighSpeedMode = SwitchCaps::MaxClockFrequency::kClockUHSSDR12;
            
            break;
        }
            
        default:
        {
            perr("Detected an invalid bus speed for UHS-I card.");
            
            return false;
        }
    }
    
    // Tell the card to switch the bus speed
    UInt8 status[64] = {};
    
    if (this->driver->CMD6(1, 0, busSpeed, status) != kIOReturnSuccess)
    {
        perr("Failed to issue the CMD6 to switch 1the bus speed.");
        
        return false;
    }
    
    // Verify the new bus speed
    if ((status[16] & 0x0F) != busSpeed)
    {
        perr("The new bus speed is not the one requested.");
        
        return false;
    }
    
    // Set the host-side timing
    if (this->driver->setBusTiming(timing) != kIOReturnSuccess)
    {
        perr("Failed to set the host-side timing.");
        
        return false;
    }
    
    // Set the host-side clock
    if (this->driver->setBusClock(this->switchCaps.maxClockFrequencies.ultraHighSpeedMode) != kIOReturnSuccess)
    {
        perr("Failed to set the host-side clock.");
        
        return false;
    }
    
    return true;
}

///
/// Get the card characteristics
///
/// @return A dictionary that contains card characteristics which can be recognized by the System Profiler.
/// @note The caller is responsible for releasing the returned dictionary.
///
OSDictionaryPtr IOSDCard::getCardCharacteristics()
{
    OSDictionary* dictionary = OSDictionary::withCapacity(8);

    char name[8] = {};
    
    char revision[8] = {};
    
    char date[8] = {};
    
    passert(this->getCardName(name, sizeof(name)), "Should be able to get the card name.");
    
    passert(this->getCardRevision(revision, sizeof(revision)), "Should be able to get the card revision.");
    
    passert(this->getCardProductionDate(date, sizeof(date)), "Should be able to get the card production date.");
    
    if (dictionary != nullptr &&
        OSDictionaryAddStringToDictionary(dictionary, "Card Type", this->getCardType()) &&
        OSDictionaryAddStringToDictionary(dictionary, "Specification Version", this->getSpecificationVersion()) &&
        OSDictionaryAddStringToDictionary(dictionary, "Product Name", name) &&
        OSDictionaryAddStringToDictionary(dictionary, "Product Revision Level", revision) &&
        OSDictionaryAddStringToDictionary(dictionary, "Manufacturing Date", date) &&
        OSDictionaryAddDataToDictionary(dictionary, "Serial Number", &this->cid.serial, sizeof(this->cid.serial)) &&
        OSDictionaryAddDataToDictionary(dictionary, "Manufacturer ID", &this->cid.manufacturer, sizeof(this->cid.manufacturer)) &&
        OSDictionaryAddDataToDictionary(dictionary, "Application ID", &this->cid.oem, sizeof(this->cid.oem)))
    {
        return dictionary;
    }
    
    OSSafeReleaseNULL(dictionary);
    
    return nullptr;
}

///
/// Create the card and initialize it with the given OCR value
///
/// @param driver The non-null host driver
/// @param ocr The OCR value that contains the voltage level supported by the host and the card
/// @return A non-null card instance on success, `nullptr` otherwise.
///
IOSDCard* IOSDCard::createWithOCR(IOSDHostDriver* driver, UInt32 ocr)
{
    auto card = OSTypeAlloc(IOSDCard);
    
    if (card == nullptr)
    {
        perr("Failed to allocate a card instance.")
        
        return nullptr;
    }
    
    if (!card->init(driver, ocr))
    {
        perr("Failed to initialize the card.");
        
        card->release();
        
        return nullptr;
    }
    
    return card;
}
