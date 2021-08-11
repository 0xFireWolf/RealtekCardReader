//
//  RealtekUSBCardReaderController.cpp
//  RealtekCardReader
//
//  Created by FireWolf on 7/9/21.
//

#include "RealtekUSBCardReaderController.hpp"
#include "RealtekUSBRegisters.hpp"
#include "RealtekUSBSDXCSlot.hpp"
#include "BitOptions.hpp"

//
// MARK: - Meta Class Definitions
//

OSDefineMetaClassAndStructors(RealtekUSBCardReaderController, RealtekCardReaderController);

//
// MARK: - Constants: Bus Timing Tables
//

/// A sequence of chip registers to switch the bus speed mode to UHS-I SDR50/SDR104
const RealtekUSBCardReaderController::ChipRegValuePair RealtekUSBCardReaderController::kBusTimingTablePairsSDR50[] =
{
    {
        RTSX::UCR::Chip::SD::rCFG1,
        RTSX::UCR::Chip::SD::CFG1::kModeMask | RTSX::UCR::Chip::SD::CFG1::kAsyncFIFONotRST,
        RTSX::UCR::Chip::SD::CFG1::kModeSD30 | RTSX::UCR::Chip::SD::CFG1::kAsyncFIFONotRST
    },
    {
        RTSX::UCR::Chip::CARD::rCLKSRC,
        0xFF,
        RTSX::UCR::Chip::CARD::CLKSRC::kCRCVarClock0 | RTSX::UCR::Chip::CARD::CLKSRC::kSD30FixClock | RTSX::UCR::Chip::CARD::CLKSRC::kSampleVarClock1
    },
};

const RealtekUSBCardReaderController::SimpleRegValuePairs RealtekUSBCardReaderController::kBusTimingTableSDR50 =
{
    RealtekUSBCardReaderController::kBusTimingTablePairsSDR50
};

/// A sequence of chip registers to switch the bus speed mode to UHS-I DDR50
const RealtekUSBCardReaderController::ChipRegValuePair RealtekUSBCardReaderController::kBusTimingTablePairsDDR50[] =
{
    {
        RTSX::UCR::Chip::SD::rCFG1,
        RTSX::UCR::Chip::SD::CFG1::kModeMask  | RTSX::UCR::Chip::SD::CFG1::kAsyncFIFONotRST,
        RTSX::UCR::Chip::SD::CFG1::kModeSDDDR | RTSX::UCR::Chip::SD::CFG1::kAsyncFIFONotRST
    },
    {
        RTSX::UCR::Chip::CARD::rCLKSRC,
        0xFF,
        RTSX::UCR::Chip::CARD::CLKSRC::kCRCVarClock0 | RTSX::UCR::Chip::CARD::CLKSRC::kSD30FixClock | RTSX::UCR::Chip::CARD::CLKSRC::kSampleVarClock1
    },
    {
        RTSX::UCR::Chip::SD::rPPCTL,
        RTSX::UCR::Chip::SD::PPCTL::kDDRVarTxCommandData,
        RTSX::UCR::Chip::SD::PPCTL::kDDRVarTxCommandData
    },
    {
        RTSX::UCR::Chip::SD::rSPCTL,
        RTSX::UCR::Chip::SD::SPCTL::kDDRVarRxData | RTSX::UCR::Chip::SD::SPCTL::kDDRVarRxCommand,
        RTSX::UCR::Chip::SD::SPCTL::kDDRVarRxData | RTSX::UCR::Chip::SD::SPCTL::kDDRVarRxCommand
    },
};

const RealtekUSBCardReaderController::SimpleRegValuePairs RealtekUSBCardReaderController::kBusTimingTableDDR50 =
{
    RealtekUSBCardReaderController::kBusTimingTablePairsDDR50
};

/// A sequence of chip registers to switch the bus speed mode to High Speed
const RealtekUSBCardReaderController::ChipRegValuePair RealtekUSBCardReaderController::kBusTimingTablePairsHighSpeed[] =
{
    {
        RTSX::UCR::Chip::SD::rCFG1,
        RTSX::UCR::Chip::SD::CFG1::kModeMask,
        RTSX::UCR::Chip::SD::CFG1::kModeSD20
    },
    {
        RTSX::UCR::Chip::CARD::rCLKSRC,
        0xFF,
        RTSX::UCR::Chip::CARD::CLKSRC::kCRCFixClock | RTSX::UCR::Chip::CARD::CLKSRC::kSD30VarClock0 | RTSX::UCR::Chip::CARD::CLKSRC::kSampleVarClock1
    },
    {
        RTSX::UCR::Chip::SD::rPPCTL,
        RTSX::UCR::Chip::SD::PPCTL::kSD20TxSelMask,
        RTSX::UCR::Chip::SD::PPCTL::kSD20Tx14Ahead
    },
    {
        RTSX::UCR::Chip::SD::rSPCTL,
        RTSX::UCR::Chip::SD::SPCTL::kSD20RxSelMask,
        RTSX::UCR::Chip::SD::SPCTL::kSD20Rx14Delay
    },
};

const RealtekUSBCardReaderController::SimpleRegValuePairs RealtekUSBCardReaderController::kBusTimingTableHighSpeed =
{
    RealtekUSBCardReaderController::kBusTimingTablePairsHighSpeed
};

/// A sequence of chip registers to switch the bus speed mode to Default Speed
const RealtekUSBCardReaderController::ChipRegValuePair RealtekUSBCardReaderController::kBusTimingTablePairsDefaultSpeed[] =
{
    {
        RTSX::UCR::Chip::SD::rCFG1,
        RTSX::UCR::Chip::SD::CFG1::kModeMask,
        RTSX::UCR::Chip::SD::CFG1::kModeSD20
    },
    {
        RTSX::UCR::Chip::CARD::rCLKSRC,
        0xFF,
        RTSX::UCR::Chip::CARD::CLKSRC::kCRCFixClock | RTSX::UCR::Chip::CARD::CLKSRC::kSD30VarClock0 | RTSX::UCR::Chip::CARD::CLKSRC::kSampleVarClock1
    },
    {
        RTSX::UCR::Chip::SD::rPPCTL,
        0xFF,
        0x00
    },
    {
        RTSX::UCR::Chip::SD::rSPCTL,
        RTSX::UCR::Chip::SD::SPCTL::kSD20RxSelMask,
        RTSX::UCR::Chip::SD::SPCTL::kSD20RxPosEdge
    },
};

const RealtekUSBCardReaderController::SimpleRegValuePairs RealtekUSBCardReaderController::kBusTimingTableDefaultSpeed =
{
    RealtekUSBCardReaderController::kBusTimingTablePairsDefaultSpeed
};

//
// MARK: - SD Pull Control Tables
//

/// A sequence of registers to transfer to enable SD pull control (LQFP48 Package)
const RealtekUSBCardReaderController::ChipRegValuePair RealtekUSBCardReaderController::kSDEnablePullControlTablePairs_lqfp48[] =
{
    { RTSX::UCR::Chip::CARD::PULL::rCTL1, 0xAA },
    { RTSX::UCR::Chip::CARD::PULL::rCTL2, 0xAA },
    { RTSX::UCR::Chip::CARD::PULL::rCTL3, 0xA9 },
    { RTSX::UCR::Chip::CARD::PULL::rCTL4, 0x55 },
    { RTSX::UCR::Chip::CARD::PULL::rCTL5, 0x55 },
    { RTSX::UCR::Chip::CARD::PULL::rCTL6, 0xA5 },
};

const RealtekUSBCardReaderController::SimpleRegValuePairs RealtekUSBCardReaderController::kSDEnablePullControlTable_lqfp48 =
{
    RealtekUSBCardReaderController::kSDEnablePullControlTablePairs_lqfp48
};

/// A sequence of registers to transfer to enable SD pull control (QFN24 Package)
const RealtekUSBCardReaderController::ChipRegValuePair RealtekUSBCardReaderController::kSDEnablePullControlTablePairs_qfn24[] =
{
    { RTSX::UCR::Chip::CARD::PULL::rCTL1, 0xA5 },
    { RTSX::UCR::Chip::CARD::PULL::rCTL2, 0x9A },
    { RTSX::UCR::Chip::CARD::PULL::rCTL3, 0xA5 },
    { RTSX::UCR::Chip::CARD::PULL::rCTL4, 0x9A },
    { RTSX::UCR::Chip::CARD::PULL::rCTL5, 0x65 },
    { RTSX::UCR::Chip::CARD::PULL::rCTL6, 0x5A },
};

const RealtekUSBCardReaderController::SimpleRegValuePairs RealtekUSBCardReaderController::kSDEnablePullControlTable_qfn24 =
{
    RealtekUSBCardReaderController::kSDEnablePullControlTablePairs_qfn24
};

/// A sequence of registers to transfer to disable SD pull control (LQFP48 Package)
const RealtekUSBCardReaderController::ChipRegValuePair RealtekUSBCardReaderController::kSDDisablePullControlTablePairs_lqfp48[] =
{
    { RTSX::UCR::Chip::CARD::PULL::rCTL1, 0x55 },
    { RTSX::UCR::Chip::CARD::PULL::rCTL2, 0x55 },
    { RTSX::UCR::Chip::CARD::PULL::rCTL3, 0x95 },
    { RTSX::UCR::Chip::CARD::PULL::rCTL4, 0x55 },
    { RTSX::UCR::Chip::CARD::PULL::rCTL5, 0x55 },
    { RTSX::UCR::Chip::CARD::PULL::rCTL6, 0xA5 },
};

const RealtekUSBCardReaderController::SimpleRegValuePairs RealtekUSBCardReaderController::kSDDisablePullControlTable_lqfp48 =
{
    RealtekUSBCardReaderController::kSDDisablePullControlTablePairs_lqfp48
};

