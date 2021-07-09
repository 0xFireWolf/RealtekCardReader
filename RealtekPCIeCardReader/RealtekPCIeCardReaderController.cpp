//
//  RealtekPCIeCardReaderController.cpp
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 2/18/21.
//

#include "RealtekPCIeCardReaderController.hpp"
#include "RealtekSDXCSlot.hpp"
#include "BitOptions.hpp"
#include "IOPCIeDevice.hpp"

//
// MARK: - Meta Class Definitions
//

OSDefineMetaClassAndAbstractStructors(RealtekPCIeCardReaderController, AppleSDXC);

//
// MARK: - Access Memory Mapped Registers (Generic, Final)
//

///
/// Read a byte from a memory mapped register at the given address
///
/// @param address The register address
/// @return The register value.
/// @note Port: This function replaces the macro `rtsx_pci_readb()` defined in `rtsx_pci.h`.
///
UInt8 RealtekPCIeCardReaderController::readRegister8(UInt32 address)
{
    return this->readRegister<UInt8>(address);
}

///
/// Read a word from a memory mapped register at the given address
///
/// @param address The register address
/// @return The register value.
/// @note Port: This function replaces the macro `rtsx_pci_readw()` defined in `rtsx_pci.h`.
///
UInt16 RealtekPCIeCardReaderController::readRegister16(UInt32 address)
{
    return this->readRegister<UInt16>(address);
}

///
/// Read a double word from a memory mapped register at the given address
///
/// @param address The register address
/// @return The register value.
/// @note Port: This function replaces the macro `rtsx_pci_readl()` defined in `rtsx_pci.h`.
///
UInt32 RealtekPCIeCardReaderController::readRegister32(UInt32 address)
{
    return this->readRegister<UInt32>(address);
}

///
/// Write a byte to a memory mapped register at the given address
///
/// @param address The register address
/// @param value The register value
/// @note Port: This function replaces the macro `rtsx_pci_writeb()` defined in `rtsx_pci.h`.
///
void RealtekPCIeCardReaderController::writeRegister8(UInt32 address, UInt8 value)
{
    this->writeRegister(address, value);
}

///
/// Write a word to a memory mapped register at the given address
///
/// @param address The register address
/// @param value The register value
/// @note Port: This function replaces the macro `rtsx_pci_writew()` defined in `rtsx_pci.h`.
///
void RealtekPCIeCardReaderController::writeRegister16(UInt32 address, UInt16 value)
{
    this->writeRegister(address, value);
}

///
/// Write a double word to a memory mapped register at the given address
///
/// @param address The register address
/// @param value The register value
/// @note Port: This function replaces the macro `rtsx_pci_writel()` defined in `rtsx_pci.h`.
///
void RealtekPCIeCardReaderController::writeRegister32(UInt32 address, UInt32 value)
{
    this->writeRegister(address, value);
}

//
// MARK: - Access Chip Registers (Generic, Final)
//

///
/// Read a byte from the chip register at the given address
///
/// @param address The register address
/// @param value The register value on return
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out.
/// @note Port: This function replaces `rtsx_pci_read_register()` defined in `rtsx_psr.c`.
///
IOReturn RealtekPCIeCardReaderController::readChipRegister(UInt16 address, UInt8& value)
{
    // The driver will program MMIO registers to access chip registers
    using namespace RTSX::MMIO;
    
    pinfo("Reading the chip register at 0x%04x...", address);
    
    // Start the operation by writing the address with busy bit set to the chip
    this->writeRegister32(rHAIMR, HAIMR::RegValueForReadOperation(address));
    
    // Wait until the device is free to finish the operation
    for (auto attempt = 0; attempt < HAIMR::kMaxAttempts; attempt += 1)
    {
        UInt32 regVal = this->readRegister32(rHAIMR);
        
        if (!HAIMR::IsBusy(regVal))
        {
            // Extract the register value
            value = static_cast<UInt8>(regVal & 0xFF);
            
            return kIOReturnSuccess;
        }
    }
    
    // Timed out
    perr("Timed out when reading the chip register at 0x%04x.", address);
    
    return kIOReturnTimeout;
}

///
/// Write a byte to the chip register at the given address
///
/// @param address The register address
/// @param mask The register value mask
/// @param value The register value
/// @return `kIOReturnSuccess` on success;
///         `kIOReturnTimeout` if timed out;
///         `kIOReturnError` if the new register value is not `value`.
/// @note Port: This function replaces `rtsx_pci_write_register()` defined in `rtsx_psr.c`.
///
IOReturn RealtekPCIeCardReaderController::writeChipRegister(UInt16 address, UInt8 mask, UInt8 value)
{
    // The driver will program MMIO registers to access chip registers
    using namespace RTSX::MMIO;
    
    pinfo("Writing 0x%02x with mask 0x%02x to the chip register at 0x%04x...", value, mask, address);
    
    // Start the operation by writing the address, mask and value with busy and write bits set to the chip
    this->writeRegister32(rHAIMR, HAIMR::RegValueForWriteOperation(address, mask, value));
    
    // Wait until the device is free to finish the operation
    for (auto attempt = 0; attempt < HAIMR::kMaxAttempts; attempt += 1)
    {
        UInt32 regVal = this->readRegister32(rHAIMR);
        
        if (HAIMR::IsBusy(regVal))
        {
            continue;
        }
        
        // The device is now free
        // Guard: Extract and verify the new register value
        regVal &= 0xFF;
        
        if (regVal != value)
        {
            perr("The new register value is 0x%02x and does not match the given value 0x%02x.", regVal, value);
            
            return kIOReturnError;
        }
        
        // All done
        return kIOReturnSuccess;
    }
    
    // Timed out
    perr("Timed out when writing 0x%02x with mask 0x%02x to the chip register at 0x%04x.", value, mask, address);
    
    return kIOReturnTimeout;
}

///
/// Write to multiple chip registers conveniently
///
/// @param pairs A sequence of pairs of register address and value
/// @return `kIOReturnSuccess` on success;
///         `kIOReturnTimeout` if timed out;
///         `kIOReturnError` if the new register value is not `value`.
///
IOReturn RealtekPCIeCardReaderController::writeChipRegisters(const ChipRegValuePairs& pairs)
{
    IOReturn retVal = kIOReturnSuccess;
    
    IOItemCount count = pairs.size();
    
    pinfo("Writing %u chip registers...", count);
    
    for (auto index = 0; index < count; index += 1)
    {
        ChipRegValuePair pair = pairs.get(index);
        
        retVal = this->writeChipRegister(pair.address, pair.mask, pair.value);
        
        if (retVal != kIOReturnSuccess)
        {
            perr("[%02d] Failed to write 0x%02x to register 0x%04x. Error = 0x%x.", index, pair.value, pair.address, retVal);
            
            return retVal;
        }
    }
    
    pinfo("Wrote %u chip registers. Returns 0x%x.", count, retVal);
    
    return retVal;
}

///
/// Dump the chip registers in the given range
///
/// @param range The range of register addresses
///
void RealtekPCIeCardReaderController::dumpChipRegisters(ClosedRange<UInt16> range)
{
    pinfo("Dumping chip registers from 0x%04x to 0x%04x.", range.lowerBound, range.upperBound);
    
    UInt8 value = 0;
    
    for (auto address = range.lowerBound; address <= range.upperBound; address += 1)
    {
        if (this->readChipRegister(address, value) != kIOReturnSuccess)
        {
            pinfo("[0x%04d] 0x%02x", address, value);
        }
        else
        {
            pinfo("[0x%04d] ERROR", address);
        }
    }
}

//
// MARK: - Access Physical Layer Registers (Default, Overridable)
//

///
/// Read a word from the physical layer register at the given address
///
/// @param address The register address
/// @param value The register value on return
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
/// @note Port: This function replaces `rtsx_pci_read_phy_register()` defined in `rtsx_psr.c`.
/// @note Subclasses may override this function to provide a device-specific implementation.
///
IOReturn RealtekPCIeCardReaderController::readPhysRegister(UInt8 address, UInt16& value)
{
    // The driver will program chip registers to access PHY registers
    using namespace RTSX::Chip;
    
    pinfo("Read the PHY register at 0x%02x.", address);
    
    // Guard: Initiate the operation
    IOReturn retVal;
    
    const ChipRegValuePair pairs[] =
    {
        // Set the address of the register
        { PHY::rADDR, address },
        
        // Set the data direction
        { PHY::rRWCTL, PHY::RWCTL::kSetReadOperation }
    };
    
    retVal = this->writeChipRegisters(SimpleRegValuePairs(pairs));
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to initiate the operation to read the PHY register at 0x%02x.", address);
        
        return retVal;
    }
    
    // Wait until the device is free to finish the operation
    for (auto attempt = 0; attempt < PHY::RWCTL::kMaximumAttempts; attempt += 1)
    {
        // The device status
        UInt8 regVal;
        
        // Guard: Fetch the device status
        retVal = this->readChipRegister(PHY::rRWCTL, regVal);
        
        if (retVal != kIOReturnSuccess)
        {
            perr("Failed to fetch the device status. Error = 0x%x.", retVal);
            
            return retVal;
        }
        
        // Guard: Verify the device status
        if (PHY::RWCTL::IsBusy(regVal))
        {
            continue;
        }
        
        // The device is now free
        // Fetch the register value
        UInt8 l8, h8;
        
        // Guard: Fetch the low 8 bits
        retVal = this->readChipRegister(PHY::rDATA0, l8);
        
        if (retVal != kIOReturnSuccess)
        {
            perr("Failed to fetch the low 8 bits of the register value. Error = 0x%x.", retVal);
            
            return retVal;
        }
        
        // Guard: Fetch the high 8 bits
        retVal = this->readChipRegister(PHY::rDATA1, h8);
        
        if (retVal != kIOReturnSuccess)
        {
            perr("Failed to fetch the high 8 bits of the register value. Error = 0x%x.", retVal);
            
            return retVal;
        }
        
        // All done
        value = static_cast<UInt16>(h8) << 8 | l8;
        
        return kIOReturnSuccess;
    }
    
    // Timed out
    perr("Timed out when reading the PHY regiater at 0x%02x.", address);
    
    return kIOReturnTimeout;
}

///
/// Write a word to the physical layer register at the given address
///
/// @param address The register address
/// @param value The register value
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
/// @note Port: This function replaces `rtsx_pci_write_phy_register()` defined in `rtsx_psr.c`.
/// @note Subclasses may override this function to provide a device-specific implementation.
///
IOReturn RealtekPCIeCardReaderController::writePhysRegister(UInt8 address, UInt16 value)
{
    // The driver will program chip registers to access PHY registers
    using namespace RTSX::Chip;
    
    pinfo("Write 0x%02x to the PHY register at 0x%02x.", value, address);
    
    // Guard: Initiate the operation
    pinfo("Initiating the PHY write operation...");
    
    IOReturn retVal;
    
    const ChipRegValuePair pairs[] =
    {
        // Set the low 8 bits of the new register value
        { PHY::rDATA0, static_cast<UInt8>(value & 0xFF) },
        
        // Set the high 8 bits of the new register value
        { PHY::rDATA1, static_cast<UInt8>(value >> 8) },
        
        // Set the address of the register
        { PHY::rADDR, address },
        
        // Set the data direction
        { PHY::rRWCTL, PHY::RWCTL::kSetWriteOperation }
    };
    
    retVal = this->writeChipRegisters(SimpleRegValuePairs(pairs));
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to initiate the operation to write 0x%04x to the PHY register at 0x%02x.", value, address);
        
        return retVal;
    }
    
    pinfo("Informed the card reader the register address and the value.");
    
    // Wait until the device is free to finish the operation
    for (auto attempt = 0; attempt < PHY::RWCTL::kMaximumAttempts; attempt += 1)
    {
        // The device status
        UInt8 regVal;
        
        // Guard: Fetch the device status
        retVal = this->readChipRegister(PHY::rRWCTL, regVal);
        
        if (retVal != kIOReturnSuccess)
        {
            perr("Failed to fetch the device status. Error = 0x%x.", retVal);
            
            return retVal;
        }
        
        // Guard: Verify the device status
        if (!PHY::RWCTL::IsBusy(regVal))
        {
            return kIOReturnSuccess;
        }
    }
    
    // Timed out
    perr("Timed out when writing the PHY register at 0x%02x.", address);
    
    return kIOReturnTimeout;
}

