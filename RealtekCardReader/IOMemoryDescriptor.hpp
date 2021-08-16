//
//  IOMemoryDescriptor.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 8/15/21.
//

#ifndef IOMemoryDescriptor_hpp
#define IOMemoryDescriptor_hpp

#include <IOKit/IOMemoryDescriptor.h>
#include <IOKit/IOBufferMemoryDescriptor.h>
#include "Debug.hpp"

///
/// Run the given action while the memory descriptor is prepared
///
/// @param descriptor A non-null, unprepared memory descriptor
/// @param action A callable action that takes the prepared memory descriptor as its sole argument and returns an `IOReturn` code
/// @return The value returned by the given action, other values if failed to prepare the given memory descriptor.
/// @warning The given memory descriptor becomes unprepared after the given action routine returns.
///
template <typename Action>
IOReturn IOMemoryDescriptorRunActionWhilePrepared(IOMemoryDescriptor* descriptor, Action action)
{
    // Guard: Page in and wire down the given memory descriptor
    IOReturn retVal = descriptor->prepare();
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to prepare the given memory descriptor.");
        
        return retVal;
    }
    
    // Run the given action
    retVal = action(descriptor);
    
    // Complete the memory descriptor
    psoftassert(descriptor->complete() == kIOReturnSuccess,
                "Failed to complete the given memory descriptor.");
    
    return retVal;
}

///
/// Run a custom action with an intermediate buffer that contains the content of the given memory descriptor
///
/// @param descriptor A non-null, prepared memory descriptor as the data source
/// @param offset Specify the offset into the memory descriptor's memory
/// @param length Specify the number of bytes to copy from the memory descriptor's memory
/// @param action A callable action that takes a non-null buffer as its sole argument and returns an `IOReturn` code
/// @return The value returned by the given callable action, other values if failed to read from the given memory descriptor at the beginning.
/// @note Signature of the action: `IOReturn operator()(const UInt8*)`.
/// @warning The caller should not reuse the intermediate buffer after the given action routine returns.
/// @warning The caller must ensure that the given memory descriptor is prepared.
///          Use `IOMemoryDescriptorPrepareWithIntermediateSourceBuffer()` with an unprepared memory descriptor instead.
///
template <typename Action>
IOReturn IOMemoryDescriptorWithIntermediateSourceBuffer(IOMemoryDescriptor* descriptor, IOByteCount offset, IOByteCount length, Action action)
{
    // Guard: Allocate the intermediate buffer
    UInt8* buffer = reinterpret_cast<UInt8*>(IOMalloc(length));
    
    if (buffer == nullptr)
    {
        perr("Failed to allocate the intermediate buffer.");
        
        return kIOReturnNoMemory;
    }
    
    // Guard: Copy the data from the memory descriptor
    IOReturn retVal = kIOReturnSuccess;
    
    if (descriptor->readBytes(offset, buffer, length) == length)
    {
        retVal = action(static_cast<const UInt8*>(buffer));
        
        pinfo("The given action routine returns 0x%08x.", retVal);
    }
    else
    {
        retVal = kIOReturnInvalid;
        
        perr("Failed to copy from the given memory descriptor to the intermediate buffer.");
    }
    
    // Cleanup
    IOFree(buffer, length);
    
    return retVal;
}

///
/// Run a custom action with an intermediate buffer that will be written to the given memory descriptor
///
/// @param descriptor A non-null, prepared memory descriptor as the data destination
/// @param offset Specify the offset into the memory descriptor's memory
/// @param length Specify the number of bytes to copy to the memory descriptor's memory
/// @param action A callable action that takes a non-null buffer as its sole argument and returns an `IOReturn` code
/// @return The value returned by the given callable action, other values if failed to write to the given memory descriptor at the end.
/// @note Signature of the action: `IOReturn operator()(UInt8*)`.
/// @warning The caller should not reuse the intermediate buffer after the given action routine returns.
/// @warning The caller must ensure that the given memory descriptor is prepared.
///          Use `IOMemoryDescriptorPrepareWithIntermediateDestinationBuffer()` with an unprepared memory descriptor instead.
///
template <typename Action>
IOReturn IOMemoryDescriptorWithIntermediateDestinationBuffer(IOMemoryDescriptor* descriptor, IOByteCount offset, IOByteCount length, Action action)
{
    // Guard: Allocate the intermediate buffer
    UInt8* buffer = reinterpret_cast<UInt8*>(IOMalloc(length));
    
    if (buffer == nullptr)
    {
        perr("Failed to allocate the intermediate buffer.");
        
        return kIOReturnNoMemory;
    }
    
    // Run the given action
    IOReturn retVal = action(buffer);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("The given action returns an error code 0x%08x.", retVal);
        
        IOFree(buffer, length);
        
        return retVal;
    }
    
    // Guard: Copy the data back to the descriptor
    if (descriptor->writeBytes(offset, buffer, length) != length)
    {
        perr("Failed to copy from the intermediate buffer back to the given memory descriptor.")
        
        retVal = kIOReturnError;
    }
    else
    {
        retVal = kIOReturnSuccess;
    }
    
    // Cleanup
    IOFree(buffer, length);
    
    return retVal;
}

