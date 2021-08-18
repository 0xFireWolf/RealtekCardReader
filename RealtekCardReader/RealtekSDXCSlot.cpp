//
//  RealtekSDXCSlot.cpp
//  RealtekCardReader
//
//  Created by FireWolf on 5/27/21.
//

#include "RealtekCommonRegisters.hpp"
#include "RealtekSDXCSlot.hpp"
#include "IOSDHostDriver.hpp"

//
// MARK: - Meta Class Definitions
//

OSDefineMetaClassAndAbstractStructors(RealtekSDXCSlot, WolfsSDXCSlot);

//
// MARK: - Private Data Structures
//

/// A wrapper of the SD host command that provides Realtek-specific operations
struct RealtekSDHostCommand
{
    /// Wrapped SD host command
    IOSDHostCommand& command;
    
    /// Wrap the given SD host command
    explicit RealtekSDHostCommand(IOSDHostCommand& command) : command(command) {}
    
    ///
    /// Get the register value to be passed to the `SD_CFG2` register
    ///
    /// @return The register value.
    ///
    inline UInt8 getCFG2() const
    {
        using namespace RTSX::COM::Chip;
        
        switch (this->command.getResponseType())
        {
            case IOSDHostCommand::ResponseType::kR0:
            {
                return SD::CFG2::kResponseTypeR0;
            }
                
            case IOSDHostCommand::ResponseType::kR1:
            {
                return SD::CFG2::kResponseTypeR1;
            }
                
            case IOSDHostCommand::ResponseType::kR1b:
            {
                return SD::CFG2::kResponseTypeR1b;
            }
                
            case IOSDHostCommand::ResponseType::kR2:
            {
                return SD::CFG2::kResponseTypeR2;
            }
                
            case IOSDHostCommand::ResponseType::kR3:
            {
                return SD::CFG2::kResponseTypeR3;
            }
                
            case IOSDHostCommand::ResponseType::kR6:
            {
                return SD::CFG2::kResponseTypeR6;
            }
                
            case IOSDHostCommand::ResponseType::kR7:
            {
                return SD::CFG2::kResponseTypeR7;
            }
                
            default:
            {
                pfatal("Missing cases.");
            }
        }
    }
    
    ///
    /// Get the response length in bytes
    ///
    /// @return The actual response length in the host buffer.
    ///
    inline IOByteCount getRealResponseLength() const
    {
        // Note that we reserve 1 byte for the value of `SD_STAT1`
        return max(1, this->command.getResponseLength());
    }
    
    ///
    /// Check whether the start and the transmission bits in the response are valid or not
    ///
    /// @return `true` if both bits are valid, `false` otherwise.
    ///
    inline bool verifyStartAndTransmissionBitsInResponse() const
    {
        using namespace RTSX::COM::Chip::SD::CFG2;
        
        // Guard: Check whether the response contains the ST bits
        if (this->command.getResponseType() == IOSDHostCommand::ResponseType::kR0)
        {
            return true;
        }
        else
        {
            return (this->command.getResponseBuffer()[0] & 0xC0) == 0;
        }
    }
    
    ///
    /// Check whether the CRC7 checksum is valid
    ///
    /// @return `true` if the checksum is valid, `false` otherwise.
    /// @note Since the last byte in the actual response is the value of the register `SD_STAT1`,
    ///       this function checks the register value instead of calculating the CRC7 checksum manually.
    ///
    inline bool verifyCRC7InResponse() const
    {
        using namespace RTSX::COM::Chip::SD;
        
        // Guard: Check whether the driver should ignore the CRC7 checksum
        if (BitOptions<UInt8>(this->getCFG2()).contains(CFG2::kNoCheckCRC7))
        {
            return true;
        }
        else
        {
            return (this->command.getResponseBuffer()[this->getRealResponseLength() - 1] & STAT1::kCRC7Error) == 0;
        }
    }
    
    /// Access the wrapped host command conveniently
    IOSDHostCommand& operator*()
    {
        return this->command;
    }
    
    /// Access the wrapped host command conveniently
    IOSDHostCommand* operator->()
    {
        return &this->command;
    }
    
    /// Wraps the given host command
    static inline RealtekSDHostCommand wraps(IOSDHostCommand& command)
    {
        return RealtekSDHostCommand(command);
    }
};

//
// MARK: - SD Commander
//

///
/// [Shared] [Helper] Inform the card reader which SD command to be executed
///
/// @param command The SD command to be executed
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `sd_cmd_set_sd_cmd()` defined in `rtsx_pci_sdmmc.c`.
/// @warning This function is valid only when there is an active transfer session.
///          i.e. The caller should invoke this function in between `Controller::beginCommandTransfer()` and `Controller::endCommandTransfer()`.
///
IOReturn RealtekSDXCSlot::setSDCommandOpcodeAndArgument(const IOSDHostCommand& command)
{
    using namespace RTSX::COM::Chip::SD;
    
    UInt32 argument = command.getArgument();
    
    const ChipRegValuePair pairs[] =
    {
        // Command index
        { rCMD0, 0xFF, command.getOpcodeWithStartAndTransmissionBits() },
        
        // Command argument in big endian
        { rCMD1, 0xFF, static_cast<UInt8>(argument >> 24) },
        { rCMD2, 0xFF, static_cast<UInt8>(argument >> 16) },
        { rCMD3, 0xFF, static_cast<UInt8>(argument >>  8) },
        { rCMD4, 0xFF, static_cast<UInt8>(argument & 0xFF)},
    };
    
    return this->controller->enqueueWriteRegisterCommands(SimpleRegValuePairs(pairs));
}

///
/// [Shared] [Helper] Inform the card reader the number of blocks to access
///
/// @param nblocks The number of blocks
/// @param blockSize The number of bytes in each block
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `sd_cmd_set_data_len()` defined in `rtsx_pci_sdmmc.c`.
/// @warning This function is valid only when there is an active transfer session.
///          i.e. The caller should invoke this function in between `Controller::beginCommandTransfer()` and `Controller::endCommandTransfer()`.
///
IOReturn RealtekSDXCSlot::setSDCommandDataLength(UInt16 nblocks, UInt16 blockSize)
{
    using namespace RTSX::COM::Chip::SD;
    
    pinfo("Setting the data length: NumBlocks = %d; Block Size = %d Bytes.", nblocks, blockSize);
    
    const ChipRegValuePair pairs[] =
    {
        { rBLOCKCNTL, 0xFF, static_cast<UInt8>(nblocks & 0xFF) },
        { rBLOCKCNTH, 0xFF, static_cast<UInt8>(nblocks >> 8) },
        { rBYTECNTL, 0xFF, static_cast<UInt8>(blockSize & 0xFF) },
        { rBYTECNTH, 0xFF, static_cast<UInt8>(blockSize >> 8) }
    };
    
    return this->controller->enqueueWriteRegisterCommands(SimpleRegValuePairs(pairs));
}

