//
//  RealtekPCISDXCSlot.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 7/13/21.
//

#ifndef RealtekPCISDXCSlot_hpp
#define RealtekPCISDXCSlot_hpp

#include "RealtekSDXCSlot.hpp"

///
/// Represents a PCI-based card slot
///
class RealtekPCISDXCSlot: public RealtekSDXCSlot
{
    //
    // MARK: - Constructors & Destructors
    //
    
    OSDeclareDefaultStructors(RealtekPCISDXCSlot);
    
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
    /// @note Port: This function replaces `sdmmc_execute_tuning()` defined in `rtsx_pci_sdmmc.c`.
    ///
    IOReturn executeTuning(const IOSDBusConfig& config) override;
};

#endif /* RealtekPCISDXCSlot_hpp */
