//
//  RealtekRTS5260Controller.cpp
//  RealtekCardReader
//
//  Created by FireWolf on 7/15/21.
//

#include "RealtekRTS5260Controller.hpp"
#include "IOPCIeDevice.hpp"

//
// MARK: - Meta Class Definitions
//

OSDefineMetaClassAndStructors(RealtekRTS5260Controller, RealtekPCICardReaderController);

//
// MARK: - Driving Tables for RTS5260
//

/// SD 3.0 drive table (1.8V)
const RealtekRTS5260Controller::DrivingTable RealtekRTS5260Controller::kSD30DriveTable1d8V =
{{
    {0x35, 0x33, 0x33},
    {0x8A, 0x88, 0x88},
    {0xBD, 0xBB, 0xBB},
    {0x9B, 0x99, 0x99},
}};

/// SD 3.0 drive table (3.3V)
const RealtekRTS5260Controller::DrivingTable RealtekRTS5260Controller::kSD30DriveTable3d3V =
{{
    {0x11, 0x11, 0x11},
    {0x22, 0x22, 0x22},
    {0x55, 0x55, 0x55},
    {0x33, 0x33, 0x33},
}};

//
// MARK: - SD Pull Control Tables
//

/// A sequence of registers to transfer to enable SD pull control
const RealtekRTS5260Controller::ChipRegValuePair RealtekRTS5260Controller::kSDEnablePullControlTablePairs[] =
{
    { RTSX::PCR::Chip::CARD::PULL::rCTL1, 0x66 },
    { RTSX::PCR::Chip::CARD::PULL::rCTL2, 0xAA },
    { RTSX::PCR::Chip::CARD::PULL::rCTL3, 0xE9 },
    { RTSX::PCR::Chip::CARD::PULL::rCTL4, 0xAA },
};

const RealtekRTS5260Controller::SimpleRegValuePairs RealtekRTS5260Controller::kSDEnablePullControlTable =
{
    RealtekRTS5260Controller::kSDEnablePullControlTablePairs
};

/// A sequence of registers to transfer to disable SD pull control
const RealtekRTS5260Controller::ChipRegValuePair RealtekRTS5260Controller::kSDDisablePullControlTablePairs[] =
{
    { RTSX::PCR::Chip::CARD::PULL::rCTL1, 0x66 },
    { RTSX::PCR::Chip::CARD::PULL::rCTL2, 0x55 },
    { RTSX::PCR::Chip::CARD::PULL::rCTL3, 0xD5 },
    { RTSX::PCR::Chip::CARD::PULL::rCTL4, 0x55 },
};

const RealtekRTS5260Controller::SimpleRegValuePairs RealtekRTS5260Controller::kSDDisablePullControlTable =
{
    RealtekRTS5260Controller::kSDDisablePullControlTablePairs
};

//
// MARK: - Clear Error
//

///
/// Clear any transfer error on the host side
///
/// @note This function is invoked when a command or data transfer has failed.
/// @note Port: This function replaces `rtsx_pci_stop_cmd()` defined in `rtsx_psr.c`.
///             RTS5228, RTS5260 and RTS5261 controllers must override this function.
///
void RealtekRTS5260Controller::clearHostError()
{
    using namespace RTSX::MMIO;
    
    this->writeRegister32(rHCBCTLR, HCBCTLR::kStopCommand);
    
    this->writeRegister32(rHDBCTLR, HDBCTLR::kStopDMA);
    
    using namespace RTSX::PCR::Chip;
    
    const ChipRegValuePair pairs[] =
    {
        // Reset DMA
        { DMA::rRSTCTL0, DMA::RSTCTL0::kResetAllMask, DMA::RSTCTL0::kResetAll },
        
        // Flush the ring buffer
        { RBUF::rCTL, RBUF::CTL::kFlushMask, RBUF::CTL::kFlushValue }
    };
    
    psoftassert(this->writeChipRegisters(SimpleRegValuePairs(pairs)) == kIOReturnSuccess,
                "Failed to stop clear the host error.");
}

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
IOReturn RealtekRTS5260Controller::turnOnLED()
{
    using namespace RTSX::PCR::Chip::GPIO;
    
    return this->writeChipRegister(rCTL0, CTL0::kLEDMask, CTL0::kTurnOnLEDValue);
}