///
/// [Case 1] Send a SD command and wait for the response
///
/// @param command The SD command to be sent
/// @param timeout The amount of time in milliseconds to wait for the response until timed out
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `sd_send_cmd_get_rsp()` defined in `rtsx_pci/usb_sdmmc.c`.
/// @note This function checks whether the start and the transmission bits and the CRC7 checksum in the response are valid.
///       Upon a successful return, the response is guaranteed to be valid, but the caller is responsible for verifying the content.
/// @note This function is invoked by `IOSDHostDriver::CMD*()` and `IOSDHostDriver::ACMD*()` that do not involve a data transfer.
///
IOReturn RealtekSDXCSlot::runSDCommand(IOSDHostCommand& command, UInt32 timeout)
{
    using namespace RTSX::COM::Chip;
    
    // Wrap the given host command
    RealtekSDHostCommand wcmd = RealtekSDHostCommand::wraps(command);
    
    // Fetch the response type and set up the timeout value
    IOSDHostCommand::ResponseType responseType = wcmd->getResponseType();
    
    if (responseType == IOSDHostCommand::ResponseType::kR1b)
    {
        timeout = wcmd->getBusyTimeout(3000);
    }
    
    IOByteCount responseLength = wcmd.getRealResponseLength();
    
    pinfo("SDCMD = %02d; Arg = 0x%08X; Response Length = %llu bytes; Timeout = %d ms.",
          wcmd->getOpcode(), wcmd->getArgument(), responseLength, timeout);
    
    // Start a command transfer session
    IOReturn retVal = this->controller->beginCommandTransfer();
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to initiate a new command transfer session. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // Set the command opcode and its argument
    pinfo("Setting the command opcode and the argument...");
    
    retVal = this->setSDCommandOpcodeAndArgument(*wcmd);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to set the command index and argument. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    pinfo("The command opcode and the argument are set.");
    
    // Set the data source for the card
    // Use the ping pong buffer for SD commands that do not transfer data
    retVal = this->controller->selectCardDataSourceToPingPongBuffer();
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to select the card data source to be the ping pong buffer. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // Set the transfer properties
    pinfo("Setting the transfer properties...");
    
    const ChipRegValuePair wpairs[] =
    {
        // Set the response type
        { SD::rCFG2, 0xFF, wcmd.getCFG2() },
        
        // Transfer an SD command and receive the response
        { SD::rTRANSFER, 0xFF, SD::TRANSFER::kTMCmdResp | SD::TRANSFER::kTransferStart },
    };
    
    retVal = this->controller->enqueueWriteRegisterCommands(SimpleRegValuePairs(wpairs));
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to enqueue a sequence of write operations. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    pinfo("Transfer properties are set.");
    
    // Once the transfer finishes, we need to check the end and the idle bits in the `SD_TRANSFER` register.
    // Note that the Linux driver issues a CHECK REGISTER operation but ignores its return value.
    // We will check the register value manually.
    retVal = this->controller->enqueueCheckRegisterCommand(SD::rTRANSFER,
                                                           SD::TRANSFER::kTransferEnd | SD::TRANSFER::kTransferIdle,
                                                           SD::TRANSFER::kTransferEnd | SD::TRANSFER::kTransferIdle);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to enqueue a check operation to verify the transfer status. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // Find out where to find the response
    pinfo("Setting the location of the command response...");
    
    if (responseType == IOSDHostCommand::ResponseType::kR2)
    {
        // Read 16 bytes from the ping pong buffer 2
        pinfo("Command Response: Will read 16 bytes from the ping pong buffer 2.");
        
        retVal = this->controller->enqueueReadRegisterCommands(ContiguousRegValuePairsForReadAccess(PPBUF::rBASE2, 16));
    }
    else if (responseType != IOSDHostCommand::ResponseType::kR0)
    {
        // Read 5 bytes from the command registers `SD_CMD{0-4}`
        pinfo("Command Response: Will read 5 bytes from the command registers.");
        
        retVal = this->controller->enqueueReadRegisterCommands(ContiguousRegValuePairsForReadAccess(SD::rCMD0, 5));
    }
    else
    {
        // No response
        pinfo("Command Response: No response.");
    }
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to enqueue a sequence of read operations to load the command response. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    pinfo("The location of the command response has been set.");
    
    // The card reader checks the CRC7 value of the response
    // We need to read the value of `SD_STAT1` to ensure that the checksum is valid
    retVal = this->controller->enqueueReadRegisterCommand(SD::rSTAT1);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to enqueue a read operation to load the command status. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // Finish the command transfer session and wait for the response
    retVal = this->controller->endCommandTransfer(timeout, this->controller->getDataTransferFlags().command);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to terminate the command transfer session. Error = 0x%x.", retVal);
        
        this->controller->clearError();
        
        return retVal;
    }
    
    pinfo("Verifying the transfer result...");
    
    //
    // Layout of the host command buffer at this moment:
    //
    // Offset 00: Value of `SD_TRANSFER`
    // Offset 01: The command response
    //            0 byte if the response type is R0;
    //            6 bytes if the response type is not R2;
    //            17 bytes if the response type is R2.
    // Offset LB: Value of `SD_STAT1`
    //
    // Guard: Verify the transfer status
    BitOptions transferStatus = this->controller->readHostBufferValue<UInt8>(0);
    
    if (!transferStatus.contains(SD::TRANSFER::kTransferEnd | SD::TRANSFER::kTransferIdle))
    {
        perr("Failed to find the end and the idle bits in the transfer status (0x%02x).", transferStatus.flatten());

        return kIOReturnInvalid;
    }
    
    pinfo("The transfer status has been verified.");
    
    // Load the response
    retVal = this->controller->readHostBuffer(1, wcmd->getResponseBuffer(), responseLength);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to read the response from the host command buffer. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    wcmd->printResponse();
    
    if (!wcmd.verifyStartAndTransmissionBitsInResponse())
    {
        perr("The start and the transmission bits in the response are invalid.");
        
        return kIOReturnInvalid;
    }
    
    if (!wcmd.verifyCRC7InResponse())
    {
        perr("The CRC7 checksum is invalid.");
        
        return kIOReturnInvalid;
    }
    
    pinfo("The command response is loaded and verified.");
    
    return kIOReturnSuccess;
}

///
/// [Case 1] Send a SD command and wait for the response
///
/// @param request The SD command request to process
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `sd_send_cmd_get_rsp()` defined in `rtsx_pci/usb_sdmmc.c`.
/// @note This function is invoked by `IOSDHostDriver::CMD*()` and `IOSDHostDriver::ACMD*()` that do not involve a data transfer.
/// @note This function serves as the processor routine that handles any simple command requests.
/// @seealso `IOSDHostRequest::processor` and `IOSDHostRequestFactory::commandProcessor`.
///
IOReturn RealtekSDXCSlot::processSDCommandRequest(IOSDCommandRequest& request)
{
    using namespace RTSX::COM::Chip;
    
    // The host must toggle the clock if the command is CMD11
    if (request.command.getOpcode() != IOSDHostCommand::Opcode::kVoltageSwitch)
    {
        return this->runSDCommand(request.command);
    }
    
    // Guard: Toggle the clock
    IOReturn retVal = this->controller->writeChipRegister(SD::rBUSSTAT, 0xFF, SD::BUSSTAT::kClockToggleEnable);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to toggle the clock. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // Guard: Run the command
    retVal = this->runSDCommand(request.command);
    
    if (retVal != kIOReturnSuccess)
    {
        psoftassert(this->controller->writeChipRegister(SD::rBUSSTAT, SD::BUSSTAT::kClockToggleEnable | SD::BUSSTAT::kClockForceStop, 0) == kIOReturnSuccess,
                    "Failed to stop the clock.");
    }
    
    return retVal;
}

///
/// [Case 2] Send a SD command and read the data
///
/// @param command The SD command to be sent
/// @param descriptor A buffer to store the response data and is nullable if the given command is one of the tuning command
/// @param length The number of bytes to read (up to 512 bytes)
/// @param timeout The amount of time in milliseonds to wait for the response data until timed out
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `sd_read_data()` defined in `rtsx_pci_sdmmc.c`.
/// @note This function is invoked by `IOSDHostDriver::CMD*()` and `IOSDHostDriver::ACMD*()` that involve a data transfer.
///
IOReturn RealtekSDXCSlot::runSDCommandAndReadData(IOSDHostCommand& command, IOMemoryDescriptor* descriptor, IOByteCount length, UInt32 timeout)
{
    using namespace RTSX::COM::Chip;
    
    pinfo("SDCMD = %d; Arg = 0x%08X; Data Buffer = 0x%08x%08x; Data Length = %llu bytes; Timeout = %d ms.",
          command.getOpcode(), command.getArgument(), KPTR(descriptor), length, timeout);
    
    // Start a command transfer session
    IOReturn retVal = this->controller->beginCommandTransfer();
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to initiate a new command transfer session. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // Set the command index and the argument
    pinfo("Setting the command opcode and the argument...");
    
    retVal = this->setSDCommandOpcodeAndArgument(command);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to set the command index and argument. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    pinfo("The command opcode and the argument are set.");
    
    // Set the number of data blocks and the size of each block
    pinfo("Setting the length of the data blocks associated with the command...");
    
    retVal = this->setSDCommandDataLength(1, length);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to set the command data blocks and length. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    pinfo("the data length has been set.");
    
    // Set the transfer properties
    pinfo("Setting the transfer properties...");
    
    ChipRegValuePair pairs[] =
    {
        { SD::rCFG2, 0xFF, SD::CFG2::kCalcCRC7 | SD::CFG2::kCheckCRC7 | SD::CFG2::kCheckCRC16 | SD::CFG2::kNoWaitBusyEnd | SD::CFG2::kResponseLength6 },
        
        { SD::rTRANSFER, 0xFF, SD::TRANSFER::kTransferStart }
    };
    
    // Adjust the transfer mode
    if (command.getOpcode() == IOSDHostCommand::Opcode::kSendTuningBlock)
    {
        pairs[1].value |= SD::TRANSFER::kTMAutoTuning;
    }
    else
    {
        pairs[1].value |= SD::TRANSFER::kTMNormalRead;
        
        // Ask the card to send the data to the ping pong buffer
        // This step is omitted if `command` is a tuning command (i.e., CMD19)
        retVal = this->controller->selectCardDataSourceToPingPongBuffer();
        
        if (retVal != kIOReturnSuccess)
        {
            perr("Failed to select the card data source to be the ping pong buffer. Error = 0x%x.", retVal);
            
            return retVal;
        }
    }
    
    retVal = this->controller->enqueueWriteRegisterCommands(SimpleRegValuePairs(pairs));
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to set the transfer property. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    pinfo("Transfer properties are set.");
    
    // Once the transfer finishes, we need to check the end bit in the `SD_TRANSFER` register.
    // Note that the Linux driver issues a CHECK REGISTER operation but ignores its return value.
    // We will check the register value manually.
    retVal = this->controller->enqueueCheckRegisterCommand(SD::rTRANSFER, SD::TRANSFER::kTransferEnd, SD::TRANSFER::kTransferEnd);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to enqueue a read operation to load the transfer status. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // Finish the command transfer session and wait for the response
    retVal = this->controller->endCommandTransfer(timeout, this->controller->getDataTransferFlags().commandWithInboundDataTransfer);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to complete the command transfer session. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // Verify the transfer status
    pinfo("Verifying the transfer result...");
    
    BitOptions<UInt8> transferStatus = this->controller->readHostBufferValue<UInt8>(0);
    
    pinfo("Transfer status = 0x%02x.", transferStatus.flatten());
    
    if (!transferStatus.contains(SD::TRANSFER::kTransferEnd))
    {
        perr("Failed to find the end and the idle bits in the transfer status (0x%02x).", transferStatus.flatten());

        return kIOReturnInvalid;
    }
    
    // Read the response from the ping pong buffer
    if (command.getOpcode() == IOSDHostCommand::Opcode::kSendTuningBlock)
    {
        pinfo("Tuning Command: No need to read the response from the ping pong buffer.");
        
        return kIOReturnSuccess;
    }
    
    // Warning: The USB driver separates the loading process into two parts:
    //          the 2-byte aligned part and the unaligned part.
    //          We keep the implementation that is designed for the PCIe-based card reader driver,
    //          because the host driver guarantees that the number of bytes to read is always 2-byte aligned.
    //          For example, it sends a CMD2 to retrieve the card identification data which is 16 bytes long.
    //          Ideally, the host device (i.e. RealtekSDXCSlot) should not assume that the caller always passes a length that is 2-byte aligned,
    //          but I don't have a USB-based card driver so I cannot verify whether the card driver accepts a 2-byte aligned length only.
    //          Besides, Linux's rtsx_usb_read_ppbuf() further separates the read into two parts, the 4-byte aligned part and the unaligned part,
    //          so why do we need to do aligned read at here?
    //          P.S. Our implementation of `rtsx_usb_read_ppbuf()` ignores the 4-byte alignment,
    //               because the host driver always passes a length that can be divided by 4 :).
    pinfo("Loading the command response from the ping pong buffer...");
    
    retVal = this->controller->readPingPongBuffer(descriptor, length);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to load the command response from the ping pong buffer. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    pbuf(descriptor, length);
    
    pinfo("The command response has been loaded from the ping pong buffer.");
    
    return kIOReturnSuccess;
}