///
/// Write to multiple physical layer registers conveniently
///
/// @param pairs An array of registers and their values
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
///
IOReturn RealtekPCIeCardReaderController::writePhysRegisters(const PhysRegValuePair* pairs, size_t count)
{
    pinfo("Writing %lu PHY registers...", count);
    
    for (auto index = 0; index < count; index += 1)
    {
        auto retVal = this->writePhysRegister(pairs[index].address, pairs[index].value);
        
        if (retVal != kIOReturnSuccess)
        {
            return retVal;
        }
    }
    
    return kIOReturnSuccess;
}

///
/// Modify a physical layer register conveniently
///
/// @param address The register address
/// @param modifier A modifier that takes the current register value and return the new value
/// @param context A nullable context that will be passed to the modifier function
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
/// @note Port: This function replaces `rtsx_pci_update_phy()` defined in `rtsx_pci.h`.
///
IOReturn RealtekPCIeCardReaderController::modifyPhysRegister(UInt8 address, PhysRegValueModifier modifier, const void* context)
{
    UInt16 regVal;
    
    // Guard: Read the current register value
    IOReturn retVal = this->readPhysRegister(address, regVal);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to read the current register value.");
        
        return retVal;
    }
    
    // Modify the register value
    regVal = (*modifier)(regVal, context);
    
    // Guard: Write the new register value
    retVal = this->writePhysRegister(address, regVal);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to write the new register value.");
    }
    
    return retVal;
}

///
/// Set a bit at the given index in a physical layer register conveniently
///
/// @param address The register address
/// @param index The zero-based bit index
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
///
IOReturn RealtekPCIeCardReaderController::setPhysRegisterBitAtIndex(UInt8 address, UInt32 index)
{
    // Guard: The given bit index must be valid
    if (index >= sizeof(UInt16) * 8)
    {
        perr("The given bit index %d is invalid.", index);
        
        return kIOReturnBadArgument;
    }
    
    // Modify the register value
    auto modifier = [](UInt16 regVal, const void* context) -> UInt16
    {
        return regVal | 1 << *reinterpret_cast<const UInt32*>(context);
    };
    
    return this->modifyPhysRegister(address, modifier, &index);
}

///
/// Clear a bit at the given index in a physical layer register conveniently
///
/// @param address The register address
/// @param index The zero-based bit index
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
///
IOReturn RealtekPCIeCardReaderController::clearPhysRegisterBitAtIndex(UInt8 address, UInt32 index)
{
    // Guard: The given bit index must be valid
    if (index >= sizeof(UInt16) * 8)
    {
        perr("The given bit index %d is invalid.", index);
        
        return kIOReturnBadArgument;
    }
    
    // Modify the register value
    auto modifier = [](UInt16 regVal, const void* context) -> UInt16
    {
        return regVal & ~(1 << *reinterpret_cast<const UInt32*>(context));
    };
    
    return this->modifyPhysRegister(address, modifier, &index);
}

///
/// Append the given value to a physical layer register conveniently
///
/// @param address The register address
/// @param mask The register value mask
/// @param append The value to be appended to the current value
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
/// @note Port: This function replaces `rtsx_pci_update_phy()` defined in `rtsx_pci.h`.
///
IOReturn RealtekPCIeCardReaderController::appendPhysRegister(UInt8 address, UInt16 mask, UInt16 append)
{
    Pair<UInt16, UInt16> pair = { mask, append };
    
    auto modifier = [](UInt16 regVal, const void* context) -> UInt16
    {
        auto pair = reinterpret_cast<const Pair<UInt16, UInt16>*>(context);
        
        return (regVal & pair->first) | pair->second;
    };
    
    return this->modifyPhysRegister(address, modifier, &pair);
}

///
/// Append each given value to the corresponding physical layer register conveniently
///
/// @param pairs An array of registers and their masks and values
/// @param count The number of elements in the array
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
///
IOReturn RealtekPCIeCardReaderController::appendPhysRegisters(const PhysRegMaskValuePair* pairs, size_t count)
{
    IOReturn retVal = kIOReturnSuccess;
    
    for (auto index = 0; index < count; index += 1)
    {
        retVal = this->appendPhysRegister(pairs[index].address, pairs[index].mask, pairs[index].value);
        
        if (retVal != kIOReturnSuccess)
        {
            break;
        }
    }
    
    return retVal;
}

//
// MARK: - Host Buffer Management
//

///
/// Read from the host command buffer into the given buffer
///
/// @param offset A byte offset into the host buffer
/// @param buffer A non-null buffer
/// @param length The number of bytes to read
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note This function coordinates all accesses to the host buffer.
///
IOReturn RealtekPCIeCardReaderController::readHostBuffer(IOByteCount offset, void* buffer, IOByteCount length)
{
    // Guard: The given buffer must be non-null
    if (buffer == nullptr)
    {
        perr("The given buffer is NULL.");
        
        return kIOReturnBadArgument;
    }
    
    // Guard: The number of bytes to read must not exceed 4096 bytes
    if (offset + length > RealtekPCIeCardReaderController::kHostBufferSize)
    {
        perr("Detected an out-of-bounds read: Request to read %llu bytes at offset %llu.", length, offset);
        
        return kIOReturnBadArgument;
    }
    
    // The read routine will run in a gated context
    auto action = [](OSObject* controller, void* offset, void* buffer, void* length, void*) -> IOReturn
    {
        // Retrieve the controller instance
        auto instance = OSDynamicCast(RealtekPCIeCardReaderController, controller);
        
        passert(instance != nullptr, "The controller instance is invalid.");
        
        auto off = *reinterpret_cast<IOByteCount*>(offset);
        
        auto len = *reinterpret_cast<IOByteCount*>(length);
        
        passert(instance->hostBufferDMACommand->readBytes(off, buffer, len) == len,
                "Should be able to read from the host buffer at this moment.");
        
        return kIOReturnSuccess;
    };
    
    return this->commandGate->runAction(action, &offset, buffer, &length);
}

///
/// Write to the host buffer form the given buffer
///
/// @param offset A byte offset into the host buffer
/// @param buffer A non-null buffer
/// @param length The number of bytes to write
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note This function coordinates all accesses to the host buffer.
///
IOReturn RealtekPCIeCardReaderController::writeHostBuffer(IOByteCount offset, const void* buffer, IOByteCount length)
{
    // Guard: The given buffer must be non-null
    if (buffer == nullptr)
    {
        perr("The given buffer is NULL.");
        
        return kIOReturnBadArgument;
    }
    
    // Guard: The number of bytes to read must not exceed 4096 bytes
    if (offset + length > RealtekPCIeCardReaderController::kHostBufferSize)
    {
        perr("Detected an out-of-bounds write: Request to write %llu bytes at offset %llu.", length, offset);
        
        return kIOReturnBadArgument;
    }
    
    // The write routine will run in a gated context
    auto action = [](OSObject* controller, void* offset, void* buffer, void* length, void*) -> IOReturn
    {
        // Retrieve the controller instance
        auto instance = OSDynamicCast(RealtekPCIeCardReaderController, controller);
        
        passert(instance != nullptr, "The controller instance is invalid.");
        
        auto off = *reinterpret_cast<IOByteCount*>(offset);
        
        auto len = *reinterpret_cast<IOByteCount*>(length);
        
        passert(instance->hostBufferDMACommand->writeBytes(off, buffer, len) == len,
                "Should be able to write to the host buffer at this moment.");
        
        return kIOReturnSuccess;
    };
    
    return this->commandGate->runAction(action, &offset, const_cast<void*>(buffer), &length);
}

///
/// Dump the host buffer contents
///
/// @param offset A byte offset into the host data buffer
/// @param length The number of bytes to dump
/// @param column The number of columns to print
///
void RealtekPCIeCardReaderController::dumpHostBuffer(IOByteCount offset, IOByteCount length, IOByteCount column)
{
    UInt8* buffer = reinterpret_cast<UInt8*>(IOMalloc(length));
    
    this->readHostBuffer(offset, buffer, length);
    
    pbuf(buffer, length, column);
    
    IOFree(buffer, length);
}

//
// MARK: - Host Command Management
//

///
/// Start a new host command transfer session
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note The caller must invoke this function before any subsequent calls to `enqueue*Command()`.
///       Any commands enqueued before this function call will be overwritten.
///       Once all commands are enqueued, the caller should invoke `endCommandTransfer()` to send commands to the device.
/// @note Port: This function replaces `rtsx_pci_init_cmd()` defined in `rtsx_pci.h`.
///
IOReturn RealtekPCIeCardReaderController::beginCommandTransfer()
{
    // The reset counter routine will run in a gated context
    auto action = [](OSObject* controller, void*, void*, void*, void*) -> IOReturn
    {
        // Retrieve the controller instance
        auto instance = OSDynamicCast(RealtekPCIeCardReaderController, controller);
        
        passert(instance != nullptr, "The controller instance is invalid.");
        
        // Reset the counter
        instance->hostCommandCount = 0;
        
        return kIOReturnSuccess;
    };
    
    pinfo("Begin a command transfer session.");
    
    return this->commandGate->runAction(action);
}

///
/// Enqueue a command
///
/// @param command The command
/// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full, `kIOReturnError` otherwise.
/// @note Port: This function replaces `rtsx_pci_add_cmd()` defined in `rtsx_psr.c`.
///
IOReturn RealtekPCIeCardReaderController::enqueueCommand(const Command& command)
{
    // The enqueue routine will run in a gated context
    auto action = [](OSObject* controller, void* command, void*, void*, void*) -> IOReturn
    {
        // Retrieve the controller instance
        auto instance = OSDynamicCast(RealtekPCIeCardReaderController, controller);
        
        passert(instance != nullptr, "The controller instance is invalid.");
        
        // Guard: Ensure that the queue is not full
        if (instance->hostCommandCount >= RTSX::MMIO::HCBAR::kMaxNumCmds)
        {
            pwarning("The host command buffer queue is full.");
            
            return kIOReturnBusy;
        }
        
        // Retrieve and write the command value
        UInt64 offset = instance->hostCommandCount * sizeof(UInt32);
        
        UInt32 value = OSSwapHostToLittleInt32(reinterpret_cast<Command*>(command)->value);
        
        passert(instance->hostBufferDMACommand->writeBytes(offset, &value, sizeof(UInt32)) == sizeof(UInt32),
                "Should be able to write the command value to the host buffer at this moment.");
        
        instance->hostCommandCount += 1;
        
        return kIOReturnSuccess;
    };
    
    return this->commandGate->runAction(action, const_cast<void*>(reinterpret_cast<const void*>(&command)));
}

///
/// Enqueue a command to read the register at the given address conveniently
///
/// @param address The register address
/// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full.
/// @note Port: This function replaces `rtsx_pci_add_cmd()` defined in `rtsx_psr.c`.
///
IOReturn RealtekPCIeCardReaderController::enqueueReadRegisterCommand(UInt16 address)
{
    pinfo("Enqueue: Address = 0x%04x; Mask = 0x%02x; Value = 0x%02x.", address, 0, 0);
    
    return this->enqueueCommand(Command(Command::kReadRegister, address, 0, 0));
}

///
/// Enqueue a command to write a value to the register at the given address conveniently
///
/// @param address The register address
/// @param mask The register value mask
/// @param value The register value
/// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full.
/// @note Port: This function replaces `rtsx_pci_add_cmd()` defined in `rtsx_psr.c`.
///
IOReturn RealtekPCIeCardReaderController::enqueueWriteRegisterCommand(UInt16 address, UInt8 mask, UInt8 value)
{
    pinfo("Enqueue: Address = 0x%04x; Mask = 0x%02x; Value = 0x%02x.", address, mask, value);
    
    return this->enqueueCommand(Command(Command::kWriteRegister, address, mask, value));
}

///
/// Enqueue a command to check the value of the register at the given address conveniently
///
/// @param address The register address
/// @param mask The register value mask
/// @param value The register value
/// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full.
/// @note Port: This function replaces `rtsx_pci_add_cmd()` defined in `rtsx_psr.c`.
///
IOReturn RealtekPCIeCardReaderController::enqueueCheckRegisterCommand(UInt16 address, UInt8 mask, UInt8 value)
{
    pinfo("Enqueue: Address = 0x%04x; Mask = 0x%02x; Value = 0x%02x.", address, mask, value);
    
    return this->enqueueCommand(Command(Command::kCheckRegister, address, mask, value));
}

