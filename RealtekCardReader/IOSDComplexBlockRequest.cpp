//
//  IOSDComplexBlockRequest.cpp
//  RealtekCardReader
//
//  Created by FireWolf on 6/16/21.
//

#include "IOSDComplexBlockRequest.hpp"
#include <IOKit/IOSubMemoryDescriptor.h>
#include "IOMemoryDescriptor.hpp"
#include "IOSDHostDriver.hpp"
#include "Utilities.hpp"
#include "Debug.hpp"

//
// MARK: - Meta Class Definitions
//

OSDefineMetaClassAndStructors(IOSDComplexBlockRequest, IOSDSimpleBlockRequest);

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
void IOSDComplexBlockRequest::init(IOSDHostDriver* driver,
                                   Processor processor,
                                   IOMemoryDescriptor* buffer,
                                   UInt64 block, UInt64 nblocks,
                                   IOStorageAttributes* attributes,
                                   IOStorageCompletion* completion)
{
    super::init(driver, processor, buffer, block, nblocks, attributes, completion);
    
    this->fullBuffer = buffer;
    
    this->cblock = block;
    
    this->cnblocks = 0;
}

///
/// Deinitialize a block request
///
/// @note The caller must invoke this function to balance the call of `IOSDBlockRequest::init()`.
///
void IOSDComplexBlockRequest::deinit()
{
    this->fullBuffer = nullptr;
    
    this->cblock = 0;
    
    this->cnblocks = 0;
    
    super::deinit();
}

///
/// Service the block request
///
/// @note This function is invoked by the processor work loop to fully service the request.
///
void IOSDComplexBlockRequest::service()
{
    // Guard: A buffer that describes a portion of data to be transfered in the current DMA transaction
    IOSubMemoryDescriptor* buffer = OSTypeAlloc(IOSubMemoryDescriptor);
    
    if (buffer == nullptr)
    {
        IOStorage::complete(&this->completion, kIOReturnNoMemory, 0);
        
        return;
    }
    
    this->buffer = buffer;
    
    // The service status
    IOReturn status = kIOReturnSuccess;
    
    // The maximum number of blocks to be transferred in one transaction
    UInt64 maxRequestNumBlocks = this->driver->getHostDevice()->getDMALimits().maxRequestNumBlocks();
    
    pinfo("BREQ: Servicing the complex request: Start index = %llu; Number of blocks = %llu.", this->block, this->nblocks);
    
    // Divide the original request into multiple transactions
    while (this->cblock < this->block + this->nblocks)
    {
        // Calculate the number of blocks to be transfered
        this->cnblocks = min(maxRequestNumBlocks, this->block + this->nblocks - this->cblock);
        
        pinfo("BREQ: Servicing the intermediate transaction: Current start index = %llu; Number of blocks = %llu.", this->cblock, this->cnblocks);
        
        // Specify the portion of data to be transfered
        if (!buffer->initSubRange(this->fullBuffer, (this->cblock - this->block) * 512, this->cnblocks * 512, this->fullBuffer->getDirection()))
        {
            perr("Failed to initialize the sub-memory descriptor.");
            
            status = kIOReturnError;
            
            break;
        }
        
        // Service the intermediate request
        status = this->serviceOnce();
        
        if (status != kIOReturnSuccess)
        {
            perr("Failed to process the intermediate transaction. Error = 0x%x.", status);
            
            break;
        }
        
        // Guard: Complete the intermediate request
        pinfo("BREQ: Serviced the intermediate transaction: Current start index = %llu; Number of blocks = %llu.", this->cblock, this->cnblocks);
        
        // The intermediate request completes without errors
        this->cblock += maxRequestNumBlocks;
    }
    
    // Complete the request
    UInt64 actualByteCount = status == kIOReturnSuccess ? this->nblocks * 512 : 0;

    IOStorage::complete(&this->completion, status, actualByteCount);
    
    OSSafeReleaseNULL(this->buffer);
    
    pinfo("The request is completed. Return value = 0x%08x.", retVal);
}

///
/// Get the index of the start block to service the request
///
/// @note This function is invoked by the processor routine to service the request either fully or partially.
///
UInt64 IOSDComplexBlockRequest::getBlockOffset()
{
    return this->cblock;
}

///
/// Get the number of blocks to service to service the request
///
/// @note This function is invoked by the processor routine to service the request either fully or partially.
///
UInt64 IOSDComplexBlockRequest::getNumBlocks()
{
    return this->cnblocks;
}