///
/// Turn off the LED
///
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
/// @note Port: This function replaces `turn_off_led()` defined in `struct pcr_ops`.
///             The base controller class implements this function by changing the value of `GPIO_CTL`.
///             RTS5209, 5260, 5286, 5287 and 5289 controllers must override this function.
///
IOReturn RealtekRTS5260Controller::turnOffLED()
{
    using namespace RTSX::PCR::Chip::GPIO;
    
    return this->writeChipRegister(rCTL0, CTL0::kLEDMask, CTL0::kTurnOffLEDValue);
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
IOReturn RealtekRTS5260Controller::powerOnCard()
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
    
    // Power on the card
    pinfo("Powering on the card...");
    
    const ChipRegValuePair pairs1[] =
    {
        { LDO::rCFG2, LDO::CFG2::kDV331812VDD1, LDO::CFG2::kDV331812VDD1 },
        { LDO::VCC::rCFG0, LDO::VCC::CFG0::kTuneMask, LDO::VCC::CFG0::kTune3V3 },
        { LDO::VCC::rCFG1, LDO::VCC::CFG1::kLDOPowSDVDD1Mask, LDO::VCC::CFG1::kLDOPowSDVDD1On },
        { LDO::rCFG2, LDO::CFG2::kDV331812Mask, LDO::CFG2::kDV331812PowerOn },
    };
    
    retVal = this->transferWriteRegisterCommands(SimpleRegValuePairs(pairs1));
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to power on the card. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    IOSleep(20);
    
    pinfo("the card power is now on.");
    
    // Configure the card
    pinfo("Configuring the card...");
    
    const ChipRegValuePair pairs2[] =
    {
        // Setup the bus speed mode for UHS-I SDR50/104
        // Interesting. Why does the Linux driver set the timing at here.
        // The following four entries are identical to `kBusTimingTablePairsSDR50`.
        { SD::rCFG1, SD::CFG1::kModeMask | SD::CFG1::kAsyncFIFONotRST, SD::CFG1::kModeSD30 | SD::CFG1::kAsyncFIFONotRST },
        { CLK::rCTL, CLK::CTL::kLowFrequency, CLK::CTL::kLowFrequency },
        { CARD::rCLKSRC, 0xFF, CARD::CLKSRC::kCRCVarClock0 | CARD::CLKSRC::kSD30FixClock | CARD::CLKSRC::kSampleVarClock1 },
        { CLK::rCTL, CLK::CTL::kLowFrequency, 0 },
        
        // Setup the SD_CFG1 register
        { SD::rCFG1, 0xFF, SD::CFG1::kClockDivider128 | SD::CFG1::kModeSD20 },
        { SD::rSPCTL, 0xFF, SD::SPCTL::kSD20RxPosEdge },
        { SD::rPPCTL, 0xFF, 0 },
        { CARD::rSTOP, CARD::STOP::kStopSD | CARD::STOP::kClearSDError, CARD::STOP::kStopSD | CARD::STOP::kClearSDError },
        
        // Setup the SD_CFG3 register
        { SD::rCFG3, SD::CFG3::kEnableSD30ClockEnd, 0 },
        { SD::rSTOPCLK, SD::STOPCLK::kEnableConfig | SD::STOPCLK::kConfig1 | SD::STOPCLK::kConfig0, 0 },
        { CARD::rPRWMODE, CARD::PRWMODE::kEnableInfiniteMode, 0 },
    };
    
    retVal = this->transferWriteRegisterCommands(SimpleRegValuePairs(pairs2));
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to configure the card. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    return kIOReturnSuccess;
}