///
/// Finish the existing host command transfer session
///
/// @param timeout Specify the amount of time in milliseconds
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
/// @note Port: This function replaces `rtsx_pci_send_cmd()` defined in `rtsx_psr.c`.
/// @note This function sends all commands in the queue to the device.
///
IOReturn RealtekPCIeCardReaderController::endCommandTransfer(UInt32 timeout)
{
    // The transfer routine will run in a gated context
    // Returns `kIOReturnTimeout` if the transfer has timed out;
    //         `kIOReturnSuccess` if the transfer has succeeded;
    //         `kIOReturnError` otherwise.
    auto action = [](OSObject* controller, void* timeout, void*, void*, void*) -> IOReturn
    {
        // Retrieve the controller instance
        auto instance = OSDynamicCast(RealtekPCIeCardReaderController, controller);
        
        passert(instance != nullptr, "The controller instance is invalid.");
        
        // Tell the device the location of the command buffer and to start executing commands
        using namespace RTSX::MMIO;
        
        instance->hostBufferTransferStatus = kIOReturnNotReady;
        
        instance->writeRegister32(rHCBAR, instance->hostBufferAddress);
        
        instance->writeRegister32(rHCBCTLR, HCBCTLR::RegValueForStartCommand(instance->hostCommandCount));
        
        // Set up the timer
        passert(instance->hostBufferTimer != nullptr, "The host buffer timer should not be NULL.");
        
        passert(instance->hostBufferTimer->setTimeoutMS(*reinterpret_cast<UInt32*>(timeout)) == kIOReturnSuccess, "Should be able to set the timeout.");
        
        // Wait for the transfer result
        // Block the current thread and release the gate
        // Either the timeout handler or the interrupt handler will modify the status and wakeup the current thread
        instance->commandGate->commandSleep(&instance->hostBufferTransferStatus);
        
        // When the sleep function returns, the transfer is done
        return instance->hostBufferTransferStatus;
    };
    
    IOReturn retVal = this->commandGate->runAction(action, &timeout);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to transfer host commands. Error = 0x%x.", retVal);
        
        psoftassert(this->stopTransfer() == kIOReturnSuccess,
                    "Failed to terminate the current transfer session.");
    }
    
    pinfo("Finished a command transfer session. Returns 0x%x.", retVal);
    
    return retVal;
}

///
/// Finish the existing host command transfer session without waiting for the completion interrupt
///
/// @note Port: This function replaces `rtsx_pci_send_cmd_no_wait()` defined in `rtsx_psr.c`.
/// @note This function sends all commands in the queue to the device.
///
void RealtekPCIeCardReaderController::endCommandTransferNoWait()
{
    // The transfer routine will run in a gated context
    auto action = [](OSObject* controller, void* timeout, void*, void*, void*) -> IOReturn
    {
        // Retrieve the controller instance
        auto instance = OSDynamicCast(RealtekPCIeCardReaderController, controller);
        
        passert(instance != nullptr, "The controller instance is invalid.");
        
        // Tell the device the location of the command buffer and to start executing commands
        using namespace RTSX::MMIO;
        
        instance->hostBufferTransferStatus = kIOReturnNotReady;
        
        instance->writeRegister32(rHCBAR, instance->hostBufferAddress);
        
        instance->writeRegister32(rHCBCTLR, HCBCTLR::RegValueForStartCommand(instance->hostCommandCount));
        
        return kIOReturnSuccess;
    };
    
    this->commandGate->runAction(action);
}

///
/// Enqueue a sequence of commands to read registers conveniently
///
/// @param pairs A sequence of pairs of register address and value
/// @return `kIOReturnSuccess` on success, `kIOReturnError` otherwise.
/// @note This function provides a elegant way to enqueue multiple commands and handle errors.
///
IOReturn RealtekPCIeCardReaderController::enqueueReadRegisterCommands(const ChipRegValuePairs& pairs)
{
    IOItemCount count = pairs.size();
    
    for (auto index = 0; index < count; index += 1)
    {
        auto retVal = this->enqueueReadRegisterCommand(pairs.get(index).address);
        
        if (retVal != kIOReturnSuccess)
        {
            perr("Failed to enqueue the command at index %d. Error = 0x%x.", index, retVal);
            
            return retVal;
        }
    }
    
    return kIOReturnSuccess;
}

///
/// Enqueue a sequence of commands to write registers conveniently
///
/// @param pairs A sequence of pairs of register address and value
/// @return `kIOReturnSuccess` on success, `kIOReturnError` otherwise.
/// @note This function provides a elegant way to enqueue multiple commands and handle errors.
///
IOReturn RealtekPCIeCardReaderController::enqueueWriteRegisterCommands(const ChipRegValuePairs& pairs)
{
    IOItemCount count = pairs.size();
    
    for (auto index = 0; index < count; index += 1)
    {
        ChipRegValuePair pair = pairs.get(index);
        
        auto retVal = this->enqueueWriteRegisterCommand(pair.address, pair.mask, pair.value);
        
        if (retVal != kIOReturnSuccess)
        {
            perr("Failed to enqueue the command at index %d. Error = 0x%x.", index, retVal);
            
            return retVal;
        }
    }
    
    return kIOReturnSuccess;
}

///
/// Transfer a sequence of commands to read registers conveniently
///
/// @param pairs A sequence of pairs of register address and value
/// @param timeout Specify the amount of time in milliseconds
/// @return `kIOReturnSuccess` on success, `kIOReturnError` otherwise.
/// @note This function provides a elegant way to start a command transfer session and handle errors.
///       Same as calling `startCommandTransfer`, a sequence of `enqueueReadRegisterCommand` and `endCommandTransfer`.
///
IOReturn RealtekPCIeCardReaderController::transferReadRegisterCommands(const ChipRegValuePairs& pairs, UInt32 timeout)
{
    IOReturn retVal;
    
    // Guard: Begin command transfer
    retVal = this->beginCommandTransfer();
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to initiate a new command transfer session. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // Guard: Enqueue commands
    retVal = this->enqueueReadRegisterCommands(pairs);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to enqueue the given sequence of commands. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // Guard: Transfer commands
    return this->endCommandTransfer(timeout);
}

///
/// Transfer a sequence of commands to write registers conveniently
///
/// @param pairs A sequence of pairs of register address and value
/// @param timeout Specify the amount of time in milliseconds
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
/// @note This function provides a elegant way to start a command transfer session and handle errors.
///       Same as calling `startCommandTransfer`, a sequence of `enqueueWriteRegisterCommand` and `endCommandTransfer`.
///

IOReturn RealtekPCIeCardReaderController::transferWriteRegisterCommands(const ChipRegValuePairs& pairs, UInt32 timeout)
{
    IOReturn retVal;
    
    // Guard: Begin command transfer
    retVal = this->beginCommandTransfer();
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to initiate a new command transfer session. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // Guard: Enqueue commands
    retVal = this->enqueueWriteRegisterCommands(pairs);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to enqueue the given sequence of commands. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // Guard: Transfer commands
    return this->endCommandTransfer(timeout);
}

///
/// Tell the device to stop executing commands
///
/// @note Port: This function replaces `rtsx_pci_stop_cmd()` defined in `rtsx_psr.c`.
/// @note Subclasses may override this function to inject additional code before calling the generic routine.
///
IOReturn RealtekPCIeCardReaderController::stopTransfer()
{
    pinfo("Stopping the transfer...");
    
    using namespace RTSX::MMIO;
    
    this->writeRegister32(rHCBCTLR, HCBCTLR::kStopCommand);
    
    this->writeRegister32(rHDBCTLR, HDBCTLR::kStopDMA);
    
    using namespace RTSX::Chip;
    
    const ChipRegValuePair pairs[] =
    {
        // Reset DMA
        { DMA::rCTL, DMA::CTL::kResetMask, DMA::CTL::kResetValue },
        
        // Flush the ring buffer
        { RBUF::rCTL, RBUF::CTL::kFlushMask, RBUF::CTL::kFlushValue }
    };
    
    return this->writeChipRegisters(SimpleRegValuePairs(pairs));
}

//
// MARK: - Host Data Management
//

///
/// [Helper] Convert a physical scatter/gather entry to a value to be stored in the host data buffer
///
/// @param segment An entry in the scatter/gather list
/// @return The value to be written to the host data buffer.
/// @note Port: This helper function is refactored from `rtsx_pci_add_sg_tbl()` defined in `rtsx_psr.c`.
///             RTS5261 and RTS5208 controllers must override this function but should not include the `option` part in the returned value.
///
UInt64 RealtekPCIeCardReaderController::transformIOVMSegment(IODMACommand::Segment32 segment)
{
    return static_cast<UInt64>(segment.fIOVMAddr) << 32 | static_cast<UInt64>(segment.fLength) << 12;
}

///
/// [Helper] Generate a physical scather/gather list from the given DMA command and enqueue all entries into the host data buffer
///
/// @param command A non-null, perpared DMA command
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This helper function replaces `rtsx_pci_add_sg_tbl()` defined in `rtsx_psr.c`.
/// @warning The caller must ensure that the given instance of `IODMACommand` is prepared.
///
IOReturn RealtekPCIeCardReaderController::enqueueDMACommand(IODMACommand* command)
{
    using namespace RTSX::MMIO;
    
    //
    // Step 1: Generate a physical scather/gather list
    //
    // The maximum number of entries is 384
    // FIXME: Is it OK to assume that the offset to the memory descriptor is 0? (YES, IT's OK; The upper layer can guarantee this)
    // FIXME: Pass additional information about the DMA command: e.g., preferred number of segments to be generated (Yes, host device caps)
    pinfo("Generating a scather/gather list from the given DMA command...");
    
    UInt64 offset = 0;
    
    IODMACommand::Segment32 segments[256] = {};
    
    UInt32 numSegments = 256; // TODO: THIS IS THE DEVICE LIMIT (DECLARE DMA CAPS)
    
    IOReturn retVal = command->gen32IOVMSegments(&offset, segments, &numSegments);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to generate a scather/gather list from the given DMA command. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    pinfo("Generated a scather/gather list from the given DMA command. Offset = %llu; Number of entries = %d.", offset, numSegments);
    
    //
    // Step 2: Enqueue each scather/gather list entry
    //
    // Ensure that we can do a trick here to reuse the buffer
    static_assert(sizeof(IODMACommand::Segment32) == sizeof(UInt64),
                  "ABI Error: IODMACommand::Segment32 is no longer 64 bytes long.");
    
    for (auto index = 0; index < numSegments; index += 1)
    {
        UInt64 entry = this->transformIOVMSegment(segments[index]) | HDBAR::kSGOptionValid | HDBAR::kSGOptionTransferData;
        
        pinfo("[%03d] DMA Bus Address = 0x%08x; Length = %d; Entry = 0x%16llx.",
              index, segments[index].fIOVMAddr, segments[index].fLength, entry);
        
        reinterpret_cast<UInt64*>(segments)[index] = entry;
    }
    
    // The last entry must have the end bit set
    reinterpret_cast<UInt64*>(segments)[numSegments - 1] |= static_cast<UInt64>(HDBAR::kSGOptionEnd);
    
    return this->writeHostDataBuffer(0, segments, numSegments * sizeof(UInt64));
}

///
/// [Helper] Perform a DMA transfer
///
/// @param command A non-null, perpared DMA command
/// @param timeout Specify the amount of time in milliseconds
/// @param control Specify the value that will be written to the register `HDBCTLR` to customize the DMA transfer
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
/// @note Port: This function replaces `rtsx_pci_dma_transfer()` defined in `rtsx_psr.c`.
/// @note This helper function is invoked by both `performDMARead()` and `performDMAWrite()`.
///       The caller should avoid calling this function directly.
/// @warning The caller must ensure that the given instance of `IODMACommand` is prepared.
///
IOReturn RealtekPCIeCardReaderController::performDMATransfer(IODMACommand* command, UInt32 timeout, UInt32 control)
{
    // Generate the scather/gather list from the given command
    // Write all entries in the list to the host data buffer
    pinfo("Processing the DMA command...");
    
    IOReturn retVal = this->enqueueDMACommand(command);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to write the scather/gather list generated from the given DMA command to the host data buffer. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // Tell the card reader where to find the data and start the DMA transfer
    // The transfer routine will run in a gated context
    // Returns `kIOReturnTimeout` if the transfer has timed out;
    //         `kIOReturnSuccess` if the transfer has succeeded;
    //         `kIOReturnError` otherwise.
    auto action = [](OSObject* controller, void* timeout, void* control, void*, void*) -> IOReturn
    {
        // Retrieve the controller instance
        auto instance = OSDynamicCast(RealtekPCIeCardReaderController, controller);
        
        passert(instance != nullptr, "The controller instance is invalid.");
        
        // Tell the device the location of the data buffer and to start the DMA transfer
        using namespace RTSX::MMIO;
        
        instance->hostBufferTransferStatus = kIOReturnNotReady;
        
        instance->writeRegister32(rHDBAR, instance->hostBufferAddress + RealtekPCIeCardReaderController::kHostDatabufferOffset);
        
        instance->writeRegister32(rHDBCTLR, *reinterpret_cast<UInt32*>(control));
        
        // Set up the timer
        passert(instance->hostBufferTimer != nullptr, "The host buffer timer should not be NULL.");
        
        passert(instance->hostBufferTimer->setTimeoutMS(*reinterpret_cast<UInt32*>(timeout)) == kIOReturnSuccess, "Should be able to set the timeout.");
        
        // Wait for the transfer result
        // Block the current thread and release the gate
        // Either the timeout handler or the interrupt handler will modify the status and wakeup the current thread
        instance->commandGate->commandSleep(&instance->hostBufferTransferStatus);
        
        // When the sleep function returns, the transfer is done
        return instance->hostBufferTransferStatus;
    };
    
    pinfo("Initiating the DMA transfer with timeout = %d ms and control = 0x%08x...", timeout, control);
    
    retVal = this->commandGate->runAction(action, &timeout, &control);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to perform the DMA transfer. Error = 0x%x.", retVal);
        
        // TODO: INCREMENT DMA ERROR COUNT
        psoftassert(this->stopTransfer() == kIOReturnSuccess,
                    "Failed to terminate the current transfer session.");
        
        return retVal;
    }
    
    pinfo("The DMA transfer has completed.");
    
    return retVal;
}

