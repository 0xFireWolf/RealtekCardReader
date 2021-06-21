//
//  RealtekRTS5289Controller.hpp
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 6/20/21.
//

#ifndef RealtekRTS5289Controller_hpp
#define RealtekRTS5289Controller_hpp

#include "RealtekRTS8411SeriesController.hpp"

///
/// Represents the RTS5289 card reader controller
///
class RealtekRTS5289Controller: public RealtekRTS8411SeriesController
{
    //
    // MARK: - Constructors & Destructors
    //
    
    OSDeclareDefaultStructors(RealtekRTS5289Controller);
    
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

#endif /* RealtekRTS5289Controller_hpp */
