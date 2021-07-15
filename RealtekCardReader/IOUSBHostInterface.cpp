//
//  IOUSBHostInterface.cpp
//  RealtekCardReader
//
//  Created by FireWolf on 7/11/21.
//

#include "IOUSBHostInterface.hpp"
#include "Debug.hpp"

///
/// Find the pipe that represents an endpoint of the given type and direction
///
/// @param interface The host interface
/// @param type The endpoint type
/// @param direction The endpoint direction
/// @return A non-null host pipe on success, `nullptr` if the requested endpoint does not exist.
///
IOUSBHostPipe* IOUSBHostInterfaceFindPipe(IOUSBHostInterface* interface, UInt8 type, UInt8 direction)
{
    pinfo("Finding the pipe of type = %u and direction = %u.", type, direction);
    
    // Guard: Get the config descriptor
    const StandardUSB::ConfigurationDescriptor* configDescriptor = interface->getConfigurationDescriptor();
    
    if (configDescriptor == nullptr)
    {
        perr("Failed to retrieve the configuration descriptor.");
        
        return nullptr;
    }
    
    // Guard: Get the interface descriptor
    const StandardUSB::InterfaceDescriptor* interfaceDescriptor = interface->getInterfaceDescriptor();
    
    if (interfaceDescriptor == nullptr)
    {
        perr("Failed to retrieve the interface descriptor.");
        
        return nullptr;
    }
    
    // Iterate over all endpoints
    const EndpointDescriptor* endpointDescriptor = nullptr;
    
    while (true)
    {
        // Guard: Get the next endpoint descriptor
        endpointDescriptor = StandardUSB::getNextEndpointDescriptor(configDescriptor, interfaceDescriptor, endpointDescriptor);
        
        if (endpointDescriptor == nullptr)
        {
            pinfo("Cannot find the next endpoint descriptor.");
            
            return nullptr;
        }
        
        // Guard: Examine the endpoint type
        if (StandardUSB::getEndpointType(endpointDescriptor) != type)
        {
            pinfo("Found an endpoint but the type is not the requested one.");
            
            continue;
        }
        
        // Guard: Examine the endpoint direction
        if (StandardUSB::getEndpointDirection(endpointDescriptor) != direction)
        {
            pinfo("Found an endpoint but the direction is not the requested one.");
            
            continue;
        }
        
        // Found the requested endpoint
        UInt8 address = StandardUSB::getEndpointAddress(endpointDescriptor);
        
        pinfo("Found the endpoint at address = 0x%x.", address);
        
        return interface->copyPipe(address);
    }
}

///
/// Perform a custom action with an intermediate buffer that is suitable for I/O transfer
///
/// @param interface The host interface
/// @param direction Specify the direction of the intermediate buffer
/// @param capacity Specify the capacity of the intermediate buffer
/// @param action A non-null action that manipulates the intermediate buffer under the given `context`
/// @param context A nullable user-defined context to be passed to the given `action`
/// @return `kIOReturnBadArgument` if the given action is null,
///         `kIOReturnNoMemory` if failed to allocate the intermediate buffer,
///         otherwise the return value of the given action routine.
/// @note This function uses `IOUSBHostInterface::createIOBuffer()` to allocate the intermediate buffer.
///
IOReturn IOUSBHostInterfaceWithIntermediateIOBuffer(IOUSBHostInterface* interface, IODirection direction, IOByteCount capacity, IOUSBHostInterfaceBufferAction action, const void* context)
{
    // Guard: The given action cannot be null
    if (action == nullptr)
    {
        perr("The given action routine is null.");
        
        return kIOReturnBadArgument;
    }
    
    // Guard: Allocate the intermediate buffer
    IOBufferMemoryDescriptor* buffer = interface->createIOBuffer(direction, capacity);
    
    if (buffer == nullptr)
    {
        perr("Failed to create the intermediate buffer.");
        
        return kIOReturnNoMemory;
    }
    
    // Guard: Prepare the intermediate buffer
    if (buffer->prepare() != kIOReturnSuccess)
    {
        perr("Failed to page in and wire down the intermediate buffer.");
        
        buffer->release();
        
        return kIOReturnNoMemory;
    }
    
    // Invoke the action
    IOReturn retVal = (*action)(interface, buffer, context);
    
    // All done
    psoftassert(buffer->complete() == kIOReturnSuccess, "Failed to complete the buffer.");
    
    buffer->release();
    
    return retVal;
}