///
/// Perform a DMA read operation
///
/// @param command A non-null, perpared DMA command
/// @param timeout Specify the amount of time in milliseconds
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
///
IOReturn RealtekPCIeCardReaderController::performDMARead(IODMACommand* command, UInt32 timeout)
{
    using namespace RTSX::MMIO;
    
    pinfo("The host device requests a DMA read operation.");
    
    return this->performDMATransfer(command, timeout, HDBCTLR::kDMARead | HDBCTLR::kStartDMA | HDBCTLR::kUseADMA);
}

///
/// Perform a DMA write operation
///
/// @param command A non-null, perpared DMA command
/// @param timeout Specify the amount of time in milliseconds
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
///
IOReturn RealtekPCIeCardReaderController::performDMAWrite(IODMACommand* command, UInt32 timeout)
{
    using namespace RTSX::MMIO;
    
    pinfo("The host device requests a DMA write operation.");
    
    return this->performDMATransfer(command, timeout, HDBCTLR::kStartDMA | HDBCTLR::kUseADMA);
}

//
// MARK: - LED Management
//

///
/// Turn on the LED
///
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
/// @note Port: This function replaces `turn_on_led()` defined in `struct pcr_ops`.
///             The base controller class implements this function by changing the value of `GPIO_CTL`.
///             RTS5209, 5260, 5286, 5287 and 5289 controllers must override this function.
///
IOReturn RealtekPCIeCardReaderController::turnOnLED()
{
    using namespace RTSX::Chip::GPIO;
    
    return this->writeChipRegister(rCTL, CTL::kLEDMask, CTL::kTurnOnLEDValue);
}

///
/// Turn off the LED
///
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
/// @note Port: This function replaces `turn_off_led()` defined in `struct pcr_ops`.
///             The base controller class implements this function by changing the value of `GPIO_CTL`.
///             RTS5209, 5286, 5287 and 5289 controllers must override this function.
///
IOReturn RealtekPCIeCardReaderController::turnOffLED()
{
    using namespace RTSX::Chip::GPIO;
    
    return this->writeChipRegister(rCTL, CTL::kLEDMask, CTL::kTurnOffLEDValue);
}

///
/// Enable LED blinking
///
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
/// @note Port: This function replaces `enable_auto_blink()` defined in `struct pcr_ops`.
///             The base controller class implements this function by changing the value of `OLT_LED_CTL`.
///             RTS5209, 5286, 5287 and 5289 controllers must override this function.
///
IOReturn RealtekPCIeCardReaderController::enableLEDBlinking()
{
    using namespace RTSX::Chip::OLT_LED;
    
    return this->writeChipRegister(rCTL, CTL::kBlinkingMask, CTL::kEnableBlinkingValue);
}

///
/// Disable LED blinking
///
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
/// @note Port: This function replaces `enable_auto_blink()` defined in `struct pcr_ops`.
///             The base controller class implements this function by changing the value of `OLT_LED_CTL`.
///             RTS5209, 5286, 5287 and 5289 controllers must override this function.
///
IOReturn RealtekPCIeCardReaderController::disableLEDBlinking()
{
    using namespace RTSX::Chip::OLT_LED;
    
    return this->writeChipRegister(rCTL, CTL::kBlinkingMask, CTL::kDisableBlinkingValue);
}

//
// MARK: - Card Power Management
//

///
/// Set the driving parameters for the given output voltage
///
/// @param outputVoltage The target output voltage
/// @param intermediate Pass `false` to begin and complete a new session to set up the driving;
///                     Pass `true` to enqueue commands that are needed to set up the driving only.
/// @param timeout Specify the amount of time in milliseconds if `intermediate` is set to `false`.
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `*_fill_driving()` defined in each concrete controller file.
///
IOReturn RealtekPCIeCardReaderController::setDrivingForOutputVoltage(OutputVoltage outputVoltage, bool intermediate, UInt32 timeout)
{
    // Retrieve the driving table and the selector
    const DrivingTable* table;
    
    UInt8 selector;
    
    if (outputVoltage == OutputVoltage::k3d3V)
    {
        table = this->parameters.sd30DriveTable3d3V;
        
        selector = this->parameters.sd30DriveSelector3d3V;
    }
    else
    {
        table = this->parameters.sd30DriveTable1d8V;
        
        selector = this->parameters.sd30DriveSelector1d8V;
    }
    
    // Guard: Check whether the controller needs to set up the driving
    if (table == nullptr)
    {
        pwarning("The controller does not need to set up the driving.");
        
        return kIOReturnSuccess;
    }
    
    // Populate the registers and their values
    using namespace RTSX::Chip::CARD::SD30::DRVSEL;
    
    const ChipRegValuePair pairs[] =
    {
        { rCLK, 0xFF, table->entries[selector].clock },
        { rCMD, 0xFF, table->entries[selector].command },
        { rDAT, 0xFF, table->entries[selector].data }
    };
    
    // Guard: Check whether this is an intermediate operation
    if (intermediate)
    {
        return this->enqueueWriteRegisterCommands(SimpleRegValuePairs(pairs));
    }
    else
    {
        return this->transferWriteRegisterCommands(SimpleRegValuePairs(pairs), timeout);
    }
}

//
// MARK: - Clock Configurations
//

///
/// Adjust the card clock if DMA transfer errors occurred
///
/// @param cardClock The current card clock
/// @return The adjusted card clock.
/// @note Port: This function replaces the code block that reduces the card clock in `rtsx_pci_switch_clock()` defined in `rtsx_psr.c`.
///             By default, this function does not adjust the card clock and return the given clock.
///             RTS5227 controller must override this function.
///
UInt32 RealtekPCIeCardReaderController::getAdjustedCardClockOnDMAError(UInt32 cardClock)
{
    return cardClock;
}

///
/// Convert the given SSC clock to the divider N
///
/// @param clock The SSC clock in MHz
/// @return The divider N.
/// @note Port: This function replaces `conv_clk_and_div_n()` defined in `rtsx_psr.c`.
///             RTL8411 series controllers must override this function.
///
UInt32 RealtekPCIeCardReaderController::sscClock2DividerN(UInt32 clock)
{
    return clock - 2;
}

///
/// Convert the given divider N back to the SSC clock
///
/// @param n The divider N
/// @return The SSC clock in MHz.
/// @note Port: This function replaces `conv_clk_and_div_n()` defined in `rtsx_psr.c`.
///             RTL8411 series controllers must override this function.
///
UInt32 RealtekPCIeCardReaderController::dividerN2SSCClock(UInt32 n)
{
    return n + 2;
}

///
/// Switch to the given card clock
///
/// @param cardClock The card clock in Hz
/// @param sscDepth The SSC depth value
/// @param initialMode Pass `true` if the card is at its initial stage
/// @param doubleClock Pass `true` if the SSC clock should be doubled
/// @param vpclock Pass `true` if VPCLOCK is used
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_pci_switch_clock()`  defined in `rtsx_psr.c`.
/// @note Port: RTS5261 and RTS5228 controllers must override this function.
///
IOReturn RealtekPCIeCardReaderController::switchCardClock(UInt32 cardClock, SSCDepth sscDepth, bool initialMode, bool doubleClock, bool vpclock)
{
    using namespace RTSX::Chip;
    
    // Adjust the clock divider and the frequency if the card is at its initial stage
    UInt8 clockDivider = SD::CFG1::kClockDivider0;
    
    if (initialMode)
    {
        clockDivider = SD::CFG1::kClockDivider128;
        
        cardClock = MHz2Hz(30);
    }
    
    // Set the clock divider
    IOReturn retVal = this->writeChipRegister(SD::rCFG1, SD::CFG1::kClockDividerMask, clockDivider);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to set the clock divider to %d. Error = 0x%x.", clockDivider, retVal);
        
        return retVal;
    }
    
    // Get the new card clock if DMA transfer errors have occurred
    cardClock = this->getAdjustedCardClockOnDMAError(cardClock);
    
    cardClock = Hz2MHz(cardClock);
    
    pinfo("The card clock will be switched to %d MHz.", cardClock);
    
    // Check whether the host should double the SSC clock
    UInt32 sscClock = cardClock;
    
    if (doubleClock && !initialMode)
    {
        sscClock *= 2;
    }
    
    // Check whether the host should switch the clock
    pinfo("Internal SSC Clock = %d MHz; Current Clock = %d MHz.", sscClock, this->currentSSCClock);
    
    if (sscClock == this->currentSSCClock)
    {
        pinfo("No need to switch the clock.");
        
        return kIOReturnSuccess;
    }
    
    // Calculate the SSC clock divider N
    UInt32 n = this->sscClock2DividerN(sscClock);
    
    if (sscClock <= 2 || n > RealtekPCIeCardReaderController::kMaxSSCClockDividerN)
    {
        perr("The SSC clock divider N %d derived from the clock %d MHz is invalid.", n, sscClock);
        
        return kIOReturnInvalid;
    }
    
    // Calculate the MCU count
    UInt32 mcus = 125 / sscClock + 3;
    
    pinfo("MCU Count = %d.", mcus);
    
    if (mcus > 15)
    {
        mcus = 15;
    }
    
    // Ensure that the SSC clock divider N is not too small
    UInt8 divider;
    
    for (divider = CLK::DIV::k1; divider < CLK::DIV::k8; divider += 1)
    {
        if (n >= RealtekPCIeCardReaderController::kMinSSCClockDividerN)
        {
            break;
        }
        
        n = this->sscClock2DividerN(this->dividerN2SSCClock(n) * 2);
    }
    
    pinfo("SSC clock divider N = %d; Divider register value = %d.", n, divider);
    
    // Calculate the SSC depth
    static constexpr UInt8 kSSCDepthTable[] =
    {
        [static_cast<UInt32>(SSCDepth::k4M)] = SSC::CTL2::kDepth4M,
        [static_cast<UInt32>(SSCDepth::k2M)] = SSC::CTL2::kDepth2M,
        [static_cast<UInt32>(SSCDepth::k1M)] = SSC::CTL2::kDepth1M,
        [static_cast<UInt32>(SSCDepth::k500K)] = SSC::CTL2::kDepth500K,
        [static_cast<UInt32>(SSCDepth::k250K)] = SSC::CTL2::kDepth250K,
    };
    
    UInt8 sscDepthRegValue = kSSCDepthTable[static_cast<UInt32>(sscDepth)];
    
    pinfo("SSC Depth RegVal = %d.", sscDepthRegValue);
    
    // Double the SSC depth if necessary
    if (doubleClock)
    {
        sscDepthRegValue = SSC::CTL2::doubleDepth(sscDepthRegValue);
    }
    
    // Revise the SSC depth
    if (divider > CLK::DIV::k1)
    {
        if (sscDepthRegValue > (divider - 1))
        {
            sscDepthRegValue -= (divider - 1);
        }
        else
        {
            sscDepthRegValue = SSC::CTL2::kDepth4M;
        }
    }
    
    pinfo("Revised SSC Depth RegVal = %d.", sscDepthRegValue);
    
    // Send the command
    const ChipRegValuePair pairs[] =
    {
        { CLK::rCTL, CLK::CTL::kLowFrequency, CLK::CTL::kLowFrequency },
        { CLK::rDIV, 0xFF, static_cast<UInt8>(divider << 4 | mcus) },
        { SSC::rCTL1, SSC::CTL1::kRSTB, 0 },
        { SSC::rCTL2, SSC::CTL2::kDepthMask, sscDepthRegValue },
        { SSC::rDIVN0, 0xFF, static_cast<UInt8>(n) },
        { SSC::rCTL1, SSC::CTL1::kRSTB, SSC::CTL1::kRSTB },
        
        // Send the following commands if vpclock is true
        { SD::rVPCLK0CTL, SD::VPCTL::kPhaseNotReset, 0 },
        { SD::rVPCLK0CTL, SD::VPCTL::kPhaseNotReset, SD::VPCTL::kPhaseNotReset }
    };
    
    if (vpclock)
    {
        retVal = this->transferWriteRegisterCommands(SimpleRegValuePairs(pairs), 2000);
    }
    else
    {
        retVal = this->transferWriteRegisterCommands(SimpleRegValuePairs(pairs, arrsize(pairs) - 2), 2000);
    }
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to transfer commands to set the card clock. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // Wait until the SSC clock becomes stable
    IODelay(RealtekPCIeCardReaderController::kWaitStableSSCClock);
    
    retVal = this->writeChipRegister(CLK::rCTL, CLK::CTL::kLowFrequency, 0);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to toggle the clock. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    pinfo("SSC clock has been switched to %d MHz.", sscClock);
    
    this->currentSSCClock = sscClock;
    
    return kIOReturnSuccess;
}

//
// MARK: - Card Detection and Write Protection
//

///
/// Check whether the card has write protection enabled
///
/// @return `true` if the card is write protected, `false` otherwise.
///
bool RealtekPCIeCardReaderController::isCardWriteProtected()
{
    using namespace RTSX::MMIO;
    
    return BitOptions(this->readRegister32(rBIPR)).contains(BIPR::kSDWriteProtected);
}

///
/// Check whether a card is present
///
/// @return `true` if a card exists, `false` otherwise.
/// @note Port: This function replaces `rtsx_pci_card_exist()` and `cd_deglitch()` defined in `rtsx_psr.c`.
/// @warning: This function supports SD cards only.
///
bool RealtekPCIeCardReaderController::isCardPresent()
{
    using namespace RTSX::MMIO;
    
    return BitOptions(this->readRegister32(rBIPR)).contains(BIPR::kSDExists);
}

//
// MARK: - Overcurrent Protection Support
//

///
/// Initialize and enable overcurrent protection
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_pci_init_ocp()` defined in `rtsx_psr.c`.
///
IOReturn RealtekPCIeCardReaderController::initOvercurrentProtection()
{
    pinfo("Initializing the overcurrent protection...");
    
    if (!this->parameters.ocp.enable)
    {
        pinfo("The device specifies not to enable overcurrent protection.");
        
        return kIOReturnSuccess;
    }
    
    // Setup overcurrent protection registers
    using namespace RTSX::Chip;
    
    const ChipRegValuePair pairs[] =
    {
        // Power up the OCP unit
        { rFPDCTL, FPDCTL::kOCPPowerMask, FPDCTL::kOCPPowerUpValue },
        
        // Setup the time
        { OCP::rPARA1, OCP::PARA1::kSDTimeMask, OCP::PARA1::kSDTimeValue800 },
        
        // Setup the total harmonic distortion value
        { OCP::rPARA2, OCP::PARA2::kSDThdMask, this->parameters.ocp.sdTHD800mA },
        
        // Setup the glitch value
        { OCP::rGLITCH, OCP::GLITCH::kSDMask, this->parameters.ocp.sdGlitch }
    };
    
    return this->writeChipRegisters(SimpleRegValuePairs(pairs));
}