///
/// [Case 2] Send a SD command along with the data
///
/// @param command The SD command to be sent
/// @param descriptor A non-null buffer that contains the data for the given command
/// @param length The number of bytes to write (up to 512 bytes)
/// @param timeout The amount of time in milliseonds to wait for completion until timed out
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `sd_write_data()` defined in `rtsx_pci_sdmmc.c`.
/// @note This function is invoked by `IOSDHostDriver::CMD*()` and `IOSDHostDriver::ACMD*()` that involve a data transfer.
///
IOReturn RealtekSDXCSlot::runSDCommandAndWriteData(IOSDHostCommand& command, IOMemoryDescriptor* descriptor, IOByteCount length, UInt32 timeout)
{
    using namespace RTSX::COM::Chip;
    
    pinfo("SDCMD = %d; Arg = 0x%08X; Data Buffer = 0x%08x%08x; Data Length = %llu bytes; Timeout = %d ms.",
          command.getOpcode(), command.getArgument(), KPTR(descriptor), length, timeout);
    
    // Send the SD command
    pinfo("Sending the SD command...");
    
    IOReturn retVal = this->runSDCommand(command);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to send the SD command. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    pinfo("The SD command has been sent.");
    
    // Write the data to the ping pong buffer
    pinfo("Writing the data to the ping pong buffer...");
    
    pbuf(descriptor, length);
    
    retVal = this->controller->writePingPongBuffer(descriptor, length);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to write the given data to the ping pong buffer. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    pinfo("Data has been written to the ping pong buffer.");
    
    // Start a command transfer session
    retVal = this->controller->beginCommandTransfer();
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to initiate a new command transfer session. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // Set the number of data blocks and the size of each block
    pinfo("Setting the length of the data blocks associated with the command...");
    
    retVal = this->setSDCommandDataLength(1, length);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to set the command data blocks and length. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    pinfo("the data length has been set.");
    
    // Set the transfer property
    pinfo("Setting the transfer properties...");
    
    const ChipRegValuePair pairs[] =
    {
        { SD::rCFG2, 0xFF, SD::CFG2::kCalcCRC7 | SD::CFG2::kCheckCRC7 | SD::CFG2::kCheckCRC16 | SD::CFG2::kNoWaitBusyEnd | SD::CFG2::kResponseLength0 },
        
        { SD::rTRANSFER, 0xFF, SD::TRANSFER::kTransferStart | SD::TRANSFER::kTMAutoWrite3 }
    };
    
    retVal = this->controller->enqueueWriteRegisterCommands(SimpleRegValuePairs(pairs));
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to set the transfer property. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    pinfo("Transfer properties are set.");
    
    // Once the transfer finishes, we need to check the end bit in the `SD_TRANSFER` register.
    // Note that the Linux driver issues a CHECK REGISTER operation but ignores its return value.
    // We will check the register value manually.
    retVal = this->controller->enqueueCheckRegisterCommand(SD::rTRANSFER, SD::TRANSFER::kTransferEnd, SD::TRANSFER::kTransferEnd);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to enqueue a read operation to load the transfer status. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // Finish the command transfer session and wait for the response
    retVal = this->controller->endCommandTransfer(timeout, this->controller->getDataTransferFlags().commandWithOutboundDataTransfer);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to terminate the command transfer session. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // Verify the transfer status
    pinfo("Verifying the transfer result...");
    
    BitOptions<UInt8> transferStatus = this->controller->readHostBufferValue<UInt8>(0);
    
    pinfo("Transfer status = 0x%02x.", transferStatus.flatten());
    
    if (!transferStatus.contains(SD::TRANSFER::kTransferEnd))
    {
        perr("Failed to find the end bit in the transfer status (0x%02x).", transferStatus.flatten());

        return kIOReturnInvalid;
    }
    
    return kIOReturnSuccess;
}

///
/// [Case 2] Send a SD command and read the data
///
/// @param request A data transfer request to service
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces the read portion of `sd_normal_rw()` defined in `rtsx_pci_sdmmc.c`.
/// @note This function is invoked by `IOSDHostDriver::CMD*()` and `IOSDHostDriver::ACMD*()` that involve a data transfer.
/// @note This function serves as the processor routine that handles command requests that transfer control data from the card.
/// @seealso `IOSDHostRequest::processor` and `IOSDHostRequestFactory::inboundDataTransferProcessor`.
///
IOReturn RealtekSDXCSlot::processSDCommandWithInboundDataTransferRequest(IOSDDataTransferRequest& request)
{
    // In this case, the host can transfer up to 512 bytes to the card
    // Guard: Switch the clock divider to 0 if necessary
    IOReturn retVal = this->disableInitialModeIfNecessary();
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to disable the initial mode. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // Guard: Fetch the block size from the request
    psoftassert(request.data.getNumBlocks() == 1, "Warning: The number of blocks should be 1 in this case.");
    
    IOByteCount blockSize = request.data.getBlockSize();
    
    if (blockSize > 512)
    {
        perr("The block size (%llu bytes) is too large. Must not exceed 512 bytes.", blockSize);
        
        return kIOReturnBadArgument;
    }
    
    // Guard: Send the command and read the data
    retVal = this->runSDCommandAndReadData(request.command, request.data.getMemoryDescriptor(), blockSize, 200);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to send the command and read the data. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // Guard: Switch the clock divider to 128 if necessary
    retVal = this->enableInitialModeIfNecessary();
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to enable the initial mode. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    return kIOReturnSuccess;
}

///
/// [Case 2] Send a SD command along with the data
///
/// @param request A data transfer request to service
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces the write portion of `sd_normal_rw()` defined in `rtsx_pci_sdmmc.c`.
/// @note This function is invoked by `IOSDHostDriver::CMD*()` and `IOSDHostDriver::ACMD*()` that involve a data transfer.
/// @note This function serves as the processor routine that handles command requests that transfer control data to the card.
/// @seealso `IOSDHostRequest::processor` and `IOSDHostRequestFactory::outboundDataTransferProcessor`.
///
IOReturn RealtekSDXCSlot::processSDCommandWithOutboundDataTransferRequest(IOSDDataTransferRequest& request)
{
    // In this case, the host can transfer up to 512 bytes to the card
    // Guard: Fetch the block size from the request
    psoftassert(request.data.getNumBlocks() == 1, "Warning: The number of blocks should be 1 in this case.");
    
    IOByteCount blockSize = request.data.getBlockSize();
    
    if (blockSize > 512)
    {
        perr("The block size (%llu bytes) is too large. Must not exceed 512 bytes.", blockSize);
        
        return kIOReturnBadArgument;
    }
    
    // Guard: Send the command and write the data
    return this->runSDCommandAndWriteData(request.command, request.data.getMemoryDescriptor(), blockSize, 200);
}

