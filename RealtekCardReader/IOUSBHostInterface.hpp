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

///
/// Find the pipe that represents an endpoint of the given type and direction
///
/// @param interface The host interface
/// @param type The endpoint type
/// @param direction The endpoint direction
/// @return A non-null host pipe on success, `nullptr` if the requested endpoint does not exist.
///
IOUSBHostPipe* IOUSBHostInterfaceFindPipe(IOUSBHostInterface* interface, UInt8 type, UInt8 direction);

/// Type of an action that manipulates with the given intermediate I/O buffer under a user-defined context
/// The action routine does not need to release the given intermediate buffer
using IOUSBHostInterfaceBufferAction = IOReturn (*)(IOUSBHostInterface*, IOMemoryDescriptor*, const void*);

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
IOReturn IOUSBHostInterfaceWithIntermediateIOBuffer(IOUSBHostInterface* interface, IODirection direction, IOByteCount capacity, IOUSBHostInterfaceBufferAction action, const void* context = nullptr);

#endif /* IOUSBHostInterface_hpp */
