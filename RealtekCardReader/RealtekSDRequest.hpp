//
//  RealtekSDRequest.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 5/30/21.
//

#ifndef RealtekSDRequest_hpp
#define RealtekSDRequest_hpp

#include "RealtekSDCommand.hpp"
#include <IOKit/IODMACommand.h>

/// Data associated with a SD command
struct RealtekSDData
{
    /// A non-null, prepared DMA command
    IODMACommand* data;
    
    /// The number of blocks to be transfered
    UInt64 nblocks;
    
    /// The size of each block in bytes
    UInt64 blockSize;
    
    /// Create with the given DMA command and data properties
    RealtekSDData(IODMACommand* data, UInt64 nblocks, UInt64 blockSize)
        : data(data), nblocks(nblocks), blockSize(blockSize) {}
    
    /// Get the DMA command that describes the data to be transferred
    IODMACommand* getIODMACommand() const
    {
        return this->data;
    }
    
    /// Get the number of blocks to be transferred
    UInt16 getNumBlocks() const
    {
        // e.g. If the storage subsystem requests to read 131072 blocks,
        //      the host driver must split it into two requests,
        //      each of which reads 65536 blocks.
        //      The size of a physical block on a SD card is 512 bytes,
        //      so the maximum number of bytes in one DMA transfer is 32MB.
        psoftassert(this->nblocks <= UINT16_MAX,
                    "The maximum number of blocks in one transfer is 65536.");
        
        return static_cast<UInt16>(this->nblocks);
    }
    
    /// Get the size of each block in bytes
    UInt16 getBlockSize() const
    {
        psoftassert(this->blockSize <= UINT16_MAX,
                    "The maximum number of bytes in one block is 65535.");
        
        return static_cast<UInt16>(this->blockSize);
    }
    
    /// Get the total amount of data to be transferred
    UInt32 getDataLength() const
    {
        return this->getBlockSize() * this->getNumBlocks();
    }
};

/// Forward declaration
class RealtekSDXCSlot;

/// Specify the interface of an abstract SD request
struct RealtekSDRequest
{
    /// Virtual destructor
    virtual ~RealtekSDRequest() = default;
    
    ///
    /// Service the request
    ///
    /// @param slot A non-null Realtek card slot instance.
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note This function exploits a simple visitor pattern to select the service routine based on the request type.
    ///
    virtual IOReturn service(RealtekSDXCSlot* slot) = 0;
};

/// Represents a simple SD command request that does not involve data transfer (i.e. Use CMD line only)
struct RealtekSDSimpleCommandRequest: RealtekSDRequest
{
    /// A SD command that involves no data transfer
    RealtekSDCommand command;
    
    /// Create a simple SD command request
    RealtekSDSimpleCommandRequest(const RealtekSDCommand& command)
        : command(command) {}
    
    ///
    /// Service the request
    ///
    /// @param slot A non-null Realtek card slot instance.
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note This function exploits a simple visitor pattern to select the service routine based on the request type.
    ///
    IOReturn service(RealtekSDXCSlot* slot) override;
};

/// Represents an abstract SD command request that involves a data transfer (i.e. Use CMD + DAT lines)
struct RealtekSDCommandWithDataTransferRequest: RealtekSDSimpleCommandRequest
{
    /// The data associated with the command
    RealtekSDData data;
    
    /// Create a command request that involves a data transfer
    RealtekSDCommandWithDataTransferRequest(const RealtekSDCommand& command, const RealtekSDData& data)
        : RealtekSDSimpleCommandRequest(command), data(data) {}
};

/// Represents a SD command request that involves an inbound data transfer (i.e. Use CMD + DAT lines)
struct RealtekSDCommandWithInboundDataTransferRequest: RealtekSDCommandWithDataTransferRequest
{
    /// Inherit the constructor from the parent class
    using RealtekSDCommandWithDataTransferRequest::RealtekSDCommandWithDataTransferRequest;
    
    ///
    /// Service the request
    ///
    /// @param slot A non-null Realtek card slot instance.
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note This function exploits a simple visitor pattern to select the service routine based on the request type.
    ///
    IOReturn service(RealtekSDXCSlot* slot) override;
};

/// Represents a SD command request that involves an outbound data transfer (i.e. Use CMD + DAT lines)
struct RealtekSDCommandWithOutboundDataTransferRequest: RealtekSDCommandWithDataTransferRequest
{
    /// Inherit the constructor from the parent class
    using RealtekSDCommandWithDataTransferRequest::RealtekSDCommandWithDataTransferRequest;
    
    ///
    /// Service the request
    ///
    /// @param slot A non-null Realtek card slot instance.
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note This function exploits a simple visitor pattern to select the service routine based on the request type.
    ///
    IOReturn service(RealtekSDXCSlot* slot) override;
};

/// Represents an abstract SD command request that involves block-oriented data transfer
struct RealtekSDCommandWithBlockDataTransferRequest: RealtekSDCommandWithDataTransferRequest
{
    /// Inherit the constructor from the parent class
    using RealtekSDCommandWithDataTransferRequest::RealtekSDCommandWithDataTransferRequest;
};