///
/// [Case 3] Send a SD command along with an inbound DMA transfer
///
/// @param request A block-oriented data transfer request to service
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `sd_read_long_data()` defined in `rtsx_pci_sdmmc.c`.
/// @note This function is invoked by `IOSDHostDriver::CMD*()` and `IOSDHostDriver::ACMD*()` that involve a DMA transfer.
/// @note This function serves as the processor routine that handles command requests that read a single block from the card.
/// @seealso `IOSDHostRequest::processor` and `IOSDHostRequestFactory::readSingleBlockProcessor`.
///
IOReturn RealtekSDXCSlot::processSDCommandWithInboundSingleBlockDMATransferRequest(IOSDSingleBlockRequest& request)
{
    using namespace RTSX::COM::Chip;
    
    // Send the SD command
    IOReturn retVal = this->runSDCommand(request.command);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to send the SD command. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // Set up the SD_CFG2 register value
    UInt8 cfg2 = SD::CFG2::kCalcCRC7 | SD::CFG2::kCheckCRC7 | SD::CFG2::kCheckCRC16 | SD::CFG2::kNoWaitBusyEnd | SD::CFG2::kResponseLength0;
    
    // FIXME: PCIe specific???
    if (!this->isRunningInUltraHighSpeedMode())
    {
        cfg2 |= SD::CFG2::kNoCheckWaitCRCTo;
    }
    
    // Fetch the data length
    UInt64 dataLength = request.data.getDataLength();
    
    psoftassert(dataLength <= UINT32_MAX, "The data length should not exceed UINT32_MAX.");
    
    pinfo("SDCMD = %d; Arg = 0x%08X; Data Length = %llu bytes.", request.command.getOpcode(), request.command.getArgument(), dataLength);
    
    // Start a command transfer session
    retVal = this->controller->beginCommandTransfer();
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to initiate a new command transfer session. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // Set the number of data blocks and the size of each block
    pinfo("Setting the length of the data blocks associated with the command...");
    
    retVal = this->setSDCommandDataLength(request.data.getNumBlocks(), request.data.getBlockSize());
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to set the command data blocks and length. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    pinfo("The data length has been set.");
    
    // Set the transfer property
    pinfo("Setting the transfer properties...");
    
    retVal = this->controller->setupCardDMATransferProperties(static_cast<UInt32>(dataLength), kIODirectionIn);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to set the DMA transfer properties. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    retVal = this->controller->selectCardDataSourceToRingBuffer();
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to select the card data source to be the ring buffer. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    const ChipRegValuePair pairs[] =
    {
        { SD::rCFG2, 0xFF, cfg2 },
        { SD::rTRANSFER, 0xFF, SD::TRANSFER::kTransferStart | SD::TRANSFER::kTMAutoRead3 },
    };
    
    retVal = this->controller->enqueueWriteRegisterCommands(SimpleRegValuePairs(pairs));
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to set the transfer property. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    pinfo("Transfer properties are set.");
    
    // Once the transfer finishes, we need to check the end bit in the `SD_TRANSFER` register.
    // Note that the Linux driver issues a CHECK REGISTER operation but ignores its return value.
    // We will check the register value manually.
//    retVal = this->controller->enqueueCheckRegisterCommand(SD::rTRANSFER, SD::TRANSFER::kTransferEnd, SD::TRANSFER::kTransferEnd);
//
//    if (retVal != kIOReturnSuccess)
//    {
//        perr("Failed to enqueue a read operation to load the transfer status. Error = 0x%x.", retVal);
//
//        return retVal;
//    }
    
    // Send the command
    retVal = this->controller->endCommandTransfer(100, this->controller->getDataTransferFlags().commandWithInboundDMATransfer); // Linux uses NoWait().

    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to complete the command transfer session. Error = 0x%x.", retVal);

        return retVal;
    }
  
    //this->controller->endCommandTransferNoWait();
    
    // Verify the transfer status
//    pinfo("Verifying the transfer result...");
//
//    BitOptions<UInt8> transferStatus = this->controller->peekHostBuffer<UInt8>(0);
//
//    if (!transferStatus.contains(SD::TRANSFER::kTransferEnd))
//    {
//        perr("Failed to find the end and the idle bits in the transfer status (0x%02x).", transferStatus.flatten());
//
//        return kIOReturnInvalid;
//    }
    
    // Initiate the DMA transfer
    pinfo("Initiating the DMA transfer...");
    
    retVal = this->controller->performDMARead(request.data.getMemoryDescriptor(), 10000);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to perform the DMA transfer. Error = 0x%x.", retVal);
        
        this->controller->clearError();
        
        return retVal;
    }
    
    pinfo("DMA transfer completed successfully.");
    
    return kIOReturnSuccess;
}

///
/// [Case 3] Send a SD command along with an outbound DMA transfer
///
/// @param request A block-oriented data transfer request to service
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `sd_write_long_data()` defined in `rtsx_pci_sdmmc.c`.
/// @note This function is invoked by `IOSDHostDriver::CMD*()` and `IOSDHostDriver::ACMD*()` that involve a DMA transfer.
/// @note This function serves as the processor routine that handles command requests that write a single block to the card.
/// @seealso `IOSDHostRequest::processor` and `IOSDHostRequestFactory::writeSingleBlockProcessor`.
///
IOReturn RealtekSDXCSlot::processSDCommandWithOutboundSingleBlockDMATransferRequest(IOSDSingleBlockRequest& request)
{
    using namespace RTSX::COM::Chip;
    
    // Send the SD command
    IOReturn retVal = this->runSDCommand(request.command);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to send the SD command. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // Set up the SD_CFG2 register value
    UInt8 cfg2 = SD::CFG2::kNoCalcCRC7 | SD::CFG2::kNoCheckCRC7 | SD::CFG2::kCheckCRC16 | SD::CFG2::kNoWaitBusyEnd | SD::CFG2::kResponseLength0;
    
    // FIXME: PCIe specific???
    if (!this->isRunningInUltraHighSpeedMode())
    {
        cfg2 |= SD::CFG2::kNoCheckWaitCRCTo;
    }
    
    // Fetch the data length
    UInt64 dataLength = request.data.getDataLength();
    
    psoftassert(dataLength <= UINT32_MAX, "The data length should not exceed UINT32_MAX.");
    
    pinfo("SDCMD = %d; Arg = 0x%08X; Data Length = %llu bytes.", request.command.getOpcode(), request.command.getArgument(), dataLength);
    
    // Start a command transfer session
    retVal = this->controller->beginCommandTransfer();
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to initiate a new command transfer session. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // Set the number of data blocks and the size of each block
    pinfo("Setting the length of the data blocks associated with the command...");
    
    retVal = this->setSDCommandDataLength(request.data.getNumBlocks(), request.data.getBlockSize());
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to set the command data blocks and length. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    pinfo("The data length has been set.");
    
    // Set the transfer property
    pinfo("Setting the transfer properties...");
    
    retVal = this->controller->setupCardDMATransferProperties(static_cast<UInt32>(dataLength), kIODirectionOut);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to set the DMA transfer properties. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    retVal = this->controller->selectCardDataSourceToRingBuffer();
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to select the card data source to be the ring buffer. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    const ChipRegValuePair pairs[] =
    {
        { SD::rCFG2, 0xFF, cfg2 },
        { SD::rTRANSFER, 0xFF, SD::TRANSFER::kTransferStart | SD::TRANSFER::kTMAutoWrite3 },
    };
    
    retVal = this->controller->enqueueWriteRegisterCommands(SimpleRegValuePairs(pairs));
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to set the transfer property. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    pinfo("Transfer properties are set.");
    
    // Once the transfer finishes, we need to check the end bit in the `SD_TRANSFER` register.
    // Note that the Linux driver issues a CHECK REGISTER operation but ignores its return value.
    // We will check the register value manually.
//    retVal = this->controller->enqueueCheckRegisterCommand(SD::rTRANSFER, SD::TRANSFER::kTransferEnd, SD::TRANSFER::kTransferEnd);
//
//    if (retVal != kIOReturnSuccess)
//    {
//        perr("Failed to enqueue a read operation to load the transfer status. Error = 0x%x.", retVal);
//
//        return retVal;
//    }
    
    // Send the command
    retVal = this->controller->endCommandTransfer(100, this->controller->getDataTransferFlags().commandWithOutboundDMATransfer); // Linux uses NoWait().

    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to complete the command transfer session. Error = 0x%x.", retVal);

        return retVal;
    }
  
    //this->controller->endCommandTransferNoWait();
    
    // Verify the transfer status
//    pinfo("Verifying the transfer result...");
//
//    BitOptions<UInt8> transferStatus = this->controller->peekHostBuffer<UInt8>(0);
//
//    if (!transferStatus.contains(SD::TRANSFER::kTransferEnd))
//    {
//        perr("Failed to find the end and the idle bits in the transfer status (0x%02x).", transferStatus.flatten());
//
//        return kIOReturnInvalid;
//    }
    
    // Initiate the DMA transfer
    pinfo("Initiating the DMA transfer...");
    
    retVal = this->controller->performDMAWrite(request.data.getMemoryDescriptor(), 10000);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to perform the DMA transfer. Error = 0x%x.", retVal);
        
        this->controller->clearError();
        
        return retVal;
    }
    
    pinfo("DMA transfer completed successfully.");
    
    return kIOReturnSuccess;
}

