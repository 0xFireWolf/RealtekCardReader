//
//  RealtekPCICardReaderController.cpp
//  RealtekCardReader
//
//  Created by FireWolf on 2/18/21.
//

#include "RealtekPCICardReaderController.hpp"
#include "RealtekPCISDXCSlot.hpp"
#include "BitOptions.hpp"
#include "IOPCIeDevice.hpp"

//
// MARK: - Meta Class Definitions
//

OSDefineMetaClassAndAbstractStructors(RealtekPCICardReaderController, RealtekCardReaderController);

//
// MARK: - Constants: Bus Timing Tables
//

/// A sequence of chip registers to switch the bus speed mode to UHS-I SDR50/SDR104
const RealtekPCICardReaderController::ChipRegValuePair RealtekPCICardReaderController::kBusTimingTablePairsSDR50[] =
{
    {
        RTSX::PCR::Chip::SD::rCFG1,
        RTSX::PCR::Chip::SD::CFG1::kModeMask | RTSX::PCR::Chip::SD::CFG1::kAsyncFIFONotRST,
        RTSX::PCR::Chip::SD::CFG1::kModeSD30 | RTSX::PCR::Chip::SD::CFG1::kAsyncFIFONotRST
    },
    {
        RTSX::PCR::Chip::CLK::rCTL,
        RTSX::PCR::Chip::CLK::CTL::kLowFrequency,
        RTSX::PCR::Chip::CLK::CTL::kLowFrequency
    },
    {
        RTSX::PCR::Chip::CARD::rCLKSRC,
        0xFF,
        RTSX::PCR::Chip::CARD::CLKSRC::kCRCVarClock0 | RTSX::PCR::Chip::CARD::CLKSRC::kSD30FixClock | RTSX::PCR::Chip::CARD::CLKSRC::kSampleVarClock1
    },
    {
        RTSX::PCR::Chip::CLK::rCTL,
        RTSX::PCR::Chip::CLK::CTL::kLowFrequency,
        0
    },
};

const RealtekPCICardReaderController::SimpleRegValuePairs RealtekPCICardReaderController::kBusTimingTableSDR50 =
{
    RealtekPCICardReaderController::kBusTimingTablePairsSDR50
};

/// A sequence of chip registers to switch the bus speed mode to UHS-I DDR50
const RealtekPCICardReaderController::ChipRegValuePair RealtekPCICardReaderController::kBusTimingTablePairsDDR50[] =
{
    {
        RTSX::PCR::Chip::SD::rCFG1,
        RTSX::PCR::Chip::SD::CFG1::kModeMask  | RTSX::PCR::Chip::SD::CFG1::kAsyncFIFONotRST,
        RTSX::PCR::Chip::SD::CFG1::kModeSDDDR | RTSX::PCR::Chip::SD::CFG1::kAsyncFIFONotRST
    },
    {
        RTSX::PCR::Chip::CLK::rCTL,
        RTSX::PCR::Chip::CLK::CTL::kLowFrequency,
        RTSX::PCR::Chip::CLK::CTL::kLowFrequency
    },
    {
        RTSX::PCR::Chip::CARD::rCLKSRC,
        0xFF,
        RTSX::PCR::Chip::CARD::CLKSRC::kCRCVarClock0 | RTSX::PCR::Chip::CARD::CLKSRC::kSD30FixClock | RTSX::PCR::Chip::CARD::CLKSRC::kSampleVarClock1
    },
    {
        RTSX::PCR::Chip::CLK::rCTL,
        RTSX::PCR::Chip::CLK::CTL::kLowFrequency,
        0x00
    },
    {
        RTSX::PCR::Chip::SD::rPPCTL,
        RTSX::PCR::Chip::SD::PPCTL::kDDRVarTxCommandData,
        RTSX::PCR::Chip::SD::PPCTL::kDDRVarTxCommandData },
    {
        RTSX::PCR::Chip::SD::rSPCTL,
        RTSX::PCR::Chip::SD::SPCTL::kDDRVarRxData | RTSX::PCR::Chip::SD::SPCTL::kDDRVarRxCommand,
        RTSX::PCR::Chip::SD::SPCTL::kDDRVarRxData | RTSX::PCR::Chip::SD::SPCTL::kDDRVarRxCommand
    },
};

const RealtekPCICardReaderController::SimpleRegValuePairs RealtekPCICardReaderController::kBusTimingTableDDR50 =
{
    RealtekPCICardReaderController::kBusTimingTablePairsDDR50
};

/// A sequence of chip registers to switch the bus speed mode to High Speed
const RealtekPCICardReaderController::ChipRegValuePair RealtekPCICardReaderController::kBusTimingTablePairsHighSpeed[] =
{
    {
        RTSX::PCR::Chip::SD::rCFG1,
        RTSX::PCR::Chip::SD::CFG1::kModeMask,
        RTSX::PCR::Chip::SD::CFG1::kModeSD20
    },
    {
        RTSX::PCR::Chip::CLK::rCTL,
        RTSX::PCR::Chip::CLK::CTL::kLowFrequency,
        RTSX::PCR::Chip::CLK::CTL::kLowFrequency
    },
    {
        RTSX::PCR::Chip::CARD::rCLKSRC,
        0xFF,
        RTSX::PCR::Chip::CARD::CLKSRC::kCRCFixClock | RTSX::PCR::Chip::CARD::CLKSRC::kSD30VarClock0 | RTSX::PCR::Chip::CARD::CLKSRC::kSampleVarClock1
    },
    {
        RTSX::PCR::Chip::CLK::rCTL,
        RTSX::PCR::Chip::CLK::CTL::kLowFrequency,
        0x00
    },
    {
        RTSX::PCR::Chip::SD::rPPCTL,
        RTSX::PCR::Chip::SD::PPCTL::kSD20TxSelMask,
        RTSX::PCR::Chip::SD::PPCTL::kSD20Tx14Ahead
    },
    {
        RTSX::PCR::Chip::SD::rSPCTL,
        RTSX::PCR::Chip::SD::SPCTL::kSD20RxSelMask,
        RTSX::PCR::Chip::SD::SPCTL::kSD20Rx14Delay
    },
};

