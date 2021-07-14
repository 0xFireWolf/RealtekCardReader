//
//  RealtekRTS5249SeriesController.cpp
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 5/16/21.
//

#include "RealtekRTS5249SeriesController.hpp"
#include "BitOptions.hpp"
#include "IOPCIeDevice.hpp"

//
// MARK: - Meta Class Definitions
//

OSDefineMetaClassAndAbstractStructors(RealtekRTS5249SeriesController, RealtekPCICardReaderController);

//
// MARK: - Driving Tables for RTS5249, 524A, 525A
//

/// SD 3.0 drive table (1.8V)
const RealtekRTS5249SeriesController::DrivingTable RealtekRTS5249SeriesController::kSD30DriveTable1d8V =
{{
    {0xC4, 0xC4, 0xC4},
    {0x3C, 0x3C, 0x3C},
    {0xFE, 0xFE, 0xFE},
    {0xB3, 0xB3, 0xB3},
}};

/// SD 3.0 drive table (3.3V)
const RealtekRTS5249SeriesController::DrivingTable RealtekRTS5249SeriesController::kSD30DriveTable3d3V =
{{
    {0x11, 0x11, 0x18},
    {0x55, 0x55, 0x5C},
    {0xFF, 0xFF, 0xFF},
    {0x96, 0x96, 0x96},
}};

//
// MARK: - SD Pull Control Tables
//

/// A sequence of registers to transfer to enable SD pull control
const RealtekRTS5249SeriesController::ChipRegValuePair RealtekRTS5249SeriesController::kSDEnablePullControlTablePairs[] =
{
    { RTSX::PCR::Chip::CARD::PULL::rCTL1, 0x66 },
    { RTSX::PCR::Chip::CARD::PULL::rCTL2, 0xAA },
    { RTSX::PCR::Chip::CARD::PULL::rCTL3, 0xE9 },
    { RTSX::PCR::Chip::CARD::PULL::rCTL4, 0xAA },
};

const RealtekRTS5249SeriesController::SimpleRegValuePairs RealtekRTS5249SeriesController::kSDEnablePullControlTable =
{
    RealtekRTS5249SeriesController::kSDEnablePullControlTablePairs
};

/// A sequence of registers to transfer to disable SD pull control
const RealtekRTS5249SeriesController::ChipRegValuePair RealtekRTS5249SeriesController::kSDDisablePullControlTablePairs[] =
{
    { RTSX::PCR::Chip::CARD::PULL::rCTL1, 0x66 },
    { RTSX::PCR::Chip::CARD::PULL::rCTL2, 0x55 },
    { RTSX::PCR::Chip::CARD::PULL::rCTL3, 0xD5 },
    { RTSX::PCR::Chip::CARD::PULL::rCTL4, 0x55 },
};

