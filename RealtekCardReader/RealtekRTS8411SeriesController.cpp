//
//  RealtekRTS8411SeriesController.cpp
//  RealtekCardReader
//
//  Created by FireWolf on 6/20/21.
//

#include "RealtekRTS8411SeriesController.hpp"

//
// MARK: - Meta Class Definitions
//

OSDefineMetaClassAndAbstractStructors(RealtekRTS8411SeriesController, RealtekPCICardReaderController);

//
// MARK: - LED Management
//

///
/// Turn on the LED
///
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
/// @note Port: This function replaces `turn_on_led()` defined in `struct pcr_ops`.
///             The base controller class implements this function by changing the value of `GPIO_CTL`.
///             RTS5209, 5260, 5286, 5287 and 5289 controllers must override this function.
///
IOReturn RealtekRTS8411SeriesController::turnOnLED()
{
    using namespace RTSX::PCR::Chip;
    
    return this->writeChipRegister(CARD::rGPIO, CARD::GPIO::kLEDMask, CARD::GPIO::kLEDOn);
}

///
/// Turn off the LED
///
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
/// @note Port: This function replaces `turn_off_led()` defined in `struct pcr_ops`.
///             The base controller class implements this function by changing the value of `GPIO_CTL`.
///             RTS5209, 5260, 5286, 5287 and 5289 controllers must override this function.
///
IOReturn RealtekRTS8411SeriesController::turnOffLED()
{
    using namespace RTSX::PCR::Chip;
    
    return this->writeChipRegister(CARD::rGPIO, CARD::GPIO::kLEDMask, CARD::GPIO::kLEDOff);
}

///
/// Enable LED blinking
///
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
/// @note Port: This function replaces `enable_auto_blink()` defined in `struct pcr_ops`.
///             The base controller class implements this function by changing the value of `OLT_LED_CTL`.
///             RTS5209, 5286, 5287 and 5289 controllers must override this function.
///
IOReturn RealtekRTS8411SeriesController::enableLEDBlinking()
{
    using namespace RTSX::PCR::Chip;
    
    return this->writeChipRegister(CARD::rBLINK, 0xFF, 0x0D);
}

///
/// Disable LED blinking
///
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
/// @note Port: This function replaces `enable_auto_blink()` defined in `struct pcr_ops`.
///             The base controller class implements this function by changing the value of `OLT_LED_CTL`.
///             RTS5209, 5286, 5287 and 5289 controllers must override this function.
///
IOReturn RealtekRTS8411SeriesController::disableLEDBlinking()
{
    using namespace RTSX::PCR::Chip;
    
    return this->writeChipRegister(CARD::rBLINK, 0x08, 0x00);
}

//
// MARK: - Card Power Management
//

