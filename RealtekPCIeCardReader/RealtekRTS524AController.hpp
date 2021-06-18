//
//  RealtekRTS524AController.hpp
//  RealtekPCIeCardReader
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
    // MARK: - Constructors & Destructors
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
    // MARK: - Hardware Initialization and Configuration
    //
    
    ///
    /// Optimize the physical layer
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    ///
    virtual IOReturn optimizePhys() override;
    
    ///
    /// [Helper] Get a sequence of registers needed to initialize the hardware
    ///
    /// @param pairs An array of registers to be populated
    /// @return The number of registers in the array.
    ///
    IOItemCount initHardwareExtraGetChipRegValuePairs(ChipRegValuePair (&pairs)[64]) override;
};

#endif /* RealtekRTS524AController_hpp */
