//
//  IOSDComplexBlockRequest.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 6/16/21.
//

#ifndef IOSDComplexBlockRequest_hpp
#define IOSDComplexBlockRequest_hpp

#include "IOSDSimpleBlockRequest.hpp"

///
/// Represents a complex request to access blocks on a SD card
///
/// @note The total number of blocks exceeds the hardware limit,
///       so the driver must service the request in multiple DMA transactions.
///
class IOSDComplexBlockRequest: public IOSDSimpleBlockRequest
{
    /// Constructors & Destructors
    OSDeclareDefaultStructors(IOSDComplexBlockRequest);
    
    using super = IOSDSimpleBlockRequest;
    
    /// The original buffer that contains all data to be transfered
    IOMemoryDescriptor* fullBuffer;
    
    /// The current starting block number
    UInt64 cblock;
    
    /// The current number of blocks to transfer
    UInt64 cnblocks;
    
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
};

#endif /* IOSDComplexBlockRequest_hpp */