/// A sequence of registers to transfer to disable SD pull control (QFN24 Package)
const RealtekUSBCardReaderController::ChipRegValuePair RealtekUSBCardReaderController::kSDDisablePullControlTablePairs_qfn24[] =
{
    { RTSX::UCR::Chip::CARD::PULL::rCTL1, 0x65 },
    { RTSX::UCR::Chip::CARD::PULL::rCTL2, 0x55 },
    { RTSX::UCR::Chip::CARD::PULL::rCTL3, 0x95 },
    { RTSX::UCR::Chip::CARD::PULL::rCTL4, 0x55 },
    { RTSX::UCR::Chip::CARD::PULL::rCTL5, 0x56 },
    { RTSX::UCR::Chip::CARD::PULL::rCTL6, 0x59 },
};

const RealtekUSBCardReaderController::SimpleRegValuePairs RealtekUSBCardReaderController::kSDDisablePullControlTable_qfn24 =
{
    RealtekUSBCardReaderController::kSDDisablePullControlTablePairs_qfn24
};

//
// MARK: - Access Chip Registers (Common, Final)
//

///
/// Read a byte from the chip register at the given address
///
/// @param address The register address
/// @param value The register value on return
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out.
/// @note Port: This function replaces `rtsx_usb_read_register()` defined in `rtsx_usb.c`.
///
IOReturn RealtekUSBCardReaderController::readChipRegister(UInt16 address, UInt8& value)
{
    // Issue a read register command
    const ChipRegValuePair pairs[] =
    {
        { address },
    };
    
    IOReturn retVal = this->transferReadRegisterCommands(SimpleRegValuePairs(pairs), 100, Packet::Flags::kCR);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to send the command to read the chip register. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    value = this->readHostBufferValue<UInt8>(0);
    
    return kIOReturnSuccess;
}

///
/// Write a byte to the chip register at the given address
///
/// @param address The register address
/// @param mask The register value mask
/// @param value The register value
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out.
/// @note Port: This function replaces `rtsx_usb_write_register()` defined in `rtsx_usb.c`.
///
IOReturn RealtekUSBCardReaderController::writeChipRegister(UInt16 address, UInt8 mask, UInt8 value)
{
    const ChipRegValuePair pairs[] =
    {
        { address, mask, value },
    };
    
    return this->transferWriteRegisterCommands(SimpleRegValuePairs(pairs), 100, Packet::Flags::kC);
}

///
/// Read from a contiguous sequence of chip registers
///
/// @param address The starting register address
/// @param count The number of registers to read
/// @param destination A non-null buffer that stores the registers value on return
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out.
/// @note Port: This function replaces `rtsx_usb_seq_read_register()` defined in `rtsx_usb.c`.
///
IOReturn RealtekUSBCardReaderController::readChipRegistersSequentially(UInt16 address, UInt16 count, UInt8* destination)
{
    // TODO: The linux driver separates the operation into two parts, aligned read and unaligned read.
    // TODO: Does IOUSBHostPipe::io() require an aligned buffer length?
    
    // The transfer routine will run in a gated context
    auto action = [](OSObject* controller, void* address, void* count, void* destination, void*) -> IOReturn
    {
        // Retrieve the controller instance
        auto instance = OSDynamicCast(RealtekUSBCardReaderController, controller);
        
        passert(instance != nullptr, "The controller instance is invalid.");
        
        // Create the packet for reading registers sequentially and write it to the host buffer
        UInt16 nregs = *reinterpret_cast<UInt16*>(count);
        
        instance->writePacketToHostBufferGated(Packet::forSeqReadCommand(nregs));
        
        // Set the starting register address in the host buffer
        instance->writeHostBufferValueGated(Offset::kHostCmdOff, OSSwapHostToBigInt16(*reinterpret_cast<UInt16*>(address)));
        
        // Initiate the outbound bulk transfer to send the command
        IOReturn retVal = instance->performOutboundBulkTransfer(instance->hostBufferDescriptor, Offset::kSeqRegsVal, 100);
        
        if (retVal != kIOReturnSuccess)
        {
            perr("Failed to complete the outbound bulk transfer. Error = 0x%x.", retVal);
            
            return retVal;
        }
        
        // TODO: Probably too many buffer copies
        // TODO: Consider to change the signature of readPingPongBuffer():
        // TODO: Use IOMemoryDescriptor instead of the raw buffer
        // TODO: Probably also require to change the host device implementation
        // TODO: Consider to add another variant `readChipRegistersSequentially(UInt16, UInt16, IOMemoryDescriptor*)`?
        // TODO: This function is invoked by `readPingPongBuffer()` only at this moment.
        return instance->performInboundBulkTransfer(destination, nregs, 100);
    };
    
    return this->commandGate->runAction(action, &address, &count, destination);
}

///
/// Write to a contiguous sequence of chip registers
///
/// @param address The starting register address
/// @param count The number of registers to write
/// @param source A non-null buffer that contains the registers value
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out.
/// @note Port: This function replaces `rtsx_usb_seq_read_register()` defined in `rtsx_usb.c`.
///
IOReturn RealtekUSBCardReaderController::writeChipRegistersSequentially(UInt16 address, UInt16 count, const UInt8* source)
{
    // TODO: The linux driver aligns the total number of bytes to a 4 bytes boundary
    // TODO: Does IOUSBHostPipe::io() require an aligned buffer length?
    
    // The transfer routine will run in a gated context
    auto action = [](OSObject* controller, void* address, void* count, void* source, void*) -> IOReturn
    {
        // Retrieve the controller instance
        auto instance = OSDynamicCast(RealtekUSBCardReaderController, controller);
        
        passert(instance != nullptr, "The controller instance is invalid.");
        
        // Create the packet for reading registers sequentially and write it to the host buffer
        UInt16 nregs = *reinterpret_cast<UInt16*>(count);
        
        instance->writePacketToHostBufferGated(Packet::forSeqWriteCommand(nregs));
        
        // Set the starting register address in the host buffer
        instance->writeHostBufferValueGated(Offset::kHostCmdOff, OSSwapHostToBigInt16(*reinterpret_cast<UInt16*>(address)));
        
        // Copy the registers value to the host buffer
        IOReturn retVal = instance->writeHostBuffer(Offset::kSeqRegsVal, source, nregs);
        
        if (retVal != kIOReturnSuccess)
        {
            perr("Failed to copy the registers value to the host buffer. Error = 0x%x.", retVal);
            
            return retVal;
        }
        
        // Initiate the bulk transfer
        return instance->performOutboundBulkTransfer(instance->hostBufferDescriptor, Offset::kSeqRegsVal + nregs, 100);
    };
    
    return this->commandGate->runAction(action, &address, &count, const_cast<void*>(reinterpret_cast<const void*>(source)));
}

///
/// Read a byte from the chip register at the given address via the control endpoint
///
/// @param address The register address
/// @param value The register value on return
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out.
/// @note Port: This function replaces `rtsx_usb_ep0_write_register()` defined in `rtsx_usb.c`.
///
IOReturn RealtekUSBCardReaderController::readChipRegisterViaControlEndpoint(UInt16 address, UInt8& value)
{
    // Construct the control request
    StandardUSB::DeviceRequest request =
    {
        // Request type
        kDeviceRequestDirectionIn | kDeviceRequestTypeVendor | kDeviceRequestRecipientDevice,
        
        // Request
        Endpoints::Control::VendorRequest::kRegister,
        
        // Value
        Endpoints::Control::deviceRequestValueForReadRegisterCommand(address),
        
        // Index
        0,
        
        // Length
        sizeof(UInt8),
    };
    
    // Issue the request
    UInt32 bytesTransferred = 0;
    
    return this->interface->deviceRequest(request, &value, bytesTransferred, 100);
}

///
/// Write a byte to the chip register at the given address via the control endpoint
///
/// @param address The register address
/// @param mask The register value mask
/// @param value The register value
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out.
/// @note Port: This function replaces `rtsx_usb_ep0_read_register()` defined in `rtsx_usb.c`.
///
IOReturn RealtekUSBCardReaderController::writeChipRegisterViaControlEndpoint(UInt16 address, UInt8 mask, UInt8 value)
{
    // Construct the control request
    StandardUSB::DeviceRequest request =
    {
        // Request type
        kDeviceRequestDirectionOut | kDeviceRequestTypeVendor | kDeviceRequestRecipientDevice,
        
        // Request
        Endpoints::Control::VendorRequest::kRegister,
        
        // Value
        Endpoints::Control::deviceRequestValueForWriteRegisterCommand(address),
        
        // Index
        static_cast<UInt16>(value << 8 | mask),
        
        // Length
        0,
    };
    
    // Issue the request
    UInt32 bytesTransferred = 0;
    
    return this->interface->deviceRequest(request, static_cast<IOMemoryDescriptor*>(nullptr), bytesTransferred, 100);
}

//
// MARK: - Access Physical Layer Registers
//

///
/// Write a byte to the physical layer register at the given address
///
/// @param address The register address
/// @param value The register value
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
/// @note Port: This function replaces `rtsx_usb_write_phy_register()` defined in `rtsx_usb.c`.
///
IOReturn RealtekUSBCardReaderController::writePhysRegister(UInt8 address, UInt8 value)
{
    using namespace RTSX::UCR::Chip;
    
    const ChipRegValuePair pairs[]
    {
        { HS::rVSTAIN, 0xFF, value },
        { HS::rVCTRL , 0xFF, static_cast<UInt8>(address & 0x0F) },
        { HS::rVLOADM, 0xFF, 0x00 },
        { HS::rVLOADM, 0xFF, 0x00 },
        { HS::rVLOADM, 0xFF, 0x01 },
        { HS::rVCTRL , 0xFF, static_cast<UInt8>(address >> 4) },
        { HS::rVLOADM, 0xFF, 0x00 },
        { HS::rVLOADM, 0xFF, 0x00 },
        { HS::rVLOADM, 0xFF, 0x01 },
    };
    
    return this->transferWriteRegisterCommands(SimpleRegValuePairs(pairs), 100, Packet::Flags::kC);
}

//
// MARK: - Host Buffer Management
//