///
/// Power on the SD card
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_pci_card_power_on()` defined in `rtsx_psr.c`.
///
IOReturn RealtekRTS8411SeriesController::powerOnCard()
{
    using namespace RTSX::PCR::Chip;
    
    pinfo("Powering on the card partially (5%%)...");
    
    const ChipRegValuePair ppairs[] =
    {
        { CARD::rPWRCTRL, CARD::PWRCTRL::kBppPowerMask, CARD::PWRCTRL::kBppPower5PercentOn },
        { LDO::rCTL, LDO::CTL::kBppLDOMask, LDO::CTL::kBppLDOSuspend },
    };
    
    IOReturn retVal = this->transferWriteRegisterCommands(SimpleRegValuePairs(ppairs));
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to power on the card partially (5%%). Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    IOSleep(5);
    
    pinfo("Powering on the card partially (10%%)...");
    
    retVal = this->writeChipRegister(CARD::rPWRCTRL, CARD::PWRCTRL::kBppPowerMask, CARD::PWRCTRL::kBppPower10PercentOn);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to power on the card partially (10%%). Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    IOSleep(5);
    
    pinfo("Powering on the card partially (15%%)...");
    
    retVal = this->writeChipRegister(CARD::rPWRCTRL, CARD::PWRCTRL::kBppPowerMask, CARD::PWRCTRL::kBppPower15PercentOn);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to power on the card partially (15%%). Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    IOSleep(5);
    
    pinfo("Powering on the card fully...");
    
    const ChipRegValuePair fpairs[] =
    {
        { CARD::rPWRCTRL, CARD::PWRCTRL::kBppPowerMask, CARD::PWRCTRL::kBppPowerOn },
        { LDO::rCTL, LDO::CTL::kBppLDOMask, LDO::CTL::kBppLDOOn },
    };
    
    retVal = this->writeChipRegisters(SimpleRegValuePairs(fpairs));
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to fully power on the card. Error = 0x%x.", retVal);

        return retVal;
    }
    
    pinfo("The card power is fully on.");
    
    return kIOReturnSuccess;
}

///
/// Power off the SD card
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_pci_card_power_off()` defined in `rtsx_psr.c`.
///
IOReturn RealtekRTS8411SeriesController::powerOffCard()
{
    using namespace RTSX::PCR::Chip;
    
    const ChipRegValuePair pairs[] =
    {
        { CARD::rPWRCTRL, CARD::PWRCTRL::kBppPowerMask, CARD::PWRCTRL::kBppPowerOff },
        { LDO::rCTL, LDO::CTL::kBppLDOMask, LDO::CTL::kBppLDOSuspend },
    };
    
    return this->writeChipRegisters(SimpleRegValuePairs(pairs));
}

///
/// [Helper, Common] Switch to the given output voltage
///
/// @param outputVoltage The new output voltage
/// @param shift Shift `kBppRegTuned18` by the given value
/// @param asic The ASIC value for the 1.8V output voltage
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtl8411_do_switch_output_voltage()` defined in `rts8411.c`.
///
IOReturn RealtekRTS8411SeriesController::switchOutputVoltage(OutputVoltage outputVoltage, UInt8 shift, UInt8 asic)
{
    using namespace RTSX::PCR::Chip;
    
    UInt8 selector;
    
    UInt8 mask = (LDO::CTL::kBppTuned18 << shift) | LDO::CTL::kBppPadMask;
    
    UInt8 value;
    
    if (outputVoltage == OutputVoltage::k3d3V)
    {
        selector = this->parameters.sd30DriveSelector3d3V;
        
        value = (LDO::CTL::kBppAsic3V3 << shift) | LDO::CTL::kBppPad3V3;
    }
    else
    {
        selector = this->parameters.sd30DriveSelector1d8V;
        
        value = (asic << shift) | LDO::CTL::kBppPad1V8;
    }
    
    const ChipRegValuePair pairs[] =
    {
        { CARD::SD30::DRVSEL::rCFG, 0x07, selector },
        { LDO::rCTL, mask, value },
    };
    
    return this->writeChipRegisters(SimpleRegValuePairs(pairs));
}

///
/// Switch to the given output voltage
///
/// @param outputVoltage The new output voltage
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_pci_switch_output_voltage()` defined in `rtsx_psr.c`.
/// @note Port: This function replaces `rtl8411_switch_output_voltage()` definde in `rts8411.c`.
///             RTS5286 controller must override this function and provide the correct shift and ASIC values.
///
IOReturn RealtekRTS8411SeriesController::switchOutputVoltage(OutputVoltage outputVoltage)
{
    using namespace RTSX::PCR::Chip;
    
    return this->switchOutputVoltage(outputVoltage, LDO::CTL::kBppTuned18Shift_8411, LDO::CTL::kBppAsic1V8);
}

//
// MARK: - Card Detection and Write Protection
//

