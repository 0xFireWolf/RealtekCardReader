//
//  RealtekRTS5229Controller.cpp
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 6/18/21.
//

#include "RealtekRTS5229Controller.hpp"

//
// MARK: - Meta Class Definitions
//

OSDefineMetaClassAndStructors(RealtekRTS5229Controller, RealtekPCICardReaderController);

//
// MARK: - SD Pull Control Tables
//

/// A sequence of registers to transfer to enable SD pull control
const RealtekRTS5229Controller::ChipRegValuePair RealtekRTS5229Controller::kSDEnablePullControlTablePairs[] =
{
    { RTSX::PCR::Chip::CARD::PULL::rCTL2, 0xAA },
    { RTSX::PCR::Chip::CARD::PULL::rCTL3, 0xE9 },
};

const RealtekRTS5229Controller::SimpleRegValuePairs RealtekRTS5229Controller::kSDEnablePullControlTable =
{
    RealtekRTS5229Controller::kSDEnablePullControlTablePairs
};

const RealtekRTS5229Controller::ChipRegValuePair RealtekRTS5229Controller::kSDEnablePullControlTablePairsRevC[] =
{
    { RTSX::PCR::Chip::CARD::PULL::rCTL2, 0xAA },
    { RTSX::PCR::Chip::CARD::PULL::rCTL3, 0xD9 },
};

const RealtekRTS5229Controller::SimpleRegValuePairs RealtekRTS5229Controller::kSDEnablePullControlTableRevC =
{
    RealtekRTS5229Controller::kSDEnablePullControlTablePairsRevC
};

/// A sequence of registers to transfer to disable SD pull control
const RealtekRTS5229Controller::ChipRegValuePair RealtekRTS5229Controller::kSDDisablePullControlTablePairs[] =
{
    { RTSX::PCR::Chip::CARD::PULL::rCTL2, 0x55 },
    { RTSX::PCR::Chip::CARD::PULL::rCTL3, 0xD5 },
};

const RealtekRTS5229Controller::SimpleRegValuePairs RealtekRTS5229Controller::kSDDisablePullControlTable =
{
    RealtekRTS5229Controller::kSDDisablePullControlTablePairs
};

const RealtekRTS5229Controller::ChipRegValuePair RealtekRTS5229Controller::kSDDisablePullControlTablePairsRevC[] =
{
    { RTSX::PCR::Chip::CARD::PULL::rCTL2, 0x55 },
    { RTSX::PCR::Chip::CARD::PULL::rCTL3, 0xE5 },
};

const RealtekRTS5229Controller::SimpleRegValuePairs RealtekRTS5229Controller::kSDDisablePullControlTableRevC =
{
    RealtekRTS5229Controller::kSDDisablePullControlTablePairsRevC
};

//
// MARK: - Card Power Management
//