///
/// Read from the host buffer into the given buffer
///
/// @param offset A byte offset into the host buffer
/// @param buffer A non-null buffer
/// @param length The number of bytes to read
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note This function runs in a gated context.
///
IOReturn RealtekUSBCardReaderController::readHostBufferGated(IOByteCount offset, void* buffer, IOByteCount length)
{
    // The non-gated version guarantees that the offset and the length are valid
    passert(this->hostBufferDescriptor->readBytes(offset, buffer, length) == length,
            "Should be able to read from the host buffer at this moment.");
    
    return kIOReturnSuccess;
}

///
/// Write to the host buffer form the given buffer
///
/// @param offset A byte offset into the host buffer
/// @param buffer A non-null buffer
/// @param length The number of bytes to write
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note This function runs in a gated context.
///
IOReturn RealtekUSBCardReaderController::writeHostBufferGated(IOByteCount offset, const void* buffer, IOByteCount length)
{
    // The non-gated version guarantees that the offset and the length are valid
    passert(this->hostBufferDescriptor->writeBytes(offset, buffer, length) == length,
            "Should be able to write to the host buffer at this moment.");
    
    return kIOReturnSuccess;
}

//
// MARK: - Host Command Management (Core)
//

///
/// Enqueue a command
///
/// @param command The command
/// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full, `kIOReturnError` otherwise.
/// @note Port: This function replaces `rtsx_usb_add_cmd()` defined in `rtsx_usb.c`.
/// @note This function runs in a gated context.
///
IOReturn RealtekUSBCardReaderController::enqueueCommandGated(const Command& command)
{
    // Guard: Ensure that the queue is not full
    if (this->hostCommandCounter.total >= kMaxNumCommands)
    {
        pwarning("The host command buffer queue is full.");
        
        return kIOReturnBusy;
    }
    
    // Retrieve and write the command value
    IOByteCount offset = + Offset::kHostCmdOff + this->hostCommandCounter.total * sizeof(Command);
    
    UInt32 value = OSSwapHostToBigInt32(command.getValue());
    
    // No need to use `writeHostBufferValue()`
    // We can skip the checks because `&value` is non-null and `offset` is valid
    this->writeHostBufferValueGated(offset, value);
    
    // Increment the counter
    this->hostCommandCounter.increment(command.getType());
    
    return kIOReturnSuccess;
}

///
/// Finish the existing host command transfer session
///
/// @param timeout Specify the amount of time in milliseconds
/// @param flags An optional flag, 0 by default
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
/// @note Port: This function replaces `rtsx_usb_send_cmd()` and `rtsx_usb_get_rsp()` defined in `rtsx_usb.c`.
///             Unlike PCIe-based card reader controllers, USB-based controllers must explictly specify the response length
///             and issue an inbound bulk transfer to retrieve the response to a command transfer session.
///             This is handled by the function `rtsx_usb_get_rsp()` defined in `rtsx_usb.c`,
///             and the response length is the sum of number of read and check register commands.
///             Our card reader controller keeps track of how many read and check register commands are in the host buffer,
///             so `endCommandTransfer()` can initiate an inbound bulk transfer to load the response automatically,
///             after it has completed the outbound bulk transfer that sends the host commands to the card reader.
///             As such, the host device avoids the explict invocation of `rtsx_usb_get_rsp()` after `rtsx_usb_send_cmd()`,
///             which makes the host device implementation independent of the card reader controller as much as possible.
/// @note This function sends all commands in the queue to the device and initiates an inbound bulk transfer
///       if and only if the current transfer session contains at least one read or check register command.
/// @note This function runs in a gated context.
///
IOReturn RealtekUSBCardReaderController::endCommandTransferGated(UInt32 timeout, UInt32 flags)
{
    // Create the packet for the batch command transfer and write it to the host buffer
    IOItemCount ncmds = this->hostCommandCounter.total;
    
    this->writePacketToHostBufferGated(Packet::forBatchCommand(ncmds, static_cast<Packet::Flags>(flags & 0xFF)));
    
    // Initiate the outbound bulk transfer to send the host commands
    UInt32 nbytes = Offset::kHostCmdOff + ncmds * sizeof(Command);
    
    IOReturn retVal = this->performOutboundBulkTransfer(this->hostBufferDescriptor, nbytes, timeout);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to send the host commands to the device. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // Check whether the controller needs to load the response
    IOByteCount rlength = this->hostCommandCounter.getResponseLength();
    
    if (rlength == 0)
    {
        pinfo("The current transfer session does not contain read or check register command. No need to load the response.");
        
        return kIOReturnSuccess;
    }
    
    // Initiate the inbound bulk transfer to load the response to the host commands
    pinfo("The response length is %llu bytes.", rlength);
    
    return this->performInboundBulkTransfer(this->hostBufferDescriptor, align(rlength, 4ULL), timeout);
}

//
// MARK: - Host Data Management
//

///
/// Perform a DMA read operation
///
/// @param command A non-null, perpared DMA command
/// @param timeout Specify the amount of time in milliseconds
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
///
IOReturn RealtekUSBCardReaderController::performDMARead(IODMACommand* command, UInt32 timeout)
{
    const IOMemoryDescriptor* buffer = command->getMemoryDescriptor();
    
    IOReturn retVal = this->performInboundBulkTransfer(const_cast<IOMemoryDescriptor*>(buffer), buffer->getLength(), timeout);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to perform the inbound bulk transfer. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // TODO: WARNING: FIXME: EXTRA BUFFER COPIES
    // Later the host driver will complete the given DMA command,
    // which implicitly copies IODMACommand buffers back to the memory descriptor.
    // However, `IOUSBHostPipe::io()` reads the data into the memory descriptor,
    // so `IODMACommand::complete()` will overwrite the data read from the card.
    // We have to copy IOMemoryDescriptor memory to the DMA command at here,
    // which can be avoided if we pass the memory descriptor to the controller instead.
    // In other words, controllers should be responsible for
    // 1) Maintaining a pool of preallocated DMA commands.
    // 2) (De)associate the incoming memory descriptor with a DMA command (IODMACommand::set/clearMemoryDescriptor()).
    // TODO: Sigh. Need to adjust a lot of things, from the host driver to the controller and the SD request data structures.
    retVal = command->synchronize(kIODirectionOut);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to synchronize the buffer. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    return kIOReturnSuccess;
}

///
/// Perform a DMA write operation
///
/// @param command A non-null, perpared DMA command
/// @param timeout Specify the amount of time in milliseconds
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
///
IOReturn RealtekUSBCardReaderController::performDMAWrite(IODMACommand* command, UInt32 timeout)
{
    // Note that we do not have the buffer synchronization issue here
    const IOMemoryDescriptor* buffer = command->getMemoryDescriptor();
    
    return this->performOutboundBulkTransfer(const_cast<IOMemoryDescriptor*>(buffer), buffer->getLength(), timeout);
}

//
// MARK: - Clear Error
//

///
/// Clear any transfer error on the card side
///
/// @note Port: This function replaces the code block that stops the card and clears the card errorin `sd_clear_error()` defined in `rtsx_pci/usb_sdmmc.c`.
///
void RealtekUSBCardReaderController::clearCardError()
{
    using namespace RTSX::UCR::Chip::CARD;
    
    psoftassert(this->writeChipRegisterViaControlEndpoint(rSTOP, STOP::kStopSD | STOP::kClearSDError, STOP::kStopSD | STOP::kClearSDError) == kIOReturnSuccess,
                "Failed to clear the card error.");
}

///
/// Clear any transfer error on the host side
///
/// @note This function is invoked when a command or data transfer has failed.
/// @note Port: This function replaces `rtsx_usb_clear_fsm_err()` and `rtsx_usb_clear_dma_err()` defined in `rtsx_usb.h`.
///
void RealtekUSBCardReaderController::clearHostError()
{
    using namespace RTSX::UCR::Chip;
    
    psoftassert(this->writeChipRegisterViaControlEndpoint(rSFSM, 0xF8, 0xF8) == kIOReturnSuccess, "Failed to clear the FSFM error.");
    
    psoftassert(this->writeChipRegisterViaControlEndpoint(MC::FIFO::rCTL, MC::FIFO::CTL::kFlush, MC::FIFO::CTL::kFlush) == kIOReturnSuccess, "Failed to flush the FIFO queue.");
    
    psoftassert(this->writeChipRegisterViaControlEndpoint(MC::DMA::rRST, MC::DMA::RST::kReset, MC::DMA::RST::kReset) == kIOReturnSuccess, "Failed to reset the DMA.");
}

//
// MARK: - LED Management
//

///
/// Turn on the LED
///
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
/// @note Port: This function replaces `rtsx_usb_turn_on_led()` defined in `rtsx_usb.h`.
///
IOReturn RealtekUSBCardReaderController::turnOnLED()
{
    using namespace RTSX::UCR::Chip::CARD;
    
    return this->writeChipRegisterViaControlEndpoint(rGPIO, GPIO::kLEDMask, GPIO::kLEDOn);
}

///
/// Turn off the LED
///
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
/// @note Port: This function replaces `rtsx_usb_turn_off_led()` defined in `rtsx_usb.h`.
///
IOReturn RealtekUSBCardReaderController::turnOffLED()
{
    using namespace RTSX::UCR::Chip::CARD;
    
    return this->writeChipRegisterViaControlEndpoint(rGPIO, GPIO::kLEDMask, GPIO::kLEDOff);
}

///
/// Enable LED blinking
///
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
/// @note Port: USB-based card reader controllers do not support this feature,
///             so this function simply returns `kIOReturnSuccess`.
///
IOReturn RealtekUSBCardReaderController::enableLEDBlinking()
{
    return kIOReturnSuccess;
}

///
/// Disable LED blinking
///
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
/// @note Port: USB-based card reader controllers do not support this feature,
///             so this function simply returns `kIOReturnSuccess`.
///
IOReturn RealtekUSBCardReaderController::disableLEDBlinking()
{
    return kIOReturnSuccess;
}

//
// MARK: - Card Selection, Share Mode and Transfer Properties
//