///
/// [Case 3] Send a SD command along with an inbound DMA transfer
///
/// @param request A block-oriented data transfer request to service
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `sd_read_long_data()` defined in `rtsx_pci_sdmmc.c`.
/// @note This function is invoked by `IOSDHostDriver::CMD*()` and `IOSDHostDriver::ACMD*()` that involve a DMA transfer.
/// @note This function serves as the processor routine that handles command requests that read multiple blocks from the card.
/// @seealso `IOSDHostRequest::processor` and `IOSDHostRequestFactory::readMultiBlocksProcessor`.
///
IOReturn RealtekSDXCSlot::processSDCommandWithInboundMultiBlocksDMATransferRequest(IOSDMultiBlocksRequest& request)
{
    IOReturn retVal = this->processSDCommandWithInboundSingleBlockDMATransferRequest(request);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to service the request that reads multiple blocks. Error = 0x%x.", retVal);
    }
    
    psoftassert(this->runSDCommand(request.stopCommand) == kIOReturnSuccess, "Failed to send the STOP command.");
    
    return retVal;
}

///
/// [Case 3] Send a SD command along with an outbound DMA transfer
///
/// @param request A block-oriented data transfer request to service
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `sd_write_long_data()` defined in `rtsx_pci_sdmmc.c`.
/// @note This function is invoked by `IOSDHostDriver::CMD*()` and `IOSDHostDriver::ACMD*()` that involve a DMA transfer.
/// @note This function serves as the processor routine that handles command requests that write multiple blocks to the card.
/// @seealso `IOSDHostRequest::processor` and `IOSDHostRequestFactory::writeMultiBlocksProcessor`.
///
IOReturn RealtekSDXCSlot::processSDCommandWithOutboundMultiBlocksDMATransferRequest(IOSDMultiBlocksRequest& request)
{
    IOReturn retVal = this->processSDCommandWithOutboundSingleBlockDMATransferRequest(request);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to service the request that writes multiple blocks. Error = 0x%x.", retVal);
    }
    
    psoftassert(this->runSDCommand(request.stopCommand) == kIOReturnSuccess, "Failed to send the STOP command.");
    
    return retVal;
}

///
/// Process the given SD command request
///
/// @param request A SD command request
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `sd_request()` defined in `rtsx_pci_sdmmc.c`.
///
IOReturn RealtekSDXCSlot::processRequest(IOSDHostRequest& request)
{
    // Guard: Check whether the card is still present
    if (!this->controller->isCardPresent())
    {
        perr("The card is not present. Will abort the request.");
        
        return kIOReturnNoMedia;
    }
    
    // Notify the card reader to enter the worker state
    this->controller->enterWorkerState();
    
    pinfo("The host driver has sent a SD command request.");
    
    // Guard: Switch the clock
    pinfo("Switching the clock...");
    
    IOReturn retVal = this->controller->switchCardClock(this->cardClock, this->sscDepth, this->initialMode, this->doubleClock, this->vpclock);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to switch the clock for the incoming request. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    pinfo("The clock has been switched.");
    
    // Guard: Select the card
    pinfo("Selecting the SD card...");
    
    auto action = [&]() -> IOReturn
    {
        passert(this->controller->selectCard() == kIOReturnSuccess, "Failed to select the card.");
        
        passert(this->controller->configureCardShareMode() == kIOReturnSuccess, "Failed to configure the card share mode.");
        
        return kIOReturnSuccess;
    };
    
    retVal = this->controller->withCustomCommandTransfer(action);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to select the SD card. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    pinfo("The SD card has been selected.");
    
    // Guard: Dispatch the request
    pinfo("Servicing the request...");
    
    retVal = request.process();
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to service the request. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    pinfo("The request has been serviced.");
    
    return kIOReturnSuccess;
}

//
// MARK: - SD Bus Configurator
//

///
/// Check whether the host device is running in the ultra high speed mode
///
/// @return `true` if the current bus timing mode is one of SDR12/25/50/104 and DDR50
///
bool RealtekSDXCSlot::isRunningInUltraHighSpeedMode()
{
    return this->busConfig.busTiming >= IOSDBusConfig::BusTiming::kUHSSDR12 &&
           this->busConfig.busTiming <= IOSDBusConfig::BusTiming::kUHSDDR50;
}

///
/// Set the bus width
///
/// @param width The target bus width
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `sd_set_bus_width()` defined in `rtsx_pci_sdmmc.c`.
/// @note This function acts as a service routine of `RealtekSDXCSlot::setIOConfig()`.
///
IOReturn RealtekSDXCSlot::setBusWidth(IOSDBusConfig::BusWidth width)
{
    using namespace RTSX::COM::Chip::SD;
    
    UInt8 regValue = 0;
    
    switch (width)
    {
        case IOSDBusConfig::BusWidth::k1Bit:
        {
            regValue = CFG1::kBusWidth1Bit;
            
            pinfo("Will set the bus width to 1 bit.");
            
            break;
        }
            
        case IOSDBusConfig::BusWidth::k4Bit:
        {
            regValue = CFG1::kBusWidth4Bit;
            
            pinfo("Will set the bus width to 4 bits.");
            
            break;
        }
            
        case IOSDBusConfig::BusWidth::k8Bit:
        {
            regValue = CFG1::kBusWidth8Bit;
            
            pinfo("Will set the bus width to 8 bits.");
            
            break;
        }
    }
    
    return this->controller->writeChipRegister(rCFG1, CFG1::kBusWidthMask, regValue);
}

///
/// [Helper] Power on the card slot
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `sd_power_on()` defined in `rtsx_pci_sdmmc.c`.
/// @note This function acts as a service routine of `RealtekSDXCSlot::setIOConfig()`.
///
IOReturn RealtekSDXCSlot::powerOn()
{
    pinfo("Powering on the card slot...");
    
    // Select the SD card and enable the clock
    pinfo("Selecting and enabling the SD card...");
    
    auto action1 = [&]() -> IOReturn
    {
        passert(this->controller->selectCard() == kIOReturnSuccess, "Failed to select the SD card.");
        
        passert(this->controller->configureCardShareMode() == kIOReturnSuccess, "Failed to configure the card share mode.");
        
        passert(this->controller->enableCardClock() == kIOReturnSuccess, "Failed to enable the card clock.");
        
        return kIOReturnSuccess;
    };
    
    IOReturn retVal = this->controller->withCustomCommandTransfer(action1);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to select the SD card and enable the clock. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    pinfo("The SD card is selected and enabled.");
    
    // Enable the card pull control
    pinfo("Enabling the pull control for the SD card.");
    
    retVal = this->controller->enablePullControlForSDCard();
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to enable the pull control for the SD card. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    pinfo("The pull control has been enabled for the SD card.");
    
    // Power on the SD card
    pinfo("Powering on the SD card...");
    
    retVal = this->controller->powerOnCard();
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to power on the SD card. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    pinfo("The SD card power is now on.");
    
    // Enable the card output
    pinfo("Enabling the card output...");
    
    auto action2 = [&]() -> IOReturn
    {
        return this->controller->enableCardOutput();
    };
    
    retVal = this->controller->withCustomCommandTransfer(action2);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to enable the card output. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    pinfo("The card output has been enabled.");
    
    return kIOReturnSuccess;
}

///
/// [Helper] Power off the card slot
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `sd_power_off()` defined in `rtsx_pci_sdmmc.c`.
/// @note This function acts as a service routine of `RealtekSDXCSlot::setIOConfig()`.
///
IOReturn RealtekSDXCSlot::powerOff()
{
    pinfo("Powering off the card slot...");
    
    // Disable the card clock and the output
    pinfo("Disabling the card clock and output...");
    
    auto action = [&]() -> IOReturn
    {
        passert(this->controller->disableCardClock() == kIOReturnSuccess, "Failed to disable the card clock.");
        
        passert(this->controller->disableCardOutput() == kIOReturnSuccess, "Failed to disable the card output.");
        
        return kIOReturnSuccess;
    };
    
    IOReturn retVal = this->controller->withCustomCommandTransfer(action);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to disable the clock and the output. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    pinfo("The card clock and output have been disabled.");
    
    // Power off the card
    pinfo("Powering off the card...");
    
    retVal = this->controller->powerOffCard();
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to power off the SD card. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    pinfo("The card power is now off.");
    
    // Disable the card pull control
    pinfo("Disabling the pull control for the SD card...")
    
    retVal = this->controller->disablePullControlForSDCard();
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to disable the card control. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    pinfo("The pull control has been disabled for the card.");
    
    return kIOReturnSuccess;
}

///
/// Set the power mode
///
/// @param mode The target power mode
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `sd_set_power_mode()` defined in `rtsx_pci_sdmmc.c`.
/// @note This function acts as a service routine of `RealtekSDXCSlot::setIOConfig()`.
///
IOReturn RealtekSDXCSlot::setPowerMode(IOSDBusConfig::PowerMode mode)
{
    // Guard: Check the current bus power mode
    if (this->powerMode == mode)
    {
        pinfo("The current bus power mode is the same as the requested one.");
        
        return kIOReturnSuccess;
    }
    
    // Set the power mode
    IOReturn retVal = mode == IOSDBusConfig::PowerMode::kPowerOff ? this->powerOff() : this->powerOn();
    
    if (retVal == kIOReturnSuccess)
    {
        this->powerMode = mode;
    }
    
    return retVal;
}

