//
//  WolfsSDXC.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 6/10/21.
//

#ifndef WolfsSDXC_hpp
#define WolfsSDXC_hpp

#include <IOKit/IOService.h>

///
/// An empty class from which the card reader controller inherits,
/// so the system profiler can see the card reader and list it.
///
/// @see /System/Library/SystemProfiler/SPCardReaderReporter.spreporter
///
class WolfsSDXC: public IOService
{
    //
    // MARK: - Constructors & Destructors
    //
    
    OSDeclareDefaultStructors(WolfsSDXC);
    
    using super = IOService;
};

#endif /* WolfsSDXC_hpp */