///
/// Select the SD card
///
/// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full, `kIOReturnError` otherwise.
/// @note This function invokes `enqueueWriteRegisterCommand()` thus must be invoked between `beginCommandTransfer()` and `endCommandTransfer()`.
/// @note The caller may use `withCustomCommandTransfer()` to combine this operation with other ones.
///
IOReturn RealtekUSBCardReaderController::selectCard()
{
    using namespace RTSX::UCR::Chip;
    
    return this->enqueueWriteRegisterCommand(CARD::rSEL, CARD::SEL::kMask, CARD::SEL::kSD);
}

///
/// Select the data source for the SD card
///
/// @param ppbuf `True` if the data source should be set to the ping pong buffer;
///              `False` if the data source should be the ring buffer instead
/// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full, `kIOReturnError` otherwise.
/// @note This function invokes `enqueueWriteRegisterCommand()` thus must be invoked between `beginCommandTransfer()` and `endCommandTransfer()`.
/// @note The caller may use `withCustomCommandTransfer()` to combine this operation with other ones.
///
IOReturn RealtekUSBCardReaderController::selectCardDataSource(bool ppbuf)
{
    using namespace RTSX::UCR::Chip;
    
    return this->enqueueWriteRegisterCommand(CARD::rDATASRC, CARD::DATASRC::kMask, ppbuf ? CARD::DATASRC::kPingPongBuffer : CARD::DATASRC::kRingBuffer);
}

///
/// Configure the card share mode
///
/// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full, `kIOReturnError` otherwise.
/// @note This function invokes `enqueueWriteRegisterCommand()` thus must be invoked between `beginCommandTransfer()` and `endCommandTransfer()`.
/// @note The caller may use `withCustomCommandTransfer()` to combine this operation with other ones.
///
IOReturn RealtekUSBCardReaderController::configureCardShareMode()
{
    using namespace RTSX::UCR::Chip;
    
    return this->enqueueWriteRegisterCommand(CARD::rSHAREMODE, CARD::SHAREMODE::kMask, CARD::SHAREMODE::kSD);
}

///
/// Setup the properties for a DMA transfer session
///
/// @param length The number of bytes to be transferred
/// @param direction `kIODirectionIn` if it is an inbound DMA transfer;
///                  `kIODirectionOut` if it is an outbound DMA transfer
/// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full, `kIOReturnError` otherwise.
/// @note This function invokes `enqueueWriteRegisterCommand()` thus must be invoked between `beginCommandTransfer()` and `endCommandTransfer()`.
/// @note The caller may use `withCustomCommandTransfer()` to combine this operation with other ones.
/// @note The given direction is guaranteed to be either `kIODirectionIn` or `kIODirectionOut`.
///
IOReturn RealtekUSBCardReaderController::setupCardDMATransferProperties(UInt32 length, IODirection direction)
{
    using namespace RTSX::UCR::Chip;
    
    UInt8 regVal = direction == kIODirectionIn ? MC::DMA::CTL::kDirectionFromCard : MC::DMA::CTL::kDirectionToCard;
    
    const ChipRegValuePair pairs[] =
    {
        // Set up the data length
        { MC::DMA::rTC3, 0xFF, static_cast<UInt8>(length >> 24) },
        { MC::DMA::rTC2, 0xFF, static_cast<UInt8>(length >> 16) },
        { MC::DMA::rTC1, 0xFF, static_cast<UInt8>(length >>  8) },
        { MC::DMA::rTC0, 0xFF, static_cast<UInt8>(length & 0xFF) },
        
        // Set up the direction and the pack size
        {
            MC::DMA::rCTL,
            MC::DMA::CTL::kEnable | MC::DMA::CTL::kDirectionMask | MC::DMA::CTL::kPackSizeMask,
            static_cast<UInt8>(MC::DMA::CTL::kEnable | regVal | MC::DMA::CTL::kPackSize512)
        },
    };
    
    return this->enqueueWriteRegisterCommands(SimpleRegValuePairs(pairs));
}

//
// MARK: - Card Power Management
//

///
/// Enable the card clock
///
/// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full, `kIOReturnError` otherwise.
/// @note This function invokes `enqueueWriteRegisterCommand()` thus must be invoked between `beginCommandTransfer()` and `endCommandTransfer()`.
/// @note The caller may use `withCustomCommandTransfer()` to combine this operation with other ones.
///
IOReturn RealtekUSBCardReaderController::enableCardClock()
{
    using namespace RTSX::UCR::Chip;
    
    return this->enqueueWriteRegisterCommand(CARD::rCLK, CARD::CLK::kEnableSD, CARD::CLK::kEnableSD);
}

///
/// Disable the card clock
///
/// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full, `kIOReturnError` otherwise.
/// @note This function invokes `enqueueWriteRegisterCommand()` thus must be invoked between `beginCommandTransfer()` and `endCommandTransfer()`.
/// @note The caller may use `withCustomCommandTransfer()` to combine this operation with other ones.
///
IOReturn RealtekUSBCardReaderController::disableCardClock()
{
    using namespace RTSX::UCR::Chip;
    
    return this->enqueueWriteRegisterCommand(CARD::rCLK, CARD::CLK::kEnableSD, CARD::CLK::kDisable);
}

///
/// Enable the card output
///
/// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full, `kIOReturnError` otherwise.
/// @note This function invokes `enqueueWriteRegisterCommand()` thus must be invoked between `beginCommandTransfer()` and `endCommandTransfer()`.
/// @note The caller may use `withCustomCommandTransfer()` to combine this operation with other ones.
///
IOReturn RealtekUSBCardReaderController::enableCardOutput()
{
    using namespace RTSX::UCR::Chip;
    
    return this->enqueueWriteRegisterCommand(CARD::rOUTPUT, CARD::OUTPUT::kSDMask, CARD::OUTPUT::kEnableSDValue);
}

///
/// Disable the card output
///
/// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full, `kIOReturnError` otherwise.
/// @note This function invokes `enqueueWriteRegisterCommand()` thus must be invoked between `beginCommandTransfer()` and `endCommandTransfer()`.
/// @note The caller may use `withCustomCommandTransfer()` to combine this operation with other ones.
///
IOReturn RealtekUSBCardReaderController::disableCardOutput()
{
    using namespace RTSX::UCR::Chip;
    
    return this->enqueueWriteRegisterCommand(CARD::rOUTPUT, CARD::OUTPUT::kSDMask, CARD::OUTPUT::kDisableSDValue);
}

///
/// Power on the SD card
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces the code block that powers on the card in `sd_power_on()` defined in `rtsx_usb_sdmmc.c`.
///
IOReturn RealtekUSBCardReaderController::powerOnCard()
{
    using namespace RTSX::UCR::Chip;
    
    IOReturn retVal;
    
    // Power on the card partially
    pinfo("Powering on the card partially...");
    
    retVal = this->writeChipRegister(CARD::rPWRCTRL, CARD::PWRCTRL::kSDPowerMask, CARD::PWRCTRL::kSDPartialPowerOn);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to power on the card partially. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    IOSleep(5);
    
    pinfo("The card power is partially on.");
    
    // Power on the card fully
    retVal = this->writeChipRegister(CARD::rPWRCTRL,
                                     CARD::PWRCTRL::kSDPowerMask | CARD::PWRCTRL::kLDO3318PowerMask,
                                     CARD::PWRCTRL::kSDPowerOn | CARD::PWRCTRL::kLDO3318PowerOn);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to power on the card fully. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    pinfo("The card power is fully on.");
    
    return kIOReturnSuccess;
}

///
/// Power off the SD card
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces the code block that powers off the card in `sd_power_off()` defined in `rtsx_usb_sdmmc.c`.
///
IOReturn RealtekUSBCardReaderController::powerOffCard()
{
    using namespace RTSX::UCR::Chip;
    
    const ChipRegValuePair pairs[] =
    {
        { CARD::rPWRCTRL, CARD::PWRCTRL::kSDPowerMask, CARD::PWRCTRL::kSDPowerOff },
        { CARD::rPWRCTRL, CARD::PWRCTRL::kSDPowerMask | CARD::PWRCTRL::kLDO3318PowerMask, CARD::PWRCTRL::kSDPowerOff | CARD::PWRCTRL::kLDO3318PowerSuspend },
    };
    
    return this->transferWriteRegisterCommands(SimpleRegValuePairs(pairs), 100, Packet::Flags::kC);
}

///
/// Switch to the given output voltage
///
/// @param outputVoltage The new output voltage
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces the code block that changes the voltage in `sdmmc_switch_voltage()` defined in `rtsx_usb_sdmmc.c`.
///
IOReturn RealtekUSBCardReaderController::switchOutputVoltage(OutputVoltage outputVoltage)
{
    using namespace RTSX::UCR::Chip;
    
    ChipRegValuePair pairs[] =
    {
        { SD::rBUSSTAT, SD::BUSSTAT::kClockToggleEnable | SD::BUSSTAT::kClockForceStop, SD::BUSSTAT::kClockForceStop },
        { SD::rPADCTL, SD::PADCTL::kUse1d8V, SD::PADCTL::kUse1d8V },
        { LDO::rPWRCFG, LDO::PWRCFG::kMask, LDO::PWRCFG::k1V8 },
    };
    
    if (outputVoltage == OutputVoltage::k1d8V)
    {
        return this->transferWriteRegisterCommands(SimpleRegValuePairs(pairs), 100, Packet::Flags::kC);
    }
    else
    {
        pairs[1].value = SD::PADCTL::kUse3d3V;
        
        pairs[2].value = LDO::PWRCFG::k3V3;
        
        return this->transferWriteRegisterCommands(SimpleRegValuePairs(&pairs[1], 2), 100, Packet::Flags::kC);
    }
}

//
// MARK: - Card Clock Configurations
//

///
/// Calculate the number of MCUs from the given SSC clock
///
/// @param clock The SSC clock in MHz
/// @return The MCU count.
/// @note Port: This function replaces the code block that calculates the MCU count in `rtsx_pci/usb_switch_clock()` defined in `rtsx_psr/usb.c`.
/// @note Concrete controllers must ensure that the returned MCU count is less than or equal to 15.
///
UInt32 RealtekUSBCardReaderController::sscClock2MCUCount(UInt32 clock)
{
    return min(60 / clock + 3, 15);
}

