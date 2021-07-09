//
//  RealtekRTS5227Controller.cpp
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 6/20/21.
//

#include "RealtekRTS5227Controller.hpp"
#include "Registers.hpp"
#include "Debug.hpp"

//
// MARK: - Meta Class Definitions
//

OSDefineMetaClassAndStructors(RealtekRTS5227Controller, RealtekRTS5227SeriesController);

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
IOReturn RealtekRTS5227Controller::switchOutputVoltage(OutputVoltage outputVoltage)
{
    using namespace RTSX::PHYS;
    
    const PhysRegValuePair pairs[] =
    {
        // 1.8V only
        { rBACR, 0x3C02 },
        
        // 1.8V Tune
        { rTUNE, 0x4C80 | 0x24 },
    };
    
    IOReturn retVal;
    
    if (outputVoltage == OutputVoltage::k3d3V)
    {
        retVal = this->writePhysRegister(rTUNE, 0x4FC0 | 0x24);
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
// MARK: - Power Management
//

///
/// Power down the controller forcedly
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_base_force_power_down()` defined in `rtsx_psr.c` and `*_force_power_down()` defined in each controller file.
///
IOReturn RealtekRTS5227Controller::forcePowerDown()
{
    using namespace RTSX::Chip;
    
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
IOReturn RealtekRTS5227Controller::optimizePhys()
{
    pinfo("Optimizing the physical layer...");
    
    using namespace RTSX::Chip;
    
    pinfo("Enabling the D3 delink mode...");

    IOReturn retVal = this->writeChipRegister(PM::rCTRL3_52XA, PM::CTRL3::kEnableD3DelinkMode, 0x00);

    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to enable D3 delink mode. Error = 0x%x.", retVal);

        return retVal;
    }

    pinfo("D3 delink mode has been enabled.");
    
    return this->writePhysRegister(RTSX::PHYS::rPCR, 0xBA42);
}
