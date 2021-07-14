//
//  RealtekUSBSDXCSlot.hpp
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 7/13/21.
//

#ifndef RealtekUSBSDXCSlot_hpp
#define RealtekUSBSDXCSlot_hpp

#include "RealtekSDXCSlot.hpp"

///
/// Represents a USB-based card slot
///
class RealtekUSBSDXCSlot: public RealtekSDXCSlot
{
    //
    // MARK: - Constructors & Destructors
    //
    
    OSDeclareDefaultStructors(RealtekUSBSDXCSlot);
    
    using super = RealtekSDXCSlot;
    
    //
    // MARK: - Tuning
    //

public:
    ///
    /// Execute the tuning algorithm
    ///
    /// @param config An I/O config that contains the current I/O settings
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `sdmmc_execute_tuning()` defined in `rtsx_usb_sdmmc.c`.
    ///
    IOReturn executeTuning(const IOSDBusConfig& config) override;
};

#endif /* RealtekUSBSDXCSlot_hpp */
