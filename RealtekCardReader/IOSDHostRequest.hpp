//
//  IOSDHostRequest.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 8/14/21.
//

#ifndef IOSDHostRequest_hpp
#define IOSDHostRequest_hpp

#include <IOKit/IOTypes.h>
#include <IOKit/IOMemoryDescriptor.h>
#include "Utilities.hpp"
#include "Debug.hpp"

/// Represents a host command to be sent to the SD card
struct IOSDHostCommand
{
public:
    /// Enumerates common SD command opcodes
    enum class Opcode: UInt32
    {
        // General Commands
        kGoIdleState = 0,
        kAllSendCID = 2,
        kSendRelativeAddress = 3,
        kSwitchFunction = 6,
        kSelectCard = 7,
        kSendIfCond = 8,
        kSendCSD = 9,
        kVoltageSwitch = 11,
        kStopTransmission = 12,
        kSendStatus = 13,
        kReadSingleBlock = 17,
        kReadMultipleBlocks = 18,
        kSendTuningBlock = 19,
        kWriteSingleBlock = 24,
        kWriteMultipleBlocks = 25,
        kAppCommand = 55,
        
        // Application Commands
        kAppSetBusWidth = 6,
        kAppSDStatus = 13,
        kAppSetEraseCount = 23,
        kAppSendOpCond = 41,
        kAppSendSCR = 51,
    };
    
    /// Enumerates all possible response types
    enum class ResponseType: UInt16
    {
        kR0, kR1, kR1b, kR2, kR3, kR6, kR7,
    };
    
private:
    /// The command index
    Opcode opcode;
    
    /// The command argument
    UInt32 argument;
    
    /// The response buffer
    UInt8 response[17];
    
    /// Padding fields
    UInt8 reserved;
    
    /// The response type
    ResponseType responseType;
    
    /// The amount of time in ms to detect the busy bit until timed out
    UInt32 busyTimeout;
    
public:
    //
    // MARK: - Constructors
    //
    
    ///
    /// Create a host command
    ///
    IOSDHostCommand(Opcode opcode, UInt32 argument, ResponseType responseType, UInt32 busyTimeout = 0)
        : opcode(opcode), argument(argument), response(), reserved(0), responseType(responseType), busyTimeout(busyTimeout) {}
    
    //
    // MARK: - Get Command Properties
    //
    
    ///
    /// Get the command opcode
    ///
    /// @return The command opcode.
    ///
    inline Opcode getOpcode() const
    {
        return this->opcode;
    }
    
    ///
    /// Get the opcode with the start and the transmission bits set properly
    ///
    /// @return The opcode with the start bit cleared and the transmission bit set.
    ///
    inline UInt8 getOpcodeWithStartAndTransmissionBits() const
    {
        return 0x40 | static_cast<UInt8>(this->opcode);
    }
    
    ///
    /// Get the command argument
    ///
    /// @return The command argument.
    ///
    inline UInt32 getArgument() const
    {
        return this->argument;
    }
    
    ///
    /// Get the amount of time in ms to detect the busy bit until timed out
    ///
    /// @param defaultTimeout If the amount of time is not defined (i.e. 0),
    ///                       use the value specified by this parameter instead.
    ///
    inline UInt32 getBusyTimeout(UInt32 defaultTimeout) const
    {
        return this->busyTimeout != 0 ? this->busyTimeout : defaultTimeout;
    }
    
    ///
    /// Get the storage for the command response
    ///
    /// @return A non-null pointer to the response buffer.
    ///
    inline UInt8* getResponseBuffer()
    {
        return this->response;
    }
    
    ///
    /// Reinterpret the command response as the specified type
    ///
    /// @return A non-null pointer to the response of the specified type.
    ///
    template <typename Response>
    const Response* reinterpretResponseAs() const
    {
        return reinterpret_cast<const Response*>(this->response);
    }
    
    ///
    /// Get the response type
    ///
    /// @return The reponse type.
    ///
    inline ResponseType getResponseType() const
    {
        return this->responseType;
    }
    
    ///
    /// Get the response length in bytes
    ///
    /// @return The number of bytes in the response.
    ///
    inline IOByteCount getResponseLength() const
    {
        switch (this->responseType)
        {
            case ResponseType::kR0:
            {
                return 0;
            }
                
            case ResponseType::kR1:
            case ResponseType::kR1b:
            case ResponseType::kR3:
            case ResponseType::kR6:
            case ResponseType::kR7:
            {
                return 6;
            }
                
            case ResponseType::kR2:
            {
                return 17;
            }
                
            default:
            {
                pfatal("Invalid response type.");
            }
        }
    }
    