///
/// Power off the SD card
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_pci_card_power_off()` defined in `rtsx_psr.c`.
///
IOReturn RealtekRTS5260Controller::powerOffCard()
{
    using namespace RTSX::PCR::Chip;
    
    pinfo("Powering off the card...");
    
    this->clearHostError();
 
    IOReturn retVal = this->switchOutputVoltage(OutputVoltage::k3d3V);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to switch the output voltage to 3.3V. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    const ChipRegValuePair pairs[] =
    {
        { LDO::VCC::rCFG1, LDO::VCC::CFG1::kLDOPowSDVDD1Mask, LDO::VCC::CFG1::kLDOPowSDVDD1Off },
        { LDO::rCFG2, LDO::CFG2::kDV331812Mask, LDO::CFG2::kDV331812PowerOff },
    };
    
    retVal = this->transferWriteRegisterCommands(SimpleRegValuePairs(pairs));
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to power off the card. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    if (this->parameters.ocp.enable)
    {
        psoftassert(this->disableOvercurrentProtection() == kIOReturnSuccess, "Failed to disable the overcurrent protection.");
    }
    
    pinfo("The card power is now off.");
    
    return kIOReturnSuccess;
}

///
/// Switch to the given output voltage
///
/// @param outputVoltage The new output voltage
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_pci_switch_output_voltage()` defined in `rtsx_psr.c`.
///
IOReturn RealtekRTS5260Controller::switchOutputVoltage(OutputVoltage outputVoltage)
{
    using namespace RTSX::PCR::Chip;
    
    ChipRegValuePair pairs[] =
    {
        // Select the VDD
        { LDO::rCFG2, LDO::CFG2::kDV331812VDD1, LDO::CFG2::kDV331812VDD1 },
        
        // 3.3V
        { LDO::DV18::rCFG, LDO::DV18::CFG::kDV331812Mask, LDO::DV18::CFG::kDV331812_3V3 },
        { SD::rPADCTL, SD::PADCTL::kUse1d8V, 0 },
    };
    
    if (outputVoltage == OutputVoltage::k1d8V)
    {
        pairs[1].value = LDO::DV18::CFG::kDV331812_1V7;
        
        pairs[2].value = SD::PADCTL::kUse1d8V;
    }
    
    IOReturn retVal = this->transferWriteRegisterCommands(SimpleRegValuePairs(pairs));
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to inform the hardware the new output voltage level. Error = 0x%x.", retVal);
        
        return retVal;
    }    
    
    return this->setDrivingForOutputVoltage(outputVoltage, false, 100);
}

//
// MARK: - Overcurrent Protection Support
//

///
/// Initialize and enable overcurrent protection
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_pci_init_ocp()` defined in `rtsx_psr.c`.
///             RTS5228, RTS5260 and RTS5261 controllers must override this function.
///
IOReturn RealtekRTS5260Controller::initOvercurrentProtection()
{
    using namespace RTSX::PCR::Chip;
    
    pinfo("Initializing the overcurrent protection...");
    
    if (!this->parameters.ocp.enable)
    {
        pinfo("The device specifies not to enable overcurrent protection.");
        
        return this->writeChipRegister(rDVCCCTL, DVCCCTL::kEnableOCP | DVCCCTL::kEnableOCPCL, 0);
    }
    
    const ChipRegValuePair pairs[] =
    {
        { rDVCCCTL, DVCCCTL::kOCPThdMask, this->parameters.ocp.sdTHD800mA },
        { rDV331812CTL, DV331812CTL::kOCPThdMask, DV331812CTL::kOCPThd270 },
        { OCP::rGLITCH, OCP::GLITCH::kSDMask, this->parameters.ocp.sdGlitch },
        { rDVCCCTL, DVCCCTL::kEnableOCP | DVCCCTL::kEnableOCPCL, DVCCCTL::kEnableOCP | DVCCCTL::kEnableOCPCL },
    };
    
    return this->transferWriteRegisterCommands(SimpleRegValuePairs(pairs));
}

///
/// Enable overcurrent protection
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_pci_enable_ocp()` defined in `rtsx_psr.c`.
///             RTS5228, RTS5260 and RTS5261 controllers must override this function.
///
IOReturn RealtekRTS5260Controller::enableOvercurrentProtection()
{
    using namespace RTSX::PCR::Chip;
    
    return this->writeChipRegister(OCP::rCTL, OCP::CTL::kSDDetectionAndInterruptMask, OCP::CTL::kSDEnableDetectionAndInterruptValue);
}