const RealtekPCICardReaderController::SimpleRegValuePairs RealtekPCICardReaderController::kBusTimingTableHighSpeed =
{
    RealtekPCICardReaderController::kBusTimingTablePairsHighSpeed
};

/// A sequence of chip registers to switch the bus speed mode to Default Speed
const RealtekPCICardReaderController::ChipRegValuePair RealtekPCICardReaderController::kBusTimingTablePairsDefaultSpeed[] =
{
    {
        RTSX::PCR::Chip::SD::rCFG1,
        RTSX::PCR::Chip::SD::CFG1::kModeMask,
        RTSX::PCR::Chip::SD::CFG1::kModeSD20
    },
    {
        RTSX::PCR::Chip::CLK::rCTL,
        RTSX::PCR::Chip::CLK::CTL::kLowFrequency,
        RTSX::PCR::Chip::CLK::CTL::kLowFrequency
    },
    {
        RTSX::PCR::Chip::CARD::rCLKSRC,
        0xFF,
        RTSX::PCR::Chip::CARD::CLKSRC::kCRCFixClock | RTSX::PCR::Chip::CARD::CLKSRC::kSD30VarClock0 | RTSX::PCR::Chip::CARD::CLKSRC::kSampleVarClock1
    },
    {
        RTSX::PCR::Chip::CLK::rCTL,
        RTSX::PCR::Chip::CLK::CTL::kLowFrequency,
        0x00
    },
    {
        RTSX::PCR::Chip::SD::rPPCTL,
        0xFF, 0
    },
    {
        RTSX::PCR::Chip::SD::rSPCTL,
        RTSX::PCR::Chip::SD::SPCTL::kSD20RxSelMask,
        RTSX::PCR::Chip::SD::SPCTL::kSD20RxPosEdge
    },
};

