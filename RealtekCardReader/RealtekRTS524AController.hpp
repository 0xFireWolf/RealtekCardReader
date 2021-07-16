//
//  RealtekRTS524AController.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 6/18/21.
//

#ifndef RealtekRTS524AController_hpp
#define RealtekRTS524AController_hpp

#include "RealtekRTS5249SeriesController.hpp"

///
/// Represents the RTS524A card reader controller
///
class RealtekRTS524AController: public RealtekRTS5249SeriesController
{
    //
    // MARK: - Constructors & Destructors
    //
    
    OSDeclareDefaultStructors(RealtekRTS524AController);
    
    using super = RealtekRTS5249SeriesController;
    
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
    IOReturn readPhysRegister(UInt8 address, UInt16& value) override;
    
    ///
    /// Write a word to the physical layer register at the given address
    ///
    /// @param address The register address
    /// @param value The register value
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    /// @note Port: This function replaces `rtsx_pci_write_phy_register()` defined in `rtsx_psr.c`.
    /// @note Subclasses may override this function to provide a device-specific implementation.
    ///
    IOReturn writePhysRegister(UInt8 address, UInt16 value) override;
    
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
    bool shouldEnableClockPowerManagement() override;
    
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
    /// [Helper] Get a sequence of registers needed to initialize the hardware
    ///
    /// @param pairs An array of registers to be populated
    /// @return The number of registers in the array.
    ///
    IOItemCount initHardwareExtraGetChipRegValuePairs(ChipRegValuePair (&pairs)[64]) override;
};

#endif /* RealtekRTS524AController_hpp */
