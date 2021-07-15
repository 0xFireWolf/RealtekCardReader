//
//  RealtekRTS525AController.cpp
//  RealtekCardReader
//
//  Created by FireWolf on 2/17/21.
//

#include "RealtekRTS525AController.hpp"

//
// MARK: - Meta Class Definitions
//

OSDefineMetaClassAndStructors(RealtekRTS525AController, RealtekRTS5249SeriesController);

//
// MARK: - Card Power Management
//

///
/// Power on the SD card
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_pci_card_power_on()` defined in `rtsx_psr.c`.
///
IOReturn RealtekRTS525AController::powerOnCard()
{
    using namespace RTSX::PCR::Chip::LDO::VCC;
    
    auto retVal = this->writeChipRegister(rCFG1, CFG1::kTuneMask, CFG1::k3d3V);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to set the VCC to 3.3V. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    return super::powerOnCard();
}

///
/// Switch to the given output voltage
///
/// @param outputVoltage The new output voltage
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_pci_switch_output_voltage()` defined in `rtsx_psr.c`.
///
IOReturn RealtekRTS525AController::switchOutputVoltage(OutputVoltage outputVoltage)
{
    using namespace RTSX::PCR::Chip;
    
    ChipRegValuePair pairs[] =
    {
        { LDO::rCFG2, LDO::CFG2::kD3318Mask, LDO::CFG2::kD33183d3V },
        
        { SD::rPADCTL, SD::PADCTL::kUse1d8V, 0 }
    };
    
    if (outputVoltage == OutputVoltage::k1d8V)
    {
        pairs[0].value = LDO::CFG2::kD33181d8V;
        
        pairs[1].value = SD::PADCTL::kUse1d8V;
    }
    
    auto retVal = this->writeChipRegisters(SimpleRegValuePairs(pairs));
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to inform the hardware the new output voltage level. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    return this->setDrivingForOutputVoltage(outputVoltage, false, 100);
}

//
// MARK: - Hardware Initialization and Configuration
//

///
/// Optimize the physical layer
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
///
IOReturn RealtekRTS525AController::optimizePhys()
{
    pinfo("Optimizing the physical layer...");
    
    using namespace RTSX::PCR::Chip;
    
    pinfo("Enabling the D3 delink mode...");

    IOReturn retVal = this->writeChipRegister(PM::rCTRL3_52XA, PM::CTRL3::kEnableD3DelinkMode, 0x00);

    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to enable D3 delink mode. Error = 0x%x.", retVal);

        return retVal;
    }

    pinfo("D3 delink mode has been enabled.");
    
    using namespace RTSX::PHYS;

    const PhysRegValuePair pairs[] =
    {
        { rFLD0_525A, FLD0_525A::kClkReq20c | FLD0_525A::kRxIdleEnable | FLD0_525A::kBitErrRstn | FLD0_525A::kBerCount | FLD0_525A::kBerTimer | FLD0_525A::kCheckEnable },
        { rANA3, ANA3::kTimerMax  | ANA3::kOOBSDebugEnable | ANA3::kCMUDebugEnable },
        { rREV0, REV0::kFilterOut | REV0::kCDRBypassPFD | REV0::kCDRRxIdleBypass }
    };

    if (this->parameters.revision.isRevA())
    {
        return this->writePhysRegisters(pairs);
    }
    else
    {
        return this->writePhysRegisters(pairs, 2);
    }
    
    return kIOReturnSuccess;
}

///
/// Initialize hardware-specific parameters
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `*_init_params()` defined in each controller file.
///
IOReturn RealtekRTS525AController::initParameters()
{
    pinfo("Initializing the device-specific parameters...");
    
    IOReturn retVal = super::initParameters();
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to initialize common parameters. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    this->parameters.initialTxClockPhase = {25, 29, 11};
    
    this->parameters.pm.ltrL1OffSSPowerGate = LTRDefaults::kL1OffSSPowerGate_525A;
    
    this->parameters.pm.ltrL1OffSnoozeSSPowerGate = LTRDefaults::kL1OffSnoozeSSPowerGate_525A;
    
    this->parameters.ocp.enable = true;
    
    this->parameters.ocp.sdGlitch = RTSX::PCR::Chip::OCP::GLITCH::kSDValue10M;
    
    this->parameters.ocp.sdTHD800mA = RTSX::PCR::Chip::OCP::PARA2::kSDThdValue800_525A;
    
    pinfo("Device-specific parameters have been initialized.");
    
    return kIOReturnSuccess;
}

//
// MARK: - OOBS Polling
//