///
/// Convert the given SSC depth to the actual register value
///
/// @param depth The SSC depth
/// @return The register value.
///
UInt8 RealtekUSBCardReaderController::sscDepth2RegValue(SSCDepth depth)
{
    using namespace RTSX::UCR::Chip;
    
    switch (depth)
    {
        case SSCDepth::k2M:
        {
            return SSC::CTL2::kDepth2M;
        }
            
        case SSCDepth::k1M:
        {
            return SSC::CTL2::kDepth1M;
        }
            
        case SSCDepth::k512K:
        {
            return SSC::CTL2::kDepth512K;
        }
            
        default:
        {
            pfatal("The given SSC depth %u is unsupported by the USB card reader.", depth);
        }
    }
}

///
/// Revise the given SSC depth register value
///
/// @param depth The SSC depth register value
/// @param divider The SSC clock divider value
/// @return The revised SSC depth register value.
///
UInt8 RealtekUSBCardReaderController::reviseSSCDepthRegValue(UInt8 depth, UInt8 divider)
{
    using namespace RTSX::UCR::Chip;
    
    if (divider > CLK::DIV::k1)
    {
        if (depth > (divider - 1))
        {
            depth -= (divider - 1);
        }
        else
        {
            depth = SSC::CTL2::kDepth2M;
        }
    }
    
    return depth;
}

///
/// Switch to the given SSC clock
///
/// @param depth The SSC depth register value
/// @param n The SSC clock n value
/// @param divider The SSC clock divider
/// @param mcus The number of MCUs
/// @param vpclock `true` if VPCLOCK should be used
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note This function does the actual clock switching and is controller-dependent.
///
IOReturn RealtekUSBCardReaderController::switchCardClock(UInt8 depth, UInt8 n, UInt8 divider, UInt8 mcus, bool vpclock)
{
    using namespace RTSX::UCR::Chip;
    
    pinfo("Switching the card clock with SSC depth = %u, N = %u, Divider = %u, MCU Count = %u, Use VPClock = %s.",
          depth, n, divider, mcus, YESNO(vpclock));
    
    // Host commands to change the card clock
    const ChipRegValuePair pairs[] =
    {
        { CLK::rDIV, CLK::DIV::kChangeClock, CLK::DIV::kChangeClock },
        { CLK::rDIV, 0x3F, static_cast<UInt8>(divider << 4 | mcus) },
        { SSC::rCTL1, SSC::CTL1::kRSTB, 0 },
        { SSC::rCTL2, SSC::CTL2::kDepthMask, depth },
        { SSC::rDIVN0, 0xFF, n },
        { SSC::rCTL1, SSC::CTL1::kRSTB, SSC::CTL1::kRSTB },
        
        // Send the following the commands if the host requests to use the vpclock
        { SD::rVPCLK0CTL, SD::VPCTL::kPhaseNotReset, 0 },
        { SD::rVPCLK0CTL, SD::VPCTL::kPhaseNotReset, SD::VPCTL::kPhaseNotReset },
    };
    
    IOReturn retVal = kIOReturnSuccess;
    
    if (vpclock)
    {
        retVal = this->transferWriteRegisterCommands(SimpleRegValuePairs(pairs), 2000, Packet::Flags::kC);
    }
    else
    {
        retVal = this->transferWriteRegisterCommands(SimpleRegValuePairs(pairs, arrsize(pairs) - 2), 2000, Packet::Flags::kC);
    }
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to transfer commands to set the card clock. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    retVal = this->writeChipRegister(SSC::rCTL1, 0xFF, SSC::CTL1::kRSTB | SSC::CTL1::kEnable8x | SSC::CTL1::kSelector4M);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to set the SSC selector. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // Wait until the SSC clock becomes stable
    IODelay(RealtekUSBCardReaderController::kWaitStableSSCClock);
    
    return this->writeChipRegister(CLK::rDIV, CLK::DIV::kChangeClock, 0);
}

//
// MARK: - Card Detection and Write Protection
//

///
/// Get the card status
///
/// @param status The card status on return
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_usb_get_card_status()` and `rtsx_usb_get_status_with_bulk` defined in `rtsx_usb.c`.
///
IOReturn RealtekUSBCardReaderController::getCardStatus(UInt16& status)
{
    using namespace RTSX::UCR::Chip;
    
    pinfo("Getting the card status...");
    
    const ChipRegValuePair pairs[] =
    {
        { CARD::rEXIST },
        { OCP::rSTAT },
    };
    
    IOReturn retVal = this->transferReadRegisterCommands(SimpleRegValuePairs(pairs), 100, Packet::Flags::kCR);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to send the command to read the chip register. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    UInt16 value = OSSwapInt16(this->readHostBufferValue<UInt16>(0));
    
    status = ((value >> 10) & 0x0F) | ((value & 0x03) << 4);

    pinfo("Card status = 0x%04x.", status);
    
    return kIOReturnSuccess;
}

///
/// Check whether the card has write protection enabled
///
/// @return `true` if the card is write protected, `false` otherwise.
///
bool RealtekUSBCardReaderController::isCardWriteProtected()
{
    UInt16 status = 0;
    
    if (this->getCardStatus(status) != kIOReturnSuccess)
    {
        perr("Failed to retrieve the card status. Assume that the card is not write protected.");
        
        return false;
    }
    
    return BitOptions<UInt16>(status).contains(CardStatus::kSDWriteProtected);
}

///
/// Check whether a card is present
///
/// @return `true` if a card exists, `false` otherwise.
/// @note Port: This function replaces `sdmmc_get_cd()` defined in `rtsx_usb_sdmmc.c`.
/// @warning: This function supports SD cards only.
///
bool RealtekUSBCardReaderController::isCardPresent()
{
    UInt16 status = 0;
    
    if (this->getCardStatus(status) != kIOReturnSuccess)
    {
        perr("Failed to retrieve the card status. Assume that the card does not exist.");
        
        return false;
    }
    
    return BitOptions<UInt16>(status).contains(CardStatus::kSDPresent);
}

///
/// Check whether the command line of the card is busy
///
/// @return `true` if the card CMD line is busy, `false` otherwise.
/// @warning This function returns `true` if failed to read the register.
///
bool RealtekUSBCardReaderController::isCardCommandLineBusy()
{
    using namespace RTSX::UCR::Chip::SD;
    
    UInt8 status = 0;
    
    if (this->readChipRegisterViaControlEndpoint(rCMDSTATE, status) != kIOReturnSuccess)
    {
        perr("Failed to read the command state register. Will assume that the command line is busy.");
        
        return true;
    }
    
    return !BitOptions(status).contains(CMDSTATE::kIdle);
}

///
/// Check whether the data line of the card is busy
///
/// @return `true` if the card DAT lines are busy, `false` otherwise.
/// @warning This function returns `true` if failed to read the register.
///
bool RealtekUSBCardReaderController::isCardDataLineBusy()
{
    using namespace RTSX::UCR::Chip::SD;
    
    UInt8 status = 0;
    
    if (this->readChipRegisterViaControlEndpoint(rDATSTATE, status) != kIOReturnSuccess)
    {
        perr("Failed to read the command state register. Will assume that the data lines are busy.");
        
        return true;
    }
    
    return !BitOptions(status).contains(DATSTATE::kIdle);
}

//
// MARK: - Card Pull Control Management
//

///
/// Enable pull control for the SD card
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `sd_pull_ctl_enable_*()` defined in `rtsx_usb_sdmmc.c`.
///
IOReturn RealtekUSBCardReaderController::enablePullControlForSDCard()
{
    if (this->isLQFT48)
    {
        return this->transferWriteRegisterCommands(kSDEnablePullControlTable_lqfp48, 100, Packet::Flags::kC);
    }
    else
    {
        return this->transferWriteRegisterCommands(kSDEnablePullControlTable_qfn24, 100, Packet::Flags::kC);
    }
}

///
/// Disable pull control for the SD card
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `sd_pull_ctl_disable_*()` defined in `rtsx_usb_sdmmc.c`.
///
IOReturn RealtekUSBCardReaderController::disablePullControlForSDCard()
{
    if (this->isLQFT48)
    {
        return this->transferWriteRegisterCommands(kSDDisablePullControlTable_lqfp48, 100, Packet::Flags::kC);
    }
    else
    {
        return this->transferWriteRegisterCommands(kSDDisablePullControlTable_qfn24, 100, Packet::Flags::kC);
    }
}

//
// MARK: - Card Tuning & Phase Management
//

///
/// Change the Rx phase
///
/// @param samplePoint The sample point value
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces the Rx portion of `sd_change_phase()` defined in `rtsx_usb_sdmmc.c`.
///
IOReturn RealtekUSBCardReaderController::changeRxPhase(UInt8 samplePoint)
{
    using namespace RTSX::UCR::Chip;
    
    const ChipRegValuePair pairs[] =
    {
        { CLK::rDIV, CLK::DIV::kChangeClock, CLK::DIV::kChangeClock },
        { SD::rVPRXCTL, SD::VPCTL::kPhaseSelectMask16, samplePoint },
        { SD::rVPTXCTL, SD::VPCTL::kPhaseNotReset, 0 },
        { SD::rVPTXCTL, SD::VPCTL::kPhaseNotReset, SD::VPCTL::kPhaseNotReset },
        { CLK::rDIV, CLK::DIV::kChangeClock, 0 },
        { SD::rCFG1, SD::CFG1::kAsyncFIFONotRST, 0 }
    };
    
    return this->transferWriteRegisterCommands(SimpleRegValuePairs(pairs), 100, Packet::Flags::kC);
}

///
/// Change the Tx phase
///
/// @param samplePoint The sample point value
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces the Tx portion of `sd_change_phase()` defined in `rtsx_usb_sdmmc.c`.
///
IOReturn RealtekUSBCardReaderController::changeTxPhase(UInt8 samplePoint)
{
    using namespace RTSX::UCR::Chip;
    
    const ChipRegValuePair pairs[] =
    {
        { CLK::rDIV, CLK::DIV::kChangeClock, CLK::DIV::kChangeClock },
        { SD::rVPTXCTL, SD::VPCTL::kPhaseSelectMask16, samplePoint },
        { SD::rVPTXCTL, SD::VPCTL::kPhaseNotReset, 0 },
        { SD::rVPTXCTL, SD::VPCTL::kPhaseNotReset, SD::VPCTL::kPhaseNotReset },
        { CLK::rDIV, CLK::DIV::kChangeClock, 0 },
        { SD::rCFG1, SD::CFG1::kAsyncFIFONotRST, 0 }
    };
    
    return this->transferWriteRegisterCommands(SimpleRegValuePairs(pairs), 100, Packet::Flags::kC);
}

//
// MARK: - Ping Pong Buffer
//

///
/// Read from the ping pong buffer
///
/// @param destination The buffer to store bytes
/// @param length The number of bytes to read (cannot exceed 512 bytes)
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_usb_read_ppbuf()` defined in `rtsx_usb.c`.
///
IOReturn RealtekUSBCardReaderController::readPingPongBuffer(UInt8* destination, IOByteCount length)
{
    using namespace RTSX::UCR::Chip;
    
    pinfo("Request to read %llu bytes from the ping pong buffer.", length);
    
    // Guard: The ping pong buffer is 512 bytes long
    if (length > 512)
    {
        perr("The number of bytes requested to read should not exceed 512.");
        
        return kIOReturnBadArgument;
    }
    
    // Guard: Check the destination buffer
    if (destination == nullptr)
    {
        perr("The destination buffer cannot be NULL.");
        
        return kIOReturnBadArgument;
    }
    
    return this->readChipRegistersSequentially(PPBUF::rBASE2, static_cast<UInt16>(length), destination);
}

