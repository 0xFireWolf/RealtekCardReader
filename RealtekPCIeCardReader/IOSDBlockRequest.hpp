//
//  IOSDBlockRequest.hpp
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 6/6/21.
//

#ifndef IOSDBlockRequest_hpp
#define IOSDBlockRequest_hpp

#include <IOKit/IOCommand.h>
#include <IOKit/storage/IOStorage.h>

/// Forward Declaration
class IOSDHostDriver;

/// Represents an abstract request to access blocks on a SD card
class IOSDBlockRequest: public IOCommand
{
    /// Constructors & Destructors
    OSDeclareAbstractStructors(IOSDBlockRequest);
    
    using super = IOCommand;
    
public:
    /// The type of the processor routine that services the request
    using Processor = IOReturn (*)(IOSDHostDriver*, IOSDBlockRequest*);
    
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
    virtual void init(IOSDHostDriver* driver,
                      Processor processor,
                      IOMemoryDescriptor* buffer,
                      UInt64 block, UInt64 nblocks,
                      IOStorageAttributes* attributes,
                      IOStorageCompletion* completion) = 0;
    
    ///
    /// Deinitialize a block request
    ///
    /// @note The caller must invoke this function to balance the call of `IOSDBlockRequest::init()`.
    ///
    virtual void deinit() = 0;
    
    ///
    /// Service the block request
    ///
    /// @note This function is invoked by the processor work loop to fully service the request.
    ///
    virtual void service() = 0;
    
    ///
    /// Get the DMA command for data transfer to service the request
    ///
    /// @note This function is invoked by the processor routine to service the request either fully or partially.
    ///
    virtual IODMACommand* getDMACommand() = 0;
    
    ///
    /// Get the index of the start block to service the request
    ///
    /// @note This function is invoked by the processor routine to service the request either fully or partially.
    ///
    virtual UInt64 getBlockOffset() = 0;
    
    ///
    /// Get the number of blocks to service to service the request
    ///
    /// @note This function is invoked by the processor routine to service the request either fully or partially.
    ///
    virtual UInt64 getNumBlocks() = 0;
};

#endif /* IOSDBlockRequest_hpp */