///
/// Check whether the driver needs to modify the PHY::RCR0 register to enable or disable OOBS polling
///
/// @return `true` if the controller is not RTS525A nor RTS5260.
/// @note RTS525A and RTS5260 controllers should override this function and return `false`.
///
bool RealtekRTS525AController::oobsPollingRequiresPhyRCR0Access()
{
    return false;
}

///
/// Check whether the hardware supports OOBS polling
///
/// @return `true` if supported, `false` otherwise.
/// @note By default, this function returns `false`.
/// @note e.g., RTS522A, RTS524A and RTS525A should override this function and return `true`.
///
bool RealtekRTS525AController::supportsOOBSPolling()
{
    return true;
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
IOReturn RealtekRTS525AController::setL1OffSubConfigD0(bool active)
{
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
/// Power down the controller forcedly
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_base_force_power_down()` defined in `rtsx_psr.c` and `*_force_power_down()` defined in each controller file.
///
IOReturn RealtekRTS525AController::forcePowerDown()
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
/// [Helper] Get a sequence of registers needed to initialize the hardware
///
/// @param pairs An array of registers to be populated
/// @return The number of registers in the array.
///
IOItemCount RealtekRTS525AController::initHardwareExtraGetChipRegValuePairs(ChipRegValuePair (&pairs)[64])
{
    using namespace RTSX::PCR::Chip;
 
    IOItemCount count = 0;
    
    auto populate = [&](UInt16 address, UInt8 mask, UInt8 value) -> void
    {
        pairs[count] = { address, mask, value };
        
        count += 1;
    };
    
    // COMM: Reset the L1SUB config
    populate(L1SUB::rCFG3, 0xFF, 0x00);

    // COMM: Turn on the LED
    populate(GPIO::rCTL, GPIO::CTL::kLEDMask, GPIO::CTL::kTurnOnLEDValue);

    // COMM: Reset the ASPM state
    populate(rAFCTL, 0x3F, 0x00);

    // COMM: Switch the LDO3318 source from DV33 to Card33
    populate(LDO::rPWRSEL, 0x03, 0x00);
    populate(LDO::rPWRSEL, 0x03, 0x01);

    // COMM: Set the initial LED cycle period
    populate(OLT_LED::rCTL, 0x0F, 0x02);

    // COMM: Set reversed socket
    populate(rPETXCFG, 0xB0, this->parameters.isSocketReversed ? 0xB0 : 0x80);
        
    // SPEC: ???
    populate(rVREF, VREF::kEnablePwdSuspnd, VREF::kEnablePwdSuspnd);

    // SPEC: Disable PM wake
    populate(PM::rCTRL3_52XA, PM::CTRL3::kEnablePmWake, 0x00);

    // SPEC: Disable D3 delink mode
    populate(PM::rCTRL3_52XA, PM::CTRL3::kEnableD3DelinkMode, 0x00);

    // SPEC: ???
    populate(rPFCTL_52XA, 0x30, 0x20);

    // COMM: Request clock
    populate(rPETXCFG, PETXCFT::kForceClockRequestDelinkMask, this->parameters.pm.forceClockRequest ? PETXCFT::kForceClockRequestLow : PETXCFT::kForceClockRequestHigh);

    // SPEC: Power off the EFUSE
    populate(rPFCTL_52XA, PFCTL_52XA::kEfusePowerMask, PFCTL_52XA::kEfusePowerOff);

    // SPEC: ???
    populate(rCLKCFG3_525A, CLKCFG3_525A::kMemPD, CLKCFG3_525A::kMemPD);
    
    // SPEC: ???
    populate(rPCLKCTL, PCKLCTL::kModeSelector, PCKLCTL::kModeSelector);
    
    // SPEC: Revision A
    if (this->parameters.revision.isRevA())
    {
        populate(L1SUB::rCFG2, L1SUB::CFG2::kAutoConfig, L1SUB::CFG2::kAutoConfig);
        
        populate(rRREFCFG, RREFCFG::kVBGSelectorMask, RREFCFG::kVBGSelector1V25);
        
        populate(LDO::VIO::rCFG, LDO::VIO::CFG::kTuneMask, LDO::VIO::CFG::k1d7V);
        
        populate(LDO::DV12S::rCFG, LDO::DV12S::CFG::kD12TuneMask, LDO::DV12S::CFG::kD12TuneDefault);
        
        populate(LDO::AV12S::rCFG, LDO::AV12S::CFG::kTuneMask, LDO::AV12S::CFG::kTuneDefault);
        
        populate(LDO::VCC::rCFG0, LDO::VCC::CFG0::kLMTVTHMask, LDO::VCC::CFG0::kLMTVTH2A);
        
        populate(OOBS::rCFG, OOBS::CFG::kAutoKDIS | OOBS::CFG::kValMask, 0x89);
    }
    
    return count;
}