///
/// Check whether a card is present
///
/// @return `true` if a card exists, `false` otherwise.
/// @note Port: This function replaces `rtsx_pci_card_exist()` and `cd_deglitch()` defined in `rtsx_psr.c`.
/// @warning: This function supports SD cards only.
///
bool RealtekRTS8411SeriesController::isCardPresent()
{
    using namespace RTSX::PCR::Chip;
    
    if (!super::isCardPresent())
    {
        const ChipRegValuePair pairs[] =
        {
            // Enable the card
            { CARD::rPADCTL, CARD::PADCTL::kDisableMask, CARD::PADCTL::kEnableAll },
            
            // Enable the card interrupt
            { EFUSE::rCONTENT, 0xE0, 0x00 },
        };
        
        psoftassert(this->writeChipRegisters(SimpleRegValuePairs(pairs)) == kIOReturnSuccess, "Failed to enable the card.");
        
        return false;
    }
    
    psoftassert(this->writeChipRegister(CARD::rPWRCTRL, CARD::PWRCTRL::kBppPowerMask, CARD::PWRCTRL::kBppPower5PercentOn) == kIOReturnSuccess,
                "Failed to power on the card partially (5%%).");
    
    IOSleep(100);
    
    bool present = super::isCardPresent();
    
    psoftassert(this->writeChipRegister(CARD::rPWRCTRL, CARD::PWRCTRL::kBppPowerMask, CARD::PWRCTRL::kBppPowerOff) == kIOReturnSuccess,
                "Failed to power off the card.");
    
    if (present)
    {
        // Disable the MS card interrupts
        const ChipRegValuePair pairs[] =
        {
            { EFUSE::rCONTENT, 0xE0, 0x80 },
            { CARD::rPADCTL, CARD::PADCTL::kDisableMask, CARD::PADCTL::kEnableSD }
        };
        
        psoftassert(this->writeChipRegisters(SimpleRegValuePairs(pairs)) == kIOReturnSuccess, "Disable the MS card interrupt.");
    }
    
    return present;
}

//
// MARK: - Card Clock Configurations
//

///
/// Convert the given SSC clock to the divider N
///
/// @param clock The SSC clock in MHz
/// @return The divider N.
/// @note Port: This function replaces `conv_clk_and_div_n()` defined in `rtsx_psr.c`.
///             RTL8411 series controllers must override this function.
///
UInt32 RealtekRTS8411SeriesController::sscClock2DividerN(UInt32 clock)
{
    return clock * 4 / 5 - 2;
}

///
/// Convert the given divider N back to the SSC clock
///
/// @param n The divider N
/// @return The SSC clock in MHz.
/// @note Port: This function replaces `conv_clk_and_div_n()` defined in `rtsx_psr.c`.
///             RTL8411 series controllers must override this function.
///
UInt32 RealtekRTS8411SeriesController::dividerN2SSCClock(UInt32 n)
{
    return (n + 2) * 5 / 4;
}

//
// MARK: - Power Management
//