const RealtekRTS5249SeriesController::SimpleRegValuePairs RealtekRTS5249SeriesController::kSDDisablePullControlTable =
{
    RealtekRTS5249SeriesController::kSDDisablePullControlTablePairs
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
IOReturn RealtekRTS5249SeriesController::powerOnCard()
{
    using namespace RTSX::PCR::Chip;
    
    IOReturn retVal;
    
    // Enable the overcurrent protection
    if (this->parameters.ocp.enable)
    {
        pinfo("Enabling the overcurrent protection...");
        
        retVal = this->enableOvercurrentProtection();
        
        if (retVal != kIOReturnSuccess)
        {
            perr("Failed to enable the overcurrent protection. Error = 0x%x.", retVal);
            
            return retVal;
        }
        
        pinfo("The overcurrent protection has been enabled.");
    }
    
    // Enable the card power in two phases
    // Phase 1: Partial Power
    pinfo("Powering on the card partially...");
    
    const ChipRegValuePair ppairs[] =
    {
        { CARD::rPWRCTRL, CARD::PWRCTRL::kSDPowerMask, CARD::PWRCTRL::kSDVCCPartialPowerOn },
        { rPWRGATECTRL, PWRGATECTRL::kMask, PWRGATECTRL::kVCC1 }
    };
    
    retVal = this->transferWriteRegisterCommands(SimpleRegValuePairs(ppairs));
    
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
        { CARD::rPWRCTRL, CARD::PWRCTRL::kSDPowerMask, CARD::PWRCTRL::kSDVCCPowerOn },
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
IOReturn RealtekRTS5249SeriesController::powerOffCard()
{
    using namespace RTSX::PCR::Chip;
    
    // Disable the overcurrent protection
    if (this->parameters.ocp.enable)
    {
        psoftassert(this->disableOvercurrentProtection() == kIOReturnSuccess,
                    "Failed to disable the overcurrent protection.");
    }
    
    // Power off the card
    const ChipRegValuePair pairs[] =
    {
        { CARD::rPWRCTRL, CARD::PWRCTRL::kSDPowerMask, CARD::PWRCTRL::kSDPowerOff },
        { rPWRGATECTRL, PWRGATECTRL::kMask, PWRGATECTRL::kOn }
    };
    
    return this->transferWriteRegisterCommands(SimpleRegValuePairs(pairs));
}

//
// MARK: - Hardware Initialization and Configuration
//

///
/// Initialize hardware-specific parameters
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `*_init_params()` defined in each controller file.
///
IOReturn RealtekRTS5249SeriesController::initParameters()
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
    
    this->parameters.sd30DriveSelector3d3V = RTSX::PCR::Chip::CARD::SD30::DRVSEL::CFG::kDriverTypeB;
    
    this->parameters.sd30DriveTable1d8V = &RealtekRTS5249SeriesController::kSD30DriveTable1d8V;
    
    this->parameters.sd30DriveTable3d3V = &RealtekRTS5249SeriesController::kSD30DriveTable3d3V;
    
    this->parameters.sdEnablePullControlTable = &RealtekRTS5249SeriesController::kSDEnablePullControlTable;
    
    this->parameters.sdDisablePullControlTable = &RealtekRTS5249SeriesController::kSDDisablePullControlTable;
    
    this->parameters.initialTxClockPhase = {1, 29, 16};
    
    this->parameters.initialRxClockPhase = {24, 6, 5};
    
    this->parameters.ocp.enable = false;
    
    this->parameters.pm.isASPML1Enabled = true;
    
    this->parameters.pm.enableLTRL1SSPowerGateCheckCard = true;
    
    this->parameters.pm.enableLTRL1SSPowerGate = true;
    
    this->parameters.pm.enableLTRMode = true;
    
    this->parameters.pm.ltrActiveLatency = LTR_ACTIVE_LATENCY_DEF;
    
    this->parameters.pm.ltrIdleLatency = LTR_IDLE_LATENCY_DEF;
    
    this->parameters.pm.ltrL1OffLatency = LTR_L1OFF_LATENCY_DEF;
    
    this->parameters.pm.ltrSnoozeDelay = L1_SNOOZE_DELAY_DEF;
    
    this->parameters.pm.ltrL1OffSSPowerGate = LTR_L1OFF_SSPWRGATE_5249_DEF;
    
    this->parameters.pm.ltrL1OffSnoozeSSPowerGate = LTR_L1OFF_SNOOZE_SSPWRGATE_5249_DEF;
    
    pinfo("Device-specific parameters have been initialized.");
    
    return kIOReturnSuccess;
}

///
/// Initialize vendor-specific parameters
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `fetch_vendor_settings()` defined in the `pcr->ops`.
///
IOReturn RealtekRTS5249SeriesController::initVendorSpecificParameters()
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
    
    this->parameters.sd30DriveSelector1d8V = this->vsGetSD30DriveSelector1d8V(regVal);
    
    this->parameters.cardDriveSelector &= 0x3F;
    
    this->parameters.cardDriveSelector |= this->vsGetCardDriveSelector(regVal);
    
    pinfo("PCR CFG1: RegVal = 0x%08x.", regVal);
    
    pinfo("PCR CFG1: ASPM L1 Enabled: %s.", YESNO(this->parameters.pm.isASPML1Enabled));
    
    pinfo("PCR CFG1: SD30 Drive Selector (1.8V) = %d.", this->parameters.sd30DriveSelector1d8V);
    
    pinfo("PCR CFG1: SD Card Drive Selector = 0x%x.", this->parameters.cardDriveSelector);
    
    // PCR Config 2
    regVal = this->device->configRead32(RTSX::PCR::kSREG2);
    
    this->parameters.caps.noMMCCommandsDuringInitialization = !this->vsIsMMCCommandsNeededDuringInitialiation(regVal);
    
    this->parameters.sd30DriveSelector3d3V = this->vsGetSD30DriveSelector3d3V(regVal);
    
    this->parameters.isSocketReversed = this->vsIsSocketReversed(regVal);
    
    pinfo("PCR CFG2: RegVal = 0x%08x.", regVal);
    
    pinfo("PCR CFG2: SD30 Drive Selector (3.3V) = %d.", this->parameters.sd30DriveSelector3d3V);
    
    pinfo("PCR CFG2: Socket Reversed = %s.", YESNO(this->parameters.isSocketReversed));
    
    pinfo("Vendor-specific parameters have been initialized.");
    
    return kIOReturnSuccess;
}