///
/// Enable overcurrent protection
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_pci_enable_ocp()` defined in `rtsx_psr.c`.
///
IOReturn RealtekPCIeCardReaderController::enableOvercurrentProtection()
{
    using namespace RTSX::Chip;
    
    const ChipRegValuePair pairs[] =
    {
        // Ensure that OCP power is up
        { rFPDCTL, FPDCTL::kOCPPowerMask, FPDCTL::kOCPPowerUpValue },
        
        // Enable OCP detection and hardware interrupts
        { OCP::rCTL, OCP::CTL::kSDDetectionAndInterruptMask, OCP::CTL::kSDEnableDetectionAndInterruptValue }
    };
    
    return this->writeChipRegisters(SimpleRegValuePairs(pairs));
}

///
/// Disable overcurrent protection
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_pci_disable_ocp()` defined in `rtsx_psr.c`.
///
IOReturn RealtekPCIeCardReaderController::disableOvercurrentProtection()
{
    using namespace RTSX::Chip;
    
    const ChipRegValuePair pairs[] =
    {
        // Disable OCP detection and hardware interrupts
        { OCP::rCTL, OCP::CTL::kSDDetectionAndInterruptMask, OCP::CTL::kSDDisableDetectionAndInterruptValue },
        
        // Power down the OCP unit
        { rFPDCTL, FPDCTL::kOCPPowerMask, FPDCTL::kOCPPowerDownValue }
    };
    
    return this->writeChipRegisters(SimpleRegValuePairs(pairs));
}

///
/// Get the overcurrent protection status
///
/// @param status The status on return
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_pci_get_ocpstat()` defined in `rtsx_psr.c`.
///
IOReturn RealtekPCIeCardReaderController::getOvercurrentProtectionStatus(UInt8& status)
{
    using namespace RTSX::Chip;
    
    pinfo("Fetching the current overcurrent protection status...");
    
    return this->readChipRegister(OCP::rSTAT, status);
}

///
/// Clear the overcurrent protection status
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_pci_clear_ocpstat()` defined in `rtsx_psr.c`.
///
IOReturn RealtekPCIeCardReaderController::clearOvercurrentProtectionStatus()
{
    using namespace RTSX::Chip;
    
    UInt8 mask = OCP::CTL::kSDClearStatMask | OCP::CTL::kSDClearInterruptMask;
    
    UInt8 value = OCP::CTL::kSDClearStatValue | OCP::CTL::kSDClearInterruptValue;
    
    IOReturn retVal = this->writeChipRegister(OCP::rCTL, mask, value);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to clear the OCP status (step 1). Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    IODelay(100);
    
    retVal = this->writeChipRegister(OCP::rCTL, mask, 0);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to clear the OCP status (step 2). Error = 0x%x.", retVal);
    }
    
    return retVal;
}

//
// MARK: - Card Pull Control Management
//


///
/// Enable pull control for the SD card
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_pci_card_pull_ctl_enable()` defined in `rtsx_psr.c`.
///
IOReturn RealtekPCIeCardReaderController::enablePullControlForSDCard()
{
    return this->transferWriteRegisterCommands(*this->parameters.sdEnablePullControlTable);
}

///
/// Disable pull control for the SD card
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_pci_card_pull_ctl_disable()` defined in `rtsx_psr.c`.
///
IOReturn RealtekPCIeCardReaderController::disablePullControlForSDCard()
{
    return this->transferWriteRegisterCommands(*this->parameters.sdDisablePullControlTable);
}

//
// MARK: - OOBS Polling
//

///
/// Check whether the driver needs to modify the PHY::RCR0 register to enable or disable OOBS polling
///
/// @return `true` if the controller is not RTS525A nor RTS5260.
/// @note RTS525A and RTS5260 controllers should override this function and return `false`.
///
bool RealtekPCIeCardReaderController::oobsPollingRequiresPhyRCR0Access()
{
    return true;
}

///
/// Check whether the hardware supports OOBS polling
///
/// @return `true` if supported, `false` otherwise.
/// @note By default, this function returns `false`.
/// @note e.g., RTS524A and RTS525A should override this function and return `true`.
///
bool RealtekPCIeCardReaderController::supportsOOBSPolling()
{
    return false;
}

///
/// Enable OOBS polling
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
///
IOReturn RealtekPCIeCardReaderController::enableOOBSPolling()
{
    // Check whether RCR0 needs to be modified to enable OOBS polling
    if (this->oobsPollingRequiresPhyRCR0Access())
    {
        using namespace RTSX::PHYS;
        
        IOReturn retVal = this->setPhysRegisterBitAtIndex(rRCR0, RCR0::kOOBSControlBitIndex);
        
        if (retVal != kIOReturnSuccess)
        {
            perr("Failed to set the OOBS bit in the PHY::RCR0 register. Error = 0x%x.", retVal);
            
            return retVal;
        }
    }
    
    // Configure chip registers to enable OOBS polling
    using namespace RTSX::Chip;
    
    const ChipRegValuePair pairs[] =
    {
        { OOBS::rOFFTIMER,   0x32 },
        { OOBS::rONTIMER,    0x05 },
        { OOBS::rVCMONTIMER, 0x83 },
        { OOBS::rPOLLING,    0xDE },
    };
    
    return this->writeChipRegisters(SimpleRegValuePairs(pairs));
}

