//
//  AppleSDXCSlot.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 6/10/21.
//

#ifndef AppleSDXCSlot_hpp
#define AppleSDXCSlot_hpp

#include "IOSDHostDevice.hpp"

///
/// An empty class from which the host device inherits,
/// so the system profiler can see the card slot and list it.
///
/// @see /System/Library/SystemProfiler/SPCardReaderReporter.spreporter
///
class AppleSDXCSlot: public IOSDHostDevice
{
    //
    // MARK: - Constructors & Destructors
    //
    
    OSDeclareAbstractStructors(AppleSDXCSlot);
    
    using super = IOSDHostDevice;
};

#endif /* AppleSDXCSlot_hpp */
