//
//  RealtekRTS5209Controller.cpp
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 6/17/21.
//

#include "RealtekRTS5209Controller.hpp"

//
// MARK: - Meta Class Definitions
//

OSDefineMetaClassAndStructors(RealtekRTS5209Controller, RealtekPCIeCardReaderController);

//
// MARK: - SD Pull Control Tables
//

/// A sequence of registers to transfer to enable SD pull control
const RealtekRTS5209Controller::ChipRegValuePair RealtekRTS5209Controller::kSDEnablePullControlTablePairs[] =
{
    { RTSX::Chip::CARD::PULL::rCTL1, 0xAA },
    { RTSX::Chip::CARD::PULL::rCTL2, 0xAA },
    { RTSX::Chip::CARD::PULL::rCTL3, 0xE9 },
};

const RealtekRTS5209Controller::SimpleRegValuePairs RealtekRTS5209Controller::kSDEnablePullControlTable =
{
    RealtekRTS5209Controller::kSDEnablePullControlTablePairs
};

/// A sequence of registers to transfer to disable SD pull control
const RealtekRTS5209Controller::ChipRegValuePair RealtekRTS5209Controller::kSDDisablePullControlTablePairs[] =
{
    { RTSX::Chip::CARD::PULL::rCTL1, 0x55 },
    { RTSX::Chip::CARD::PULL::rCTL2, 0x55 },
    { RTSX::Chip::CARD::PULL::rCTL3, 0xD5 },
};

const RealtekRTS5209Controller::SimpleRegValuePairs RealtekRTS5209Controller::kSDDisablePullControlTable =
{
    RealtekRTS5209Controller::kSDDisablePullControlTablePairs
};

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
IOReturn RealtekRTS5209Controller::turnOnLED()
{
    using namespace RTSX::Chip;
    
    return this->writeChipRegister(CARD::rGPIO, CARD::GPIO::kLEDMask, CARD::GPIO::kLEDOn);
}

