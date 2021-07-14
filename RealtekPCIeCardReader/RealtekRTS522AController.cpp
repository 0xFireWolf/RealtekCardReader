//
//  RealtekRTS522AController.cpp
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 6/20/21.
//

#include "RealtekRTS522AController.hpp"
#include "Registers.hpp"
#include "Debug.hpp"

//
// MARK: - Meta Class Definitions
//

OSDefineMetaClassAndStructors(RealtekRTS522AController, RealtekRTS5227SeriesController);

//
// MARK: - Card Power Management
//

///
/// Switch to the given output voltage
///
/// @param outputVoltage The new output voltage
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_pci_switch_output_voltage()` defined in `rtsx_psr.c`.
///
IOReturn RealtekRTS522AController::switchOutputVoltage(OutputVoltage outputVoltage)
{
    using namespace RTSX::PHYS;
    
    const PhysRegValuePair pairs[] =
    {
        // 1.8V only
        { rBACR, 0x3C02 },
        
        // 1.8V Tune
        { rTUNE, 0x54A4 },
    };
    
    IOReturn retVal;
    
    if (outputVoltage == OutputVoltage::k3d3V)
    {
        retVal = this->writePhysRegister(rTUNE, 0x57E4);
    }
    else
    {
        retVal = this->writePhysRegisters(pairs);
    }
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to update the physical registers for the target voltage. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    return this->setDrivingForOutputVoltage(outputVoltage, false, 100);
}

//
// MARK: - OOBS Polling
//

///
/// Check whether the hardware supports OOBS polling
///
/// @return `true` if supported, `false` otherwise.
/// @note By default, this function returns `false`.
/// @note e.g., RTS522A, RTS524A and RTS525A should override this function and return `true`.
///
bool RealtekRTS522AController::supportsOOBSPolling()
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
IOReturn RealtekRTS522AController::setL1OffSubConfigD0(bool active)
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
IOReturn RealtekRTS522AController::forcePowerDown()
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
/// Optimize the physical layer
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
///
IOReturn RealtekRTS522AController::optimizePhys()
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
    
    if (this->parameters.revision.isRevA())
    {
        const PhysRegValuePair pairs[] =
        {
            { rRCR2, RCR2::kInit27s },
            { rRCR1, RCR1::kInit27s },
            { rFLD0, FLD0::kInit27s },
            { rFLD3, FLD3::kInit27s },
            { rFLD4, FLD4::kInit27s },
        };
        
        return this->writePhysRegisters(pairs);
    }
    
    return kIOReturnSuccess;
}

///
/// Initialize hardware-specific parameters
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `*_init_params()` defined in each controller file.
///
IOReturn RealtekRTS522AController::initParameters()
{
    pinfo("Initializing the device-specific parameters...");
    
    IOReturn retVal = super::initParameters();
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to initialize common parameters. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    this->parameters.initialTxClockPhase = {20, 20, 11};
    
    this->parameters.pm.enableLTRL1SSPowerGate = true;
    
    this->parameters.pm.enableLTRMode = true;
    
    this->parameters.pm.ltrActiveLatency = LTR_ACTIVE_LATENCY_DEF;
    
    this->parameters.pm.ltrIdleLatency = LTR_IDLE_LATENCY_DEF;
    
    this->parameters.pm.ltrL1OffLatency = LTR_L1OFF_LATENCY_DEF;
    
    this->parameters.pm.ltrSnoozeDelay = L1_SNOOZE_DELAY_DEF;
    
    this->parameters.pm.ltrL1OffSSPowerGate = LTR_L1OFF_SSPWRGATE_522A_DEF;
    
    this->parameters.pm.ltrL1OffSnoozeSSPowerGate = LTR_L1OFF_SNOOZE_SSPWRGATE_522A_DEF;
    
    this->parameters.ocp.enable = true;
    
    this->parameters.ocp.sdGlitch = RTSX::PCR::Chip::OCP::GLITCH::kSDValue10M;
    
    this->parameters.ocp.sdTHD800mA = RTSX::PCR::Chip::OCP::PARA2::kSDThdValue800_522A;
    
    pinfo("Device-specific parameters have been initialized.");
    
    return kIOReturnSuccess;
}

///
/// [Helper] Get a sequence of registers needed to initialize the hardware
///
/// @param pairs An array of registers to be populated
/// @return The number of registers in the array.
///
IOItemCount RealtekRTS522AController::initHardwareExtraGetChipRegValuePairs(ChipRegValuePair (&pairs)[64])
{
    using namespace RTSX::PCR::Chip;
    
    IOItemCount count = super::initHardwareExtraGetChipRegValuePairs(pairs);
    
    auto populate = [&](UInt16 address, UInt8 mask, UInt8 value) -> void
    {
        pairs[count] = { address, mask, value };
        
        count += 1;
    };
    
    // Adjust the register that disables the D3 delink mode
    pairs[count - 1].address = PM::rCTRL3_52XA;
    
    // 522A Specific
    populate(rFCTL, FCTL::kUpmeXmtDebug, FCTL::kUpmeXmtDebug);
    
    populate(rPCLKCTL, 0x04, 0x04);
    
    populate(rPEDBG, PEDBG::kDebug0, PEDBG::kDebug0);
    
    populate(rPMCLKFCTL, 0xFF, 0x11);
    
    return count;
}