///
/// [Helper] Initialize the hardware from the PCI config
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rts5249_init_from_cfg()` and `rts5249_init_from_hw()` defined in `rts5249.c`.
///
IOReturn RealtekRTS5249SeriesController::initHardwareFromConfig()
{
    pinfo("Initializing the hardware from the PCIe config.");
    
    UInt32 l1ss = this->device->extendedFindPCICapability(static_cast<UInt32>(kIOPCIExpressCapabilityIDL1PMSubstates));
    
    if (l1ss == 0)
    {
        pwarning("Skipped: L1SS is not available.")
        
        return kIOReturnSuccess;
    }
    
    BitOptions regVal = this->device->configRead32(PCI_L1SS_CTL1 + l1ss);
    
    pinfo("L1SS RegVal = 0x%08x.", regVal.flatten());
    
    if (this->supportsOOBSPolling())
    {
        pinfo("Configuring the OOBS polling...");
        
        IOReturn retVal;
        
        if (!regVal.contains(PCI_L1SS_CTL1_L1SS_MASK))
        {
            pinfo("Enabling the OOBS polling...");
            
            retVal = this->enableOOBSPolling();
        }
        else
        {
            pinfo("Disabling the OOBS polling...");
            
            retVal = this->disableOOBSPolling();
        }
        
        if (retVal != kIOReturnSuccess)
        {
            perr("Failed to configure OOBS polling. Error = 0x%x.", retVal);
            
            return retVal;
        }
        
        pinfo("OOBS polling has been configured.");
    }
    
    pinfo("Setting the ASPM...");
    
    this->parameters.pm.enableASPML11 = regVal.contains(PCI_L1SS_CTL1_ASPM_L1_1);
    
    this->parameters.pm.enableASPML12 = regVal.contains(PCI_L1SS_CTL1_ASPM_L1_2);
    
    this->parameters.pm.enablePCIPML11 = regVal.contains(PCI_L1SS_CTL1_PCIPM_L1_1);
    
    this->parameters.pm.enablePCIPML12 = regVal.contains(PCI_L1SS_CTL1_PCIPM_L1_2);
    
    if (this->parameters.pm.enableLTRMode)
    {
        if (BitOptions(IOPCIeDeviceConfigRead16(this->device, PCI_EXP_DEVCTL2)).contains(PCI_EXP_DEVCTL2_LTR_EN))
        {
            this->parameters.pm.isLTRModeEnabled = true;
            
            this->parameters.pm.isLTRModeActive = true;
            
            psoftassert(this->setLTRLatency(this->parameters.pm.ltrActiveLatency) == kIOReturnSuccess,
                        "Failed to set the LTR latency for the active mode.");
            
            pinfo("LTR mode is enabled and active.");
        }
        else
        {
            this->parameters.pm.isLTRModeEnabled = false;
            
            pinfo("LTR mode is disabled.");
        }
    }
    
    this->parameters.pm.forceClockRequest = !(this->parameters.pm.enableASPML11  &&
                                              this->parameters.pm.enableASPML12  &&
                                              this->parameters.pm.enablePCIPML11 &&
                                              this->parameters.pm.enablePCIPML12);
    
    pinfo("Force Clock Request = %s.", YESNO(this->parameters.pm.forceClockRequest));
    
    pinfo("Initialized the hardware from the PCIe config.");
    
    return kIOReturnSuccess;
}

///
/// Initialize the hardware (Extra Part)
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `extra_init_hw()` defined in defined in the `pcr->ops`.
///
IOReturn RealtekRTS5249SeriesController::initHardwareExtra()
{
    pinfo("Initializing the card reader (device-specific)...");
    
    // Initialize power management settings
    IOReturn retVal = this->initHardwareFromConfig();
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to initialize the hardware from the PCI config. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // Set up registers that are needed to initialize the hardware
    pinfo("Configuring the card reader chip (device-specific)...");
    
    ChipRegValuePair pairs[64] = {};
    
    IOItemCount count = this->initHardwareExtraGetChipRegValuePairs(pairs);
    
    retVal = this->transferWriteRegisterCommands(SimpleRegValuePairs(pairs, count));
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to write registers that are needed to initialize the hardware. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    pinfo("The card reader chip has been configured (device-specific).");
    
    // Set up the driving for 3.3V output voltage
    pinfo("Setting the driving table for the 3.3V output.")
    
    retVal = this->setDrivingForOutputVoltage(OutputVoltage::k3d3V, false, 100);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to set the driving for 3.3V output voltage. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    pinfo("Initialized the card reader (device-specific).");
    
    return kIOReturnSuccess;
}