    ///
    /// Print the response in bytes
    ///
    inline void printResponse() const
    {
        pinfo("Command Response:");
        
        pbufcol(this->response, sizeof(this->response));
    }
    
    //
    // MARK: - Command Factory
    //

    static inline IOSDHostCommand CMD0()
    {
        return IOSDHostCommand(Opcode::kGoIdleState, 0, ResponseType::kR0);
    }

    static inline IOSDHostCommand CMD2()
    {
        return IOSDHostCommand(Opcode::kAllSendCID, 0, ResponseType::kR2);
    }

    static inline IOSDHostCommand CMD3()
    {
        return IOSDHostCommand(Opcode::kSendRelativeAddress, 0, ResponseType::kR6);
    }

    static inline IOSDHostCommand CMD6(UInt32 mode, UInt32 group, UInt32 value)
    {
        UInt32 argument = mode << 31 | 0x00FFFFFF;

        argument &= ~(0xF << (group * 4));

        argument |= value << (group * 4);

        return IOSDHostCommand(Opcode::kSwitchFunction, argument, ResponseType::kR1);
    }

    static inline IOSDHostCommand CMD7(UInt32 rca)
    {
        return IOSDHostCommand(Opcode::kSelectCard, rca << 16, rca == 0 ? ResponseType::kR0 : ResponseType::kR1);
    }

    static inline IOSDHostCommand CMD8(UInt8 vhs, UInt8 checkPattern)
    {
        return IOSDHostCommand(Opcode::kSendIfCond, static_cast<UInt32>(vhs << 8 | checkPattern), ResponseType::kR7);
    }

    static inline IOSDHostCommand CMD9(UInt32 rca)
    {
        return IOSDHostCommand(Opcode::kSendCSD, rca << 16, ResponseType::kR2);
    }

    static inline IOSDHostCommand CMD11()
    {
        return IOSDHostCommand(Opcode::kVoltageSwitch, 0, ResponseType::kR1);
    }

    static inline IOSDHostCommand CMD12()
    {
        return IOSDHostCommand(Opcode::kStopTransmission, 0, ResponseType::kR1);
    }

    static inline IOSDHostCommand CMD12b()
    {
        return IOSDHostCommand(Opcode::kStopTransmission, 0, ResponseType::kR1b);
    }

    static inline IOSDHostCommand CMD13(UInt32 rca)
    {
        return IOSDHostCommand(Opcode::kSendStatus, rca << 16, ResponseType::kR1);
    }

    static inline IOSDHostCommand CMD17(UInt32 offset)
    {
        return IOSDHostCommand(Opcode::kReadSingleBlock, offset, ResponseType::kR1);
    }

    static inline IOSDHostCommand CMD18(UInt32 offset)
    {
        return IOSDHostCommand(Opcode::kReadMultipleBlocks, offset, ResponseType::kR1);
    }

    static inline IOSDHostCommand CMD19()
    {
        return IOSDHostCommand(Opcode::kSendTuningBlock, 0, ResponseType::kR1);
    }

    static inline IOSDHostCommand CMD24(UInt32 offset)
    {
        return IOSDHostCommand(Opcode::kWriteSingleBlock, offset, ResponseType::kR1);
    }

    static inline IOSDHostCommand CMD25(UInt32 offset)
    {
        return IOSDHostCommand(Opcode::kWriteMultipleBlocks, offset, ResponseType::kR1);
    }

    static inline IOSDHostCommand CMD55(UInt32 rca)
    {
        return IOSDHostCommand(Opcode::kAppCommand, rca << 16, ResponseType::kR1);
    }

    static inline IOSDHostCommand ACMD6(UInt32 busWidth)
    {
        return IOSDHostCommand(Opcode::kAppSetBusWidth, busWidth, ResponseType::kR1);
    }

    static inline IOSDHostCommand ACMD13()
    {
        return IOSDHostCommand(Opcode::kAppSDStatus, 0, ResponseType::kR1);
    }

    static inline IOSDHostCommand ACMD23(UInt32 nblocks)
    {
        return IOSDHostCommand(Opcode::kAppSetEraseCount, nblocks, ResponseType::kR1);
    }

    static inline IOSDHostCommand ACMD41(UInt32 ocr)
    {
        return IOSDHostCommand(Opcode::kAppSendOpCond, ocr, ResponseType::kR3);
    }