/// Represents a request that reads a single block from the card (i.e. CMD17)
struct RealtekSDSingleBlockReadRequest: RealtekSDCommandWithBlockDataTransferRequest
{
    /// Inherit the constructor from the parent class
    using RealtekSDCommandWithBlockDataTransferRequest::RealtekSDCommandWithBlockDataTransferRequest;
    
    /// Create a command request that reads a single block from the card
    RealtekSDSingleBlockReadRequest(UInt32 offset, IODMACommand* data, UInt64 nblocks, UInt64 blockSize)
        : RealtekSDCommandWithBlockDataTransferRequest(RealtekSDCommand::CMD17(offset), RealtekSDData(data, nblocks, blockSize)) {}
    
    ///
    /// Service the request
    ///
    /// @param slot A non-null Realtek card slot instance.
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note This function exploits a simple visitor pattern to select the service routine based on the request type.
    ///
    IOReturn service(RealtekSDXCSlot* slot) override;
};

/// Represents a request that writes a single block to the card (i.e. CMD24)
struct RealtekSDSingleBlockWriteRequest: RealtekSDCommandWithBlockDataTransferRequest
{
    /// Inherit the constructor from the parent class
    using RealtekSDCommandWithBlockDataTransferRequest::RealtekSDCommandWithBlockDataTransferRequest;
    
    /// Create a command request that writes a single block to the card
    RealtekSDSingleBlockWriteRequest(UInt32 offset, IODMACommand* data, UInt64 nblocks, UInt64 blockSize)
        : RealtekSDCommandWithBlockDataTransferRequest(RealtekSDCommand::CMD24(offset), RealtekSDData(data, nblocks, blockSize)) {}
    
    ///
    /// Service the request
    ///
    /// @param slot A non-null Realtek card slot instance.
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note This function exploits a simple visitor pattern to select the service routine based on the request type.
    ///
    IOReturn service(RealtekSDXCSlot* slot) override;
};

/// Represents a request that reads multiple blocks from the card (i.e. CMD18)
/// Note that Realtek's driver does not use CMD23 at this moment,
/// so this is an open ended request that contains a CMD18 and a CMD12.
struct RealtekSDMultiBlockReadRequest: RealtekSDSingleBlockReadRequest
{
    /// Create a command request that reads multiple blocks from the card
    RealtekSDMultiBlockReadRequest(UInt32 offset, IODMACommand* data, UInt64 nblocks, UInt64 blockSize)
        : RealtekSDSingleBlockReadRequest(RealtekSDCommand::CMD18(offset), RealtekSDData(data, nblocks, blockSize)),
          stopCommand(RealtekSDCommand::CMD12()) {}
    
    ///
    /// A CMD12 to terminate the transmission
    ///
    /// @note If the driver uses a predefined multi-block transfer mode,
    ///       it still needs to send a CMD12 if there is an error occurred during the transfer.
    ///       If the driver uses an open ended transfer mode,
    ///       it must send the final CMD12 to ask the card to stop transmission.
    ///       As a result, the stop command must be non-null.
    ///
    RealtekSDCommand stopCommand;
    
    ///
    /// Service the request
    ///
    /// @param slot A non-null Realtek card slot instance.
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note This function exploits a simple visitor pattern to select the service routine based on the request type.
    ///
    IOReturn service(RealtekSDXCSlot* slot) override;
};

/// Represents a request that writes multiple blocks to the card (i.e. CMD25)
/// Note that Realtek's driver does not use CMD23 at this moment,
/// so this is an open ended request that contains a CMD25 and a CMD12.
/// However, we issue an ACMD23 to set a number of blocks to be pre-erased to improve the write performance.
/// Note that ACMD23 is mandatory for all writable SD cards.
struct RealtekSDMultiBlockWriteRequest: RealtekSDSingleBlockWriteRequest
{
    /// Create a command request that writes multiple blocks to the card
    RealtekSDMultiBlockWriteRequest(UInt32 offset, IODMACommand* data, UInt64 nblocks, UInt64 blockSize)
        : RealtekSDSingleBlockWriteRequest(RealtekSDCommand::CMD25(offset), RealtekSDData(data, nblocks, blockSize)),
          stopCommand(RealtekSDCommand::CMD12b()) {}
    
    ///
    /// A CMD12 to terminate the transmission
    ///
    /// @note If the driver uses a predefined multi-block transfer mode,
    ///       it still needs to send a CMD12 if there is an error occurred during the transfer.
    ///       If the driver uses an open ended transfer mode,
    ///       it must send the final CMD12 to ask the card to stop transmission.
    ///       As a result, the stop command must be non-null.
    ///
    RealtekSDCommand stopCommand;
    
    ///
    /// Service the request
    ///
    /// @param slot A non-null Realtek card slot instance.
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note This function exploits a simple visitor pattern to select the service routine based on the request type.
    ///
    IOReturn service(RealtekSDXCSlot* slot) override;
};

