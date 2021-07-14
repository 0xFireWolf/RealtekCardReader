//
//  RealtekRTS524AController.cpp
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 6/18/21.
//

#include "RealtekRTS524AController.hpp"

//
// MARK: - Meta Class Definitions
//

OSDefineMetaClassAndStructors(RealtekRTS524AController, RealtekRTS5249SeriesController);

//
// MARK: - Access Physical Layer Registers (Default, Overridable)
//

///
/// Read a word from the physical layer register at the given address
///
/// @param address The register address
/// @param value The register value on return
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
/// @note Port: This function replaces `rtsx_pci_read_phy_register()` defined in `rtsx_psr.c`.
/// @note Subclasses may override this function to provide a device-specific implementation.
///
IOReturn RealtekRTS524AController::readPhysRegister(UInt8 address, UInt16& value)
{
    if ((address & 0x80) != 0)
    {
        address &= 0x7F;
        
        address |= 0x40;
    }
    
    return super::readPhysRegister(address, value);
}

///
/// Write a word to the physical layer register at the given address
///
/// @param address The register address
/// @param value The register value
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
/// @note Port: This function replaces `rtsx_pci_write_phy_register()` defined in `rtsx_psr.c`.
/// @note Subclasses may override this function to provide a device-specific implementation.
///
IOReturn RealtekRTS524AController::writePhysRegister(UInt8 address, UInt16 value)
{
    if ((address & 0x80) != 0)
    {
        address &= 0x7F;
        
        address |= 0x40;
    }
    
    return super::writePhysRegister(address, value);
}

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
IOReturn RealtekRTS524AController::switchOutputVoltage(OutputVoltage outputVoltage)
{
    using namespace RTSX::PHYS;
    
    IOReturn retVal;
    
    if (outputVoltage == OutputVoltage::k3d3V)
    {
        retVal = this->appendPhysRegister(rTUNE, TUNE::kVoltageMask, TUNE::kVoltage3V3);
    }
    else
    {
        retVal = this->appendPhysRegister(rTUNE, TUNE::kVoltageMask, TUNE::kD18_1V8);
    }
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to inform the hardware the new output voltage. Error = 0x%x.", retVal);
        
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
bool RealtekRTS524AController::supportsOOBSPolling()
{
    return true;
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
IOReturn RealtekRTS524AController::forcePowerDown()
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
IOReturn RealtekRTS524AController::optimizePhys()
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
    
    const PhysRegValuePair kDefault[] =
    {
        { rPCR, PCR::kForceCode | PCR::kOOBSCali50 | PCR::kOOBSVCM08 | PCR::kOOBSSEN90 | PCR::kEnableRSSI },
        { rSSCCR3, SSCCR3::kStepIn | SSCCR3::kCheckDelay },
        { rANA8, ANA8::kRxEQDCGain | ANA8::kSelRxEnable | ANA8::kRxEQValue | ANA8::kSCP | ANA8::kSelIPI },
    };
    
    const PhysRegValuePair kRevA[] =
    {
        { rPCR, PCR::kForceCode | PCR::kOOBSCali50 | PCR::kOOBSVCM08 | PCR::kOOBSSEN90 | PCR::kEnableRSSI },
        { rSSCCR3, SSCCR3::kStepIn | SSCCR3::kCheckDelay },
        { rSSCCR2, SSCCR2::kPLLNcode | SSCCR2::kTime0 | SSCCR2::kTime2Width },
        { rANA1A, ANA1A::kTXRLoopback | ANA1A::kRXTBist | ANA1A::kTXRBist | ANA1A::kRev },
        { rANA1D, ANA1D::kDebugAddress },
        { rDIG1E, DIG1E::kRev | DIG1E::kD0XD1 | DIG1E::kRxOnHost | DIG1E::kRCLKRefHost | DIG1E::kRCLKTxEnableKeep | DIG1E::kRCLKTxTermKeep | DIG1E::kRCLKRxEIdleOn | DIG1E::kTxTermKeep | DIG1E::kRxTermKeep | DIG1E::kTxEnableKeep | DIG1E::kRxEnableKeep },
        { rANA8, ANA8::kRxEQDCGain | ANA8::kSelRxEnable | ANA8::kRxEQValue | ANA8::kSCP | ANA8::kSelIPI },
    };
    
    if (this->parameters.revision.isRevA())
    {
        return this->writePhysRegisters(kRevA);
    }
    else
    {
        return this->writePhysRegisters(kDefault);
    }
}

///
/// [Helper] Get a sequence of registers needed to initialize the hardware
///
/// @param pairs An array of registers to be populated
/// @return The number of registers in the array.
///
IOItemCount RealtekRTS524AController::initHardwareExtraGetChipRegValuePairs(ChipRegValuePair (&pairs)[64])
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

    // SPEC: ??? (Differs from 525A)
    populate(rFCTL, AFCTL::kEnableASPML1, AFCTL::kEnableASPML1);
    
    // SPEC: Debug power management events
    populate(rPEDBG, PEDBG::kDebug0, PEDBG::kDebug0);
    
    // SPEC: LDO VCC
    populate(LDO::VCC::rCFG1, LDO::VCC::CFG1::kLimitEnable, LDO::VCC::CFG1::kLimitEnable);
    
    // SPEC: ???
    populate(rPCLKCTL, PCKLCTL::kModeSelector, PCKLCTL::kModeSelector);
    
    // SPEC: Revision A
    if (this->parameters.revision.isRevA())
    {
        populate(LDO::DV18::rCFG, LDO::DV18::CFG::kSRMask, LDO::DV18::CFG::kSRDefault);
        
        populate(LDO::VCC::rCFG1, LDO::VCC::CFG1::kRefTuneMask, LDO::VCC::CFG1::kRef1d2V);
        
        populate(LDO::VIO::rCFG, LDO::VIO::CFG::kRefTuneMask, LDO::VIO::CFG::kRef1d2V);
        
        populate(LDO::VIO::rCFG, LDO::VIO::CFG::kSRMask, LDO::VIO::CFG::kSRDefault);
        
        populate(LDO::DV12S::rCFG, LDO::DV12S::CFG::kRef12TuneMask, LDO::DV12S::CFG::kRef12TuneDefault);
        
        populate(LDO::rCTL1, LDO::CTL1::kSD40VIOTuneMask, LDO::CTL1::kSD40VIOTune1V7);
    }
    
    return count;
}