    static inline IOSDHostCommand ACMD51()
    {
        return IOSDHostCommand(Opcode::kAppSendSCR, 0, ResponseType::kR1);
    }
};

/// Represents the data associated with a SD host command
struct IOSDHostData
{
private:
    /// A non-null, prepared memory descriptor
    IOMemoryDescriptor* data;
    
    /// The number of blocks to be transfered
    UInt64 nblocks;
    
    /// The size of each block in bytes
    UInt64 blockSize;
    
public:
    /// Create with the given memory descriptor and data properties
    IOSDHostData(IOMemoryDescriptor* data, UInt64 nblocks, UInt64 blockSize)
        : data(data), nblocks(nblocks), blockSize(blockSize) {}
    
    /// Get the memory descriptor that describes the data to be transferred
    inline IOMemoryDescriptor* getMemoryDescriptor() const
    {
        return this->data;
    }
    
    /// Get the number of blocks to be transferred
    inline UInt64 getNumBlocks() const
    {
        return this->nblocks;
    }
    
    /// Get the size of each block in bytes
    inline UInt64 getBlockSize() const
    {
        return this->blockSize;
    }
    
    /// Get the total amount of data to be transferred
    inline UInt64 getDataLength() const
    {
        return this->nblocks * this->blockSize;
    }
};

/// Represents a host request to be processed by the SD card
struct IOSDHostRequest
{
    /// Type of the function that processes the host request
    using Processor = IOReturn (*)(void* target, IOSDHostRequest& request);
    
    /// An opaque client-supplied pointer (or the instance pointer for a C++ processor function)
    void* target;
    
    /// A client-supplied routine that processes the host request
    Processor processor;
    
    /// Create a host request with the given processor
    IOSDHostRequest(void* target, Processor processor)
        : target(target), processor(processor) {}
    
    ///
    /// Invoke the client-supplied processor function to process the request
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note This function becomes a noop if the processor is null.
    ///
    IOReturn process()
    {
        return this->processor != nullptr ? (*this->processor)(this->target, *this) : kIOReturnSuccess;
    }
};

/// Represents a request that does not involve any data transfer (e.g., CMD0)
struct IOSDCommandRequest: IOSDHostRequest
{
    /// A SD command that involves no data transfer
    IOSDHostCommand command;
    
    /// Create a command request
    IOSDCommandRequest(void* target, Processor processor, const IOSDHostCommand& command)
        : IOSDHostRequest(target, processor), command(command) {}
};

/// Represents a request that involves a data transfer (e.g., CMD6)
struct IOSDDataTransferRequest: IOSDCommandRequest
{
    /// The data associated with the command
    IOSDHostData data;
    
    /// Create a request that involves a data transfer
    IOSDDataTransferRequest(void* target, Processor processor, const IOSDHostCommand& command, const IOSDHostData& data)
        : IOSDCommandRequest(target, processor, command), data(data) {}
};

/// Represents a request that accesses a single block on the card (e.g., CMD17/CMD24)
struct IOSDSingleBlockRequest: IOSDDataTransferRequest
{
    /// Inherit the constructor from the super class
    using IOSDDataTransferRequest::IOSDDataTransferRequest;
};

/// Represents a request that accesses multiple blocks on the card (e.g., CMD18/CMD25)
struct IOSDMultiBlocksRequest: IOSDSingleBlockRequest
{
    ///
    /// A CMD12 to terminate the transmission
    ///
    /// @note If the driver uses a predefined multi-block transfer mode,
    ///       it still needs to send a CMD12 if there is an error occurred during the transfer.
    ///       If the driver uses an open ended transfer mode,
    ///       it must send the final CMD12 to ask the card to stop transmission.
    ///       As a result, the stop command must be non-null.
    ///
    IOSDHostCommand stopCommand;
    
    /// Create a request that accesses multiple blocks on the card
    IOSDMultiBlocksRequest(void* target, Processor processor, const IOSDHostCommand& command, const IOSDHostData& data, const IOSDHostCommand& stopCommand)
        : IOSDSingleBlockRequest(target, processor, command, data), stopCommand(stopCommand) {}
};

///
/// A factory that creates host requests
///
/// @note By default, the factory creates requests without setting the processor routine.
///       Host device may provide a custom factory to exploit the vistor pattern to process different types of requests.
///
struct IOSDHostRequestFactory
{
private:
    /// An opaque client-supplied pointer (or the instance pointer for a C++ processor function)
    void* target;
    
    /// The processor routine that processes a simple command request
    IOSDHostRequest::Processor commandProcessor;
    
