//
//  RealtekRTS522AController.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 6/20/21.
//

#ifndef RealtekRTS522AController_hpp
#define RealtekRTS522AController_hpp

#include "RealtekRTS5227SeriesController.hpp"

///
/// Represents the RTS522A card reader controller
///
class RealtekRTS522AController: public RealtekRTS5227SeriesController
{
    //
    // MARK: - Constructors & Destructors
    //
    
    OSDeclareDefaultStructors(RealtekRTS522AController);
    
    using super = RealtekRTS5227SeriesController;
    
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
    IOReturn switchOutputVoltage(OutputVoltage outputVoltage) override;
    
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
    bool supportsOOBSPolling() override;
    
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
    IOReturn setL1OffSubConfigD0(bool active) override;
    
    //
    // MARK: - Power Management
    //
    
    ///
    /// Power down the controller forcedly
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_base_force_power_down()` defined in `rtsx_psr.c` and `*_force_power_down()` defined in each controller file.
    ///
    IOReturn forcePowerDown() override;
    
    //
    // MARK: - Hardware Initialization and Configuration
    //
    
    ///
    /// Optimize the physical layer
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    ///
    IOReturn optimizePhys() override;
    
    ///
    /// Initialize hardware-specific parameters
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `*_init_params()` defined in each controller file.
    ///
    IOReturn initParameters() override;
    
    ///
    /// [Helper] Get a sequence of registers needed to initialize the hardware
    ///
    /// @param pairs An array of registers to be populated
    /// @return The number of registers in the array.
    ///
    IOItemCount initHardwareExtraGetChipRegValuePairs(ChipRegValuePair (&pairs)[64]) override;
};

#endif /* RealtekRTS522AController_hpp */
