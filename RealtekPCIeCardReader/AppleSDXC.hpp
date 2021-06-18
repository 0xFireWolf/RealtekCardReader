//
//  AppleSDXC.hpp
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 6/10/21.
//

#ifndef AppleSDXC_hpp
#define AppleSDXC_hpp

#include <IOKit/IOService.h>

///
/// An empty class from which the card reader controller inherits,
/// so the system profiler can see the card reader and list it.
///
/// @see /System/Library/SystemProfiler/SPCardReaderReporter.spreporter
///
class AppleSDXC: public IOService
{
    //
    // MARK: - Constructors & Destructors
    //
    
    OSDeclareDefaultStructors(AppleSDXC);
    
    using super = IOService;
};

#endif /* AppleSDXC_hpp */