///
/// Disable overcurrent protection
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_pci_disable_ocp()` defined in `rtsx_psr.c`.
///             RTS5228, RTS5260 and RTS5261 controllers must override this function.
///
IOReturn RealtekRTS5260Controller::disableOvercurrentProtection()
{
    using namespace RTSX::PCR::Chip;
    
    return this->writeChipRegister(OCP::rCTL, OCP::CTL::kSDDetectionAndInterruptMask, OCP::CTL::kSDDisableDetectionAndInterruptValue);
}

///
/// Clear the overcurrent protection status
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_pci_clear_ocpstat()` defined in `rtsx_psr.c`.
///             RTS5228, RTS5260 and RTS5261 controllers must override this function.
///
IOReturn RealtekRTS5260Controller::clearOvercurrentProtectionStatus()
{
    using namespace RTSX::PCR::Chip;
    
    const ChipRegValuePair pairs1[] =
    {
        {
            OCP::rCTL,
            OCP::CTL::kSDClearStatMask  | OCP::CTL::kSDClearInterruptMask,
            OCP::CTL::kSDClearStatValue | OCP::CTL::kSDClearInterruptValue
        },
        {
            OCP::DV3318::rCTL,
            OCP::DV3318::CTL::kClearInterrupt | OCP::DV3318::CTL::kClear,
            OCP::DV3318::CTL::kClearInterrupt | OCP::DV3318::CTL::kClear
        },
    };
    
    const ChipRegValuePair pairs2[] =
    {
        {
            OCP::rCTL,
            OCP::CTL::kSDClearStatMask  | OCP::CTL::kSDClearInterruptMask,
            0
        },
        {
            OCP::DV3318::rCTL,
            OCP::DV3318::CTL::kClearInterrupt | OCP::DV3318::CTL::kClear,
            0
        },
    };
    
    psoftassert(this->transferWriteRegisterCommands(SimpleRegValuePairs(pairs1)) == kIOReturnSuccess, "Failed to clear the OCP status [Stage 1].");
    
    IODelay(10);
    
    psoftassert(this->transferWriteRegisterCommands(SimpleRegValuePairs(pairs2)) == kIOReturnSuccess, "Failed to clear the OCP status [Stage 2].");
    
    return kIOReturnSuccess;
}

///
/// Check whether the controller should power off the card when it receives an OCP interrupt
///
/// @return `true` if the controller should take necessary actions to protect the card, `false` otherwise.
/// @note Port: This function replaces the code block that examines the OCP status in `rtsx_pci_process_ocp()` defined in `rtsx_psr.c`.
///             RTS5260 controller must override this function.
/// @note This function runs in a gated context and is invoked by the OCP interrupt service routine.
///
bool RealtekRTS5260Controller::shouldPowerOffCardOnOvercurrentInterruptGated()
{
    using namespace RTSX::PCR::Chip;
    
    UInt8 status = 0;
    
    if (this->readChipRegister(OCP::DV3318::rSTAT, status) != kIOReturnSuccess)
    {
        perr("Failed to retrieve the DV3318 status. Will use the base status only to determine the outcome.");
        
        return super::shouldPowerOffCardOnOvercurrentInterruptGated();
    }
    
    return BitOptions(status).containsOneOf(OCP::DV3318::STAT::kNow, OCP::DV3318::STAT::kEver) ||
           super::shouldPowerOffCardOnOvercurrentInterruptGated();
}

//
// MARK: - Active State Power Management
//