///
/// Set the bus speed mode
///
/// @param timing The target bus speed mode
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `sd_set_timing()` defined in `rtsx_pci_sdmmc.c`.
/// @note This function acts as a service routine of `RealtekSDXCSlot::setIOConfig()`.
///
IOReturn RealtekSDXCSlot::setBusTiming(IOSDBusConfig::BusTiming timing)
{
    const SimpleRegValuePairs* pairs = nullptr;
    
    switch (timing)
    {
        case IOSDBusConfig::BusTiming::kUHSSDR104:
        case IOSDBusConfig::BusTiming::kUHSSDR50:
        {
            pinfo("Setting the bus timing for the UHS-I SDR50/SDR104 mode...");
            
            pairs = this->controller->getBusTimingTables().sdr50;
            
            break;
        }
            
        case IOSDBusConfig::BusTiming::kMMCDDR52:
        case IOSDBusConfig::BusTiming::kUHSDDR50:
        {
            pinfo("Setting the bus timing for the UHS-I DDR50 mode...");
            
            pairs = this->controller->getBusTimingTables().ddr50;
            
            break;
        }
            
        case IOSDBusConfig::BusTiming::kMMCHighSpeed:
        case IOSDBusConfig::BusTiming::kSDHighSpeed:
        {
            pinfo("Setting the bus timing for the high speed mode...");
            
            pairs = this->controller->getBusTimingTables().highSpeed;
            
            break;
        }
            
        default:
        {
            pinfo("Setting the bus timing for the default speed mode...");
            
            pairs = this->controller->getBusTimingTables().defaultSpeed;
            
            break;
        }
    }
    
    // Hack: Need to pass `Packet::Flags::kC` to the transfer function,
    //       but the default value of the `flags` argument has the same value as `kC`.
    return this->controller->transferWriteRegisterCommands(*pairs);
}

///
/// Apply the given I/O configuration
///
/// @param config An I/O config
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `sdmmc_set_ios()` defined in `rtsx_pci_sdmmc.c`.
///
IOReturn RealtekSDXCSlot::setBusConfig(const IOSDBusConfig& config)
{
    pinfo("The host driver requests to change the bus configuration.");
    
    // Notify the card reader to enter the worker state
    this->controller->enterWorkerState();
    
    // Set the bus width
    pinfo("Setting the bus width...");
    
    IOReturn retVal = this->setBusWidth(config.busWidth);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to set the bus width to %d. Error = 0x%x.", 1 << static_cast<int>(config.busWidth), retVal);
        
        return retVal;
    }
    
    pinfo("Bus width has been set.");
    
    // Set the power mode
    pinfo("Setting the power mode...");
    
    retVal = this->setPowerMode(config.powerMode);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to set the power mode to %hhu. Error = 0x%x.", config.powerMode, retVal);
        
        return retVal;
    }
    
    pinfo("The power mode has been set.");
    
    // Set the bus timing
    pinfo("Setting the bus timing mode...");
    
    retVal = this->setBusTiming(config.busTiming);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to set the bus timing mode to %hhu. Error = 0x%x.", config.busTiming, retVal);
        
        return retVal;
    }
    
    pinfo("The bus timing mode has been set.");
    
    // Set the SSC depth and clock properties
    pinfo("Setting the clock...");
    
    switch (config.busTiming)
    {
        case IOSDBusConfig::BusTiming::kUHSSDR104:
        case IOSDBusConfig::BusTiming::kUHSSDR50:
        {
            this->sscDepth = SSCDepth::k2M;
            
            this->vpclock = true;
            
            this->doubleClock = false;
            
            break;
        }
            
        case IOSDBusConfig::BusTiming::kMMCDDR52:
        case IOSDBusConfig::BusTiming::kUHSDDR50:
        case IOSDBusConfig::BusTiming::kUHSSDR25:
        {
            this->sscDepth = SSCDepth::k1M;
            
            this->vpclock = false;
            
            this->doubleClock = true;
            
            break;
        }
            
        default:
        {
            this->sscDepth = SSCDepth::k500K;
            
            this->vpclock = false;
            
            this->doubleClock = true;
            
            break;
        }
    }
    
    this->initialMode = config.clock <= MHz2Hz(1);
    
    this->cardClock = config.clock;
    
    retVal = this->controller->switchCardClock(this->cardClock, this->sscDepth, this->initialMode, this->doubleClock, this->vpclock);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to switch the clock.");
        
        return retVal;
    }
    
    pinfo("The new bus configuration is now active.");
    
    return kIOReturnSuccess;
}

///
/// [Helper] [Phase 1] Wait until the signal voltage becomes stable
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `sd_wait_voltage_stable_1()` defined in `rtsx_pci_sdmmc.c`.
/// @note This function is called after the host driver sends a CMD11 to switch to 1.8V.
///
IOReturn RealtekSDXCSlot::waitVoltageStable1()
{
    using namespace RTSX::COM::Chip;
    
    // After the host driver sends a CMD11 and receives the response,
    // wait for 1 ms so that the card can drive both CMD and DATA lines to low
    IOSleep(1);
    
    // Read the current status of both CMD and DATA lines
    UInt8 status = 0;
    
    IOReturn retVal = kIOReturnSuccess;
    
    // Some cards are just slow so let's be graceful
    // Wait until the card drive both CMD and DATA lines to low
    for (auto attempt = 0; attempt < 200; attempt += 1)
    {
        IOSleep(20);
        
        pinfo("[%02d] Reading the status of all lines...", attempt);
        
        retVal = this->controller->readChipRegister(SD::rBUSSTAT, status);
        
        if (retVal != kIOReturnSuccess)
        {
            perr("Failed to read the command and the data line status. Error = 0x%x.", retVal);
            
            continue;
        }
        
        pinfo("[%02d] Current line status = 0x%02x.", attempt, status);
        
        // Guard: Ensure that both lines are low
        if (BitOptions(status).containsOneOf(SD::BUSSTAT::kCommandStatus,
                                             SD::BUSSTAT::kData0Status,
                                             SD::BUSSTAT::kData1Status,
                                             SD::BUSSTAT::kData2Status,
                                             SD::BUSSTAT::kData3Status))
        {
            perr("At least one of lines is high.");
            
            continue;
        }
        
        pinfo("Both command and data lines are low.");
        
        // Stop the SD clock
        retVal = this->controller->writeChipRegister(SD::rBUSSTAT, 0xFF, SD::BUSSTAT::kClockForceStop);
        
        if (retVal != kIOReturnSuccess)
        {
            perr("Failed to stop the SD clock. Error = 0x%x.", retVal);
            
            return retVal;
        }
        
        return kIOReturnSuccess;
    }
    
    perr("Timed out while waiting for the card to drive both lines to low.");
    
    return kIOReturnTimeout;
}

///
/// [Helper] [Phase 2] Wait until the signal voltage becomes stable
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `sd_wait_voltage_stable_2()` defined in `rtsx_pci_sdmmc.c`.
/// @note This function is called after the controller switches the signal voltage.
///
IOReturn RealtekSDXCSlot::waitVoltageStable2()
{
    using namespace RTSX::COM::Chip;
    
    // Wait until the regulator becomes stable
    IOSleep(50);
    
    // Guard: Enable the SD clock
    IOReturn retVal = this->controller->writeChipRegister(SD::rBUSSTAT, 0xFF, SD::BUSSTAT::kClockToggleEnable);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to enable the SD clock. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // The card should now start at SDR12 with 1.8V
    // Read the current status of both CMD and DATA lines
    UInt8 status = 0;
    
    // Wait until the card drive both CMD and DATA lines to high
    for (auto attempt = 0; attempt < 200; attempt += 1)
    {
        IOSleep(20);
        
        pinfo("[%02d] Reading the status of all lines...", attempt);
        
        retVal = this->controller->readChipRegister(SD::rBUSSTAT, status);
        
        if (retVal != kIOReturnSuccess)
        {
            perr("Failed to read the command and the data line status. Error = 0x%x.", retVal);
            
            continue;
        }
        
        pinfo("[%02d] Current line status = 0x%02x.", attempt, status);
        
        // Guard: Ensure that both lines are high
        if (BitOptions(status).contains(SD::BUSSTAT::kAllLinesStatus))
        {
            return kIOReturnSuccess;
        }
    }
    
    // Stop the card clock
    perr("Current line status is 0x%x. At least one of lines is low.", status);
    
    psoftassert(this->controller->writeChipRegister(SD::rBUSSTAT, SD::BUSSTAT::kClockToggleEnable | SD::BUSSTAT::kClockForceStop, 0) == kIOReturnSuccess,
                "Failed to stop and disable the SD clock.");
    
    auto action = [&]() -> IOReturn
    {
        return this->controller->disableCardClock();
    };
    
    psoftassert(this->controller->withCustomCommandTransfer(action) == kIOReturnSuccess, "Failed to disable the card clock.");
    
    return kIOReturnInvalid;
}

///
/// [Helper] Switch the signal voltage to 1.8V
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note This helper function is called by `RealtekSDXCSlot::switchSignalVoltage()` to simplify error handling.
///
IOReturn RealtekSDXCSlot::switchSignalVoltage1d8V()
{
    IOReturn retVal = this->waitVoltageStable1();
    
    if (retVal != kIOReturnSuccess)
    {
        return retVal;
    }
    
    retVal = this->controller->switchOutputVoltage(OutputVoltage::k1d8V);
    
    if (retVal != kIOReturnSuccess)
    {
        return retVal;
    }
    
    return this->waitVoltageStable2();
}