    /// The processor routine that processes an inbound data transfer request
    IOSDHostRequest::Processor inboundDataTransferProcessor;
    
    /// The processor routine that processes an outbound data transfer request
    IOSDHostRequest::Processor outboundDataTransferProcessor;
    
    /// The processor routine that processes a request that reads a single block
    IOSDHostRequest::Processor readSingleBlockProcessor;
    
    /// The processor routine that processes a request that writes a single block
    IOSDHostRequest::Processor writeSingleBlockProcessor;
    
    /// The processor routine that processes a request that reads multiple blocks
    IOSDHostRequest::Processor readMultiBlocksProcessor;
    
    /// The processor routine that processes a request that writes multiple blocks
    IOSDHostRequest::Processor writeMultiBlocksProcessor;
    
public:
    ///
    /// Create a request factory with the given processors
    ///
    IOSDHostRequestFactory(void* target = nullptr,
                           IOSDHostRequest::Processor commandProcessor = nullptr,
                           IOSDHostRequest::Processor inboundDataTransferProcessor = nullptr,
                           IOSDHostRequest::Processor outboundDataTransferProcessor = nullptr,
                           IOSDHostRequest::Processor readSingleBlockProcessor = nullptr,
                           IOSDHostRequest::Processor writeSingleBlockProcessor = nullptr,
                           IOSDHostRequest::Processor readMultiBlocksProcessor = nullptr,
                           IOSDHostRequest::Processor writeMultiBlocksProcessor = nullptr)
        : target(target),
          commandProcessor(commandProcessor),
          inboundDataTransferProcessor(inboundDataTransferProcessor),
          outboundDataTransferProcessor(outboundDataTransferProcessor),
          readSingleBlockProcessor(readSingleBlockProcessor),
          writeSingleBlockProcessor(writeSingleBlockProcessor),
          readMultiBlocksProcessor(readMultiBlocksProcessor),
          writeMultiBlocksProcessor(writeMultiBlocksProcessor) {}
    
private:
    ///
    /// Create a simple command request that does not involve any data transfer
    ///
    /// @param command The SD host command
    /// @return The command request.
    ///
    inline IOSDCommandRequest makeCommandRequest(const IOSDHostCommand& command) const
    {
        return IOSDCommandRequest(this->target, this->commandProcessor, command);
    }
    
    ///
    /// Create an inbound data transfer request
    ///
    /// @param command The SD host command
    /// @param data The data associated with the command
    /// @return The data transfer request.
    ///
    inline IOSDDataTransferRequest makeInboundDataTransferRequest(const IOSDHostCommand& command, const IOSDHostData& data) const
    {
        return IOSDDataTransferRequest(this->target, this->inboundDataTransferProcessor, command, data);
    }
    
    ///
    /// Create an outbound data transfer request
    ///
    /// @param command The SD host command
    /// @param data The data associated with the command
    /// @return The data transfer request.
    ///
    inline IOSDDataTransferRequest makeOutboundDataTransferRequest(const IOSDHostCommand& command, const IOSDHostData& data) const
    {
        return IOSDDataTransferRequest(this->target, this->outboundDataTransferProcessor, command, data);
    }
    
    ///
    /// Create a request that reads a single block from the card
    ///
    /// @param command The SD host command
    /// @param data The data associated with the command
    /// @return A single block access request.
    ///
    inline IOSDSingleBlockRequest makeReadSingleBlockRequest(const IOSDHostCommand& command, const IOSDHostData& data) const
    {
        return IOSDSingleBlockRequest(this->target, this->readSingleBlockProcessor, command, data);
    }
    
    ///
    /// Create a request that writes a single block to the card
    ///
    /// @param command The SD host command
    /// @param data The data associated with the command
    /// @return A single block access request.
    ///
    inline IOSDSingleBlockRequest makeWriteSingleBlockRequest(const IOSDHostCommand& command, const IOSDHostData& data) const
    {
        return IOSDSingleBlockRequest(this->target, this->writeSingleBlockProcessor, command, data);
    }
    
    ///
    /// Create a request that reads multiple blocks from the card
    ///
    /// @param command The SD host command
    /// @param data The data associated with the command
    /// @return A multiple blocks access request.
    ///
    inline IOSDMultiBlocksRequest makeReadMultiBlocksRequest(const IOSDHostCommand& command, const IOSDHostData& data, const IOSDHostCommand& stopCommand) const
    {
        return IOSDMultiBlocksRequest(this->target, this->readMultiBlocksProcessor, command, data, stopCommand);
    }
    
