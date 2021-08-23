//
//  IOSDSimpleBlockRequest.cpp
//  RealtekCardReader
//
//  Created by FireWolf on 6/16/21.
//

#include "IOSDSimpleBlockRequest.hpp"
#include "IOSDHostDriver.hpp"
#include "IOMemoryDescriptor.hpp"
#include "Debug.hpp"

//
// MARK: - Meta Class Definitions
//

OSDefineMetaClassAndStructors(IOSDSimpleBlockRequest, IOSDBlockRequest);

///
/// Initialize a block request
///
/// @param driver A non-null host driver that processes the request
/// @param processor A non-null routine to process the request
/// @param buffer The data transfer buffer
/// @param block The starting block number
/// @param nblocks The number of blocks to transfer
/// @param attributes Attributes of the data transfer
/// @param completion The completion routine to call once the data transfer completes
/// @note This function retains the given host driver.
///       The caller must invoke `IOSDBlockRequest::deinit()` before returning the request to the pool.
///
void IOSDSimpleBlockRequest::init(IOSDHostDriver* driver,
                                  Processor processor,
                                  IOMemoryDescriptor* buffer,
                                  UInt64 block, UInt64 nblocks,
                                  IOStorageAttributes* attributes,
                                  IOStorageCompletion* completion)
{
    this->driver = driver;
    
    this->driver->retain();
    
    this->processor = processor;
    
    this->buffer = buffer;
    
    this->block = block;
    
    this->nblocks = nblocks;
    
    this->attributes = attributes;
    
    this->completion = *completion;
}

///
/// Deinitialize a block request
///
/// @note The caller must invoke this function to balance the call of `IOSDBlockRequest::init()`.
///
void IOSDSimpleBlockRequest::deinit()
{
    this->driver->release();
    
    this->driver = nullptr;
    
    this->processor = nullptr;
    
    this->buffer = nullptr;
    
    this->block = 0;
    
    this->nblocks = 0;
    
    this->attributes = nullptr;
    
    this->completion.target = nullptr;
    
    this->completion.parameter = nullptr;
    
    this->completion.action = nullptr;
}

///
/// Get the memory descriptor that contains data to service the request
///
/// @note This function is invoked by the processor routine to service the request either fully or partially.
///
IOMemoryDescriptor* IOSDSimpleBlockRequest::getMemoryDescriptor()
{
    return this->buffer;
}

///
/// Get the index of the start block to service the request
///
/// @note This function is invoked by the processor routine to service the request either fully or partially.
///
UInt64 IOSDSimpleBlockRequest::getBlockOffset()
{
    return this->block;
}

///
/// Get the number of blocks to service to service the request
///
/// @note This function is invoked by the processor routine to service the request either fully or partially.
///
UInt64 IOSDSimpleBlockRequest::getNumBlocks()
{
    return this->nblocks;
}

///
/// Service the block request
///
void IOSDSimpleBlockRequest::service()
{
    // Service the request
    pinfo("Processing the request...");
    
    IOReturn status = this->serviceOnce();
    
    // Complete the request
    UInt64 actualByteCount = status == kIOReturnSuccess ? this->nblocks * 512 : 0;

    IOStorage::complete(&this->completion, status, actualByteCount);
    
    pinfo("The request is completed. Return value = 0x%08x.", status);
}

///
/// Service the block request once
///
/// @return `kIOReturnSuccess` if the block request completes without errors, other values otherwise.
///
IOReturn IOSDSimpleBlockRequest::serviceOnce()
{
    // Service the request while the transfer buffer is prepared
    auto action = [&](IOMemoryDescriptor*) -> IOReturn
    {
        passert(this->processor != nullptr, "The processor should not be null.");
        
        return (*this->processor)(this->driver, this);
    };
    
    return IOMemoryDescriptorRunActionWhilePrepared(this->buffer, action);
}