///
/// [Helper] Switch the signal voltage to 3.3V
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note This helper function is called by `RealtekSDXCSlot::switchSignalVoltage()` to simplify error handling.
///
IOReturn RealtekSDXCSlot::switchSignalVoltage3d3V()
{
    return this->controller->switchOutputVoltage(OutputVoltage::k3d3V);
}

///
/// Switch the signal voltage
///
/// @param config An I/O config that contains the target signal voltage level
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replace `sdmmc_switch_voltage()` defined in `rtsx_pci_sdmmc.c`.
///
IOReturn RealtekSDXCSlot::switchSignalVoltage(const IOSDBusConfig& config)
{
    // Guard: Check whether the card is still present
    if (!this->controller->isCardPresent())
    {
        perr("The card is not present. Will abort the signal switch request.");
        
        return kIOReturnNoMedia;
    }
    
    // Notify the card reader to enter the worker state
    this->controller->enterWorkerState();
    
    using namespace RTSX::COM::Chip::SD;
    
    IOReturn retVal;
    
    switch (config.signalVoltage)
    {
        case IOSDBusConfig::SignalVoltage::k3d3V:
        {
            pinfo("Will switch the signal voltage level to 3.3V.");
            
            retVal = this->switchSignalVoltage3d3V();
            
            break;
        }
            
        case IOSDBusConfig::SignalVoltage::k1d8V:
        {
            pinfo("Will switch the signal voltage level to 1.8V.");
            
            retVal = this->switchSignalVoltage1d8V();
            
            break;
        }
            
        case IOSDBusConfig::SignalVoltage::k1d2V:
        {
            perr("SD Express 1.2V is not supported.");
            
            return kIOReturnUnsupported;
        }
    }
    
    // Verify the result
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to switch the voltage level. Error = 0x%x.", retVal);
        
        psoftassert(this->controller->writeChipRegister(rBUSSTAT, BUSSTAT::kClockToggleEnable | BUSSTAT::kClockForceStop, 0) == kIOReturnSuccess,
                    "Failed to stop the SD clock.");
        
        return retVal;
    }
    
    pinfo("The signal voltage level has been switched.");
    
    // Stop toggling the SD clock while in idle
    retVal = this->controller->writeChipRegister(rBUSSTAT, BUSSTAT::kClockToggleEnable | BUSSTAT::kClockForceStop, 0);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to stop the SD clock. Error = 0x%x.", retVal);
    }
    
    return retVal;
}

//
// MARK: - Tuning
//

///
/// Get the phase length for the given bit index
///
/// @param phaseMap The phase map
/// @param sindex The index of the start bit
/// @return The phase length.
/// @note Port: This function replaces `sd_get_phase_len()` and its helper `test_phase_bit()` defined in `rtsx_pci_sdmmc.c`.
///
UInt32 RealtekSDXCSlot::getPhaseLength(UInt32 phaseMap, UInt32 sindex)
{
    UInt32 index;
    
    UInt32 numPhases = this->controller->getTuningConfig().numPhases;
    
    for (index = 0; index < numPhases; index += 1)
    {
        if (!BitOptions(phaseMap).containsBit((sindex + index) % numPhases))
        {
            break;
        }
    }
    
    return index;
}

///
/// Search for the final phase in the given map
///
/// @param phaseMap The phase map
/// @return The final sample point value.
/// @note Port: This function replaces `sd_search_final_phase()` defined in `rtsx_pci_sdmmc.c`.
///
UInt8 RealtekSDXCSlot::searchFinalPhase(UInt32 phaseMap)
{
    UInt8 finalPhase = 0xFF;
    
    // Guard: The given phase map must be valid
    if (phaseMap == 0)
    {
        pwarning("The given phase map is zero. Will use the final phase 0xFF.");
        
        return finalPhase;
    }
    
    // Find the index of the start bit that produces the longest phase
    UInt32 sindex = 0, length = 0;
    
    UInt32 fsindex = 0, flength = 0;
    
    // Fetch the number of phases from the controller
    UInt32 numPhases = this->controller->getTuningConfig().numPhases;
    
    pinfo("Number of phases = %u.", numPhases);
    
    passert(numPhases != 0, "Number of phases cannot be zero. Check the controller initialization routine.");
    
    while (sindex < numPhases)
    {
        length = this->getPhaseLength(phaseMap, sindex);
        
        if (length > flength)
        {
            fsindex = sindex;
            
            flength = length;
        }
        
        sindex += max(1, length);
    }
    
    // Calculate the final phase
    finalPhase = (fsindex + flength / 2) % numPhases;
    
    pinfo("Phase Map = 0x%x; Final Phase = %d; Start Bit Index = %d; Length = %d.",
          phaseMap, finalPhase, fsindex, flength);
    
    return finalPhase;
}

///
/// Wait until the data lines are idle
///
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, other values otherwise.
/// @note Port: This function replaces `sd_wait_data_idle()` defined in `rtsx_pci_sdmmc.c`.
///
IOReturn RealtekSDXCSlot::waitForIdleDataLine()
{
    for (auto attempt = 0; attempt < 200; attempt += 1)
    {
        if (!this->controller->isCardDataLineBusy())
        {
            return kIOReturnSuccess;
        }
        
        IODelay(100);
    }
    
    return kIOReturnTimeout;
}

///
/// Use the given sample point to tune Rx commands
///
/// @param samplePoint The sample point
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `sd_tuning_rx_cmd()` defined in `rtsx_pci_sdmmc.c` but removes tuning support for MMC cards.
///       i.e. This function assumes that the command opcode is always 19.
///
IOReturn RealtekSDXCSlot::tuningRxCommand(UInt8 samplePoint)
{
    using namespace RTSX::COM::Chip::SD;
    
    // Change the Rx phase
    IOReturn retVal = this->controller->changeRxPhase(samplePoint);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to change the Rx phase with the sample point %d. Error = 0x%x.", samplePoint, retVal);
        
        return retVal;
    }
    
    // Set the timeout if necessary
    if (this->controller->getTuningConfig().enable80ClocksTimeout)
    {
        psoftassert(this->controller->writeChipRegister(rCFG3, CFG3::kEnableResponse80ClockTimeout, CFG3::kEnableResponse80ClockTimeout) == kIOReturnSuccess,
                    "Failed to enable the timeout for the tuning command response.");
    }
    
    // Send the tuning command CMD19
    auto command = IOSDHostCommand::CMD19();
    
    retVal = this->runSDCommandAndReadData(command, nullptr, 0x40, 100);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to send the CMD19 and get the response. Error = 0x%x.", retVal);
        
        // Wait until the data lines become idle
        psoftassert(this->waitForIdleDataLine() == kIOReturnSuccess,
                    "Failed to wait for the data lines to be idle.");
        
        this->controller->clearError();
    }
    
    // Cancel the timeout if necessary
    if (this->controller->getTuningConfig().enable80ClocksTimeout)
    {
        psoftassert(this->controller->writeChipRegister(rCFG3, CFG3::kEnableResponse80ClockTimeout, 0) == kIOReturnSuccess,
                    "Failed to disable the timeout for the tuning command response.");
    }
    
    return retVal;
}

///
/// Tune the Rx phase
///
/// @return The phase map.
/// @note Port: This function replaces `sd_tuning_phase()` defined in `rtsx_pci_sdmmc.c`.
///
UInt32 RealtekSDXCSlot::tuningRxPhase()
{
    UInt32 phaseMap = 0;
    
    UInt32 numPhases = this->controller->getTuningConfig().numPhases;
    
    for (auto index = 0; index < numPhases; index += 1)
    {
        if (this->tuningRxCommand(index) == kIOReturnSuccess)
        {
            pinfo("Tuning Rx command with sample point %02d: Success.", index);
            
            phaseMap |= 1 << index;
        }
        else
        {
            pinfo("Tuning Rx command with sample point %02d: Error.", index);
        }
    }
    
    return phaseMap;
}

///
/// Tune the Rx transfer
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `sd_tuning_rx()` defined in `rtsx_pci_sdmmc.c`.
///
IOReturn RealtekSDXCSlot::tuningRx()
{
    // Populate the raw phase maps
    UInt32 rawPhaseMaps[RealtekSDXCSlot::kRxTuningCount] = {};
    
    for (auto index = 0; index < arrsize(rawPhaseMaps); index += 1)
    {
        rawPhaseMaps[index] = this->tuningRxPhase();
        
        if (rawPhaseMaps[index] == 0)
        {
            break;
        }
    }
    
    // Calculate the phase map
    // PCI: NumPhases = 32, Phase Map = 0xFFFFFFFF
    // USB: NumPhases = 16, Phase Map = 0x0000FFFF
    UInt32 numPhases = this->controller->getTuningConfig().numPhases;
    
    UInt32 phaseMap = static_cast<UInt32>((1ULL << numPhases) - 1);
    
    pinfo("Rx Phase Map [INIT] = 0x%08x.", phaseMap);
    
    for (auto index = 0; index < arrsize(rawPhaseMaps); index += 1)
    {
        pinfo("[%d] Rx Raw Phase Map = 0x%08x.", index, rawPhaseMaps[index]);
        
        phaseMap &= rawPhaseMaps[index];
    }
    
    pinfo("Rx Phase Map [TUNE] = 0x%08x.", phaseMap);
    
    // Verify the phase map
    if (phaseMap == 0)
    {
        perr("Rx Phase Map cannot be zero.");
        
        return kIOReturnInvalid;
    }
    
    // Calculate the final phase
    UInt32 finalPhase = this->searchFinalPhase(phaseMap);
    
    if (finalPhase == 0xFF)
    {
        perr("Rx Final Phase cannot be 0xFF.");
        
        return kIOReturnInvalid;
    }
    
    pinfo("Will change the Rx phase to 0x%08x.", finalPhase);
    
    return this->controller->changeRxPhase(finalPhase);
}

