//
//  RealtekRTS5249Controller.hpp
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 2/27/21.
//

#ifndef RealtekRTS5249Controller_hpp
#define RealtekRTS5249Controller_hpp

#include "RealtekRTS5249SeriesController.hpp"

///
/// Represents the RTS5249 card reader controller
///
class RealtekRTS5249Controller: public RealtekRTS5249SeriesController
{
    // MARK: - Constructors & Destructors
    OSDeclareDefaultStructors(RealtekRTS5249Controller);
    
    using super = RealtekRTS5249SeriesController;
    
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

#endif /* RealtekRTS5249Controller_hpp */