const RealtekPCICardReaderController::SimpleRegValuePairs RealtekPCICardReaderController::kBusTimingTableDefaultSpeed =
{
    RealtekPCICardReaderController::kBusTimingTablePairsDefaultSpeed
};

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
UInt8 RealtekPCICardReaderController::readRegister8(UInt32 address)
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
UInt16 RealtekPCICardReaderController::readRegister16(UInt32 address)
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
UInt32 RealtekPCICardReaderController::readRegister32(UInt32 address)
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
void RealtekPCICardReaderController::writeRegister8(UInt32 address, UInt8 value)
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
void RealtekPCICardReaderController::writeRegister16(UInt32 address, UInt16 value)
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
void RealtekPCICardReaderController::writeRegister32(UInt32 address, UInt32 value)
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
IOReturn RealtekPCICardReaderController::readChipRegister(UInt16 address, UInt8& value)
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
IOReturn RealtekPCICardReaderController::writeChipRegister(UInt16 address, UInt8 mask, UInt8 value)
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
IOReturn RealtekPCICardReaderController::writeChipRegisters(const ChipRegValuePairs& pairs)
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
IOReturn RealtekPCICardReaderController::readPhysRegister(UInt8 address, UInt16& value)
{
    // The driver will program chip registers to access PHY registers
    using namespace RTSX::PCR::Chip;
    
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
IOReturn RealtekPCICardReaderController::writePhysRegister(UInt8 address, UInt16 value)
{
    // The driver will program chip registers to access PHY registers
    using namespace RTSX::PCR::Chip;
    
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
IOReturn RealtekPCICardReaderController::writePhysRegisters(const PhysRegValuePair* pairs, size_t count)
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
IOReturn RealtekPCICardReaderController::modifyPhysRegister(UInt8 address, PhysRegValueModifier modifier, const void* context)
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
IOReturn RealtekPCICardReaderController::setPhysRegisterBitAtIndex(UInt8 address, UInt32 index)
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
IOReturn RealtekPCICardReaderController::clearPhysRegisterBitAtIndex(UInt8 address, UInt32 index)
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
IOReturn RealtekPCICardReaderController::appendPhysRegister(UInt8 address, UInt16 mask, UInt16 append)
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
IOReturn RealtekPCICardReaderController::appendPhysRegisters(const PhysRegMaskValuePair* pairs, size_t count)
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
/// Read from the host buffer into the given buffer
///
/// @param offset A byte offset into the host buffer
/// @param buffer A non-null buffer
/// @param length The number of bytes to read
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note This function runs in a gated context.
///
IOReturn RealtekPCICardReaderController::readHostBufferGated(IOByteCount offset, void* buffer, IOByteCount length)
{
    // The non-gated version guarantees that the offset and the length are valid
    passert(this->hostBufferDMACommand->readBytes(offset, buffer, length) == length,
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
IOReturn RealtekPCICardReaderController::writeHostBufferGated(IOByteCount offset, const void* buffer, IOByteCount length)
{
    // The non-gated version guarantees that the offset and the length are valid
    passert(this->hostBufferDMACommand->writeBytes(offset, buffer, length) == length,
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
/// @note Port: This function replaces `rtsx_pci_add_cmd()` defined in `rtsx_psr.c`.
/// @note This function runs in a gated context.
///
IOReturn RealtekPCICardReaderController::enqueueCommandGated(const Command& command)
{
    // Guard: Ensure that the queue is not full
    if (this->hostCommandCounter.total >= RTSX::MMIO::HCBAR::kMaxNumCmds)
    {
        pwarning("The host command buffer queue is full.");
        
        return kIOReturnBusy;
    }
    
    // Retrieve and write the command value
    UInt64 offset = this->hostCommandCounter.total * sizeof(Command);
    
    UInt32 value = OSSwapHostToLittleInt32(command.getValue());
    
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
/// @note Port: This function replaces `rtsx_pci_send_cmd()` defined in `rtsx_pcr.c`.
/// @note This function sends all commands in the queue to the device.
/// @note This function runs in a gated context.
///
IOReturn RealtekPCICardReaderController::endCommandTransferGated(UInt32 timeout, UInt32 flags)
{
    // Tell the device the location of the command buffer and to start executing commands
    using namespace RTSX::MMIO;
    
    this->hostBufferTransferStatus = kIOReturnNotReady;
    
    this->writeRegister32(rHCBAR, this->hostBufferAddress);
    
    this->writeRegister32(rHCBCTLR, HCBCTLR::RegValueForStartCommand(this->hostCommandCounter.total));
    
    // Set up the timer
    passert(this->hostBufferTimer != nullptr, "The host buffer timer should not be NULL.");
    
    passert(this->hostBufferTimer->setTimeoutMS(timeout) == kIOReturnSuccess, "Should be able to set the timeout.");
    
    // Wait for the transfer result
    // Block the current thread and release the gate
    // Either the timeout handler or the interrupt handler will modify the status and wakeup the current thread
    this->commandGate->commandSleep(&this->hostBufferTransferStatus);
    
    // When the sleep function returns, the transfer is done
    return this->hostBufferTransferStatus;
}

///
/// Finish the existing host command transfer session without waiting for the completion interrupt
///
/// @note Port: This function replaces `rtsx_pci_send_cmd_no_wait()` defined in `rtsx_psr.c`.
/// @note This function sends all commands in the queue to the device.
///
void RealtekPCICardReaderController::endCommandTransferNoWait()
{
    // The transfer routine will run in a gated context
    auto action = [](OSObject* controller, void* timeout, void*, void*, void*) -> IOReturn
    {
        // Retrieve the controller instance
        auto instance = OSDynamicCast(RealtekPCICardReaderController, controller);
        
        passert(instance != nullptr, "The controller instance is invalid.");
        
        // Tell the device the location of the command buffer and to start executing commands
        using namespace RTSX::MMIO;
        
        instance->hostBufferTransferStatus = kIOReturnNotReady;
        
        instance->writeRegister32(rHCBAR, instance->hostBufferAddress);
        
        instance->writeRegister32(rHCBCTLR, HCBCTLR::RegValueForStartCommand(instance->hostCommandCounter.total));
        
        return kIOReturnSuccess;
    };
    
    this->commandGate->runAction(action);
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
UInt64 RealtekPCICardReaderController::transformIOVMSegment(IODMACommand::Segment32 segment)
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
IOReturn RealtekPCICardReaderController::enqueueDMACommand(IODMACommand* command)
{
    using namespace RTSX::MMIO;
    
    //
    // Step 1: Generate a physical scather/gather list
    //
    pinfo("Generating a scather/gather list from the given DMA command...");
    
    UInt64 offset = 0;
    
    IODMACommand::Segment32 segments[kMaxNumSegments] = {};
    
    UInt32 numSegments = kMaxNumSegments;
    
    IOReturn retVal = command->gen32IOVMSegments(&offset, segments, &numSegments);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to generate a scather/gather list from the given DMA command. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // The maximum number of bytes in a single DMA transaction is 524288,
    // and the kernel finds free pages available for the DMA transfer.
    // so the maximum number of segments is 524288 / 4096 = 128.
    // As a result, it should be sufficient to call `gen32IOVMSegments()` once,
    // and the returned offset should be identical to the length of the memory descriptor.
    psoftassert(command->getMemoryDescriptor()->getLength() == offset, "Detected Inconsistency: Offset != DMA Buffer Length.");
    
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
IOReturn RealtekPCICardReaderController::performDMATransfer(IODMACommand* command, UInt32 timeout, UInt32 control)
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
        auto instance = OSDynamicCast(RealtekPCICardReaderController, controller);
        
        passert(instance != nullptr, "The controller instance is invalid.");
        
        // Tell the device the location of the data buffer and to start the DMA transfer
        using namespace RTSX::MMIO;
        
        instance->hostBufferTransferStatus = kIOReturnNotReady;
        
        instance->writeRegister32(rHDBAR, instance->hostBufferAddress + RealtekPCICardReaderController::kHostDatabufferOffset);
        
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
        
        this->clearHostError();
        
        if (this->dmaErrorCounter < kMaxDMATransferFailures)
        {
            this->dmaErrorCounter += 1;
        }
        
        pinfo("DMA Error Counter = %u.", this->dmaErrorCounter);
        
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
IOReturn RealtekPCICardReaderController::performDMARead(IODMACommand* command, UInt32 timeout)
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
IOReturn RealtekPCICardReaderController::performDMAWrite(IODMACommand* command, UInt32 timeout)
{
    using namespace RTSX::MMIO;
    
    pinfo("The host device requests a DMA write operation.");
    
    return this->performDMATransfer(command, timeout, HDBCTLR::kStartDMA | HDBCTLR::kUseADMA);
}

//
// MARK: - Clear Error
//

///
/// Clear any transfer error on the card side
///
/// @note Port: This function replaces the code block that stops the card and clears the card errorin `sd_clear_error()` defined in `rtsx_pci_sdmmc.c`.
///
void RealtekPCICardReaderController::clearCardError()
{
    using namespace RTSX::PCR::Chip::CARD;
    
    psoftassert(this->writeChipRegister(rSTOP, STOP::kStopSD | STOP::kClearSDError, STOP::kStopSD | STOP::kClearSDError) == kIOReturnSuccess,
                "Failed to clear the card error.");
}

///
/// Clear any transfer error on the host side
///
/// @note This function is invoked when a command or data transfer has failed.
/// @note Port: This function replaces `rtsx_pci_stop_cmd()` defined in `rtsx_psr.c`.
///             RTS5228, RTS5260 and RTS5261 controllers must override this function.
///
void RealtekPCICardReaderController::clearHostError()
{
    using namespace RTSX::MMIO;
    
    this->writeRegister32(rHCBCTLR, HCBCTLR::kStopCommand);
    
    this->writeRegister32(rHDBCTLR, HDBCTLR::kStopDMA);
    
    using namespace RTSX::PCR::Chip;
    
    const ChipRegValuePair pairs[] =
    {
        // Reset DMA
        { DMA::rCTL, DMA::CTL::kResetMask, DMA::CTL::kResetValue },
        
        // Flush the ring buffer
        { RBUF::rCTL, RBUF::CTL::kFlushMask, RBUF::CTL::kFlushValue }
    };
    
    psoftassert(this->writeChipRegisters(SimpleRegValuePairs(pairs)) == kIOReturnSuccess,
                "Failed to stop clear the host error.");
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
IOReturn RealtekPCICardReaderController::turnOnLED()
{
    using namespace RTSX::PCR::Chip::GPIO;
    
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
IOReturn RealtekPCICardReaderController::turnOffLED()
{
    using namespace RTSX::PCR::Chip::GPIO;
    
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
IOReturn RealtekPCICardReaderController::enableLEDBlinking()
{
    using namespace RTSX::PCR::Chip::OLT_LED;
    
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
IOReturn RealtekPCICardReaderController::disableLEDBlinking()
{
    using namespace RTSX::PCR::Chip::OLT_LED;
    
    return this->writeChipRegister(rCTL, CTL::kBlinkingMask, CTL::kDisableBlinkingValue);
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
IOReturn RealtekPCICardReaderController::selectCard()
{
    using namespace RTSX::PCR::Chip;
    
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
IOReturn RealtekPCICardReaderController::selectCardDataSource(bool ppbuf)
{
    using namespace RTSX::PCR::Chip;
    
    return this->enqueueWriteRegisterCommand(CARD::rDATASRC, CARD::DATASRC::kMask, ppbuf ? CARD::DATASRC::kPingPongBuffer : CARD::DATASRC::kRingBuffer);
}

///
/// Configure the card share mode
///
/// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full, `kIOReturnError` otherwise.
/// @note This function invokes `enqueueWriteRegisterCommand()` thus must be invoked between `beginCommandTransfer()` and `endCommandTransfer()`.
/// @note The caller may use `withCustomCommandTransfer()` to combine this operation with other ones.
///
IOReturn RealtekPCICardReaderController::configureCardShareMode()
{
    using namespace RTSX::PCR::Chip;
    
    return this->enqueueWriteRegisterCommand(CARD::rSHAREMODE, CARD::SHAREMODE::kMask, CARD::SHAREMODE::k48SD);
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
IOReturn RealtekPCICardReaderController::setupCardDMATransferProperties(UInt32 length, IODirection direction)
{
    using namespace RTSX::PCR::Chip;
    
    UInt8 regVal = direction == kIODirectionIn ? DMA::CTL::kDirectionFromCard : DMA::CTL::kDirectionToCard;
    
    const ChipRegValuePair pairs[] =
    {
        // Generate an interrupt when the DMA transfer is done
        { rIRQSTAT0, IRQSTAT0::kDMADoneInt, IRQSTAT0::kDMADoneInt },
        
        // Set up the data length
        { DMA::rC3, 0xFF, static_cast<UInt8>(length >> 24) },
        { DMA::rC2, 0xFF, static_cast<UInt8>(length >> 16) },
        { DMA::rC1, 0xFF, static_cast<UInt8>(length >>  8) },
        { DMA::rC0, 0xFF, static_cast<UInt8>(length & 0xFF) },
        
        // Set up the direction and the pack size
        {
            DMA::rCTL,
            DMA::CTL::kEnable | DMA::CTL::kDirectionMask | DMA::CTL::kPackSizeMask,
            static_cast<UInt8>(DMA::CTL::kEnable | regVal | DMA::CTL::kPackSize512)
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
IOReturn RealtekPCICardReaderController::enableCardClock()
{
    using namespace RTSX::PCR::Chip;
    
    return this->enqueueWriteRegisterCommand(CARD::rCLK, CARD::CLK::kEnableSD, CARD::CLK::kEnableSD);
}

///
/// Disable the card clock
///
/// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full, `kIOReturnError` otherwise.
/// @note This function invokes `enqueueWriteRegisterCommand()` thus must be invoked between `beginCommandTransfer()` and `endCommandTransfer()`.
/// @note The caller may use `withCustomCommandTransfer()` to combine this operation with other ones.
///
IOReturn RealtekPCICardReaderController::disableCardClock()
{
    using namespace RTSX::PCR::Chip;
    
    return this->enqueueWriteRegisterCommand(CARD::rCLK, CARD::CLK::kEnableSD, CARD::CLK::kDisable);
}

///
/// Enable the card output
///
/// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full, `kIOReturnError` otherwise.
/// @note This function invokes `enqueueWriteRegisterCommand()` thus must be invoked between `beginCommandTransfer()` and `endCommandTransfer()`.
/// @note The caller may use `withCustomCommandTransfer()` to combine this operation with other ones.
///
IOReturn RealtekPCICardReaderController::enableCardOutput()
{
    using namespace RTSX::PCR::Chip;
    
    return this->enqueueWriteRegisterCommand(CARD::rOUTPUT, CARD::OUTPUT::kSDMask, CARD::OUTPUT::kEnableSDValue);
}

///
/// Disable the card output
///
/// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full, `kIOReturnError` otherwise.
/// @note This function invokes `enqueueWriteRegisterCommand()` thus must be invoked between `beginCommandTransfer()` and `endCommandTransfer()`.
/// @note The caller may use `withCustomCommandTransfer()` to combine this operation with other ones.
///
IOReturn RealtekPCICardReaderController::disableCardOutput()
{
    using namespace RTSX::PCR::Chip;
    
    return this->enqueueWriteRegisterCommand(CARD::rOUTPUT, CARD::OUTPUT::kSDMask, CARD::OUTPUT::kDisableSDValue);
}

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
IOReturn RealtekPCICardReaderController::setDrivingForOutputVoltage(OutputVoltage outputVoltage, bool intermediate, UInt32 timeout)
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
    using namespace RTSX::PCR::Chip::CARD::SD30::DRVSEL;
    
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
// MARK: - Card Clock Configurations
//

///
/// Calculate the number of MCUs from the given SSC clock
///
/// @param clock The SSC clock in MHz
/// @return The MCU count.
/// @note Port: This function replaces the code block that calculates the MCU count in `rtsx_pci_switch_clock()` defined in `rtsx_psr.c`.
/// @note Concrete controllers must ensure that the returned MCU count is less than or equal to 15.
///
UInt32 RealtekPCICardReaderController::sscClock2MCUCount(UInt32 clock)
{
    return min(125 / clock + 3, 15);
}

///
/// Convert the given SSC depth to the actual register value
///
/// @param depth The SSC depth
/// @return The register value.
/// @note Port: RTS5261 and RTS5228 controllers must override this function.
///
UInt8 RealtekPCICardReaderController::sscDepth2RegValue(SSCDepth depth)
{
    using namespace RTSX::PCR::Chip;
    
    switch (depth)
    {
        case SSCDepth::k4M:
        {
            return SSC::CTL2::kDepth4M;
        }
            
        case SSCDepth::k2M:
        {
            return SSC::CTL2::kDepth2M;
        }
            
        case SSCDepth::k1M:
        {
            return SSC::CTL2::kDepth1M;
        }
            
        case SSCDepth::k500K:
        {
            return SSC::CTL2::kDepth500K;
        }
            
        case SSCDepth::k250K:
        {
            return SSC::CTL2::kDepth250K;
        }
            
        default:
        {
            pfatal("The given SSC depth %u is unsupported by the PCIe card reader.", depth);
        }
    }
}

///
/// Revise the given SSC depth register value
///
/// @param depth The SSC depth register value
/// @param divider The SSC clock divider value
/// @return The revised SSC depth register value.
/// @note Port: RTS5261 and RTS5228 controllers must override this function.
///
UInt8 RealtekPCICardReaderController::reviseSSCDepthRegValue(UInt8 depth, UInt8 divider)
{
    using namespace RTSX::PCR::Chip;
    
    if (divider > CLK::DIV::k1)
    {
        if (depth > (divider - 1))
        {
            depth -= (divider - 1);
        }
        else
        {
            depth = SSC::CTL2::kDepth4M;
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
IOReturn RealtekPCICardReaderController::switchCardClock(UInt8 depth, UInt8 n, UInt8 divider, UInt8 mcus, bool vpclock)
{
    using namespace RTSX::PCR::Chip;
    
    pinfo("Switching the card clock with SSC depth = %u, N = %u, Divider = %u, MCU Count = %u, Use VPClock = %s.",
          depth, n, divider, mcus, YESNO(vpclock));
    
    // Host commands to change the card clock
    const ChipRegValuePair pairs[] =
    {
        { CLK::rCTL, CLK::CTL::kLowFrequency, CLK::CTL::kLowFrequency },
        { CLK::rDIV, 0xFF, static_cast<UInt8>(divider << 4 | mcus) },
        { SSC::rCTL1, SSC::CTL1::kRSTB, 0 },
        { SSC::rCTL2, SSC::CTL2::kDepthMask, depth },
        { SSC::rDIVN0, 0xFF, n },
        { SSC::rCTL1, SSC::CTL1::kRSTB, SSC::CTL1::kRSTB },
        
        // Send the following commands if vpclock is true
        { SD::rVPCLK0CTL, SD::VPCTL::kPhaseNotReset, 0 },
        { SD::rVPCLK0CTL, SD::VPCTL::kPhaseNotReset, SD::VPCTL::kPhaseNotReset }
    };
    
    IOReturn retVal = kIOReturnSuccess;
    
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
    IODelay(RealtekPCICardReaderController::kWaitStableSSCClock);
    
    return this->writeChipRegister(CLK::rCTL, CLK::CTL::kLowFrequency, 0);
}

//
// MARK: - Card Detection and Write Protection
//

///
/// Check whether the card has write protection enabled
///
/// @return `true` if the card is write protected, `false` otherwise.
///
bool RealtekPCICardReaderController::isCardWriteProtected()
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
bool RealtekPCICardReaderController::isCardPresent()
{
    using namespace RTSX::MMIO;
    
    return BitOptions(this->readRegister32(rBIPR)).contains(BIPR::kSDExists);
}

///
/// Check whether the command line of the card is busy
///
/// @return `true` if the card CMD line is busy, `false` otherwise.
/// @warning This function returns `true` if failed to read the register.
///
bool RealtekPCICardReaderController::isCardCommandLineBusy()
{
    using namespace RTSX::PCR::Chip::SD;
    
    UInt8 status = 0;
    
    if (this->readChipRegister(rCMDSTATE, status) != kIOReturnSuccess)
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
bool RealtekPCICardReaderController::isCardDataLineBusy()
{
    using namespace RTSX::PCR::Chip::SD;
    
    UInt8 status = 0;
    
    if (this->readChipRegister(rDATSTATE, status) != kIOReturnSuccess)
    {
        perr("Failed to read the command state register. Will assume that the data lines are busy.");
        
        return true;
    }
    
    return !BitOptions(status).contains(DATSTATE::kIdle);
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
IOReturn RealtekPCICardReaderController::initOvercurrentProtection()
{
    pinfo("Initializing the overcurrent protection...");
    
    if (!this->parameters.ocp.enable)
    {
        pinfo("The device specifies not to enable overcurrent protection.");
        
        return kIOReturnSuccess;
    }
    
    // Setup overcurrent protection registers
    using namespace RTSX::PCR::Chip;
    
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
IOReturn RealtekPCICardReaderController::enableOvercurrentProtection()
{
    using namespace RTSX::PCR::Chip;
    
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
IOReturn RealtekPCICardReaderController::disableOvercurrentProtection()
{
    using namespace RTSX::PCR::Chip;
    
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
IOReturn RealtekPCICardReaderController::getOvercurrentProtectionStatus(UInt8& status)
{
    using namespace RTSX::PCR::Chip;
    
    pinfo("Fetching the current overcurrent protection status...");
    
    return this->readChipRegister(OCP::rSTAT, status);
}

///
/// Clear the overcurrent protection status
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_pci_clear_ocpstat()` defined in `rtsx_psr.c`.
///
IOReturn RealtekPCICardReaderController::clearOvercurrentProtectionStatus()
{
    using namespace RTSX::PCR::Chip;
    
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
IOReturn RealtekPCICardReaderController::enablePullControlForSDCard()
{
    return this->transferWriteRegisterCommands(*this->parameters.sdEnablePullControlTable);
}

///
/// Disable pull control for the SD card
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_pci_card_pull_ctl_disable()` defined in `rtsx_psr.c`.
///
IOReturn RealtekPCICardReaderController::disablePullControlForSDCard()
{
    return this->transferWriteRegisterCommands(*this->parameters.sdDisablePullControlTable);
}

//
// MARK: - Card Tuning & Phase Management
//

///
/// Change the Rx phase
///
/// @param samplePoint The sample point value
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces the Rx portion of `sd_change_phase()` defined in `rtsx_pci_sdmmc.c`.
///
IOReturn RealtekPCICardReaderController::changeRxPhase(UInt8 samplePoint)
{
    using namespace RTSX::PCR::Chip;
    
    const ChipRegValuePair pairs[] =
    {
        { CLK::rCTL, CLK::CTL::kChangeClock, CLK::CTL::kChangeClock },
        { SD::rVPRXCTL, SD::VPCTL::kPhaseSelectMask, samplePoint },
        { SD::rVPRXCTL, SD::VPCTL::kPhaseNotReset, 0 },
        { SD::rVPRXCTL, SD::VPCTL::kPhaseNotReset, SD::VPCTL::kPhaseNotReset },
        { CLK::rCTL, CLK::CTL::kChangeClock, 0 },
        { SD::rCFG1, SD::CFG1::kAsyncFIFONotRST, 0 }
    };
    
    return this->transferWriteRegisterCommands(SimpleRegValuePairs(pairs));
}

///
/// Change the Tx phase
///
/// @param samplePoint The sample point value
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces the Tx portion of `sd_change_phase()` defined in `rtsx_pci_sdmmc.c`.
///
IOReturn RealtekPCICardReaderController::changeTxPhase(UInt8 samplePoint)
{
    using namespace RTSX::PCR::Chip;
    
    const ChipRegValuePair pairs[] =
    {
        { CLK::rCTL, CLK::CTL::kChangeClock, CLK::CTL::kChangeClock },
        { SD::rVPTXCTL, SD::VPCTL::kPhaseSelectMask, samplePoint },
        { SD::rVPTXCTL, SD::VPCTL::kPhaseNotReset, 0 },
        { SD::rVPTXCTL, SD::VPCTL::kPhaseNotReset, SD::VPCTL::kPhaseNotReset },
        { CLK::rCTL, CLK::CTL::kChangeClock, 0 },
        { SD::rCFG1, SD::CFG1::kAsyncFIFONotRST, 0 }
    };
    
    return this->transferWriteRegisterCommands(SimpleRegValuePairs(pairs));
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
bool RealtekPCICardReaderController::oobsPollingRequiresPhyRCR0Access()
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
bool RealtekPCICardReaderController::supportsOOBSPolling()
{
    return false;
}

///
/// Enable OOBS polling
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
///
IOReturn RealtekPCICardReaderController::enableOOBSPolling()
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
    using namespace RTSX::PCR::Chip;
    
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
IOReturn RealtekPCICardReaderController::disableOOBSPolling()
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
    using namespace RTSX::PCR::Chip;
    
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
IOReturn RealtekPCICardReaderController::readPingPongBuffer(UInt8* destination, IOByteCount length)
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
        ContiguousRegValuePairsForReadAccess pairs(RTSX::PCR::Chip::PPBUF::rBASE2 + index, newLength);
        
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
IOReturn RealtekPCICardReaderController::writePingPongBuffer(const UInt8* source, IOByteCount length)
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
        ContiguousRegValuePairsForWriteAccess pairs(RTSX::PCR::Chip::PPBUF::rBASE2 + index, newLength, source + index);
        
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
ClockPhase RealtekPCICardReaderController::getRxClockPhase()
{
    return this->parameters.initialRxClockPhase;
}

///
/// Get the clock phase for Tx
///
/// @return The clock phase.
///
ClockPhase RealtekPCICardReaderController::getTxClockPhase()
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
IOReturn RealtekPCICardReaderController::setASPM(bool enable)
{
    using namespace RTSX::PCR::Chip;
    
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
IOReturn RealtekPCICardReaderController::setL1OffSubConfigD0(bool active)
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
IOReturn RealtekPCICardReaderController::setLTRLatency(UInt32 latency)
{
    using namespace RTSX::PCR::Chip;
    
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
void RealtekPCICardReaderController::enterWorkerState()
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
void RealtekPCICardReaderController::prepareToSleep()
{
    pinfo("Prepare to sleep...");
    
    // Turn off the LED
    psoftassert(this->turnOffLED() == kIOReturnSuccess, "Failed to turn off the LED.");
    
    // Disable all interrupts
    this->writeRegister32(RTSX::MMIO::rBIER, 0);
    
    // Set the host sleep state
    using namespace RTSX::PCR::Chip;
    
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
void RealtekPCICardReaderController::prepareToWakeUp()
{
    pinfo("Prepare to wake up...");
    
    // Set the host sleep state
    using namespace RTSX::PCR::Chip;
    
    psoftassert(this->writeChipRegister(rHSSTA, HSSTA::kMask, HSSTA::kHostWakeup) == kIOReturnSuccess, "Failed to set the host sleep state.");
    
    // Initialize the hardware
    psoftassert(this->initHardwareCommon() == kIOReturnSuccess, "Failed to initialize the hardware.");
    
    // All done
    pinfo("The hardware is ready.");
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
bool RealtekPCICardReaderController::interruptEventSourceFilter(IOFilterInterruptEventSource* source)
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
void RealtekPCICardReaderController::interruptHandlerGated(IOInterruptEventSource* sender, int count)
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
        
        this->dmaErrorCounter = 0;
    }
}

///
/// Timeout interrupt service routine
///
/// @param sender The timer event source
/// @note The timeout handler runs in a gated context.
///
void RealtekPCICardReaderController::bufferTransferTimeoutHandlerGated(IOTimerEventSource* sender)
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
void RealtekPCICardReaderController::onTransferDoneGated(bool succeeded)
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
void RealtekPCICardReaderController::onSDCardOvercurrentOccurredGated()
{
    // Guard: Retrieve the current OCP status
    using namespace RTSX::PCR::Chip;
    
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
void RealtekPCICardReaderController::onSDCardInsertedGated()
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
void RealtekPCICardReaderController::onSDCardRemovedGated()
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
IOReturn RealtekPCICardReaderController::getRevision(Revision& revision)
{
    pinfo("Fetching the chip revision...");
    
    using namespace RTSX::PCR::Chip;
    
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
IOReturn RealtekPCICardReaderController::optimizePhys()
{
    return kIOReturnSuccess;
}

///
/// Initialize the hardware (Common Part)
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_pci_init_hw()` defined in `rtsx_psr.c`.
///
IOReturn RealtekPCICardReaderController::initHardwareCommon()
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
    
    using namespace RTSX::PCR::Chip;
    
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
/// Map the device memory
///
/// @return `true` on success, `false` otherwise.
///
bool RealtekPCICardReaderController::mapDeviceMemory()
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
/// Probe the index of the message signaled interrupt
///
/// @return The index on success, `0` otherwise.
///
int RealtekPCICardReaderController::probeMSIIndex()
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
bool RealtekPCICardReaderController::setupInterrupts()
{
    // Guard: Probe the MSI interrupt index
    int index = this->probeMSIIndex();
    
    pinfo("The MSI index = %d. Will set up the interrupt event source.", index);
    
    // Guard: Setup the interrupt event source
    auto filter = OSMemberFunctionCast(IOFilterInterruptEventSource::Filter, this, &RealtekPCICardReaderController::interruptEventSourceFilter);
    
    auto handler = OSMemberFunctionCast(IOFilterInterruptEventSource::Action, this, &RealtekPCICardReaderController::interruptHandlerGated);
    
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
bool RealtekPCICardReaderController::setupHostBuffer()
{
    // Used to generate the scatter/gather list for the DMA transaction
    pinfo("Creating the host command and data buffer...");
    
    UInt64 offset = 0;
    
    IODMACommand::Segment32 segment;
    
    UInt32 numSegments = 1;
    
    // Guard: 1. Allocate memory for the host command and data buffer
    this->hostBufferDescriptor = IOBufferMemoryDescriptor::withCapacity(kHostBufferSize, kIODirectionInOut, true);
    
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
    this->hostBufferTimer = IOTimerEventSource::timerEventSource(this, OSMemberFunctionCast(IOTimerEventSource::Action, this, &RealtekPCICardReaderController::bufferTransferTimeoutHandlerGated));
    
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
bool RealtekPCICardReaderController::setupCardReader()
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
bool RealtekPCICardReaderController::createCardSlot()
{
    pinfo("Creating the card slot...");
    
    RealtekSDXCSlot* slot = OSTypeAlloc(RealtekPCISDXCSlot);
    
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
/// Tear down the interrupt event source
///
void RealtekPCICardReaderController::tearDownInterrupts()
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
/// Tear down the host command and buffer management module
///
void RealtekPCICardReaderController::tearDownHostBuffer()
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
void RealtekPCICardReaderController::unmapDeviceMemory()
{
    this->deviceMemoryDescriptor->complete();
    
    this->deviceMemoryDescriptor = nullptr;
    
    OSSafeReleaseNULL(this->deviceMemoryMap);
}

///
/// Destroy the card slot
///
void RealtekPCICardReaderController::destroyCardSlot()
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
/// @note RTS5228 and RTS5261 controllers must override this function to adjust the minimum SSC clock frequency.
///
bool RealtekPCICardReaderController::init(OSDictionary* dictionary)
{
    if (!super::init(dictionary))
    {
        perr("Failed to initialize the super class.");
        
        return false;
    }
    
    this->device = nullptr;
    
    this->deviceMemoryMap = nullptr;
    
    this->deviceMemoryDescriptor = nullptr;
    
    this->interruptEventSource = nullptr;
    
    this->hostBufferDMACommand = nullptr;
    
    this->hostBufferTimer = nullptr;
    
    this->hostBufferAddress = 0;
    
    this->hostBufferTransferStatus = kIOReturnSuccess;
    
    bzero(&this->parameters, sizeof(Parameters));
    
    this->dmaErrorCounter = 0;
    
    this->isIdle = true;
    
    this->reserved0 = 0;
    
    this->reserved1 = 0;
    
    this->reserved2 = 0;
    
    using namespace RTSX::PCR::Chip;
    
    this->sscClockLimits.rangeN = { RealtekPCICardReaderController::kMinSSCClockN, RealtekPCICardReaderController::kMaxSSCClockN };
    
    this->sscClockLimits.rangeDivider = { CLK::DIV::k1, CLK::DIV::k8 };
    
    this->sscClockLimits.minFrequencyMHz = RealtekPCICardReaderController::kMinSSCClockFrequencyMHz;
    
    this->busTimingTables.sdr50 = &RealtekPCICardReaderController::kBusTimingTableSDR50;
    
    this->busTimingTables.ddr50 = &RealtekPCICardReaderController::kBusTimingTableDDR50;
    
    this->busTimingTables.highSpeed = &RealtekPCICardReaderController::kBusTimingTableHighSpeed;
    
    this->busTimingTables.defaultSpeed = &RealtekPCICardReaderController::kBusTimingTableDefaultSpeed;
    
    this->tuningConfig.numPhases = RealtekPCICardReaderController::kTuningNumPhases;
    
    this->tuningConfig.enable80ClocksTimeout = RealtekPCICardReaderController::kTuningEnable80ClocksTimes;
    
    return true;
}

///
/// Start the controller
///
/// @param provider An instance of PCI device that represents the card reader
/// @return `true` on success, `false` otherwise.
///
bool RealtekPCICardReaderController::start(IOService* provider)
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
    
    // Set up the memory map
    if (!this->mapDeviceMemory())
    {
        perr("Failed to map the device memory.");
        
        goto error1;
    }
    
    // Set up the hardware interrupt
    if (!this->setupInterrupts())
    {
        perr("Failed to set up the hardware interrupt.");
        
        goto error2;
    }
    
    // Set up the host buffer
    if (!this->setupHostBuffer())
    {
        perr("Failed to set up the host buffer.");
        
        goto error3;
    }
    
    // Fetch the current ASPM state
    this->parameters.pm.isASPMEnabled = this->isASPMEnabled();
    
    pinfo("ASPM Enabled: %s.", YESNO(this->parameters.pm.isASPMEnabled));
    
    // Set up the card reader
    if (!this->setupCardReader())
    {
        perr("Failed to set up the card reader.");
        
        goto error4;
    }
    
    // Initialize the host UHS-I capabilities
    this->uhsCaps.sdr104 = this->parameters.caps.supportsSDSDR104;
    
    this->uhsCaps.sdr50 = this->parameters.caps.supportsSDSDR50;
    
    this->uhsCaps.ddr50 = this->parameters.caps.supportsSDDDR50;
    
    this->uhsCaps.reserved = this->parameters.caps.supportsSDExpress;
    
    // Create the card slot
    if (!this->createCardSlot())
    {
        perr("Failed to create the card slot.");
        
        goto error4;
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
    
    this->registerService();
    
    pinfo("ASPM Enabled: %s.", YESNO(this->isASPMEnabled()));
    
    pinfo("================================================");
    pinfo("The card reader controller started successfully.");
    pinfo("================================================");
    
    return true;
    
error4:
    this->tearDownHostBuffer();
    
error3:
    this->tearDownInterrupts();
    
error2:
    this->unmapDeviceMemory();
    
error1:
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
void RealtekPCICardReaderController::stop(IOService* provider)
{
    this->destroyCardSlot();
    
    this->tearDownHostBuffer();
    
    this->tearDownInterrupts();
    
    this->unmapDeviceMemory();
    
    this->device->setMemoryEnable(false);
    
    this->device->setBusMasterEnable(false);
    
    OSSafeReleaseNULL(this->device);
    
    super::stop(provider);
}
