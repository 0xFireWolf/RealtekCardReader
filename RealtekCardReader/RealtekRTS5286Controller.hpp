//
//  RealtekRTS5286Controller.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 6/20/21.
//

#ifndef RealtekRTS5286Controller_hpp
#define RealtekRTS5286Controller_hpp

#include "RealtekRTS8411SeriesController.hpp"

///
/// Represents the RTS5286 card reader controller
///
class RealtekRTS5286Controller: public RealtekRTS8411SeriesController
{
    //
    // MARK: - Constructors & Destructors
    //
    
    OSDeclareDefaultStructors(RealtekRTS5286Controller);
    
    using super = RealtekRTS8411SeriesController;
    
    //
    // MARK: - SD Pull Control Tables
    //
    
    /// A sequence of chip registers to enable SD pull control
    static const ChipRegValuePair kSDEnablePullControlTablePairs[];
    static const SimpleRegValuePairs kSDEnablePullControlTable;
    
    /// A sequence of chip registers to disable SD pull control
    static const ChipRegValuePair kSDDisablePullControlTablePairs[];
    static const SimpleRegValuePairs kSDDisablePullControlTable;
    
    //
    // MARK: - Card Power Management
    //
    
    ///
    /// Switch to the given output voltage
    ///
    /// @param outputVoltage The new output voltage
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_pci_switch_output_voltage()` defined in `rtsx_psr.c`.
    /// @note Port: This function replaces `rtl8411_switch_output_voltage()` definde in `rts8411.c`.
    ///             RTS5286 controller must override this function and provide the correct shift and ASIC values.
    ///
    IOReturn switchOutputVoltage(OutputVoltage outputVoltage) override;
    
    //
    // MARK: - Hardware Initialization and Configuration
    //
    
    ///
    /// Initialize hardware-specific parameters
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `*_init_params()` defined in each controller file.
    /// @note Port: This function replaces `rtl8411_init_common_params()` defined in `rts8411.c`.
    ///             Concrete controllers must override and extend this function to set the pull control tables.
    ///
    IOReturn initParameters() override;
};

#endif /* RealtekRTS5286Controller_hpp */