///
/// Power on the SD card
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_pci_card_power_on()` defined in `rtsx_psr.c`.
///
IOReturn RealtekRTS5229Controller::powerOnCard()
{
    using namespace RTSX::PCR::Chip;
    
    // Enable the card power in two phases
    // Phase 1: Partial Power
    pinfo("Powering on the card partially...");
    
    const ChipRegValuePair ppairs[] =
    {
        { CARD::rPWRCTRL, CARD::PWRCTRL::kSDPowerMask, CARD::PWRCTRL::kSDPartialPowerOn },
        { rPWRGATECTRL, PWRGATECTRL::kMask, PWRGATECTRL::kVCC1 }
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
        { rPWRGATECTRL, PWRGATECTRL::kMask, PWRGATECTRL::kVCC1 | PWRGATECTRL::kVCC2 }
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
IOReturn RealtekRTS5229Controller::powerOffCard()
{
    using namespace RTSX::PCR::Chip;
    
    const ChipRegValuePair pairs[] =
    {
        { CARD::rPWRCTRL, CARD::PWRCTRL::kSDPowerMask | CARD::PWRCTRL::kPMOSStrgMask, CARD::PWRCTRL::kSDPowerOff | CARD::PWRCTRL::kPMOSStrg400mA },
        { rPWRGATECTRL, PWRGATECTRL::kMask, PWRGATECTRL::kOn }
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
IOReturn RealtekRTS5229Controller::switchOutputVoltage(OutputVoltage outputVoltage)
{
    using namespace RTSX::PCR::Chip;
    
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
IOReturn RealtekRTS5229Controller::forcePowerDown()
{
    using namespace RTSX::PCR::Chip;
    
    return this->writeChipRegister(rFPDCTL, FPDCTL::kAllPowerMask, FPDCTL::kAllPowerDownValue);
}

//
// MARK: - Hardware Initialization and Configuration
//

///
/// Optimize the physical layer
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
///
IOReturn RealtekRTS5229Controller::optimizePhys()
{
    // Optimize the RX sensitivity
    return this->writePhysRegister(RTSX::PHYS::rPCR, 0xBA42);
}

///
/// Initialize hardware-specific parameters
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `*_init_params()` defined in each controller file.
///
IOReturn RealtekRTS5229Controller::initParameters()
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
    
    this->parameters.cardDriveSelector = RTSX::PCR::Chip::CARD::DRVSEL::kDefault;
    
    this->parameters.sd30DriveSelector1d8V = RTSX::PCR::Chip::CARD::SD30::DRVSEL::CFG::kDriverTypeB;
    
    this->parameters.sd30DriveSelector3d3V = RTSX::PCR::Chip::CARD::SD30::DRVSEL::CFG::kDriverTypeD;
    
    if (!this->parameters.revision.isRevC())
    {
        this->parameters.sdEnablePullControlTable = &RealtekRTS5229Controller::kSDEnablePullControlTable;
        
        this->parameters.sdDisablePullControlTable = &RealtekRTS5229Controller::kSDDisablePullControlTable;
    }
    else
    {
        this->parameters.sdEnablePullControlTable = &RealtekRTS5229Controller::kSDEnablePullControlTableRevC;
        
        this->parameters.sdDisablePullControlTable = &RealtekRTS5229Controller::kSDDisablePullControlTableRevC;
    }
    
    this->parameters.initialTxClockPhase = {27, 27, 15};
    
    this->parameters.initialRxClockPhase = {30, 6, 6};
    
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
IOReturn RealtekRTS5229Controller::initVendorSpecificParameters()
{
    pinfo("Initializing the vendor-specific parameters...");
    
    // PCR Config 1
    UInt32 regVal = this->device->configRead32(RTSX::PCR::kSREG1);
    
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
    
    // PCR Config 2
    regVal = this->device->configRead32(RTSX::PCR::kSREG2);
    
    this->parameters.sd30DriveSelector3d3V = this->vsMapDriveSelector(this->vsGetSD30DriveSelector3d3V(regVal));
    
    pinfo("PCR CFG2: RegVal = 0x%08x.", regVal);
    
    pinfo("PCR CFG2: SD30 Drive Selector (3.3V) = %d.", this->parameters.sd30DriveSelector3d3V);
    
    pinfo("Vendor-specific parameters have been initialized.");
    
    return kIOReturnSuccess;
}

///
/// Initialize the hardware (Extra Part)
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `extra_init_hw()` defined in defined in the `pcr->ops`.
///
IOReturn RealtekRTS5229Controller::initHardwareExtra()
{
    pinfo("Initializing the card reader (device-specific)...");
    
    using namespace RTSX::PCR::Chip;
    
    const ChipRegValuePair pairs[] =
    {
        // Turn on the LED
        { GPIO::rCTL, GPIO::CTL::kLEDMask, GPIO::CTL::kTurnOnLEDValue },
        
        // Reset the ASPM state
        { rAFCTL, 0x3F, 0x00 },
        
        // Force CLKREQ# PIN to drive 0 to request clock
        { rPETXCFG, 0x08, 0x08 },
        
        // Switch the LDO3318 source from DV33 to Card33
        { LDO::rPWRSEL, 0x03, 0x00 },
        { LDO::rPWRSEL, 0x03, 0x01 },
        
        // Set the initial LED cycle period
        { OLT_LED::rCTL, 0x0F, 0x02 },
        
        // Set the initial driving selector
        { CARD::SD30::DRVSEL::rCFG, 0xFF, this->parameters.sd30DriveSelector3d3V },
    };
    
    return this->writeChipRegisters(SimpleRegValuePairs(pairs));
}