///
/// Write to the ping pong buffer
///
/// @param source The buffer to write
/// @param length The number of bytes to write (cannot exceed 512 bytes)
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_usb_write_ppbuf()` defined in `rtsx_usb.c`.
///
IOReturn RealtekUSBCardReaderController::writePingPongBuffer(const UInt8* source, IOByteCount length)
{
    using namespace RTSX::UCR::Chip;
    
    pinfo("Request to write %llu bytes from the ping pong buffer.", length);
    
    // Guard: The ping pong buffer is 512 bytes long
    if (length > 512)
    {
        perr("The number of bytes requested to read should not exceed 512.");
        
        return kIOReturnBadArgument;
    }
    
    // Guard: Check the source buffer
    if (source == nullptr)
    {
        perr("The source buffer cannot be NULL.");
        
        return kIOReturnBadArgument;
    }
    
    return this->writeChipRegistersSequentially(PPBUF::rBASE2, static_cast<UInt16>(length), source);
}

//
// MARK: - USB Bulk Transfer
//

///
/// [Helper] Perform a data transfer on the bulk endpoint
///
/// @param pipe The bulk endpoint
/// @param buffer A memory descriptor that contains the data of interest
/// @param length The total number of bytes to transfer
/// @param timeout Specify the amount of time in milliseconds
/// @param retries Abort and return an error if the pipe is still stalled after a certain number of attempts
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note This function increases the timeout to 600 ms if the given timeout is less than 600 ms.
/// @note This function returns an error if the actual number of bytes transferred is not identical to the given `length`.
/// @note Port: This function in a sense replaces `rtsx_usb_transfer_data()` and `rtsx_usb_bulk_transfer_sglist()` defined in `rtsx_usb.c`.
///
IOReturn RealtekUSBCardReaderController::performBulkTransfer(IOUSBHostPipe* pipe, IOMemoryDescriptor* buffer, IOByteCount length, UInt32 timeout, UInt32 retries)
{
    pinfo("Initiating a bulk transfer with length = %llu bytes and timeout = %u ms...", length, timeout);
    
    passert(length <= UINT32_MAX, "The number of bytes to transfer cannot exceed UINT32_MAX.");
    
    IOByteCount32 bufferLength = static_cast<IOByteCount32>(length);
    
    IOByteCount32 actualLength = 0;
    
    IOReturn retVal = kIOReturnSuccess;
    
    for (auto retry = 0; retry < retries; retry += 1)
    {
        pinfo("[%02d] Requesting the bulk transfer...", retry);
        
        retVal = pipe->io(buffer, bufferLength, actualLength, timeout);
        
        if (retVal == kUSBHostReturnPipeStalled)
        {
            perr("The given pipe is stalled. Will clear the stall status.");
            
            psoftassert(pipe->clearStall(true) == kIOReturnSuccess, "Failed to clear the stall status.");
            
            continue;
        }
        
        if (retVal != kIOReturnSuccess)
        {
            perr("Failed to request the bulk transfer. Error = 0x%x.", retVal);
            
            return retVal;
        }
        
        if (actualLength != bufferLength)
        {
            perr("The number of bytes transferred (%u) is not identical to the requested one.", actualLength);
            
            return kIOReturnError;
        }
        
        return kIOReturnSuccess;
    }
    
    pinfo("Reached the maximum number of attempts. Error = 0x%x.", retVal);
    
    return retVal;
}

///
/// Perform an inbound transfer on the bulk endpoint
///
/// @param buffer A non-null buffer that stores the data transferred
/// @param length The total number of bytes to transfer
/// @param timeout Specify the amount of time in milliseconds
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note This function increases the timeout to 600 ms if the given timeout is less than 600 ms.
/// @note This function returns an error if the actual number of bytes transferred is not identical to the given `length`.
/// @note Port: This function in a sense replaces `rtsx_usb_transfer_data()` and `rtsx_usb_bulk_transfer_sglist()` defined in `rtsx_usb.c`.
///
IOReturn RealtekUSBCardReaderController::performInboundBulkTransfer(void* buffer, IOByteCount length, UInt32 timeout)
{
    auto action = [](IOUSBHostInterface* interface, IOMemoryDescriptor* descriptor, const void* context) -> IOReturn
    {
        auto argument = reinterpret_cast<const BulkTransferContext*>(context);
        
        IOReturn retVal = argument->controller->performInboundBulkTransfer(descriptor, argument->length, argument->timeout);
        
        if (retVal != kIOReturnSuccess)
        {
            perr("Failed to complete the inbound bulk transfer. Error = 0x%x.", retVal);
            
            return retVal;
        }
        
        passert(descriptor->readBytes(0, argument->buffer, argument->length) == argument->length,
                "Should be able to read from the intermediate buffer.");
        
        return kIOReturnSuccess;
    };
    
    BulkTransferContext context(this, buffer, length, timeout);
    
    return IOUSBHostInterfaceWithIntermediateIOBuffer(this->interface, kIODirectionIn, length, action, &context);
}

///
/// Perform an outbound transfer on the bulk endpoint
///
/// @param buffer A memory descriptor that contains the data to transfer
/// @param length The total number of bytes to transfer
/// @param timeout Specify the amount of time in milliseconds
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note This function increases the timeout to 600 ms if the given timeout is less than 600 ms.
/// @note This function returns an error if the actual number of bytes transferred is not identical to the given `length`.
/// @note Port: This function in a sense replaces `rtsx_usb_transfer_data()` and `rtsx_usb_bulk_transfer_sglist()` defined in `rtsx_usb.c`.
///
IOReturn RealtekUSBCardReaderController::performOutboundBulkTransfer(const void* buffer, IOByteCount length, UInt32 timeout)
{
    auto action = [](IOUSBHostInterface* interface, IOMemoryDescriptor* descriptor, const void* context) -> IOReturn
    {
        auto argument = reinterpret_cast<const BulkTransferContext*>(context);
        
        passert(descriptor->writeBytes(0, argument->buffer, argument->length) == argument->length,
                "Should be able to write to the intermediate buffer.");
        
        return argument->controller->performInboundBulkTransfer(descriptor, argument->length, argument->timeout);
    };
    
    BulkTransferContext context(this, const_cast<void*>(buffer), length, timeout);
    
    return IOUSBHostInterfaceWithIntermediateIOBuffer(this->interface, kIODirectionIn, length, action, &context);
}

//
// MARK: - Power Management
//

///
/// Prepare to enter the sleep state
///
/// @note Port: This function replaces `rtsx_usb_suspend()` defined in `rtsx_usb.c`.
///
void RealtekUSBCardReaderController::prepareToSleep()
{
    pinfo("The hardware is ready to sleep.");
}

///
/// Prepare to wake up from sleep
///
/// @note Port: This function replaces `rtsx_usb_resume()` defined in `rtsx_usb.c`.
///
void RealtekUSBCardReaderController::prepareToWakeUp()
{
    pinfo("Prepare to wake up...");
    
    psoftassert(this->resetHardware() == kIOReturnSuccess, "Failed to reset the hardware.");
    
    pinfo("The hardware is ready.");
}

//
// MARK: - Hardware Initialization and Configuration
//

///
/// Reset the card reader chip
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_usb_reset_chip()` defined in `rtsx_usb.c`.
///
IOReturn RealtekUSBCardReaderController::resetHardware()
{
    using namespace RTSX::UCR::Chip;
    
    pinfo("Resetting the card reader...");
    
    // Launch a custom command transfer session
    auto action = [](void* context) -> IOReturn
    {
        auto self = reinterpret_cast<RealtekUSBCardReaderController*>(context);
        
        IOReturn retVal = kIOReturnSuccess;
        
        if (self->isLQFT48)
        {
            const ChipRegValuePair lqft48[] =
            {
                { CARD::rPWRCTRL, CARD::PWRCTRL::kLDO3318PowerMask, CARD::PWRCTRL::kLDO3318PowerSuspend },
                { CARD::rPWRCTRL, CARD::PWRCTRL::kForceLDOPowerB, CARD::PWRCTRL::kForceLDOPowerB },
                { CARD::PULL::rCTL1, 0x30, 0x10 },
                { CARD::PULL::rCTL5, 0x03, 0x01 },
                { CARD::PULL::rCTL6, 0x0C, 0x04 },
            };
            
            retVal = self->enqueueWriteRegisterCommands(SimpleRegValuePairs(lqft48));
            
            if (retVal != kIOReturnSuccess)
            {
                perr("Failed to enqueue commands to configure the LQFT48 package. Error = 0x%x.", retVal);
                
                return retVal;
            }
        }
        
        const ChipRegValuePair pairs1[] =
        {
            { rDUMMY0, DUMMY0::kNYETMask, DUMMY0::kNYETEnable },
            { CDDEG::rWIDTH, 0xFF, 0x08 },
            { CDDEG::rCTL, CDDEG::CTL::kEnableXDDeglitch, 0x00 },
            { CARD::SD30::DRVSEL::rCLK, CARD::SD30::DRVSEL::CLK::kDriverMask, CARD::SD30::DRVSEL::CLK::kDriverTypeD },
            { CARD::rDRVSEL, CARD::DRVSEL::kMask, 0x00 },
            { LDO::rPWRCFG, 0xE0, 0x00 },
            
            // RTS5179 only
            { CARD::PULL::rCTL5, 0x03, 0x01 },
        };
        
        if (self->isRTS5179)
        {
            retVal = self->enqueueWriteRegisterCommands(SimpleRegValuePairs(pairs1));
        }
        else
        {
            retVal = self->enqueueWriteRegisterCommands(SimpleRegValuePairs(pairs1, arrsize(pairs1) - 1));
        }
        
        if (retVal != kIOReturnSuccess)
        {
            perr("Failed to enqueue write register commands. Error = 0x%x.", retVal);
            
            return retVal;
        }

        const ChipRegValuePair pairs2[] =
        {
            { CARD::rDMA1CTL, CARD::DMA1CTL::kEntendDMA1AsyncSignal, CARD::DMA1CTL::kEntendDMA1AsyncSignal },
            { CARD::rINTPEND, CARD::INTPEND::kAll, CARD::INTPEND::kAll },
        };

        return self->enqueueWriteRegisterCommands(SimpleRegValuePairs(pairs2));
    };
    
    IOReturn retVal = this->withCustomCommandTransfer(action, this, 100, Packet::Flags::kC);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to reset the card reader. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // Configure the non-crystal mode
    UInt8 value;
    
    retVal = this->readChipRegister(CFG::rMODE0, value);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to read the chip configuration mode. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    BitOptions mode = value;
    
    if (mode.contains(CFG::MODE0::kCrystalFree) ||
        mode.bitwiseAnd(CFG::MODE0::kClockModeMask).contains(CFG::MODE0::kClockModeNonCrystal))
    {
        return this->writePhysRegister(0xC2, 0x7C);
    }
    
    return kIOReturnSuccess;
}

