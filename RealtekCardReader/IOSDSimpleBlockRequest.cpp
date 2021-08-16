//
//  IOSDSimpleBlockRequest.cpp
//  RealtekCardReader
//
//  Created by FireWolf on 6/16/21.
//

#include "IOSDSimpleBlockRequest.hpp"
#include "IOSDHostDriver.hpp"
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
    
    this->command = nullptr;
    
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
    
    this->command = nullptr;
    
    this->block = 0;
    
    this->nblocks = 0;
    
    this->attributes = nullptr;
    
    this->completion.target = nullptr;
    
    this->completion.parameter = nullptr;
    
    this->completion.action = nullptr;
}

///
/// Get the DMA command for data transfer to service the request
///
/// @note This function is invoked by the processor routine to service the request either fully or partially.
///
IODMACommand* IOSDSimpleBlockRequest::getDMACommand()
{
    return this->command;
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
    // Guard: Prepare the request
    IOReturn retVal = this->prepare();
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to prepare the request. Error = 0x%x.", retVal);
        
        goto out;
    }
    
    // Guard: Service the request
    passert(this->processor != nullptr, "The processor should not be null.");
    
    retVal = (*this->processor)(this->driver, this);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to process the request. Error = 0x%x.", retVal);
    }
    
out:
    this->complete(retVal);
}

///
/// Prepare the request
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note This function allocates a DMA command and prepares the buffer accordingly.
///
IOReturn IOSDSimpleBlockRequest::prepare()
{
    // Guard: Page in and wire down the transfer buffer
    pinfo("Page in and wire down the transfer buffer.");
    
    IOReturn retVal = this->buffer->prepare();
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to page in and wire down the transfer buffer. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // Guard: Retrieve a preallocated DMA command
    pinfo("Allocating a DMA command from the pool.");
    
    IODMACommand* command = this->driver->allocateDMACommandFromPool();
    
    passert(command != nullptr, "The DMA command allocated from the pool should not be NULL.");
    
    // Guard: Associate the transfer buffer with the DMA command
    // Note that the DMA command is prepared automatically
    retVal = command->setMemoryDescriptor(this->buffer);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to associate the transfer buffer with the DMA command. Error = 0x%x.", retVal);
                
        this->driver->releaseDMACommandToPool(command);
        
        return retVal;
    }
    
    // All done
    this->command = command;
    
    pinfo("The transfer buffer is now associated with the DMA command.");
    
    return kIOReturnSuccess;
}

///
/// Complete the request
///
/// @param retVal The return value passed to the completion routine
/// @note This function releases the DMA command and completes the buffer accordingly,
///       and then invokes the storage completion routine with the given return value.
///
void IOSDSimpleBlockRequest::complete(IOReturn retVal)
{
    // Dissociate the transfer buffer from the DMA command
    pinfo("Completing the request...");
    
    if (this->command != nullptr)
    {
        psoftassert(this->command->clearMemoryDescriptor() == kIOReturnSuccess,
                    "Failed to dissociate the transfer buffer from the DMA command.");
        
        psoftassert(this->buffer->complete() == kIOReturnSuccess,
                    "Failed to complete the transfer buffer.");
        
        this->driver->releaseDMACommandToPool(this->command);
        
        this->command = nullptr;
    }
    
    UInt64 actualByteCount = retVal == kIOReturnSuccess ? this->nblocks * 512 : 0;
    
    pinfo("Invoking the completion routine.");

    IOStorage::complete(&this->completion, retVal, actualByteCount);
    
    pinfo("The request is completed. Return value = 0x%08x.", retVal);
}