///
/// Run a custom action with an intermediate buffer that contains the content of the given memory descriptor
///
/// @param descriptor A non-null, unprepared memory descriptor as the data source
/// @param offset Specify the offset into the memory descriptor's memory
/// @param length Specify the number of bytes to copy from the memory descriptor's memory
/// @param action A callable action that takes a non-null buffer as its sole argument and returns an `IOReturn` code
/// @return The value returned by the given callable action, other values if failed to read from the given memory descriptor at the beginning.
/// @note Signature of the action: `IOReturn operator()(const UInt8*)`.
/// @warning The caller should not reuse the intermediate buffer after the given action routine returns.
///
template <typename Action>
IOReturn IOMemoryDescriptorPrepareWithIntermediateSourceBuffer(IOMemoryDescriptor* descriptor, IOByteCount offset, IOByteCount length, Action action)
{
    auto bridge = [&](IOMemoryDescriptor* preparedDescriptor) -> IOReturn
    {
        return IOMemoryDescriptorWithIntermediateSourceBuffer(preparedDescriptor, offset, length, action);
    };
    
    return IOMemoryDescriptorRunActionWhilePrepared(descriptor, bridge);
}

///
/// Run a custom action with an intermediate buffer that will be written to the given memory descriptor
///
/// @param descriptor A non-null, unprepared memory descriptor as the data destination
/// @param offset Specify the offset into the memory descriptor's memory
/// @param length Specify the number of bytes to copy to the memory descriptor's memory
/// @param action A callable action that takes a non-null buffer as its sole argument and returns an `IOReturn` code
/// @return The value returned by the given callable action, other values if failed to write to the given memory descriptor at the end.
/// @note Signature of the action: `IOReturn operator()(UInt8*)`.
/// @warning The caller should not reuse the intermediate buffer after the given action routine returns.
///
template <typename Action>
IOReturn IOMemoryDescriptorPrepareWithIntermediateDestinationBuffer(IOMemoryDescriptor* descriptor, IOByteCount offset, IOByteCount length, Action action)
{
    auto bridge = [&](IOMemoryDescriptor* preparedDescriptor) -> IOReturn
    {
        return IOMemoryDescriptorWithIntermediateDestinationBuffer(preparedDescriptor, offset, length, action);
    };
    
    return IOMemoryDescriptorRunActionWhilePrepared(descriptor, bridge);
}

///
/// Allocate wired memory suitable for I/O transfer
///
/// @param length The number of bytes to allocate
/// @param direction The transfer direction, by default `kIODirectionInOut`
/// @return A non-null, prepared buffer memory descriptor on success, `nullptr` otherwise.
/// @note The caller must invoke `IOMemoryDescriptorSafeReleaseWiredBuffer()` to unwire and release the returned buffer.
///
static inline IOMemoryDescriptor* IOMemoryDescriptorAllocateWiredBuffer(IOByteCount length, IODirection direction = kIODirectionInOut)
{
    auto descriptor = IOBufferMemoryDescriptor::withCapacity(length, direction);
    
    if (descriptor == nullptr)
    {
        return nullptr;
    }
    
    if (descriptor->prepare() != kIOReturnSuccess)
    {
        descriptor->release();
        
        return nullptr;
    }
    
    return descriptor;
}

///
/// Release the given wired memory
///
/// @param descriptor A non-null, prepared memory descriptor returned by `IOMemoryDescriptorAllocateWiredBuffer()`
/// @note This function completes and releases the given memory descriptor if it is not null.
///       Upon return, the given memory descriptor is set to `nullptr`.
///
static inline void IOMemoryDescriptorSafeReleaseWiredBuffer(IOMemoryDescriptor*& descriptor)
{
    if (descriptor != nullptr)
    {
        psoftassert(descriptor->complete() == kIOReturnSuccess, "Failed to complete the given descriptor.");
        
        descriptor->release();
        
        descriptor = nullptr;
    }
}

#endif /* IOMemoryDescriptor_hpp */
