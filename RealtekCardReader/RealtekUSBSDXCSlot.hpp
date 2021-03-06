//
//  RealtekUSBSDXCSlot.hpp
//  RealtekCardReader
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
    // MARK: - SD Request Processors
    //
       
    ///
    /// Preprocess the given SD command request
    ///
    /// @param request A SD command request
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    ///
    IOReturn preprocessRequest(IOSDHostRequest& request) override;
    
    ///
    /// Postprocess the given SD command request
    ///
    /// @param request A SD command request
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces the code block that flushes the FIFO buffer in `sdmmc_request()` defined in `rtsx_usb_sdmmc.c`.
    ///
    IOReturn postprocessRequest(IOSDHostRequest& request) override;
    
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
    
    //
    // MARK: - IOService Implementations
    //
    
public:
    ///
    /// Initialize the host device
    ///
    /// @return `true` on success, `false` otherwise.
    ///
    bool init(OSDictionary* dictionary = nullptr) override;
};

#endif /* RealtekUSBSDXCSlot_hpp */
