//
//  RealtekRTS5287Controller.hpp
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 6/20/21.
//

#ifndef RealtekRTS5287Controller_hpp
#define RealtekRTS5287Controller_hpp

#include "RealtekRTS8411SeriesController.hpp"

///
/// Represents the RTS5287 card reader controller
///
class RealtekRTS5287Controller: public RealtekRTS8411SeriesController
{
    //
    // MARK: - Constructors & Destructors
    //
    
    OSDeclareDefaultStructors(RealtekRTS5287Controller);
    
    using super = RealtekRTS8411SeriesController;
    
    //
    // MARK: - SD Pull Control Tables
    //
    
    /// A sequence of chip registers to enable SD pull control (QFN48)
    static const ChipRegValuePair kSDEnablePullControlTablePairsQFN48[];
    static const SimpleRegValuePairs kSDEnablePullControlTableQFN48;
    
    /// A sequence of chip registers to disable SD pull control (QFN48)
    static const ChipRegValuePair kSDDisablePullControlTablePairsQFN48[];
    static const SimpleRegValuePairs kSDDisablePullControlTableQFN48;
    
    /// A sequence of chip registers to enable SD pull control (QFN64)
    static const ChipRegValuePair kSDEnablePullControlTablePairsQFN64[];
    static const SimpleRegValuePairs kSDEnablePullControlTableQFN64;
    
    /// A sequence of chip registers to disable SD pull control (QFN64)
    static const ChipRegValuePair kSDDisablePullControlTablePairsQFN64[];
    static const SimpleRegValuePairs kSDDisablePullControlTableQFN64;
    
    //
    // MARK: - Hardware Initialization and Configuration
    //
    
    ///
    /// Check whether the package mode is QFN48
    ///
    /// @param result Set to `true` if the package mode is QFN48 on return
    /// @return `kIOReturnSuccess` on success, other values if failed to read the register.
    ///
    IOReturn isQFN48(bool& result);
    
    ///
    /// [Helper] Extract the SD 3.0 card drive selector (3.3V) from the register value
    ///
    /// @param regVal The register value
    /// @return The SD 3.0 card drive selector (3.3V).
    /// @note RTS5287 controller must override this function.
    ///
    UInt8 vsGetSD30DriveSelector3d3V(UInt32 regVal) override
    {
        return regVal & 0x03;
    }
    
    ///
    /// Initialize hardware-specific parameters
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `*_init_params()` defined in each controller file.
    /// @note Port: This function replaces `rtl8411_init_common_params()` defined in `rts8411.c`.
    ///             Concrete controllers must override and extend this function to set the pull control tables.
    ///
    IOReturn initParameters() override;
    
    ///
    /// Initialize vendor-specific parameters
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `fetch_vendor_settings()` defined in the `pcr->ops`.
    /// @note Port: This function replaces `rtl8411_fetch_vendor_settings()` defined in `rts8411.c`.
    ///             RTS5287 controller must override this function.
    ///
    IOReturn initVendorSpecificParameters() override;
    
    ///
    /// Initialize the hardware (Extra Part)
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `extra_init_hw()` defined in defined in the `pcr->ops`.
    /// @note Port: This function replaces `rtl8411_extra_init_hw()` defined in `rts8411.c`.
    ///             RTS5287 controller must override this function.
    ///
    IOReturn initHardwareExtra() override;
};

#endif /* RealtekRTS5287Controller_hpp */
