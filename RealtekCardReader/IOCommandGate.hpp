//
//  IOCommandGate.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 8/14/21.
//

#ifndef IOCommandGate_hpp
#define IOCommandGate_hpp

#include <IOKit/IOCommandGate.h>
#include "Utilities.hpp"

///
/// Run the given callable action under a gated context
///
/// @param commandGate A non-null command gate
/// @param action A callable action that takes no arguments and returns an `IOReturn` code
/// @return The value returned by the given action.
///
template <typename Action>
IOReturn IOCommandGateRunAction(IOCommandGate* commandGate, Action action)
{
    auto wrapper = Value::of(action);
    
    using Wrapper = decltype(wrapper);
    
    auto bridge = [](OSObject*, void* wrapper, void*, void*, void*) -> IOReturn
    {
        return reinterpret_cast<Wrapper*>(wrapper)->value();
    };
    
    return commandGate->runAction(bridge, &wrapper);
}

#endif /* IOCommandGate_hpp */
