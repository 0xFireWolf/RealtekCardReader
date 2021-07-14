//
//  RealtekRTS5249Controller.cpp
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 2/27/21.
//

#include "RealtekRTS5249Controller.hpp"

//
// MARK: - Meta Class Definitions
//

OSDefineMetaClassAndStructors(RealtekRTS5249Controller, RealtekRTS5249SeriesController);

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
IOReturn RealtekRTS5249Controller::switchOutputVoltage(OutputVoltage outputVoltage)
{
    using namespace RTSX::PHYS;
    
    PhysRegMaskValuePair pairs[] =
    {
        { rBACR, BACR::kBasicMask, 0 },
        { rTUNE, TUNE::kVoltageMask, TUNE::kD18_1V7 },
    };
    
    IOReturn retVal;
    
    if (outputVoltage == OutputVoltage::k3d3V)
    {
        pairs[1].value = TUNE::kVoltage3V3;
        
        retVal = this->appendPhysRegisters(&pairs[1], 1);
    }
    else
    {
        retVal = this->appendPhysRegisters(pairs);
    }
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to inform the hardware the new output voltage. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    return this->setDrivingForOutputVoltage(outputVoltage, false, 100);
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
IOReturn RealtekRTS5249Controller::forcePowerDown()
{
    using namespace RTSX::PCR::Chip;
    
    const ChipRegValuePair pairs[] =
    {
        // Set the relink time to 0
        { rALCFG + 1, 0xFF, 0 },
        { rALCFG + 2, 0xFF, 0 },
        { rALCFG + 3, ALCFG::kRelinkTimeMask, 0 },
        
        // Disable the D3 delink mode
        { PM::rCTRL3, PM::CTRL3::kEnableD3DelinkMode, PM::CTRL3::kEnableD3DelinkMode },
        
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
IOReturn RealtekRTS5249Controller::optimizePhys()
{
    pinfo("Optimizing the physical layer...");
    
    using namespace RTSX::PCR::Chip;
    
    pinfo("Enabling the D3 delink mode...");

    IOReturn retVal = this->writeChipRegister(PM::rCTRL3, PM::CTRL3::kEnableD3DelinkMode, 0x00);

    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to enable D3 delink mode. Error = 0x%x.", retVal);

        return retVal;
    }

    pinfo("D3 delink mode has been enabled.");
    
    using namespace RTSX::PHYS;
    
    retVal = this->writePhysRegister(rREV, REV::value5249());
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to optimize the physical register REV. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    IOSleep(1);
    
    const PhysRegValuePair pairs[] =
    {
        { rBPCR, BPCR::kIBRxSel | BPCR::kIBTxSel | BPCR::kIBFilter | BPCR::kEnableCMirror },
        { rPCR, PCR::kForceCode | PCR::kOOBSCali50 | PCR::kOOBSVCM08 | PCR::kOOBSSEN90 | PCR::kEnableRSSI | PCR::kRx10K },
        { rRCR2, RCR2::kEnableEmphase | RCR2::kNADJR | RCR2::kCDRSR2 | RCR2::kFreqSel12 | RCR2::kCDRSC12P | RCR2::kCalibLate },
        { rFLD4, FLD4::kFLDENSel | FLD4::kReqRef | FLD4::kRxAmpOff | FLD4::kReqADDA | FLD4::kBerCount | FLD4::kBerTimer | FLD4::kBerCheckEnable },
        { rRDR, RDR::kRxDSEL19 | RDR::kSSCAutoPwd },
        { rRCR1, RCR1::kADPTime4 | RCR1::kVCOCoarse },
        { rFLD3, FLD3::kTimer4 | FLD3::kTimer6 | FLD3::kRxDelink },
        { rTUNE, TUNE::kTuneRef10 | TUNE::kVBGSel1252 | TUNE::kSDBus33 | TUNE::kTuneD18 | TUNE::kTuneD12 | TUNE::kTuneA12 },
    };
    
    return this->writePhysRegisters(pairs);
}

///
/// [Helper] Get a sequence of registers needed to initialize the hardware
///
/// @param pairs An array of registers to be populated
/// @return The number of registers in the array.
///
IOItemCount RealtekRTS5249Controller::initHardwareExtraGetChipRegValuePairs(ChipRegValuePair (&pairs)[64])
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

    // SPEC: Disable PM wake
    populate(PM::rCTRL3, PM::CTRL3::kEnablePmWake, 0x00);

    // SPEC: Disable D3 delink mode
    populate(PM::rCTRL3, PM::CTRL3::kEnableD3DelinkMode, 0x00);

    // SPEC: ???
    populate(rPFCTL, 0xFF, 0x30);

    // COMM: Request clock
    populate(rPETXCFG, PETXCFT::kForceClockRequestDelinkMask, this->parameters.pm.forceClockRequest ? PETXCFT::kForceClockRequestLow : PETXCFT::kForceClockRequestHigh);
    
    return count;
}