///
/// Set the L1 substates configuration
///
/// @param active Pass `true` to set the active state
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_set_l1off_sub_cfg_d0()` and `set_l1off_cfg_sub_d0()` defined in `rtsx_pcr.c`.
/// @note The base controller provides a default implementation that simply returns `kIOReturnSuccess`.
///
IOReturn RealtekRTS5260Controller::setL1OffSubConfigD0(bool active)
{
    // Same as RTS525A
    UInt8 regVal = 0;
    
    if (active)
    {
        // State: Run, Latency: 60us
        if (this->parameters.pm.enableASPML11)
        {
            regVal = this->parameters.pm.ltrL1OffSnoozeSSPowerGate;
        }
    }
    else
    {
        // State: L1off, Latency: 300us
        if (this->parameters.pm.enableASPML12)
        {
            regVal = this->parameters.pm.ltrL1OffSSPowerGate;
        }
    }
    
    if (this->parameters.pm.enableASPML11 || this->parameters.pm.enableASPML12)
    {
        if (this->parameters.pm.enableLTRL1SSPowerGateCheckCard)
        {
            if (this->isCardPresent())
            {
                regVal &= ~RTSX::PCR::Chip::L1SUB::CFG3::kMBIAS2Enable_5250;
            }
            else
            {
                regVal |= RTSX::PCR::Chip::L1SUB::CFG3::kMBIAS2Enable_5250;
            }
        }
    }
    
    return this->setL1OffSubConfig(regVal);
}

//
// MARK: - Power Management
//

///
/// Check if the controller should enable the clock power management
///
/// @return `true` if the driver should write 0x01 to `PM_CLK_FORCE_CTL`, `false` otherwise.
/// @note Port: This function replaces the code block that checks the device ID and writes to the register in `rtsx_pci_init_hw()` defined in `rtsx_psr.c`.
///             The base controller returns `false` by default.
///             RTS524A, RTS525A, RTS5260, RTS5261 and RTS5228 controllers should override this function and return `true`.
///
bool RealtekRTS5260Controller::shouldEnableClockPowerManagement()
{
    return true;
}

///
/// Power down the controller forcedly
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_base_force_power_down()` defined in `rtsx_psr.c` and `*_force_power_down()` defined in each controller file.
///
IOReturn RealtekRTS5260Controller::forcePowerDown()
{
    using namespace RTSX::PCR::Chip;
    
    const ChipRegValuePair pairs[] =
    {
        // Set the relink time to 0
        { rALCFG + 1, 0xFF, 0 },
        { rALCFG + 2, 0xFF, 0 },
        { rALCFG + 3, ALCFG::kRelinkTimeMask, 0 },
        
        // Disable the D3 delink mode
        { PM::rCTRL3_52XA, PM::CTRL3::kEnableD3DelinkMode, PM::CTRL3::kEnableD3DelinkMode },
        
        // Power down the SSC and OCP
        { rFPDCTL, FPDCTL::kAllPowerMask, FPDCTL::kAllPowerDownValue },
    };
    
    return this->writeChipRegisters(SimpleRegValuePairs(pairs));
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
IOReturn RealtekRTS5260Controller::initParameters()
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
    
    this->parameters.sdEnablePullControlTable = &RealtekRTS5260Controller::kSDEnablePullControlTable;
        
    this->parameters.sdDisablePullControlTable = &RealtekRTS5260Controller::kSDDisablePullControlTable;
    
    this->parameters.initialTxClockPhase = {27, 29, 11};
    
    this->parameters.initialRxClockPhase = {24, 6, 5};
    
    this->parameters.pm.isASPML1Enabled = true;
    
    this->parameters.pm.enableLTRL1SSPowerGateCheckCard = true;
    
    this->parameters.pm.enableLTRL1SSPowerGate = true;
    
    this->parameters.pm.enableLTRMode = true;
    
    this->parameters.pm.ltrActiveLatency = LTRDefaults::kActiveLatency;
    
    this->parameters.pm.ltrIdleLatency = LTRDefaults::kIdleLatency;
    
    this->parameters.pm.ltrL1OffLatency = LTRDefaults::kL1OffLatency;
    
    this->parameters.pm.ltrSnoozeDelay = LTRDefaults::kL1SnoozeDelay;
    
    this->parameters.pm.ltrL1OffSSPowerGate = LTRDefaults::kL1OffSSPowerGate_525A;
    
    this->parameters.pm.ltrL1OffSnoozeSSPowerGate = LTRDefaults::kL1OffSnoozeSSPowerGate_525A;
    
    this->parameters.ocp.enable = true;
    
    this->parameters.ocp.sdGlitch = RTSX::PCR::Chip::OCP::GLITCH::kSDValue100U | RTSX::PCR::Chip::OCP::GLITCH::kSDVIOValue800U;
    
    this->parameters.ocp.sdTHD400mA = RTSX::PCR::Chip::DVCCCTL::kOCPThd550;
    
    this->parameters.ocp.sdTHD800mA = RTSX::PCR::Chip::DVCCCTL::kOCPThd970;
    
    pinfo("Device-specific parameters have been initialized.");
    
    return kIOReturnSuccess;
}