///
/// Initialize the card reader chip
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_usb_init_chip()` defined in `rtsx_usb.c`.
///
IOReturn RealtekUSBCardReaderController::initHardware()
{
    using namespace RTSX::UCR::Chip;
    
    pinfo("Initializing the card reader...");
    
    // Clear the error
    this->clearError();
    
    // Power on the SSC
    IOReturn retVal = this->writeChipRegister(rFPDCTL, FPDCTL::kSSCPowerMask, FPDCTL::kSSCPowerUpValue);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to power on the SSC. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // Wait until the SSC power becomes stable
    IOSleep(1);
    
    retVal = this->writeChipRegister(CLK::rDIV, CLK::DIV::kChangeClock, 0);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to inform the chip to change the clock. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // Fetch the IC version
    retVal = this->readChipRegister(rHWVER, this->revision);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to retrieve the chip revision. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    pinfo("HWVER = 0x%02x.", this->revision);
    
    this->revision &= HWVER::kMask;
    
    pinfo("Chip revision is 0x%x.", this->revision);
    
    // Fetch the package type
    UInt8 value = 0;
    
    retVal = this->readChipRegister(CARD::rSHAREMODE, value);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to retrieve the package type. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    this->isLQFT48 = BitOptions(value).contains(CARD::SHAREMODE::kPkgSel);
    
    pinfo("Package type is %s.", this->isLQFT48 ? "LQFT48" : "QFN24");
    
    // Fetch the chip variations
    retVal = this->readChipRegister(CFG::rMODE1, value);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to retrieve the chip variations. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    this->isRTS5179 = BitOptions(value).contains(CFG::MODE1::kRTS5179);
    
    pinfo("Chip is RTS5179: %s.", YESNO(this->isRTS5179));
    
    return this->resetHardware();
}

///
/// Set the device properties
///
void RealtekUSBCardReaderController::setDeviceProperties()
{
    pinfo("Setting the USB device properties...");
    
    static const char* keys[] = { "idVendor", "idProduct", "kUSBSerialNumberString" };
    
    OSDictionary* deviceInfo = OSDictionary::withCapacity(3);
    
    for (const char* key : keys)
    {
        psoftassert(deviceInfo->setObject(key, this->device->getProperty(key)), "Failed to set the key \"%s\".", key);
    }
    
    psoftassert(this->setProperty("USB Device Info", deviceInfo), "Failed to set the USB device information.");
    
    OSSafeReleaseNULL(deviceInfo);
    
    pinfo("Setting the revision level...");
    
    char buffer[8] = {};
    
    snprintf(buffer, arrsize(buffer), "%d.00", this->revision);
    
    psoftassert(this->setProperty("Product Revision Level", buffer), "Failed to set the revision level.");
    
    pinfo("USB device properties have been set.");
}

//
// MARK: - Polling Device Status
//

///
/// Invoked when the card event has been processed
///
/// @param parameters Unused parameters
/// @param success Result of the card event
///
void RealtekUSBCardReaderController::onCardEventProcessedGated(void* parameters, bool success)
{
    pinfo("The card event has been processed. Result = %s.", success ? "Success" : "Failed");
    
    // Reset the event status
    this->cardEventLock = 0;
    
    // Resume polling the device status
    this->timer->setTimeoutMS(kPollingInterval);
}

///
/// Fetch the device status periodically
///
/// @param sender The timer event source
/// @note This funtion runs in a gated context.
///
void RealtekUSBCardReaderController::fetchDeviceStatusGated(IOTimerEventSource* sender)
{
    // -------------------------
    // | Before | Now | Action |
    // -------------------------
    // |  Yes   | Yes | Ignore |
    // -------------------------
    // |  Yes   |  No | Remove |
    // -------------------------
    // |   No   | Yes | Insert |
    // -------------------------
    // |   No   |  No | Ignore |
    // -------------------------
    
    // Check whether a card is present now
    pinfo("Fetching the device status...");
    
    bool isCardPresentNow = this->isCardPresent();
    
    // Check whether the driver should take action to process the card event
    if (this->isCardPresentBefore ^ isCardPresentNow)
    {
        // Process the card event
        this->cardEventLock = 1;
        
        auto completion = IOSDCard::Completion::withMemberFunction(this, &RealtekUSBCardReaderController::onCardEventProcessedGated);
        
        if (isCardPresentNow)
        {
            this->onSDCardInsertedGated(&completion);
        }
        else
        {
            this->onSDCardRemovedGated(&completion);
        }
        
        // Update the cached status
        this->isCardPresentBefore = isCardPresentNow;
    }
    
    // Check whether the controller is terminated
    if (this->isInactive())
    {
        pinfo("The controller is inactive. Will stop polling the device status.");
        
        return;
    }
    
    // Check whether a card event is being processed
    if (this->cardEventLock != 0)
    {
        pinfo("A card event is being processed. Will pause polling the device status.");
        
        return;
    }
    
    // Schedule the next action
    sender->setTimeoutMS(kPollingInterval);
}

//
// MARK: - Startup Routines
//

///
/// Setup the USB device and configurations
///
/// @param provider The provider
/// @return `true` on success, `false` otherwise.
///
bool RealtekUSBCardReaderController::setupUSBHostDevice(IOService* provider)
{
    // Fetch the host device
    this->device = OSDynamicCast(IOUSBHostDevice, provider);
    
    if (this->device == nullptr)
    {
        perr("The provider is not a valid host device.");
        
        return false;
    }
    
    this->device->retain();
    
    // Take the ownership of the host device
    pinfo("Taking the onwership of the USB host device...");
    
    if (!this->device->open(this))
    {
        perr("Failed to take the ownership of the device.");
        
        goto error1;
    }
    
    pinfo("The card reader controller now owns the host device.");
    
    // Set the device configuration if necessary
    // Realtek SD card readers are simple: Single configuration and single interface
    if (IOUSBHostDeviceSetConfiguration(this->device, this, 0) != kIOReturnSuccess)
    {
        perr("Failed to set the device configuration index to 0.");
        
        goto error2;
    }
    
    // Fetch the first host interface
    // Note that the returned interface has already been retained
    this->interface = IOUSBHostDeviceFindFirstInterface(this->device);
    
    if (this->interface == nullptr)
    {
        perr("Failed to fetch the first host interface.");
        
        goto error2;
    }
    
    pinfo("The host device has been set up.");
    
    return true;
    
error2:
    this->device->close(this);
    
error1:
    OSSafeReleaseNULL(this->device);
    
    return false;
}

///
/// Setup the USB interface and endpoints
///
/// @return `true` on success, `false` otherwise.
///
bool RealtekUSBCardReaderController::setupUSBHostInterface()
{
    // Take the ownership of the host interface
    pinfo("Taking the ownership of the USB host interface...");
    
    if (!this->interface->open(this))
    {
        perr("Failed to take the ownership of the interface.");
        
        return false;
    }
    
    pinfo("The card reader controller now owns the host interface.");
    
    // Get the bulk endpoints
    pinfo("Fetching the bulk endpoints...");
    
    this->inputPipe = IOUSBHostInterfaceFindPipe(this->interface, kIOUSBEndpointTypeBulk, kIOUSBEndpointDirectionIn);
    
    if (this->inputPipe == nullptr)
    {
        perr("Failed to retrieve the input bulk endpoint.");
        
        goto error1;
    }
    
    this->outputPipe = IOUSBHostInterfaceFindPipe(this->interface, kIOUSBEndpointTypeBulk, kIOUSBEndpointDirectionOut);
    
    if (this->outputPipe == nullptr)
    {
        perr("Failed to retrieve the output bulk endpoint.");
        
        goto error2;
    }
    
    pinfo("Bulk endpoints have been fetched.");
    
    return true;
    
error2:
    OSSafeReleaseNULL(this->inputPipe);
    
error1:
    this->interface->close(this);
    
    return false;
}

///
/// Setup the host command and buffer management module
///
/// @return `true` on success, `false` otherwise.
///
bool RealtekUSBCardReaderController::setupHostBuffer()
{
    // Allocate the buffer
    pinfo("Allocating the host buffer...");
    
    this->hostBufferDescriptor = this->interface->createIOBuffer(kIODirectionInOut, kHostBufferSize);
    
    if (this->hostBufferDescriptor == nullptr)
    {
        perr("Failed to allocate the host buffer.");
        
        return false;
    }
    
    // Page in and wire down the buffer
    if (this->hostBufferDescriptor->prepare() != kIOReturnSuccess)
    {
        perr("Failed to wire down and page in the buffer.");
        
        OSSafeReleaseNULL(this->hostBufferDescriptor);
        
        return false;
    }
    
    pinfo("The host buffer has been allocated.");
    
    return true;
}

///
/// Setup the polling timer
///
/// @return `true` on success, `false` otherwise.
/// @note This function does not enable the timer automatically.
///
bool RealtekUSBCardReaderController::setupPollingTimer()
{
    pinfo("Setting up the polling timer...");
    
    auto handler = OSMemberFunctionCast(IOTimerEventSource::Action, this, &RealtekUSBCardReaderController::fetchDeviceStatusGated);
    
    this->timer = IOTimerEventSource::timerEventSource(this, handler);
    
    if (this->timer == nullptr)
    {
        perr("Failed to create the timer event source.");
        
        return false;
    }
    
    if (this->workLoop->addEventSource(this->timer) != kIOReturnSuccess)
    {
        perr("Failed to add the timer event source to the work loop.");
        
        OSSafeReleaseNULL(this->timer);
        
        return false;
    }
    
    pinfo("The polling timer has been set up.");
    
    return true;
}

///
/// Create the card slot and publish it
///
/// @return `true` on success, `false` otherwise.
///
bool RealtekUSBCardReaderController::createCardSlot()
{
    pinfo("Creating the card slot...");
    
    RealtekSDXCSlot* slot = OSTypeAlloc(RealtekUSBSDXCSlot);
    
    if (slot == nullptr)
    {
        perr("Failed to allocate the card slot.");
        
        return false;
    }
    
    if (!slot->init())
    {
        perr("Failed to initialize the card slot.");
        
        slot->release();
        
        return false;
    }
    
    if (!slot->attach(this))
    {
        perr("Failed to attach the card slot.");
        
        slot->release();
        
        return false;
    }
    
    if (!slot->start(this))
    {
        perr("Failed to start the card slot.");
        
        slot->detach(this);
        
        slot->release();
        
        return false;
    }
    
    this->slot = slot;
    
    pinfo("The card slot has been created and published.");
    
    return true;
}

//
// MARK: - Teardown Routines
//

///
/// Tear down the USB device and configurations
///
void RealtekUSBCardReaderController::tearDownUSBHostDevice()
{
    OSSafeReleaseNULL(this->interface);
    
    this->device->close(this);
    
    OSSafeReleaseNULL(this->device);
}

///
/// Tear down the USB interface and endpoints
///
void RealtekUSBCardReaderController::tearDownUSBHostInterface()
{
    OSSafeReleaseNULL(this->inputPipe);
    
    OSSafeReleaseNULL(this->outputPipe);
    
    this->interface->close(this);
}

///
/// Tear down the host command and buffer management module
///
void RealtekUSBCardReaderController::tearDownHostBuffer()
{
    if (this->hostBufferDescriptor != nullptr)
    {
        this->hostBufferDescriptor->complete();
        
        this->hostBufferDescriptor->release();
        
        this->hostBufferDescriptor = nullptr;
    }
}

///
/// Destroy the polling timer
///
void RealtekUSBCardReaderController::tearDownPollingTimer()
{
    if (this->timer != nullptr)
    {
        this->timer->cancelTimeout();
        
        this->workLoop->removeEventSource(this->timer);
        
        this->timer->release();
        
        this->timer = nullptr;
    }
}

///
/// Destroy the card slot
///
void RealtekUSBCardReaderController::destroyCardSlot()
{
    pinfo("Stopping the card slot...");
    
    if (this->slot != nullptr)
    {
        this->slot->stop(this);
        
        this->slot->detach(this);
        
        OSSafeReleaseNULL(this->slot);
    }
}

//
// MARK: - IOService Implementations
//

///
/// Initialize the controller
///
/// @return `true` on success, `false` otherwise.
///
bool RealtekUSBCardReaderController::init(OSDictionary* dictionary)
{
    if (!super::init(dictionary))
    {
        perr("Failed to initialize the super class.");
        
        return false;
    }
    
    using namespace RTSX::UCR::Chip;
    
    this->sscClockLimits.rangeN = { RealtekUSBCardReaderController::kMinSSCClockN, RealtekUSBCardReaderController::kMaxSSCClockN };
    
    this->sscClockLimits.rangeDivider = { CLK::DIV::k1, CLK::DIV::k4 };
    
    this->sscClockLimits.minFrequencyMHz = kMinSSCClockFrequencyMHz;
    
    this->uhsCaps.sdr104 = false;
    
    this->uhsCaps.sdr50 = true;
    
    this->uhsCaps.ddr50 = false;
    
    this->uhsCaps.reserved = false;
    
    this->busTimingTables.sdr50 = &RealtekUSBCardReaderController::kBusTimingTableSDR50;
    
    this->busTimingTables.ddr50 = &RealtekUSBCardReaderController::kBusTimingTableDDR50;
    
    this->busTimingTables.highSpeed = &RealtekUSBCardReaderController::kBusTimingTableHighSpeed;
    
    this->busTimingTables.defaultSpeed = &RealtekUSBCardReaderController::kBusTimingTableDefaultSpeed;
    
    this->tuningConfig.numPhases = RealtekUSBCardReaderController::kTuningNumPhases;
    
    this->tuningConfig.enable80ClocksTimeout = RealtekUSBCardReaderController::kTuningEnable80ClocksTimes;
    
    this->dataTransferFlags.command = Packet::Flags::kCR;
    
    this->dataTransferFlags.commandWithInboundDataTransfer = Packet::Flags::kCR;
    
    this->dataTransferFlags.commandWithOutboundDataTransfer = Packet::Flags::kCR;
    
    this->dataTransferFlags.commandWithInboundDMATransfer = Packet::Flags::kCDIR;
    
    this->dataTransferFlags.commandWithOutboundDMATransfer = Packet::Flags::kCDOR;
    
    this->device = nullptr;
    
    this->interface = nullptr;
    
    this->inputPipe = nullptr;
    
    this->outputPipe = nullptr;
    
    this->timer = nullptr;
    
    this->isCardPresentBefore = false;
    
    this->isLQFT48 = false;
    
    this->isRTS5179 = false;
    
    this->revision = 0;
    
    this->cardEventLock = 0;
    
    return true;
}

///
/// Start the controller
///
/// @param provider An instance of USB host device that represents the card reader
/// @return `true` on success, `false` otherwise.
///
bool RealtekUSBCardReaderController::start(IOService* provider)
{
    pinfo("==================================================");
    pinfo("Starting the Realtek USB card reader controller...");
    pinfo("==================================================");
    
    // Start the super class
    if (!super::start(provider))
    {
        perr("Failed to start the super class.");
        
        return false;
    }
    
    // Setup the host device
    if (!this->setupUSBHostDevice(provider))
    {
        perr("Failed to setup the USB host device.");
        
        return false;
    }
    
    // Setup the host interface (Part 2)
    if (!this->setupUSBHostInterface())
    {
        perr("Failed to setup the USB host interface.");
        
        goto error1;
    }
    
    // Setup the host buffer
    if (!this->setupHostBuffer())
    {
        perr("Failed to setup the host buffer.");
        
        goto error2;
    }
    
    // Initialize the hardware
    if (this->initHardware() != kIOReturnSuccess)
    {
        perr("Failed to initialize the card reader.");
        
        goto error3;
    }
    
    // Setup the timer for polling the device status
    if (!this->setupPollingTimer())
    {
        perr("Failed to setup the polling timer.");
        
        goto error3;
    }
    
    // Create the card slot
    if (!this->createCardSlot())
    {
        perr("Failed to create the card slot.");
        
        goto error4;
    }
    
    // Publish the device properties
    this->setDeviceProperties();
    
    // Enable the polling timer
    psoftassert(this->timer->setTimeoutMS(kPollingInterval) == kIOReturnSuccess, "Failed to enable the polling timer.");
    
    // Register the service so that the reporter can see the device
    this->registerService();
    
    pinfo("================================================");
    pinfo("The card reader controller started successfully.");
    pinfo("================================================");
    
    return true;
    
error4:
    this->tearDownPollingTimer();
    
error3:
    this->tearDownHostBuffer();
    
error2:
    this->tearDownUSBHostInterface();
    
error1:
    this->tearDownUSBHostDevice();
    
    return false;
}

///
/// Stop the controller
///
/// @param provider An instance of USB host device that represents the card reader
///
void RealtekUSBCardReaderController::stop(IOService* provider)
{
    this->destroyCardSlot();
    
    this->tearDownPollingTimer();
    
    this->tearDownHostBuffer();
    
    this->tearDownUSBHostInterface();
    
    this->tearDownUSBHostDevice();
    
    super::stop(provider);
}