///
/// Power down the controller forcedly
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_base_force_power_down()` defined in `rtsx_psr.c` and `*_force_power_down()` defined in each controller file.
///
IOReturn RealtekRTS8411SeriesController::forcePowerDown()
{
    return this->writeChipRegister(RTSX::PCR::Chip::rFPDCTL, 0x07, 0x07);
}

//
// MARK: - Hardware Initialization and Configuration
//

///
/// Get the IC revision
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `*_get_ic_version()` defined in each controller file.
///
IOReturn RealtekRTS8411SeriesController::getRevision(Revision& revision)
{
    pinfo("Fetching the chip revision...");
    
    UInt8 value;
    
    IOReturn retVal = this->readChipRegister(RTSX::PCR::Chip::rSYSVER, value);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to fetch the chip revision. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    revision = Revision::parse(value & 0x0F);
    
    pinfo("Chip revision has been fetched: Rev %s.", revision.stringify());
    
    return kIOReturnSuccess;
}

///
/// Initialize hardware-specific parameters
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `*_init_params()` defined in each controller file.
/// @note Port: This function replaces `rtl8411_init_common_params()` defined in `rts8411.c`.
///             Concrete controllers must override and extend this function to set the pull control tables.
///
IOReturn RealtekRTS8411SeriesController::initParameters()
{
    pinfo("Initializing the device-specific parameters...");
    
    IOReturn retVal = this->getRevision(this->parameters.revision);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to get the IC revision. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    this->parameters.numSlots = 2;
    
    this->parameters.caps.supportsSDSDR50 = true;
    
    this->parameters.caps.supportsSDSDR104 = true;
    
    this->parameters.isSocketReversed = false;
    
    this->parameters.cardDriveSelector = RTSX::PCR::Chip::CARD::DRVSEL::kDefault_8411;
    
    this->parameters.sd30DriveSelector1d8V = RTSX::PCR::Chip::CARD::SD30::DRVSEL::CFG::kDriverTypeB;
    
    this->parameters.sd30DriveSelector3d3V = RTSX::PCR::Chip::CARD::SD30::DRVSEL::CFG::kDriverTypeD;
    
    this->parameters.initialTxClockPhase = {23, 7, 14};
    
    this->parameters.initialRxClockPhase = {4, 3, 10};
    
    this->parameters.ocp.enable = false;
    
    this->parameters.pm.isASPML1Enabled = true;
    
    pinfo("Device-specific parameters have been initialized.");
    
    return kIOReturnSuccess;
}

///
/// Initialize vendor-specific parameters
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `fetch_vendor_settings()` defined in the `pcr->ops`.
/// @note Port: This function replaces `rtl8411_fetch_vendor_settings()` defined in `rts8411.c`.
///             RTS5287 controller must override this function.
///
IOReturn RealtekRTS8411SeriesController::initVendorSpecificParameters()
{
    pinfo("Initializing the vendor-specific parameters...");
    
    // PCR Config 1
    UInt32 regVal = this->device->configRead32(RTSX::PCR::kSREG1);
    
    pinfo("PCR Config 1 = 0x%08x.", regVal);
    
    if (!this->vsIsRegisterValueValid(regVal))
    {
        perr("Vendor settings are invalid.");
        
        return kIOReturnError;
    }
    
    this->parameters.pm.isASPML1Enabled = this->vsIsASPML1Enabled(regVal);
    
    this->parameters.sd30DriveSelector1d8V = this->vsMapDriveSelector(this->vsGetSD30DriveSelector1d8V(regVal));
    
    this->parameters.cardDriveSelector &= 0x3F;
    
    this->parameters.cardDriveSelector |= this->vsGetCardDriveSelector(regVal);
    
    pinfo("PCR CFG1: RegVal = 0x%08x.", regVal);
    
    pinfo("PCR CFG1: ASPM L1 Enabled: %s.", YESNO(this->parameters.pm.isASPML1Enabled));
    
    pinfo("PCR CFG1: SD30 Drive Selector (1.8V) = %d.", this->parameters.sd30DriveSelector1d8V);
    
    pinfo("PCR CFG1: SD Card Drive Selector = 0x%x.", this->parameters.cardDriveSelector);
    
    // PCR Config 3
    regVal = this->device->configRead8(RTSX::PCR::kSREG3);
    
    pinfo("PCR Config 3 = 0x%08x.", regVal);
    
    this->parameters.sd30DriveSelector3d3V = this->vsGetSD30DriveSelector3d3V(regVal);
    
    pinfo("PCR CFG3: RegVal = 0x%08x.", regVal);
    
    pinfo("PCR CFG3: SD30 Drive Selector (3.3V) = %d.", this->parameters.sd30DriveSelector3d3V);
    
    pinfo("Vendor-specific parameters have been initialized.");
    
    return kIOReturnSuccess;
}

///
/// Initialize the hardware (Extra Part)
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `extra_init_hw()` defined in defined in the `pcr->ops`.
/// @note Port: This function replaces `rtl8411_extra_init_hw()` defined in `rts8411.c`.
///             RTS5287 controller must override this function.
///
IOReturn RealtekRTS8411SeriesController::initHardwareExtra()
{
    using namespace RTSX::PCR::Chip;
    
    const ChipRegValuePair pairs[] =
    {
        // Set the initial driving selector
        { CARD::SD30::DRVSEL::rCFG, 0xFF, this->parameters.sd30DriveSelector3d3V },
        
        // Enable the card
        { CARD::rPADCTL, CARD::PADCTL::kDisableMask | CARD::PADCTL::kAutoDisable, CARD::PADCTL::kEnableAll },
    };
    
    return this->transferWriteRegisterCommands(SimpleRegValuePairs(pairs));
}