///
/// Turn off the LED
///
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
/// @note Port: This function replaces `turn_off_led()` defined in `struct pcr_ops`.
///             The base controller class implements this function by changing the value of `GPIO_CTL`.
///             RTS5209, 5286, 5287 and 5289 controllers must override this function.
///
IOReturn RealtekRTS5209Controller::turnOffLED()
{
    using namespace RTSX::Chip;
    
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
IOReturn RealtekRTS5209Controller::enableLEDBlinking()
{
    using namespace RTSX::Chip;
    
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
IOReturn RealtekRTS5209Controller::disableLEDBlinking()
{
    using namespace RTSX::Chip;
    
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
IOReturn RealtekRTS5209Controller::powerOnCard()
{
    using namespace RTSX::Chip;
    
    // Enable the card power in two phases
    // Phase 1: Partial Power
    // Note that MS is not supported in the ported driver
    pinfo("Powering on the card partially...");
    
    const ChipRegValuePair ppairs[] =
    {
        { CARD::rPWRCTRL, CARD::PWRCTRL::kSDPowerMask, CARD::PWRCTRL::kSDPartialPowerOn },
        { rPWRGATECTRL, PWRGATECTRL::kMask, PWRGATECTRL::kSuspend } // Differs from RTS5229
    };
    
    IOReturn retVal = this->transferWriteRegisterCommands(SimpleRegValuePairs(ppairs));
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to partially power the card. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    IOSleep(5);
    
    pinfo("The card power is partially on.");
    
    // Phase 2: Full Power
    const ChipRegValuePair fpairs[] =
    {
        { CARD::rPWRCTRL, CARD::PWRCTRL::kSDPowerMask, CARD::PWRCTRL::kSDPowerOn },
        { rPWRGATECTRL, PWRGATECTRL::kMask, PWRGATECTRL::kOn } // Differs from RTS5229
    };

    retVal = this->transferWriteRegisterCommands(SimpleRegValuePairs(fpairs));

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
IOReturn RealtekRTS5209Controller::powerOffCard()
{
    using namespace RTSX::Chip;
    
    const ChipRegValuePair pairs[] =
    {
        { CARD::rPWRCTRL, CARD::PWRCTRL::kSDPowerMask | CARD::PWRCTRL::kPMOSStrgMask, CARD::PWRCTRL::kSDPowerOff | CARD::PWRCTRL::kPMOSStrg400mA },
        { rPWRGATECTRL, PWRGATECTRL::kMask, PWRGATECTRL::kOff }
    };
    
    return this->transferWriteRegisterCommands(SimpleRegValuePairs(pairs));
}

///
/// Switch to the given output voltage
///
/// @param outputVoltage The new output voltage
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_pci_switch_output_voltage()` defined in `rtsx_psr.c`.
///
IOReturn RealtekRTS5209Controller::switchOutputVoltage(OutputVoltage outputVoltage)
{
    // The following implementation is identical to `RealtekRTS5229Controller::switchOutputVoltage()`
    using namespace RTSX::Chip;
    
    UInt8 selector;
    
    UInt16 tune;
    
    if (outputVoltage == OutputVoltage::k3d3V)
    {
        selector = this->parameters.sd30DriveSelector3d3V;
        
        tune = 0x4FC0 | 0x24;
    }
    else
    {
        selector = this->parameters.sd30DriveSelector1d8V;
        
        tune = 0x4C40 | 0x24;
    }
    
    IOReturn retVal = this->writeChipRegister(CARD::SD30::DRVSEL::rCFG, 0x07, selector);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to change the drive selector for the target voltage level. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    using namespace RTSX::PHYS;
    
    retVal = this->writePhysRegister(rTUNE, tune);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to update the physical register value for the target voltage level. Error = 0x%x.", retVal);
        
        return retVal;
    }
        
    return kIOReturnSuccess;
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
IOReturn RealtekRTS5209Controller::forcePowerDown()
{
    return this->writeChipRegister(RTSX::Chip::rFPDCTL, 0x07, 0x07);
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
IOReturn RealtekRTS5209Controller::getRevision(Revision& revision)
{
    pinfo("Fetching the chip revision...");
    
    revision = Revision::parse(this->readRegister8(0x1C) & 0x0F);
    
    pinfo("Chip revision has been fetched: Rev %s.", revision.stringify());
    
    return kIOReturnSuccess;
}

///
/// Optimize the physical layer
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
///
IOReturn RealtekRTS5209Controller::optimizePhys()
{
    return this->writePhysRegister(RTSX::PHYS::rPCR, 0xB966);
}

///
/// Initialize hardware-specific parameters
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `*_init_params()` defined in each controller file.
///
IOReturn RealtekRTS5209Controller::initParameters()
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
    
    this->parameters.caps.supportsMMC8BitTransfer = true;
    
    this->parameters.isSocketReversed = false;
    
    this->parameters.cardDriveSelector = RTSX::Chip::CARD::DRVSEL::kDefault_5209;
    
    this->parameters.sd30DriveSelector1d8V = RTSX::Chip::CARD::SD30::DRVSEL::CFG::kDriverTypeB;
    
    this->parameters.sd30DriveSelector3d3V = RTSX::Chip::CARD::SD30::DRVSEL::CFG::kDriverTypeD;
    
    this->parameters.sdEnablePullControlTable = &RealtekRTS5209Controller::kSDEnablePullControlTable;
    
    this->parameters.sdDisablePullControlTable = &RealtekRTS5209Controller::kSDDisablePullControlTable;
    
    this->parameters.initialTxClockPhase = {27, 27, 16};
    
    this->parameters.initialRxClockPhase = {24, 6, 5};
    
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
///
IOReturn RealtekRTS5209Controller::initVendorSpecificParameters()
{
    pinfo("Initializing the vendor-specific parameters...");
    
    // Note that MS PMOS is not supported by this driver
    // PCR Config 1
    UInt32 regVal = this->device->configRead32(RTSX::PCR::kSREG1);
    
    if (!this->vsIsRegister1ValueValid(regVal))
    {
        perr("Vendor settings [1] are invalid.");
        
        return kIOReturnError;
    }
    
    this->parameters.pm.isASPML1Enabled = this->vsIsASPML1Enabled(regVal);
    
    pinfo("PCR CFG1: RegVal = 0x%08x.", regVal);
    
    pinfo("PCR CFG1: ASPM L1 Enabled: %s.", YESNO(this->parameters.pm.isASPML1Enabled));
    
    // PCR Config 2
    regVal = this->device->configRead32(RTSX::PCR::kSREG2);
    
    if (!this->vsIsRegister2ValueValid(regVal))
    {
        perr("Vendor settings [2] are invalid.");
        
        return kIOReturnError;
    }
    
    this->parameters.sd30DriveSelector3d3V = this->vsGetSD30DriveSelector3d3V(regVal);
    
    this->parameters.sd30DriveSelector1d8V = this->vsGetSD30DriveSelector1d8V(regVal);
    
    this->parameters.cardDriveSelector = this->vsGetCardDriveSelector(regVal);
    
    pinfo("PCR CFG2: RegVal = 0x%08x.", regVal);
    
    pinfo("PCR CFG1: SD30 Drive Selector (1.8V) = %d.", this->parameters.sd30DriveSelector1d8V);
    
    pinfo("PCR CFG2: SD30 Drive Selector (3.3V) = %d.", this->parameters.sd30DriveSelector3d3V);
    
    pinfo("PCR CFG1: SD Card Drive Selector = 0x%x.", this->parameters.cardDriveSelector);
    
    pinfo("Vendor-specific parameters have been initialized.");
    
    return kIOReturnSuccess;
}

///
/// Initialize the hardware (Extra Part)
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `extra_init_hw()` defined in defined in the `pcr->ops`.
///
IOReturn RealtekRTS5209Controller::initHardwareExtra()
{
    pinfo("Initializing the card reader (device-specific)...");
    
    using namespace RTSX::Chip;
    
    const ChipRegValuePair pairs[] =
    {
        // Turn off the LED
        { CARD::rGPIO, 0xFF, 0x03 },
        
        // Reset the ASPM state
        { rAFCTL, 0x3F, 0x00 },
        
        // Force CLKREQ# PIN to drive 0 to request clock
        { rPETXCFG, 0x08, 0x08 },
        
        // Configure GPIO as the output
        { CARD::rGPIODIR, 0xFF, 0x03 },
        
        // Set the initial driving selector
        { CARD::SD30::DRVSEL::rCFG, 0xFF, this->parameters.sd30DriveSelector3d3V },
    };
    
    return this->writeChipRegisters(SimpleRegValuePairs(pairs));
}