namespace RealtekSDRequestFactory
{
    static inline RealtekSDSimpleCommandRequest CMD0()
    {
        return RealtekSDSimpleCommandRequest(RealtekSDCommand::CMD0());
    }
    
    static inline RealtekSDSimpleCommandRequest CMD2()
    {
        return RealtekSDSimpleCommandRequest(RealtekSDCommand::CMD2());
    }
    
    static inline RealtekSDSimpleCommandRequest CMD3()
    {
        return RealtekSDSimpleCommandRequest(RealtekSDCommand::CMD3());
    }
    
    static inline RealtekSDCommandWithInboundDataTransferRequest CMD6(UInt32 mode, UInt32 group, UInt32 value, IODMACommand* data)
    {
        return RealtekSDCommandWithInboundDataTransferRequest(RealtekSDCommand::CMD6(mode, group, value), RealtekSDData(data, 1, 64));
    }
    
    static inline RealtekSDSimpleCommandRequest CMD7(UInt32 rca)
    {
        return RealtekSDSimpleCommandRequest(RealtekSDCommand::CMD7(rca));
    }
    
    static inline RealtekSDSimpleCommandRequest CMD8(UInt8 vhs, UInt8 checkPattern)
    {
        return RealtekSDSimpleCommandRequest(RealtekSDCommand::CMD8(vhs, checkPattern));
    }
    
    static inline RealtekSDSimpleCommandRequest CMD9(UInt32 rca)
    {
        return RealtekSDSimpleCommandRequest(RealtekSDCommand::CMD9(rca));
    }
    
    static inline RealtekSDSimpleCommandRequest CMD11()
    {
        return RealtekSDSimpleCommandRequest(RealtekSDCommand::CMD11());
    }
    
    static inline RealtekSDSimpleCommandRequest CMD13(UInt32 rca)
    {
        return RealtekSDSimpleCommandRequest(RealtekSDCommand::CMD13(rca));
    }
    
    static inline RealtekSDSingleBlockReadRequest CMD17(UInt32 offset, IODMACommand* data)
    {
        return RealtekSDSingleBlockReadRequest(offset, data, 1, 512);
    }
    
    static inline RealtekSDMultiBlockReadRequest CMD18(UInt32 offset, IODMACommand* data, UInt64 nblocks)
    {
        return RealtekSDMultiBlockReadRequest(offset, data, nblocks, 512);
    }
    
    static inline RealtekSDSingleBlockWriteRequest CMD24(UInt32 offset, IODMACommand* data)
    {
        return RealtekSDSingleBlockWriteRequest(offset, data, 1, 512);
    }
    
    static inline RealtekSDMultiBlockWriteRequest CMD25(UInt32 offset, IODMACommand* data, UInt64 nblocks)
    {
        return RealtekSDMultiBlockWriteRequest(offset, data, nblocks, 512);
    }
    
    static inline RealtekSDSimpleCommandRequest CMD55(UInt32 rca)
    {
        return RealtekSDSimpleCommandRequest(RealtekSDCommand::CMD55(rca));
    }
    
    static inline RealtekSDSimpleCommandRequest ACMD6(UInt32 busWidth)
    {
        return RealtekSDSimpleCommandRequest(RealtekSDCommand::ACMD6(busWidth));
    }
    
    static inline RealtekSDCommandWithInboundDataTransferRequest ACMD13(IODMACommand* data)
    {
        return RealtekSDCommandWithInboundDataTransferRequest(RealtekSDCommand::ACMD13(), RealtekSDData(data, 1, 64));
    }
    
    static inline RealtekSDSimpleCommandRequest ACMD23(UInt32 nblocks)
    {
        return RealtekSDSimpleCommandRequest(RealtekSDCommand::ACMD23(nblocks));
    }
    
    static inline RealtekSDSimpleCommandRequest ACMD41(UInt32 ocr)
    {
        return RealtekSDSimpleCommandRequest(RealtekSDCommand::ACMD41(ocr));
    }
    
    static inline RealtekSDCommandWithInboundDataTransferRequest ACMD51(IODMACommand* data)
    {
        return RealtekSDCommandWithInboundDataTransferRequest(RealtekSDCommand::ACMD51(), RealtekSDData(data, 1, 8));
    }
}

// Ideally, there should be two kinds of multi block read/write requests
//struct __RealtekSDMultiBlockOpenEndedReadRequest
//{
//
//};
//
//struct __RealtekSDMultiBlockPredefinedReadRequest
//{
//    ///
//    /// A CMD23 to set a predefined number of blocks to read
//    ///
//    /// @warning This command can be NULL if the host or the card does not support CMD23.
//    /// @note Realtek's driver does not use CMD23 at this moment.
//    ///       Leave this instance variable for future use.
//    ///
//    RealtekSDCommand setBlockCountCommand;
//};

#endif /* RealtekSDRequest_hpp */
