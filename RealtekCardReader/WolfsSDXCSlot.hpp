//
//  WolfsSDXCSlot.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 6/10/21.
//

#ifndef WolfsSDXCSlot_hpp
#define WolfsSDXCSlot_hpp

#include "IOSDHostDevice.hpp"

///
/// An empty class from which the host device inherits,
/// so the system profiler can see the card slot and list it.
///
/// @see /System/Library/SystemProfiler/SPCardReaderReporter.spreporter
///
class WolfsSDXCSlot: public IOSDHostDevice
{
    //
    // MARK: - Constructors & Destructors
    //
    
    OSDeclareAbstractStructors(WolfsSDXCSlot);
    
    using super = IOSDHostDevice;
};

#endif /* WolfsSDXCSlot_hpp */