//
// MARK: - Card Detection and Write Protection
//

///
/// Check whether the card has write protection enabled
///
/// @param result Set `true` if the card is write protected, `false` otherwise.
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `sdmmc_get_ro()` defined in `rtsx_pci_sdmmc.c`.
///
IOReturn RealtekSDXCSlot::isCardWriteProtected(bool& result)
{
    // Notify the card reader to enter the worker state
    this->controller->enterWorkerState();
    
    // Guard: Check whether the card is still present
    if (!this->controller->isCardPresent())
    {
        perr("The card is not present. Will abort the request.");
        
        return kIOReturnNoMedia;
    }
    
    result = this->controller->isCardWriteProtected();
    
    return kIOReturnSuccess;
}

///
/// Check whether the card exists
///
/// @param result Set `true` if the card exists, `false` otherwise.
/// @return `kIOReturnSuccess` always.
/// @note Port: This function replaces `sdmmc_get_cd()` defined in `rtsx_pci_sdmmc.c`.
///
IOReturn RealtekSDXCSlot::isCardPresent(bool& result)
{
    // Notify the card reader to enter the worker state
    this->controller->enterWorkerState();
    
    result = this->controller->isCardPresent();
    
    return kIOReturnSuccess;
}

///
/// Check whether the command line of the card is busy
///
/// @param result Set `true` if the card CMD line is high, `false` otherwise.
/// @return `kIOReturnSuccess` on success, other values otherwise.
///
IOReturn RealtekSDXCSlot::isCardCommandLineBusy(bool& result)
{
    // Guard: Check whether the card is still present
    if (!this->controller->isCardPresent())
    {
        perr("The card is not present. Will abort the request.");
        
        return kIOReturnNoMedia;
    }
    
    // Notify the card reader to enter the worker state
    this->controller->enterWorkerState();
    
    using namespace RTSX::COM::Chip::SD;
    
    UInt8 status;
    
    IOReturn retVal = this->controller->readChipRegister(rCMDSTATE, status);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to read the command line status. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    result = !BitOptions(status).contains(CMDSTATE::kIdle);
    
    return kIOReturnSuccess;
}

///
/// Check whether the data line of the card is busy
///
/// @param result Set `true` if the card DAT lines are high, `false` otherwise.
/// @return `kIOReturnSuccess` on success, other values otherwise.
///
IOReturn RealtekSDXCSlot::isCardDataLineBusy(bool& result)
{
    // Guard: Check whether the card is still present
    if (!this->controller->isCardPresent())
    {
        perr("The card is not present. Will abort the request.");
        
        return kIOReturnNoMedia;
    }
    
    // Notify the card reader to enter the worker state
    this->controller->enterWorkerState();
    
    using namespace RTSX::COM::Chip::SD;
    
    UInt8 status;
    
    IOReturn retVal = this->controller->readChipRegister(rDATSTATE, status);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to read the data line status. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    pinfo("The card data status = 0x%x.", status);
    
    result = !BitOptions(status).contains(DATSTATE::kIdle);
    
    return kIOReturnSuccess;
}

//
// MARK: - Manage Initial Modes
//

///
/// Set the clock divider to 128 if `initial mode` is true
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `sd_enable_initial_mode()` defined in `rtsx_pci_sdmmc.c`.
///
IOReturn RealtekSDXCSlot::enableInitialModeIfNecessary()
{
    using namespace RTSX::COM::Chip::SD;
    
    return this->initialMode ? this->controller->writeChipRegister(rCFG1, CFG1::kClockDividerMask, CFG1::kClockDivider128) : kIOReturnSuccess;
}

///
/// Set the clock divider to 0 if `initial mode` is true
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `sd_disable_initial_mode()` defined in `rtsx_pci_sdmmc.c`.
///
IOReturn RealtekSDXCSlot::disableInitialModeIfNecessary()
{
    using namespace RTSX::COM::Chip::SD;
    
    return this->initialMode ? this->controller->writeChipRegister(rCFG1, CFG1::kClockDividerMask, CFG1::kClockDivider0) : kIOReturnSuccess;
}

//
// MARK: - Fetch Host Capabilities
//

///
/// Fetch the host capabilities
///
void RealtekSDXCSlot::fetchHostCapabilities()
{
    if (this->controller->supportsSDR50())
    {
        pinfo("The card reader supports the UHS-I SDR50 mode.");
        
        this->capabilities |= Capability::kUHSSDR50;
    }
    
    if (this->controller->supportsSDR104())
    {
        pinfo("The card reader supports the UHS-I SDR104 mode.");
        
        this->capabilities |= Capability::kUHSSDR104;
    }
    
    if (this->controller->supportsDDR50())
    {
        pinfo("The card reader supports the UHS-I DDR50 mode.");
        
        this->capabilities |= Capability::kUHSDDR50;
    }
}

//
// MARK: - IOService Implementations
//

///
/// Initialize the host device
///
/// @return `true` on success, `false` otherwise.
///
bool RealtekSDXCSlot::init(OSDictionary* dictionary)
{
    if (!super::init(dictionary))
    {
        return false;
    }
    
    this->driver = nullptr;
    
    this->busConfig = {};
    
    this->maxCurrents = {400, 0, 800};
    
    this->clockRange = { KHz2Hz(25), MHz2Hz(208) };
    
    this->initialClock = 0;
    
    this->supportedVoltageRanges = IOSDBusConfig::VDD::k32_33 |
                                   IOSDBusConfig::VDD::k33_34 |
                                   IOSDBusConfig::VDD::k165_195;
    
    this->capabilities = Capability::k4BitData          |
                         Capability::kSDHighSpeed       |
                         Capability::kMMCHighSpeed      |
                         Capability::kBusWidthTest      |
                         Capability::kUHSSDR12          |
                         Capability::kUHSSDR25          |
                         Capability::kNoPrescanPowerUp  |
                         Capability::kFullPowerCycle    |
                         Capability::kOptimizeChipSelect;
    
    this->dmaLimits = { 256, 65536, 524288 };
    
    this->factory = {};
    
    this->controller = nullptr;
    
    this->cardClock = 0;
    
    this->sscDepth = SSCDepth::k512K;
    
    this->initialMode = true;
    
    this->vpclock = false;
    
    this->doubleClock = true;
    
    this->powerMode = IOSDBusConfig::PowerMode::kPowerUndefined;
    
    return true;
}

///
/// Start the host device
///
/// @param provider An instance of Realtek PCIe card reader controller
/// @return `true` on success, `false` otherwise.
///
bool RealtekSDXCSlot::start(IOService* provider)
{
    pinfo("=====================================================================");
    pinfo("Starting the host device with the controller at 0x%08x%08x...", KPTR(provider));
    pinfo("=====================================================================");
    
    // Start the super class
    if (!super::start(provider))
    {
        perr("Failed to start the super class.");
        
        return false;
    }
    
    // Get the card reader controller
    this->controller = OSDynamicCast(RealtekCardReaderController, provider);
    
    if (this->controller == nullptr)
    {
        perr("The provider is not a valid controller instance.");
        
        return false;
    }
    
    this->controller->retain();
    
    // Fetch the host device capabilities
    this->fetchHostCapabilities();
    
    // Publish the host driver
    pinfo("Publishing the host driver...");
    
    IOSDHostDriver* driver = OSTypeAlloc(IOSDHostDriver);

    if (driver == nullptr)
    {
        perr("Failed to allocate the host driver.");

        return false;
    }

    if (!driver->init())
    {
        perr("Failed to initialize the host driver.");

        driver->release();

        return false;
    }

    if (!driver->attach(this))
    {
        perr("Failed to attach the host driver.");

        driver->release();

        return false;
    }

    if (!driver->start(this))
    {
        perr("Failed to start the host driver.");

        driver->detach(this);

        driver->release();

        return false;
    }

    this->driver = driver;
    
    this->registerService();
    
    pinfo("The host driver has been published.");
    
    pinfo("=====================================");
    pinfo("The host device started successfully.");    
    pinfo("=====================================");
    
    return true;
}

///
/// Stop the host device
///
/// @param provider An instance of Realtek PCIe card reader controller
///
void RealtekSDXCSlot::stop(IOService* provider)
{
    pinfo("Stopping the host device...");
    
    if (this->driver != nullptr)
    {
        this->driver->stop(this);
        
        this->driver->detach(this);
        
        OSSafeReleaseNULL(this->driver);
    }
    
    OSSafeReleaseNULL(this->controller);
    
    pinfo("The host device has stopped.");
    
    super::stop(provider);
}