///
/// Initialize vendor-specific parameters
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `fetch_vendor_settings()` defined in the `pcr->ops`.
///
IOReturn RealtekRTS5260Controller::initVendorSpecificParameters()
{
    // Same as RTS5249 series
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
/// [Helper] Initialize power saving related parameters
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rts5260_pwr_saving_setting()` defined in the `rts5260.c`.
///
IOReturn RealtekRTS5260Controller::initPowerSavingParameters()
{
    using namespace RTSX::PCR::Chip;
    
    // Disable the ASPM
    IOReturn retVal = this->writeChipRegister(rAFCTL, 0xFF, AFCTL::kDisableASPM);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to disable the ASPM. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    if (this->parameters.pm.enableASPML12 || this->parameters.pm.enablePCIPML12)
    {
        pinfo("Setting parameters for L1.2...");
        
        const ChipRegValuePair pairs12[] =
        {
            { rPGLCTL, 0xFF, PGLCTL::kEnablePCIeL1_2 },
            { rDVCCCTL, DVCCCTL::kEnableOCP | DVCCCTL::kEnableOCPCL, DVCCCTL::kEnableOCP | DVCCCTL::kEnableOCPCL },
            { rPFECTL, 0xFF, PFECTL::kEnablePCIeL1_2PDFE },
        };
        
        retVal = this->writeChipRegisters(SimpleRegValuePairs(pairs12));
    }
    else if (this->parameters.pm.enableASPML11 || this->parameters.pm.enablePCIPML11)
    {
        pinfo("Setting parameters for L1.1...");
        
        const ChipRegValuePair pairs11[] =
        {
            { rPGLCTL, 0xFF, PGLCTL::kEnablePCIeL1_1 },
            { rPFECTL, 0xFF, PFECTL::kEnablePCIeL1_1PDFE },
        };
        
        retVal = this->writeChipRegisters(SimpleRegValuePairs(pairs11));
    }
    else
    {
        pinfo("Setting parameters for L1...");
        
        const ChipRegValuePair pairs10[] =
        {
            { rPGLCTL, 0xFF, PGLCTL::kEnablePCIeL1_0 },
            { rPFECTL, 0xFF, PFECTL::kEnablePCIeL1_0PDFE },
        };
        
        retVal = this->writeChipRegisters(SimpleRegValuePairs(pairs10));
    }
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to set the parameters specific to the current power mode. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // Parameters common to all power modes
    const ChipRegValuePair pairs[] =
    {
        { L10RV::rDPHY   , 0xFF, L10RV::kDefaultRetValue },
        { L10RV::rMAC    , 0xFF, L10RV::kDefaultRetValue },
        { L10RV::rCRCSD30, 0xFF, L10RV::kDefaultRetValue },
        { L10RV::rCRCSD40, 0xFF, L10RV::kDefaultRetValue },
        { L10RV::rSYS    , 0xFF, L10RV::kDefaultRetValue },
        
        { rAPHY0, 0xFF, APHY0::kDefault },
        { rAPHY1, 0xFF, APHY1::kDefault },
        { rAPHY2, 0xFF, APHY2::kDefault },
        { rAPHY3, 0xFF, APHY3::kDefault },
        
        { rPWCCDR, 0xFF, PWCCDR::kDefault },
        { rLFVAL, 0xFF, LFVAL::kDefault },
        { L10RV::rCRCMISC, 0xFF, L10RV::kDefaultRetValueCRCMisc },
    };
    
    return this->transferWriteRegisterCommands(SimpleRegValuePairs(pairs));
}

///
/// [Helper] Initialize the hardware from the PCI config
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rts5260_init_from_cfg()` defined in `rts5260.c`.
///
IOReturn RealtekRTS5260Controller::initHardwareFromConfig()
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
    
    pinfo("Setting the ASPM...");
    
    this->parameters.pm.enableASPML11 = regVal.contains(PCI_L1SS_CTL1_ASPM_L1_1);
    
    this->parameters.pm.enableASPML12 = regVal.contains(PCI_L1SS_CTL1_ASPM_L1_2);
    
    this->parameters.pm.enablePCIPML11 = regVal.contains(PCI_L1SS_CTL1_PCIPM_L1_1);
    
    this->parameters.pm.enablePCIPML12 = regVal.contains(PCI_L1SS_CTL1_PCIPM_L1_2);
    
    psoftassert(this->initPowerSavingParameters() == kIOReturnSuccess, "Failed to initialize power saving parameters.");
    
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
IOReturn RealtekRTS5260Controller::initHardwareExtra()
{
    pinfo("Initializing the card reader (device-specific)...");
    
    using namespace RTSX::PCR::Chip;
    
    const ChipRegValuePair pairs1[] =
    {
        // Set MCU count to 7 to ensure that data can be sampled properly
        { CLK::rDIV, 0x7F, 0x07 },
        { SSC::rDIVN0, 0xFF, 0x5D },
    };
    
    IOReturn retVal = this->writeChipRegisters(SimpleRegValuePairs(pairs1));
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to set the MCU count to 7. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // Initialize power management settings
    retVal = this->initHardwareFromConfig();
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to initialize the hardware from the PCI config. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // Set up registers that are needed to initialize the hardware
    pinfo("Configuring the card reader chip (device-specific)...");
    
    const ChipRegValuePair pairs2[] =
    {
        // Disable MIMO
        { rALCFG4, 0xFF, ALCFG4::kDisableMIMO },
        
        // Set the default tune parameter
        { LDO::VCC::rCFG0, LDO::VCC::CFG0::kTuneMask, LDO::VCC::CFG0::kTune3V3 },
        { rPCLKCTL, PCKLCTL::kModeSelector, PCKLCTL::kModeSelector },
        
        // Begin ported rts5260_init_hw()
        { L1SUB::rCFG1, L1SUB::CFG1::kAuxClockActiveSelectorMask, L1SUB::CFG1::kMacCkswDone },
        
        // Reset the L1SUB config
        { L1SUB::rCFG3, 0xFF, 0x00 },
        { rPMCLKFCTL, PMCLKFCTL::kEnableClockPM, PMCLKFCTL::kEnableClockPM },
        { rPWDSPEN, 0xFF, 0xFF },
        { rPWRGATECTRL, PWRGATECTRL::kEnable, PWRGATECTRL::kEnable },
        { rVREF, VREF::kEnablePwdSuspnd, VREF::kEnablePwdSuspnd },
        { RBUF::rCTL, RBUF::CTL::kAutoDMAMask, RBUF::CTL::kAutoDMADisable },
        
        // Set reversed socket
        { rPETXCFG, 0xB0, static_cast<UInt8>(this->parameters.isSocketReversed ? 0xB0 : 0x80) },
        { rOBFFCFG, OBFFCFG::kMask, OBFFCFG::kDisable },
        
        // End ported rts5260_init_hw()
        { rPETXCFG, PETXCFT::kForceClockRequestDelinkMask, this->parameters.pm.forceClockRequest ? PETXCFT::kForceClockRequestLow : PETXCFT::kForceClockRequestHigh },
        
        // Disable D3 delink mode
        { PM::rCTRL3_52XA, PM::CTRL3::kEnableD3DelinkMode, 0x00 }
    };
    
    return this->transferWriteRegisterCommands(SimpleRegValuePairs(pairs2));
}
