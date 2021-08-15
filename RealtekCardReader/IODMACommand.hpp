//
//  IODMACommand.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 8/15/21.
//

#ifndef IODMACommand_hpp
#define IODMACommand_hpp

#include <IOKit/IODMACommand.h>
#include "IOMemoryDescriptor.hpp"

///
/// Run a custom action while the given memory descriptor is associated with the DMA command
///
/// @param command A non-null DMA command
/// @param descriptor A non-null, prepared memory descriptor
/// @param action A callable action that takes the given command as its input and returns an `IOReturn` code
/// @return The value returned by the given action, other values if failed to associate the given memory descriptor with the DMA command.
/// @note This function dissociates the given memory descriptor from the DMA command automatically after the given action returns.
/// @warning Since the given memory descriptor is prepared, it's the caller's responsibility to complete it.
/// @note Use `IODMACommandRunActionWithMemoryDescriptor()` instead with an unprepared memory descriptor.
/// @note Signature of the action: `IOReturn operator()(IODMACommand*)`.
///
template <typename Action>
IOReturn IODMACommandRunActionWithPreparedMemoryDescriptor(IODMACommand* command, IOMemoryDescriptor* descriptor, Action action)
{
    // Guard: Associate the memory descriptor with the DMA command
    IOReturn retVal = command->setMemoryDescriptor(descriptor);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to associate the given memory descriptor with the DMA command. Error = 0x%08x.", retVal);
        
        return retVal;
    }
    
    // Run the given action
    retVal = action(command);
    
    // Cleanup
    psoftassert(command->clearMemoryDescriptor() == kIOReturnSuccess,
                "Failed to dissociate the memory descriptor from the DMA command.");
    
    return retVal;
}

///
/// Run a custom action while the given memory descriptor is associated with the DMA command
///
/// @param command A non-null DMA command
/// @param descriptor A non-null, unprepared memory descriptor
/// @param action A callable action that takes the given command as its input and returns an `IOReturn` code
/// @return The value returned by the given action, other values if failed to associate the given memory descriptor with the DMA command.
/// @note This function dissociates the given memory descriptor from the DMA command automatically after the given action returns.
/// @warning Since the given memory descriptor is unprepared, this function completes it after the given action returns.
/// @note Signature of the action: `IOReturn operator()(IODMACommand*)`.
///
template <typename Action>
IOReturn IODMACommandRunActionWithMemoryDescriptor(IODMACommand* command, IOMemoryDescriptor* descriptor, Action action)
{
    auto bridge = [&](IOMemoryDescriptor* preparedDescriptor) -> IOReturn
    {
        return IODMACommandRunActionWithPreparedMemoryDescriptor(command, preparedDescriptor, action);
    };
    
    return IOMemoryDescriptorRunActionWhilePrepared(descriptor, bridge);
}

#endif /* IODMACommand_hpp */