///
/// Disable OOBS polling
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
///
IOReturn RealtekPCIeCardReaderController::disableOOBSPolling()
{
    if (this->oobsPollingRequiresPhyRCR0Access())
    {
        using namespace RTSX::PHYS;
        
        IOReturn retVal = this->clearPhysRegisterBitAtIndex(rRCR0, RCR0::kOOBSControlBitIndex);
        
        if (retVal != kIOReturnSuccess)
        {
            perr("Failed to clear the OOBS bit in the PHY::RCR0 register. Error = 0x%x.", retVal);
            
            return retVal;
        }
    }
    
    // Configure chip registers to disable OOBS polling
    using namespace RTSX::Chip;
    
    const ChipRegValuePair pairs[] =
    {
        { OOBS::rVCMONTIMER, 0x03 },
        { OOBS::rPOLLING,    0x00 },
    };
    
    return this->writeChipRegisters(SimpleRegValuePairs(pairs));
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
/// @note Port: This function replaces `rtsx_pci_read_ppbuf()` defined in `rtsx_psr.c`.
///
IOReturn RealtekPCIeCardReaderController::readPingPongBuffer(UInt8* destination, IOByteCount length)
{
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
    
    // Size of the command queue is 256
    // Divide into multiple transfer sessions
    for (auto index = 0; index < length; index += 256)
    {
        // Calculate the length for this transfer session
        IOItemCount newLength = static_cast<IOItemCount>(min(length - index, 256));
        
        // Generate a sequence of register addresses
        ContiguousRegValuePairsForReadAccess pairs(RTSX::Chip::PPBUF::rBASE2 + index, newLength);
        
        // Guard: Read the current set of registers
        IOReturn retVal = this->transferReadRegisterCommands(pairs, 250);
        
        if (retVal != kIOReturnSuccess)
        {
            perr("Failed to transfer commands to read %u bytes. Error = 0x%x.", newLength, retVal);
            
            return retVal;
        }
        
        // Copy the register values from the command buffer to the destination buffer
        retVal = this->readHostBuffer(index, destination + index, newLength);
        
        if (retVal != kIOReturnSuccess)
        {
            perr("Failed to transfer %d bytes from the command buffer to the destination buffer. Error = 0x%x.", newLength, retVal);
            
            return kIOReturnError;
        }
    }
    
    pinfo("%llu bytes have been read from the ping pong buffer.", length);
    
    return kIOReturnSuccess;
}

///
/// Write to the ping pong buffer
///
/// @param source The buffer to write
/// @param length The number of bytes to write (cannot exceed 512 bytes)
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_pci_write_ppbuf()` defined in `rtsx_psr.c`.
///
IOReturn RealtekPCIeCardReaderController::writePingPongBuffer(const UInt8* source, IOByteCount length)
{
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
    
    // Size of the command queue is 256
    // Divide into multiple transfer sessions
    for (auto index = 0; index < length; index += 256)
    {
        // Calculate the length for this transfer session
        IOItemCount newLength = static_cast<IOItemCount>(min(length - index, 256));
        
        // Generate a sequence of register addresses and their values
        ContiguousRegValuePairsForWriteAccess pairs(RTSX::Chip::PPBUF::rBASE2 + index, newLength, source + index);
        
        // Guard: Read the current set of registers
        IOReturn retVal = this->transferWriteRegisterCommands(pairs, 250);
        
        if (retVal != kIOReturnSuccess)
        {
            perr("Failed to transfer commands to read %u bytes. Error = 0x%x.", newLength, retVal);
            
            return retVal;
        }
    }
    
    pinfo("%llu bytes have been written to the ping pong buffer.", length);
    
    return kIOReturnSuccess;
}

//
// MARK: - Rx/Tx Phases
//

///
/// Get the clock phase for Rx
///
/// @return The clock phase.
///
ClockPhase RealtekPCIeCardReaderController::getRxClockPhase()
{
    return this->parameters.initialRxClockPhase;
}

///
/// Get the clock phase for Tx
///
/// @return The clock phase.
///
ClockPhase RealtekPCIeCardReaderController::getTxClockPhase()
{
    return this->parameters.initialTxClockPhase;
}

//
// MARK: - Active State Power Management
//

///
/// Toggle the active state power management
///
/// @param enable `true` if ASPM should be enabled, `false` otherwise.
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_comm_set_aspm()` and `ops->set_aspm()` defined in `rtsx_pcr.c`.
///
IOReturn RealtekPCIeCardReaderController::setASPM(bool enable)
{
    using namespace RTSX::Chip;
    
    if (this->parameters.pm.isASPMEnabled == enable)
    {
        pinfo("No need to set the ASPM state.");
        
        return kIOReturnSuccess;
    }
    
    UInt8 regVal = AFCTL::kCTL0 | AFCTL::kCTL1;
    
    if (this->parameters.pm.isASPML1Enabled && enable)
    {
        regVal = 0;
    }
    
    IOReturn retVal = this->writeChipRegister(rAFCTL, AFCTL::kCTL0 | AFCTL::kCTL1, regVal);
    
    if (retVal == kIOReturnSuccess)
    {
        this->parameters.pm.isASPMEnabled = enable;
    }

    if (this->parameters.pm.isASPMEnabled && !enable)
    {
        IOSleep(10);
    }
    
    return retVal;
}

///
/// Set the L1 substates configuration
///
/// @param active Pass `true` to set the active state
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_set_l1off_sub_cfg_d0()` and `set_l1off_cfg_sub_d0()` defined in `rtsx_pcr.c`.
/// @note The base controller provides a default implementation that simply returns `kIOReturnSuccess`.
///
IOReturn RealtekPCIeCardReaderController::setL1OffSubConfigD0(bool active)
{
    return kIOReturnSuccess;
}

///
/// Set the LTR latency
///
/// @param latency The latency
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_comm_set_ltr_latency()` and `rtsx_set_ltr_latency()` defined in `rtsx_psr.c`.
///
IOReturn RealtekPCIeCardReaderController::setLTRLatency(UInt32 latency)
{
    using namespace RTSX::Chip;
    
    pinfo("Setting tht LTR latency to %d.", latency);
    
    const ChipRegValuePair pairs[] =
    {
        // Set the latency value
        { MSG::TX::rDATA0, 0xFF, static_cast<UInt8>(latency & 0xFF) },
        { MSG::TX::rDATA1, 0xFF, static_cast<UInt8>(latency >>  8) },
        { MSG::TX::rDATA2, 0xFF, static_cast<UInt8>(latency >> 16) },
        { MSG::TX::rDATA3, 0xFF, static_cast<UInt8>(latency >> 24) },
        
        // Enable Tx and select the latency mode
        { LTR::rCTL, LTR::CTL::kTxMask | LTR::CTL::kLatencyModeMask, LTR::CTL::kEnableTx | LTR::CTL::kLatencyModeSoftware }
    };
    
    return this->writeChipRegisters(SimpleRegValuePairs(pairs));
}

///
/// Notify the card reader to enter into the worker state
///
/// @note Port: This function replaces `rtsx_pci_start_run()`, `rtsx_pm_full_on()` and `rtsx_comm_pm_full_on()` defined in `rtsx_psr.c`.
///
void RealtekPCIeCardReaderController::enterWorkerState()
{
    if (!this->isIdle)
    {
        return;
    }
    
    pinfo("Entering the worker state...");
    
    this->isIdle = false;
    
    //psoftassert(this->enableLEDBlinking() == kIOReturnSuccess, "Failed to enable the LED.");
    
    psoftassert(this->disableASPM() == kIOReturnSuccess, "Failed to disable the ASPM.");
    
    IOSleep(1); // Fix the DMA transfer timeout issue after disabling ASPM on RTS5260
    
    if (this->parameters.pm.isLTRModeEnabled)
    {
        pinfo("LTR mode is currently enabled. Will adjust the LTR latency.");
        
        psoftassert(this->setLTRLatency(this->parameters.pm.ltrActiveLatency) == kIOReturnSuccess, "Failed to adjust the LTR latency.");
    }
    
    if (this->parameters.pm.enableLTRL1SSPowerGate)
    {
        psoftassert(this->setL1OffSubConfigD0(true) == kIOReturnSuccess, "Failed to set the active L1 substate.");
    }
}

//
// MARK: - Power Management
//

///
/// Prepare to enter the sleep state
///
/// @note Port: This function replaces `rtsx_pci_suspend()` defined in `rtsx_psr.c`.
///
void RealtekPCIeCardReaderController::prepareToSleep()
{
    pinfo("Prepare to sleep...");
    
    // Turn off the LED
    psoftassert(this->turnOffLED() == kIOReturnSuccess, "Failed to turn off the LED.");
    
    // Disable all interrupts
    this->writeRegister32(RTSX::MMIO::rBIER, 0);
    
    // Set the host sleep state
    using namespace RTSX::Chip;
    
    const ChipRegValuePair pairs[] =
    {
        { rPETXCFG, 0x08, 0x08 },
        { rHSSTA, HSSTA::kMask, HSSTA::kHostEnterS3 },
    };
    
    psoftassert(this->writeChipRegisters(SimpleRegValuePairs(pairs)) == kIOReturnSuccess, "Failed to set the host sleep state.");
    
    // Power down the controller
    psoftassert(this->forcePowerDown() == kIOReturnSuccess, "Failed to power down the controller.");
    
    // All done
    pinfo("The hardware is ready to sleep.");
}

///
/// Prepare to wake up from sleep
///
/// @note Port: This function replaces `rtsx_pci_resume()` defined in `rtsx_psr.c`.
///
void RealtekPCIeCardReaderController::prepareToWakeUp()
{
    pinfo("Prepare to wake up...");
    
    // Set the host sleep state
    using namespace RTSX::Chip;
    
    psoftassert(this->writeChipRegister(rHSSTA, HSSTA::kMask, HSSTA::kHostWakeup) == kIOReturnSuccess, "Failed to set the host sleep state.");
    
    // Initialize the hardware
    psoftassert(this->initHardwareCommon() == kIOReturnSuccess, "Failed to initialize the hardware.");
    
    // All done
    pinfo("The hardware is ready.");
}

///
/// Adjust the power state in response to system-wide power events
///
/// @param powerStateOrdinal The number in the power state array of the state the driver is being instructed to switch to
/// @param whatDevice A pointer to the power management object which registered to manage power for this device
/// @return `kIOPMAckImplied` always.
///
IOReturn RealtekPCIeCardReaderController::setPowerState(unsigned long powerStateOrdinal, IOService* whatDevice)
{
    pinfo("Setting the power state from %u to %lu.", this->getPowerState(), powerStateOrdinal);
    
    if (powerStateOrdinal == 0)
    {
        this->prepareToSleep();
    }
    else
    {
        this->prepareToWakeUp();
    }
    
    pinfo("The power state has been set to %lu.", powerStateOrdinal);
    
    return kIOPMAckImplied;
}

//
// MARK: - Hardware Interrupt Management
//

///
/// A filter that examines the interrupt event source
///
/// @param source The interrupt event source
/// @return `true` if the interrupt is of interest, `false` otherwise.
/// @note The filter runs in the primary interrupt context.
///
bool RealtekPCIeCardReaderController::interruptEventSourceFilter(IOFilterInterruptEventSource* source)
{
    // Retrieve and examine pending interrupts
    using namespace RTSX::MMIO;
    
    UInt32 pendingInterrupts = this->readRegister32(rBIPR);
    
    UInt32 enabledInterrupts = this->readRegister32(rBIER);
    
    return (pendingInterrupts & enabledInterrupts) != 0;
}

///
/// Interrupt service routine
///
/// @param sender The interrupt event source
/// @param count The number of interrupts seen before delivery
/// @note The interrupt service routine runs in a gated context.
///       It checks hardware registers to identify the reason and invoke helper handlers.
/// @note The interrupt event source filter guarantees that there is a pending interrupt when the handler is invoked.
///
void RealtekPCIeCardReaderController::interruptHandlerGated(IOInterruptEventSource* sender, int count)
{
    // Retrieve pending interrupts
    using namespace RTSX::MMIO;
    
    pinfo("Interrupt handler started.");
    
    BitOptions pendingInterrupts = this->readRegister32(rBIPR);
    
    // Acknowledge the interrupt
    this->writeRegister32(rBIPR, pendingInterrupts.flatten());
    
    pinfo("Interrupts are ackownledged.");
    
    pinfo("Pending interrupts = 0x%x.", pendingInterrupts.flatten());
    
    // Check whether the interrupt is already handled
    if (pendingInterrupts.flatten() == 0xFFFFFFFF)
    {
        pinfo("Interrupts are already handled.");
        
        return;
    }
    
    // Filter out disabled interrupts but keep the low 23 bits
    pendingInterrupts.mutativeBitwiseAnd(this->readRegister32(rBIER) | 0x7FFFFF);
    
    pinfo("Filtered pending interrupts = 0x%x.", pendingInterrupts.flatten());
    
    // Examine pending interrupts
    // Case 1: SD Overcurrent Protection Interrupts
    if (pendingInterrupts.contains(BIPR::kSDOvercurrentOccurred) && this->parameters.ocp.enable)
    {
        this->onSDCardOvercurrentOccurredGated();
    }
    
    // Case 2: Host Command/Data Transfer Interrupts
    if (pendingInterrupts.contains(BIPR::kTransferSucceeded))
    {
        this->onTransferDoneGated(true);
    }
    
    if (pendingInterrupts.contains(BIPR::kTransferFailed))
    {
        this->onTransferDoneGated(false);
    }
    
    // Case 3: Card Insertion/Removal Interrupts
    if (pendingInterrupts.contains(BIPR::kSD))
    {
        if (pendingInterrupts.contains(BIPR::kSDExists))
        {
            this->onSDCardInsertedGated();
        }
        else
        {
            this->onSDCardRemovedGated();
        }
    }
}

///
/// Timeout interrupt service routine
///
/// @param sender The timer event source
/// @note The timeout handler runs in a gated context.
///
void RealtekPCIeCardReaderController::bufferTransferTimeoutHandlerGated(IOTimerEventSource* sender)
{
    // The transfer has timed out
    pinfo("The current transfer session has timed out.");
    
    pinfo("BIER = 0x%08x.", this->readRegister32(RTSX::MMIO::rBIER));
    
    pinfo("BIPR = 0x%08x.", this->readRegister32(RTSX::MMIO::rBIPR));
    
    // Update the transfer status
    this->hostBufferTransferStatus = kIOReturnTimeout;
    
    // Wakeup the client thread
    this->commandGate->commandWakeup(&this->hostBufferTransferStatus);
}

///
/// Helper interrupt service routine when a host command or data transfer is done
///
/// @param succeeded `true` if the transfer has succeeded. `false` otherwise.
/// @note This interrupt service routine runs in a gated context.
///
void RealtekPCIeCardReaderController::onTransferDoneGated(bool succeeded)
{
    // All accesses to the status variable is performed in a gated context
    // Guard: Check whether the transfer has already timed out
    if (this->hostBufferTransferStatus == kIOReturnTimeout)
    {
        pinfo("The current transfer session has already timed out.");
        
        return;
    }
    
    pinfo("The current transfer session has completed. Succeeded = %s.", YESNO(succeeded));
    
    // The transfer has not yet timed out
    // Cancel the timer but don't remove it from the work loop and release it
    // We will reuse the timer later
    this->hostBufferTimer->cancelTimeout();
    
    // Update the transfer status
    this->hostBufferTransferStatus = succeeded ? kIOReturnSuccess : kIOReturnError;
    
    // Wakeup the client thread
    this->commandGate->commandWakeup(&this->hostBufferTransferStatus);
}

///
/// Helper interrupt service routine when an overcurrent is detected
///
/// @note This interrupt service routine runs in a gated context.
///
void RealtekPCIeCardReaderController::onSDCardOvercurrentOccurredGated()
{
    // Guard: Retrieve the current OCP status
    using namespace RTSX::Chip;
    
    pinfo("Detected an overcurrent.");
    
    UInt8 status = 0;
    
    if (this->getOvercurrentProtectionStatus(status) != kIOReturnSuccess)
    {
        perr("Failed to retrieve the current OCP status.");
        
        return;
    }
    
    // Guard: Check the current OCP status
    if (!BitOptions(status).contains(OCP::STAT::kSDNow | OCP::STAT::kSDEver))
    {
        pinfo("The current OCP status 0x%x shows that no overcurrent event detected on SD.", status);
        
        return;
    }
    
    // Overcurrent Detected
    psoftassert(this->powerOffCard() == kIOReturnSuccess,
                "Failed to power off the card.");
    
    psoftassert(this->writeChipRegister(CARD::rOUTPUT, CARD::OUTPUT::kSDMask, CARD::OUTPUT::kDisableSDValue) == kIOReturnSuccess,
                "Failed to disable the card output.");
    
    psoftassert(this->clearOvercurrentProtectionStatus() == kIOReturnSuccess,
                "Failed to clear the OCP status.");
}

///
/// Helper interrupt service routine when a SD card is inserted
///
/// @note This interrupt service routine runs in a gated context.
/// @note Port: This function replaces `rtsx_pci_card_detect()` defined in `rtsx_psr.c` but has a completely different design and implementation.
///
void RealtekPCIeCardReaderController::onSDCardInsertedGated()
{
    // Notify the host device
    pinfo("A SD card is inserted.");
    
    this->slot->onSDCardInsertedGated();
}

///
/// Helper interrupt service routine when a SD card is removed
///
/// @note This interrupt service routine runs in a gated context.
/// @note Port: This function replaces `rtsx_pci_card_detect()` defined in `rtsx_psr.c` but has a completely different design and implementation.
///
void RealtekPCIeCardReaderController::onSDCardRemovedGated()
{
    // Notify the host device
    pinfo("The SD card has been removed.");
    
    this->slot->onSDCardRemovedGated();
}

//
// MARK: - Hardware Initialization and Configuration
//

///
/// Get the IC revision
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `*_get_ic_version()` defined in each controller file.
///
IOReturn RealtekPCIeCardReaderController::getRevision(Revision& revision)
{
    pinfo("Fetching the chip revision...");
    
    using namespace RTSX::Chip;
    
    UInt8 regVal = 0xFF;
    
    IOReturn retVal = this->readChipRegister(rDUMMY, regVal);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to read the revision register. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    revision = Revision::parse(regVal & DUMMY::kICRevisionMask);
    
    pinfo("Chip revision has been fetched: Rev %s.", revision.stringify());
    
    return kIOReturnSuccess;
}

///
/// Optimize the physical layer
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `optimize_phy()` defined in the the `pcr->ops`.
///
IOReturn RealtekPCIeCardReaderController::optimizePhys()
{
    return kIOReturnSuccess;
}

///
/// Initialize the hardware (Common Part)
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_pci_init_hw()` defined in `rtsx_psr.c`.
///
IOReturn RealtekPCIeCardReaderController::initHardwareCommon()
{
    pinfo("Initializing the card reader...");
    
    // Enable the bus interrupt
    using namespace RTSX::MMIO;
    
    pinfo("Enabling the bus interrupt...");
    
    UInt32 bier = BIER::kEnableTransferSuccess |
                  BIER::kEnableTransferFailure |
                  BIER::kEnableSD;
    
    if (this->parameters.ocp.enable)
    {
        bier |= BIER::kEnableSDOvercurrent;
    }
    
    this->writeRegister32(rBIER, bier);
    
    pinfo("Bus interrupt has been enabled. MMIO::BIER = 0x%08x.", bier);
    
    // Power on the SSC
    // TODO: Create a virtual function to support RTS5261
    pinfo("Powering on the SSC...");
    
    using namespace RTSX::Chip;
    
    IOReturn retVal = this->writeChipRegister(rFPDCTL, FPDCTL::kSSCPowerMask, FPDCTL::kSSCPowerUpValue);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to power on the SSC. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // Wait until the SSC power becomes stable
    IODelay(200);
    
    pinfo("SSC has been powered on.")
    
    // Disable the ASPM during the initialization
    psoftassert(this->disableASPM() == kIOReturnSuccess, "Failed to disable the ASPM.");
    
    // Optimize the physical layer
    retVal = this->optimizePhys();
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to optimize the physical layer. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // Configure the card reader
    pinfo("Configuring the card reader chip...");
    
    const ChipRegValuePair pairs[] =
    {
        // Set `mcu_cnt` to 7 to ensure that data can be sampled properly
        { CLK::rDIV, 0x07, 0x07 },
        
        // Set the host sleep state
        { rHSSTA, HSSTA::kMask, HSSTA::kHostWakeup },
        
        // Disable the card clock
        { CARD::rCLK, CARD::CLK::kMask, CARD::CLK::kDisable },
        
        // Reset the link state
        { rLINKSTA, 0x0A, 0x00 },
        
        // Set the card drive selector
        { CARD::rDRVSEL, 0xFF, this->parameters.cardDriveSelector },
        
        // Enable the SSC clock
        { SSC::rCTL1, 0xFF, SSC::CTL1::kEnable8x | SSC::CTL1::kSelector4M },
        
        // Set the SSC depth
        // TODO: Create a virtual function to retrieve the value of `SSC Depth 2M` to support RTS5261 and RTS5228
        { SSC::rCTL2, 0xFF, SSC::CTL2::kDepth2M },
        
        // Disable the power save mode of the card
        { rLINKSTA, 0x16, 0x10 },
        
        // Clear the link ready interrupt
        { rIRQSTAT0, IRQSTAT0::kLinkReadyInt, IRQSTAT0::kLinkReadyInt },
        
        // Enlarge the estimation window of PERST# glitch to reduce the chance of invalid card interrupt
        { rPGWIDTH, 0xFF, 0x80 },
        
        // Update the RC oscillator to 400k
        // Bit[0] F_HIGH: for RC oscillator, Rst_value is 1'b1, 1: 2M, 0: 400k
        { rRCCTL, 0x01, 0x00 },
        
        // Set interrupt write clear
        // Bit 1: U_elbi_if_rd_clr_en
        // 1: Enable ELBI interrupt[31:22] & [7:0] flag read clear
        // 0: ELBI interrupt flag[31:22] & [7:0] only can be write clear
        { rNFTSTXCTL, 0x02, 0x00 },
    };
    
    retVal = this->transferWriteRegisterCommands(SimpleRegValuePairs(pairs));
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to configure the card reader. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    pinfo("The card reader chip has been configured.");
    
    // Initialize the overcurrent protection
    retVal = this->initOvercurrentProtection();
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to initialize the overcurrent protection. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    pinfo("Overcurrent protection has been initialized.");
    
    // Enable the clock power management
    // Need to write this register if the chip is RTS5250/524A/525A/5260/5261/5228
    // TODO: Create a virtual function `enableClockPowerManagement()`
    pinfo("Enabling the clock power management...");
    
    retVal = this->writeChipRegister(rPMCLKFCTL, PMCLKFCTL::kEnableClockPM, PMCLKFCTL::kEnableClockPM);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to enable the clock power management. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // TODO: RELOCATE THIS
#define PCI_EXP_LNKCTL        16    /* Link Control */
#define  PCI_EXP_LNKCTL_ASPMC    0x0003    /* ASPM Control */
#define  PCI_EXP_LNKCTL_ASPM_L0S 0x0001    /* L0s Enable */
#define  PCI_EXP_LNKCTL_ASPM_L1  0x0002    /* L1 Enable */
#define  PCI_EXP_LNKCTL_RCB    0x0008    /* Read Completion Boundary */
#define  PCI_EXP_LNKCTL_LD    0x0010    /* Link Disable */
#define  PCI_EXP_LNKCTL_RL    0x0020    /* Retrain Link */
#define  PCI_EXP_LNKCTL_CCC    0x0040    /* Common Clock Configuration */
#define  PCI_EXP_LNKCTL_ES    0x0080    /* Extended Synch */
#define  PCI_EXP_LNKCTL_CLKREQ_EN 0x0100 /* Enable clkreq */
#define  PCI_EXP_LNKCTL_HAWD    0x0200    /* Hardware Autonomous Width Disable */
#define  PCI_EXP_LNKCTL_LBMIE    0x0400    /* Link Bandwidth Management Interrupt Enable */
#define  PCI_EXP_LNKCTL_LABIE    0x0800    /* Link Autonomous Bandwidth Interrupt Enable */
    
    IOPCIeDeviceConfigSet16(this->device, PCI_EXP_LNKCTL, PCI_EXP_LNKCTL_CLKREQ_EN);
    
    pinfo("Clock power management has been enabled.");
    
    // Enter L1 when the host is idle
    // TODO: CHECK THIS
    this->device->configWrite8(0x70F, 0x5B);
    
    // Perform device-specific initialization
    retVal = this->initHardwareExtra();
    
    if (retVal != kIOReturnSuccess)
    {
        perr("The device-specific initialization routine returns an error 0x%x.", retVal);
        
        return retVal;
    }
    
    // Configure the ASPM
    // TODO: CHECK THIS
    retVal = this->writeChipRegister(rAFCTL, AFCTL::kCTL0 | AFCTL::kCTL1, AFCTL::kCTL0 | AFCTL::kCTL1);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to set the ASPM::CTL0 and ASPM::CTL1. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    pinfo("Initialized the card reader...");
    
    return kIOReturnSuccess;
}

//
// MARK: - Startup Routines
//

///
/// Setup the power management
///
/// @return `true` on success, `false` otherwise.
///
bool RealtekPCIeCardReaderController::setupPowerManagement()
{
    static IOPMPowerState kPowerStates[] =
    {
        { kIOPMPowerStateVersion1, kIOPMPowerOff, kIOPMPowerOff, kIOPMPowerOff, 0, 0, 0, 0, 0, 0, 0, 0 },
        { kIOPMPowerStateVersion1, kIOPMPowerOn | kIOPMDeviceUsable | kIOPMInitialDeviceState, kIOPMPowerOn, kIOPMPowerOn, 0, 0, 0, 0, 0, 0, 0, 0 },
    };
    
    this->PMinit();
    
    this->device->joinPMtree(this);
    
    return this->registerPowerDriver(this, kPowerStates, arrsize(kPowerStates)) == kIOPMNoErr;
}

///
/// Map the device memory
///
/// @return `true` on success, `false` otherwise.
///
bool RealtekPCIeCardReaderController::mapDeviceMemory()
{
    UInt8 bar = this->getDeviceMemoryMapBaseAddressRegister();
    
    pinfo("Prepare to map the device memory (BAR = 0x%02x)...", bar);
    
    this->deviceMemoryMap = this->device->mapDeviceMemoryWithRegister(bar);
    
    if (this->deviceMemoryMap == nullptr)
    {
        perr("Failed to map the device memory.");
        
        return false;
    }
    
    // The descriptor is valid as long as the controller holds a reference to the device memory map
    this->deviceMemoryDescriptor = this->deviceMemoryMap->getMemoryDescriptor();
    
    if (this->deviceMemoryDescriptor->prepare() != kIOReturnSuccess)
    {
        perr("Failed to prepare the device memory.");
        
        this->deviceMemoryMap->release();
        
        this->deviceMemoryMap = nullptr;
        
        return false;
    }
    
    pinfo("The device memory has been mapped successfully.");
    
    return true;
}

///
/// Setup the workloop
///
/// @return `true` on success, `false` otherwise.
///
bool RealtekPCIeCardReaderController::setupWorkLoop()
{
    pinfo("Creating the workloop and the command gate...");
    
    this->workLoop = IOWorkLoop::workLoop();
    
    if (this->workLoop == nullptr)
    {
        perr("Failed to create the workloop.");
        
        goto error;
    }
    
    this->commandGate = IOCommandGate::commandGate(this);
    
    if (this->commandGate == nullptr)
    {
        perr("Failed to create the command gate.");
        
        goto error;
    }
    
    this->workLoop->addEventSource(this->commandGate);
    
    pinfo("The workloop and the command gate have been created.");
    
    return true;
    
error:
    OSSafeReleaseNULL(this->commandGate);
    
    OSSafeReleaseNULL(this->workLoop);
    
    pinfo("Failed to create the workloop and the command gate.");
    
    return false;
}

///
/// Probe the index of the message signaled interrupt
///
/// @return The index on success, `0` otherwise.
///
int RealtekPCIeCardReaderController::probeMSIIndex()
{
    int index = 0;
    
    int interruptType = 0;
    
    while (this->device->getInterruptType(index, &interruptType) == kIOReturnSuccess)
    {
        if (interruptType & kIOInterruptTypePCIMessaged)
        {
            pinfo("Found the MSI index = %d.", index);
            
            return index;
        }
        
        index += 1;
    }
    
    pwarning("Failed to probe the MSI index. Will use the default value 0 instead.");
    
    return 0;
}

///
/// Setup the interrupt management module
///
/// @return `true` on success, `false` otherwise.
///
bool RealtekPCIeCardReaderController::setupInterrupts()
{
    // Guard: Probe the MSI interrupt index
    int index = this->probeMSIIndex();
    
    pinfo("The MSI index = %d. Will set up the interrupt event source.", index);
    
    // Guard: Setup the interrupt event source
    auto filter = OSMemberFunctionCast(IOFilterInterruptEventSource::Filter, this, &RealtekPCIeCardReaderController::interruptEventSourceFilter);
    
    auto handler = OSMemberFunctionCast(IOFilterInterruptEventSource::Action, this, &RealtekPCIeCardReaderController::interruptHandlerGated);
    
    this->interruptEventSource = IOFilterInterruptEventSource::filterInterruptEventSource(this, handler, filter, this->device, index);
    
    if (this->interruptEventSource == nullptr)
    {
        perr("Failed to create the interrupt event source.");
        
        return false;
    }
    
    this->workLoop->addEventSource(this->interruptEventSource);
    
    this->interruptEventSource->enable();
    
    pinfo("Interrupt event source and its handler have been registerd.");
    
    return true;
}

///
/// Setup the host command and buffer management module
///
/// @return `true` on success, `false` otherwise.
///
bool RealtekPCIeCardReaderController::setupHostBuffer()
{
    // Used to generate the scatter/gather list for the DMA transaction
    pinfo("Creating the host command and data buffer...");
    
    UInt64 offset = 0;
    
    IODMACommand::Segment32 segment;
    
    UInt32 numSegments = 1;
    
    // Guard: 1. Allocate memory for the host command and data buffer
    this->hostBufferDescriptor = IOBufferMemoryDescriptor::withCapacity(4096, kIODirectionInOut, true);
    
    if (this->hostBufferDescriptor == nullptr)
    {
        perr("Failed to allocate the memory for the host buffer.");
        
        goto error1;
    }
    
    // Guard: 2. Setup the DMA command
    this->hostBufferDMACommand = IODMACommand::withSpecification(kIODMACommandOutputHost32, 32, 0, IODMACommand::kMapped, 0, 1);
    
    if (this->hostBufferDMACommand == nullptr)
    {
        perr("Failed to allocate the DMA command.");
        
        goto error2;
    }
    
    // Guard: 3. Page in and wire down the buffer
    if (this->hostBufferDescriptor->prepare() != kIOReturnSuccess)
    {
        perr("Failed to prepare the command buffer descriptor.");
        
        goto error3;
    }
    
    // Guard: 4. Associate the memory descriptor with the DMA command
    // Guard: 5. Prepare to start the DMA transaction (auto prepare is set to true)
    if (this->hostBufferDMACommand->setMemoryDescriptor(this->hostBufferDescriptor) != kIOReturnSuccess)
    {
        perr("Failed to associate the memory descriptor with the DMA command and start the transaction.");
        
        goto error5;
    }
    
    // Guard: 6. Generate the scatter/gather list for the DMA transaction
    // The buffer should fit into a single segment
    if (this->hostBufferDMACommand->gen32IOVMSegments(&offset, &segment, &numSegments) != kIOReturnSuccess)
    {
        perr("Failed to generate the segment for the host buffer.");
        
        goto error6;
    }
    
    // Guard: 7. Setup the timer for the DMA transaction
    this->hostBufferTimer = IOTimerEventSource::timerEventSource(this, OSMemberFunctionCast(IOTimerEventSource::Action, this, &RealtekPCIeCardReaderController::bufferTransferTimeoutHandlerGated));
    
    if (this->hostBufferTimer == nullptr)
    {
        perr("Failed to create a timer for the DMA transaction.");
        
        goto error7;
    }
    
    // Guard: 8. Register the timer event source
    if (this->workLoop->addEventSource(this->hostBufferTimer) != kIOReturnSuccess)
    {
        perr("Failed to register the timer event source to the work loop.");
        
        goto error8;
    }
    
    // All done: Save the address
    this->hostBufferAddress = segment.fIOVMAddr;
    
    this->hostCommandCount = 0;
    
    this->hostBufferTransferStatus = kIOReturnSuccess;
    
    pinfo("The host command and data buffer has been created. Bus address = 0x%08x.", segment.fIOVMAddr);
    
    return true;
    
    // Error handlers
error8:
    this->hostBufferTimer->release();
    
    this->hostBufferTimer = nullptr;
    
error7:
error6:
    this->hostBufferDMACommand->clearMemoryDescriptor();

error5:
    this->hostBufferDescriptor->complete();
    
error3:
    this->hostBufferDMACommand->release();
    
    this->hostBufferDMACommand = nullptr;
    
error2:
    this->hostBufferDescriptor->release();
    
    this->hostBufferDescriptor = nullptr;
    
error1:
    return false;
}

///
/// Setup the SD card reader
///
/// @return `true` on success, `false` otherwise.
/// @note Port: This function replaces `rtsx_pci_init_chip()` defined in `rtsx_pci.c`.
///
bool RealtekPCIeCardReaderController::setupCardReader()
{
    // Initialize the device-specific parameters
    pinfo("Initializing the device-specific parameters...");
    
    if (this->initParameters() != kIOReturnSuccess)
    {
        perr("Failed to initialize device-specific parameters.");
        
        return false;
    }
    
    pinfo("Device-specific parameters have been initialized.");
    
    // Fetch the vendor-specific parameters
    pinfo("Initializing the vendor-specific parameters...");
    
    if (this->initVendorSpecificParameters() != kIOReturnSuccess)
    {
        perr("Failed to initialize vendor-specific parameters. Will use the default values.");
    }
    
    pinfo("Vendor-specific parameters have been initialized.");
    
    // Initialize the card reader
    pinfo("Initializing the card reader...");
    
    if (this->initHardwareCommon() != kIOReturnSuccess)
    {
        perr("Failed to initialize the card reader.");
        
        return false;
    }
    
    pinfo("The card reader has been initialized.");
    
    return true;
}

///
/// Create the card slot and publish it
///
/// @return `true` on success, `false` otherwise.
///
bool RealtekPCIeCardReaderController::createCardSlot()
{
    pinfo("Creating the card slot...");
    
    RealtekSDXCSlot* slot = OSTypeAlloc(RealtekSDXCSlot);
    
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
/// Tear down the power management
///
void RealtekPCIeCardReaderController::tearDownPowerManagement()
{
    this->PMstop();
}

///
/// Tear down the interrupt event source
///
void RealtekPCIeCardReaderController::tearDownInterrupts()
{
    if (this->interruptEventSource != nullptr)
    {
        this->interruptEventSource->disable();
        
        this->workLoop->removeEventSource(this->interruptEventSource);
        
        this->interruptEventSource->release();
        
        this->interruptEventSource = nullptr;
    }
}

///
/// Tear down the workloop
///
void RealtekPCIeCardReaderController::tearDownWorkLoop()
{
    OSSafeReleaseNULL(this->commandGate);
    
    OSSafeReleaseNULL(this->workLoop);
}

///
/// Tear down the host command and buffer management module
///
void RealtekPCIeCardReaderController::tearDownHostBuffer()
{
    // R7: Cancel the timer for the DMA transaction
    // R8: Deregister the timer event source
    if (this->hostBufferTimer != nullptr)
    {
        this->hostBufferTimer->cancelTimeout();
        
        psoftassert(this->workLoop->removeEventSource(this->hostBufferTimer) == kIOReturnSuccess,
                    "Failed to remove the timer event source from the work loop,");
        
        this->hostBufferTimer->release();
        
        this->hostBufferTimer = nullptr;
    }
    
    // R5: Complete the DMA transaction (auto complete is set to true)
    // R4: Deassociate with the buffer descriptor
    // R2: Release the DMA command
    if (this->hostBufferDMACommand != nullptr)
    {
        psoftassert(this->hostBufferDMACommand->clearMemoryDescriptor() == kIOReturnSuccess,
                    "Failed to deassociate the buffer descriptor with the DMA command and stop the transaction.");
        
        this->hostBufferDMACommand->release();
        
        this->hostBufferDMACommand = nullptr;
    }
    
    // R3: Unwire and release the buffer
    if (this->hostBufferDescriptor != nullptr)
    {
        psoftassert(this->hostBufferDescriptor->complete() == kIOReturnSuccess,
                    "Failed to complete the command buffer descriptor.");
        
        this->hostBufferDescriptor->release();
        
        this->hostBufferDescriptor = nullptr;
    }
}

///
/// Unmap the device memory
///
void RealtekPCIeCardReaderController::unmapDeviceMemory()
{
    this->deviceMemoryDescriptor->complete();
    
    this->deviceMemoryDescriptor = nullptr;
    
    OSSafeReleaseNULL(this->deviceMemoryMap);
}

///
/// Destroy the card slot
///
void RealtekPCIeCardReaderController::destroyCardSlot()
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
/// Start the controller
///
/// @param provider An instance of PCI device that represents the card reader
/// @return `true` on success, `false` otherwise.
///
bool RealtekPCIeCardReaderController::start(IOService* provider)
{
    pinfo("===================================================");
    pinfo("Starting the Realtek PCIe card reader controller...");
    pinfo("===================================================");
    
    // Start the super class
    if (!super::start(provider))
    {
        perr("Failed to start the super class.");
        
        return false;
    }
    
    // Set up the controller device
    this->device = OSDynamicCast(IOPCIDevice, provider);
    
    if (this->device == nullptr)
    {
        perr("The provider is not an valid PCI device.");
        
        return false;
    }
    
    this->device->retain();
    
    this->device->setBusMasterEnable(true);
    
    this->device->setMemoryEnable(true);
    
    // Setup the power management
    if (!this->setupPowerManagement())
    {
        perr("Failed to setup the power management.");
        
        goto error1;
    }
    
    // Set up the memory map
    if (!this->mapDeviceMemory())
    {
        perr("Failed to map the device memory.");
        
        goto error1;
    }
    
    // Set up the work loop
    if (!this->setupWorkLoop())
    {
        perr("Failed to set up the work loop.");
        
        goto error2;
    }
    
    // Set up the hardware interrupt
    if (!this->setupInterrupts())
    {
        perr("Failed to set up the hardware interrupt.");
        
        goto error3;
    }
    
    // Set up the host buffer
    if (!this->setupHostBuffer())
    {
        perr("Failed to set up the host buffer.");
        
        goto error4;
    }
    
    // Fetch the current ASPM state
    this->parameters.pm.isASPMEnabled = this->isASPMEnabled();
    
    pinfo("ASPM Enabled: %s.", YESNO(this->parameters.pm.isASPMEnabled));
    
    // Set up the card reader
    if (!this->setupCardReader())
    {
        perr("Failed to set up the card reader.");
        
        goto error5;
    }
    
    // Create the card slot
    if (!this->createCardSlot())
    {
        perr("Failed to create the card slot.");
        
        goto error5;
    }
    
    // If the card is already inserted when the driver starts,
    // there will be no card interrupt, so we check whether the card exists here.
    if (this->isCardPresent())
    {
        pinfo("Detected a card when the driver starts. Will notify the host device.");
        
        // Notify the host device
        this->onSDCardInsertedGated();
    }
    else
    {
        pinfo("The card is not present when the driver starts.");
    }
    
    //this->registerService();
    
    this->isIdle = true;
    
    pinfo("ASPM Enabled: %s.", YESNO(this->isASPMEnabled()));
    
    pinfo("================================================");
    pinfo("The card reader controller started successfully.");
    pinfo("================================================");
    
    return true;
    
error5:
    this->tearDownHostBuffer();
    
error4:
    this->tearDownInterrupts();
    
error3:
    this->tearDownWorkLoop();
    
error2:
    this->unmapDeviceMemory();
    
error1:
    this->tearDownPowerManagement();
    
    this->device->setMemoryEnable(false);
    
    this->device->setBusMasterEnable(false);
    
    OSSafeReleaseNULL(this->device);
    
    pinfo("===========================================");
    pinfo("Failed to start the card reader controller.");
    pinfo("===========================================");
    
    return false;
}

///
/// Stop the controller
///
/// @param provider An instance of PCI device that represents the card reader
///
void RealtekPCIeCardReaderController::stop(IOService* provider)
{
    this->tearDownPowerManagement();
    
    this->destroyCardSlot();
    
    this->tearDownHostBuffer();
    
    this->tearDownInterrupts();
    
    this->tearDownWorkLoop();
    
    this->unmapDeviceMemory();
    
    this->device->setMemoryEnable(false);
    
    this->device->setBusMasterEnable(false);
    
    OSSafeReleaseNULL(this->device);
    
    super::stop(provider);
}