    ///
    /// Create a request that writes multiple blocks to the card
    ///
    /// @param command The SD host command
    /// @param data The data associated with the command
    /// @return A multiple blocks access request.
    ///
    inline IOSDMultiBlocksRequest makeWriteMultiBlocksRequest(const IOSDHostCommand& command, const IOSDHostData& data, const IOSDHostCommand& stopCommand) const
    {
        return IOSDMultiBlocksRequest(this->target, this->writeMultiBlocksProcessor, command, data, stopCommand);
    }
    
public:
    inline IOSDCommandRequest CMD0() const
    {
        return this->makeCommandRequest(IOSDHostCommand::CMD0());
    }

    inline IOSDCommandRequest CMD2() const
    {
        return this->makeCommandRequest(IOSDHostCommand::CMD2());
    }

    inline IOSDCommandRequest CMD3() const
    {
        return this->makeCommandRequest(IOSDHostCommand::CMD3());
    }

    inline IOSDDataTransferRequest CMD6(UInt32 mode, UInt32 group, UInt32 value, IOMemoryDescriptor* data) const
    {
        return this->makeInboundDataTransferRequest(IOSDHostCommand::CMD6(mode, group, value), IOSDHostData(data, 1, 64));
    }

    inline IOSDCommandRequest CMD7(UInt32 rca) const
    {
        return this->makeCommandRequest(IOSDHostCommand::CMD7(rca));
    }

    inline IOSDCommandRequest CMD8(UInt8 vhs, UInt8 checkPattern) const
    {
        return this->makeCommandRequest(IOSDHostCommand::CMD8(vhs, checkPattern));
    }

    inline IOSDCommandRequest CMD9(UInt32 rca) const
    {
        return this->makeCommandRequest(IOSDHostCommand::CMD9(rca));
    }

    inline IOSDCommandRequest CMD11() const
    {
        return this->makeCommandRequest(IOSDHostCommand::CMD11());
    }

    inline IOSDCommandRequest CMD13(UInt32 rca) const
    {
        return this->makeCommandRequest(IOSDHostCommand::CMD13(rca));
    }

    inline IOSDSingleBlockRequest CMD17(UInt32 offset, IOMemoryDescriptor* data) const
    {
        return this->makeReadSingleBlockRequest(IOSDHostCommand::CMD17(offset), IOSDHostData(data, 1, 512));
    }

    inline IOSDMultiBlocksRequest CMD18(UInt32 offset, IOMemoryDescriptor* data, UInt64 nblocks) const
    {
        return this->makeReadMultiBlocksRequest(IOSDHostCommand::CMD18(offset), IOSDHostData(data, nblocks, 512), IOSDHostCommand::CMD12());
    }

    inline IOSDSingleBlockRequest CMD24(UInt32 offset, IOMemoryDescriptor* data) const
    {
        return this->makeWriteSingleBlockRequest(IOSDHostCommand::CMD24(offset), IOSDHostData(data, 1, 512));
    }

    inline IOSDMultiBlocksRequest CMD25(UInt32 offset, IOMemoryDescriptor* data, UInt64 nblocks) const
    {
        return this->makeWriteMultiBlocksRequest(IOSDHostCommand::CMD25(offset), IOSDHostData(data, nblocks, 512), IOSDHostCommand::CMD12b());
    }

    inline IOSDCommandRequest CMD55(UInt32 rca) const
    {
        return this->makeCommandRequest(IOSDHostCommand::CMD55(rca));
    }

    inline IOSDCommandRequest ACMD6(UInt32 busWidth) const
    {
        return this->makeCommandRequest(IOSDHostCommand::ACMD6(busWidth));
    }

    inline IOSDDataTransferRequest ACMD13(IOMemoryDescriptor* data) const
    {
        return this->makeInboundDataTransferRequest(IOSDHostCommand::ACMD13(), IOSDHostData(data, 1, 64));
    }

    inline IOSDCommandRequest ACMD23(UInt32 nblocks) const
    {
        return this->makeCommandRequest(IOSDHostCommand::ACMD23(nblocks));
    }

    inline IOSDCommandRequest ACMD41(UInt32 ocr) const
    {
        return this->makeCommandRequest(IOSDHostCommand::ACMD41(ocr));
    }

    inline IOSDDataTransferRequest ACMD51(IOMemoryDescriptor* data) const
    {
        return this->makeInboundDataTransferRequest(IOSDHostCommand::ACMD51(), IOSDHostData(data, 1, 8));
    }
};

#endif /* IOSDHostRequest_hpp */
