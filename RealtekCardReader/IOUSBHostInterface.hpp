//
//  IOUSBHostInterface.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 7/11/21.
//

#ifndef IOUSBHostInterface_hpp
#define IOUSBHostInterface_hpp

// Get rid of a bunch of documentation warnings in the USB headers
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#include <IOKit/usb/IOUSBHostInterface.h>
#pragma clang diagnostic pop
#include "IOMemoryDescriptor.hpp"
#include "Debug.hpp"
#include "Utilities.hpp"

///
/// Find the pipe that represents an endpoint of the given type and direction
///
/// @param interface The host interface
/// @param type The endpoint type
/// @param direction The endpoint direction
/// @return A non-null host pipe on success, `nullptr` if the requested endpoint does not exist.
///
IOUSBHostPipe* IOUSBHostInterfaceFindPipe(IOUSBHostInterface* interface, UInt8 type, UInt8 direction);

///
/// Perform a custom action with an intermediate buffer that is suitable for I/O transfer
///
/// @param interface The host interface
/// @param direction Specify the direction of the intermediate buffer
/// @param capacity Specify the capacity of the intermediate buffer
/// @param action A callable action that takes the host interface and the intermediate buffer as input and returns an `IOReturn` code
/// @return `kIOReturnNoMemory` if failed to allocate the intermediate buffer, otherwise the return value of the given action routine.
/// @note This function uses `IOUSBHostInterface::createIOBuffer()` to allocate the intermediate buffer.
/// @note This function completes and releases the intermediate buffer after the given action returns,
///       so the caller should not use the intermediate buffer once the action routine returns.
/// @note Signature of the action: `IOReturn operator()(IOUSBHostInterface*, IOMemoryDescriptor*)`.
///
template <typename Action>
IOReturn IOUSBHostInterfaceWithIntermediateIOBuffer(IOUSBHostInterface* interface, IODirection direction, IOByteCount capacity, Action action)
{
    // Guard: Allocate the intermediate buffer
    IOBufferMemoryDescriptor* buffer = interface->createIOBuffer(direction, capacity);
    
    if (buffer == nullptr)
    {
        perr("Failed to create the intermediate buffer.");
        
        return kIOReturnNoMemory;
    }
    
    // Invoke the given action while the intermediate buffer is prepared
    // Control Flow:
    // 1) Page in and wire down the intermediate buffer
    // 2) Run the given action
    // 3) Complete the intermediate buffer
    auto bridge = [&](IOMemoryDescriptor* preparedDescriptor) -> IOReturn
    {
        return action(interface, preparedDescriptor);
    };
    
    IOReturn retVal = IOMemoryDescriptorRunActionWhilePrepared(buffer, bridge);
    
    // Cleanup
    buffer->release();
    
    return retVal;
}

#endif /* IOUSBHostInterface_hpp */
