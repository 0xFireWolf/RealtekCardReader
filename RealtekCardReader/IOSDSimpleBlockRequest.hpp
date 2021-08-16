//
//  IOSDSimpleBlockRequest.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 6/16/21.
//

#ifndef IOSDSimpleBlockRequest_hpp
#define IOSDSimpleBlockRequest_hpp

#include "IOSDBlockRequest.hpp"
#include <IOKit/IOMemoryDescriptor.h>
#include <IOKit/IODMACommand.h>

///
/// Represents a simple request to access blocks on a SD card
///
/// @note The total number of blocks does not exceed the hardware limit,
///       so the driver can service the request in a single DMA transaction.
///
class IOSDSimpleBlockRequest: public IOSDBlockRequest
{
    /// Constructors & Destructors
    OSDeclareDefaultStructors(IOSDSimpleBlockRequest);
    
    using super = IOSDBlockRequest;
    
    /// The host driver that processes the request
    IOSDHostDriver* driver;
    
    ///
    /// A non-null routine that processes the request
    ///
    /// @note This allows us to allocate a pool of requests at compile time
    ///       but deal with different types of requests at runtime.
    ///
    Processor processor;
    
    /// The data transfer buffer
    IOMemoryDescriptor* buffer;
    
    /// The starting block number
    UInt64 block;
    
    /// The number of blocks to transfer
    UInt64 nblocks;
    
    /// Attributes of the data transfer
    IOStorageAttributes* attributes;
    
    /// The completion routine to call once the data transfer completes
    IOStorageCompletion completion;
    
public:
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
    void init(IOSDHostDriver* driver,
              Processor processor,
              IOMemoryDescriptor* buffer,
              UInt64 block, UInt64 nblocks,
              IOStorageAttributes* attributes,
              IOStorageCompletion* completion) override;
    
    ///
    /// Deinitialize a block request
    ///
    /// @note The caller must invoke this function to balance the call of `IOSDBlockRequest::init()`.
    ///
    void deinit() override;
    
    ///
    /// Service the block request
    ///
    /// @note This function is invoked by the processor work loop to fully service the request.
    ///
    void service() override;
    
    ///
    /// Get the memory descriptor that contains data to service the request
    ///
    /// @note This function is invoked by the processor routine to service the request either fully or partially.
    ///
    IOMemoryDescriptor* getMemoryDescriptor() override;
    
    ///
    /// Get the index of the start block to service the request
    ///
    /// @note This function is invoked by the processor routine to service the request either fully or partially.
    ///
    UInt64 getBlockOffset() override;
    
    ///
    /// Get the number of blocks to service to service the request
    ///
    /// @note This function is invoked by the processor routine to service the request either fully or partially.
    ///
    UInt64 getNumBlocks() override;
    
protected:
    ///
    /// Prepare the request
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note This function allocates a DMA command and prepares the buffer accordingly.
    ///
    IOReturn prepare();
    
    ///
    /// Complete the request
    ///
    /// @param retVal The return value passed to the completion routine
    /// @note This function releases the DMA command and completes the buffer accordingly,
    ///       and then invokes the storage completion routine with the given return value.
    ///
    void complete(IOReturn retVal);
};

#endif /* IOSDSimpleBlockRequest_hpp */
