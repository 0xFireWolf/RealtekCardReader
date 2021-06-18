//
//  Registers.hpp
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 2/17/21.
//

#ifndef Registers_hpp
#define Registers_hpp

#include <libkern/OSTypes.h>
#include "Utilities.hpp"

//
// Overview
//
// Broadly, registers are classified into three categories, and controllers provide specical routines to access them.
// 1) MMIO: Registers have a 32-bit address and can be accessed via `readRegister8/16/32` and `writeRegister8/16/32`.
// 2) Chip: Registers have a 16-bit address and a 8-bit value.
//          Can be accessed via `readChipRegister` and `writeChipRegister`.
// 3)  PHY: Registers have a 8-bit address and a 16-bit value.
//          Can be accessed via `readPhysRegister` and `writePhysRegister`.
// Categories are reflected by the namespace.
// Some registers may have special masks and flags, and these values are defined in a nested namespace named after the register.
//

//
// MARK: - 0. Macros & Data Structures
//

#define RTSXDeclareMMIORegister(name, address) \
    static constexpr UInt32 name = address;

#define RTSXDeclareChipRegister(name, address) \
    static constexpr UInt16 name = address;

#define RTSXDeclarePhysRegister(name, address) \
    static constexpr UInt8 name = address;

#define RTSXDeclareMMIORegisterValue32(name, value) \
    static constexpr UInt32 name = value;

#define RTSXDeclareChipRegisterValue(name, value) \
    static constexpr UInt8 name = value;

#define RTSXDeclarePhysRegisterValue(name, value) \
    static constexpr UInt16 name = value;


/// Represents a pair of register address and its value
struct ChipRegValuePair
{
    /// The register address
    UInt16 address;
    
    /// The register value mask
    UInt8 mask;
    
    /// The register value
    UInt8 value;
    
    /// Create a pair
    ChipRegValuePair(UInt16 address, UInt8 mask, UInt8 value) :
        address(address), mask(mask), value(value) {}

    /// Create a pair with mask set to 0xFF
    ChipRegValuePair(UInt16 address, UInt8 value) :
        ChipRegValuePair(address, 0xFF, value) {}
    
    /// Create an empty pair
    ChipRegValuePair() :
        ChipRegValuePair(0, 0, 0) {}
};

/// Interface of a sequence of pairs of register address and its value
struct ChipRegValuePairs
{
    /// Virtual destructor
    virtual ~ChipRegValuePairs() = default;
    
    /// Retrieve the i-th register value pair
    virtual struct ChipRegValuePair get(size_t index) const = 0;
    
    /// Retrieve the total number of pairs
    virtual IOItemCount size() const = 0;
};

///
/// Composed of a simple array of pairs of register address and value
///
/// @note Commonly used to access a small number of registers.
///
struct SimpleRegValuePairs: ChipRegValuePairs
{
private:
    /// An array of pairs
    const ChipRegValuePair* pairs;
    
    /// The number of pair
    const IOItemCount count;
    
public:
    /// Create a simple sequence of pairs
    SimpleRegValuePairs(const ChipRegValuePair* pairs, IOItemCount count) :
        pairs(pairs), count(count) {}
    
    /// Create a simple sequence of pairs
    template <size_t N>
    SimpleRegValuePairs(const ChipRegValuePair (&pairs)[N]) :
        pairs(pairs), count(N) {}
    
    /// Retrieve the i-th register value pair
    struct ChipRegValuePair get(size_t index) const override
    {
        return this->pairs[index];
    }
    
    /// Retrieve the total number of pairs
    IOItemCount size() const override
    {
        return this->count;
    }
};

///
/// Composed of the base register address to generate a forward sequence of pairs for read access dynamically
///
/// @note Convenient to read a contiguous sequence of registers.
///
struct ContiguousRegValuePairsForReadAccess: ChipRegValuePairs
{
private:
    /// The base register address
    const UInt16 baseRegister;
    
    /// The number of pairs to generate
    const IOItemCount count;
    
public:
    ///
    /// Create a contiguous sequence of pairs for read access
    ///
    /// @param baseRegister The base register address
    /// @param count The number of registers
    ///
    ContiguousRegValuePairsForReadAccess(UInt16 baseRegister, IOItemCount count) :
        baseRegister(baseRegister), count(count) {}
    
    /// Retrieve the i-th register value pair
    struct ChipRegValuePair get(size_t index) const override
    {
        return ChipRegValuePair(static_cast<UInt16>(this->baseRegister + index), 0, 0);
    }
    
    /// Retrieve the total number of pairs
    IOItemCount size() const override
    {
        return this->count;
    }
};

///
/// Composed of the base register address to generate a forward sequence of pairs for write access dynamically
///
/// @note Convenient to read a contiguous sequence of registers.
///
struct ContiguousRegValuePairsForWriteAccess: ChipRegValuePairs
{
private:
    /// The base register address
    const UInt16 baseRegister;
    
    /// The register value mask
    const UInt8 mask;
    
    /// The number of pairs to generate
    const IOItemCount count;
    
    /// The register values
    const UInt8* values;
    
public:
    ///
    /// Create a contiguous sequence of pairs for write access
    ///
    /// @param baseRegister The base register address
    /// @param count The number of registers
    /// @param values An array of register values
    /// @param mask The regiter value mask, 0xFF by default
    ///
    ContiguousRegValuePairsForWriteAccess(UInt16 baseRegister, IOItemCount count, const UInt8* values, UInt8 mask = 0xFF) :
        baseRegister(baseRegister), mask(mask), count(count), values(values) {}
    
    /// Retrieve the i-th register value pair
    struct ChipRegValuePair get(size_t index) const override
    {
        return ChipRegValuePair(static_cast<UInt16>(this->baseRegister + index), this->mask, this->values[index]);
    }
    
    /// Retrieve the total number of pairs
    IOItemCount size() const override
    {
        return this->count;
    }
};

/// Represents a pair of physical register and its value
struct PhysRegValuePair
{
    UInt8 address;
    
    UInt8 reserved;
    
    UInt16 value;
    
    PhysRegValuePair(UInt8 address, UInt16 value)
        : address(address), reserved(0), value(value) {}
};

/// Represents a pair of physical register and its mask and value
struct PhysRegMaskValuePair
{
    UInt8 reserved[3];
    
    UInt8 address;
    
    UInt16 mask;
    
    UInt16 value;
  
    PhysRegMaskValuePair(UInt8 address, UInt16 mask, UInt16 value)
        : reserved(), address(address), mask(mask), value(value) {}
};

//
// MARK: - 1. MMIO Registers
//

/// MMIO registers that controls the host command buffer
namespace RTSX::MMIO
{
    /// Address of the register that receives the host command buffer address
    RTSXDeclareMMIORegister(rHCBAR, 0x00);
    
    /// Special values and flags for the HCBAR register
    namespace HCBAR
    {
        /// The maximum number of commands in the queue
        static constexpr IOItemCount kMaxNumCmds = 256;
    }
    
    /// Address of the register that initiates and terminates the execution of host commands
    RTSXDeclareMMIORegister(rHCBCTLR, 0x04);
    
    /// Special values and flags for the HCBCTLR register
    namespace HCBCTLR
    {
        /// Bit 31 is set to start executing commands
        RTSXDeclareMMIORegisterValue32(kStartCommand, 1 << 31);
        
        /// Bit 30 is set to turn on the automatic hardware response
        RTSXDeclareMMIORegisterValue32(kAutoResponse, 1 << 30);
        
        /// Bit 28 is set to terminate executing commands
        RTSXDeclareMMIORegisterValue32(kStopCommand, 1 << 28);
        
        /// Register value to start executing commands
        static inline UInt32 RegValueForStartCommand(IOItemCount count)
        {
            return kStartCommand | kAutoResponse | ((count * sizeof(UInt32)) & 0x00FFFFFF);
        }
    }
}

/// MMIO registers that controls the host data buffer
namespace RTSX::MMIO
{
    /// Address of the register that receives the host data buffer address
    RTSXDeclareMMIORegister(rHDBAR, 0x08);
    
    /// Special values and flags for the HDBAR register
    namespace HDBAR
    {
        /// The maximum number of data elements in the queue
        static constexpr IOItemCount kMaxNumElements = 384;
        
        /// The following values are appended to each scatter/gather list entry in the host data buffer
        static constexpr UInt8 kSGOptionInt = 0x04;
        static constexpr UInt8 kSGOptionEnd = 0x02;
        static constexpr UInt8 kSGOptionValid = 0x01;
        static constexpr UInt8 kSGOptionNoOp = 0x00;
        static constexpr UInt8 kSGOptionTransferData = 0x02 << 4;
        static constexpr UInt8 kSGOptionLinkDescriptor = 0x03 << 4;
    }
    
    /// Address of the register that initiates and terminates the transfer of host data buffer
    RTSXDeclareMMIORegister(rHDBCTLR, 0x0C);
    
    /// Special values and flags for the HDBCTLR register
    namespace HDBCTLR
    {
        /// Bit 31 is set to start a DMA transfer
        RTSXDeclareMMIORegisterValue32(kStartDMA, 1 << 31);
        
        /// Bit 29 is set to indicate a read operation
        RTSXDeclareMMIORegisterValue32(kDMARead, 1 << 29);
        
        /// Bit 28 is set to stop a DMA transfer
        RTSXDeclareMMIORegisterValue32(kStopDMA, 1 << 28);
        
        /// Bit 26-27 is set to 0b10 to turn on ADMA mode
        RTSXDeclareMMIORegisterValue32(kUseADMA, 2 << 26);
    }
}

/// MMIO registers that can be programmed to access chip registers
namespace RTSX::MMIO
{
    /// Address of the register used to access chip registers
    RTSXDeclareMMIORegister(rHAIMR, 0x10);
    
    /// Special values and flags for the HAIMR register
    namespace HAIMR
    {
        /// The maximum number of attempts
        static constexpr UInt32 kMaxAttempts = 1024;
        
        /// Bit 31 is set when the chip is busy
        RTSXDeclareMMIORegisterValue32(kStatusBusy, 1 << 31)
        
        /// Bit 30 is set to perform a write operation
        RTSXDeclareMMIORegisterValue32(kWriteOperation, 1 << 30)
        
        /// Register value to start a read operation
        static inline UInt32 RegValueForReadOperation(UInt16 address)
        {
            // Set the busy bit and clear the write operation bit
            return kStatusBusy | static_cast<UInt32>(address & 0x3FFF) << 16;
        }
        
        /// Register value to start a write operation
        static inline UInt32 RegValueForWriteOperation(UInt16 address, UInt8 mask, UInt8 value)
        {
            // Set the busy bit and the write operation bit
            return kStatusBusy | kWriteOperation | static_cast<UInt32>(address & 0x3FFF) << 16 | static_cast<UInt32>(mask) << 8 | value;
        }
        
        /// Check whether the device is busy by examining its register value
        static inline bool IsBusy(UInt32 value)
        {
            return (value & kStatusBusy) == kStatusBusy;
        }
    }
}

/// MMIO registers that reports which hardware interrupts are pending
namespace RTSX::MMIO
{
    /// Address of the register used to retrieve all pending interrupts
    RTSXDeclareMMIORegister(rBIPR, 0x14);
    
    /// Special values and flags for the BIER register
    namespace BIPR
    {
        /// Bit 31 is set when command transfer is done
        RTSXDeclareMMIORegisterValue32(kCommandTransferDone, 1 << 31);
        
        /// Bit 30 is set when data transfer is done
        RTSXDeclareMMIORegisterValue32(kDataTransferDone, 1 << 30);
        
        /// Bit 29 is set when transfer has succeeded
        RTSXDeclareMMIORegisterValue32(kTransferSucceeded, 1 << 29);
        
        /// Bit 28 is set when transfer has failed
        RTSXDeclareMMIORegisterValue32(kTransferFailed, 1 << 28);
        
        /// Bit 27 is set when a XD interrupt is pending
        RTSXDeclareMMIORegisterValue32(kXD, 1 << 27);
        
        /// Bit 26 is set when a MS interrupt is pending
        RTSXDeclareMMIORegisterValue32(kMS, 1 << 26);
        
        /// Bit 25 is set when a SD interrupt is pending
        RTSXDeclareMMIORegisterValue32(kSD, 1 << 25);
        
        /// Bit 24 is set when a GPIO0 interrupt is pending
        RTSXDeclareMMIORegisterValue32(kGPIO0, 1 << 24);
        
        /// Bit 23 is set when MS overcurrent has occurred
        RTSXDeclareMMIORegisterValue32(kMSOvercurrentOccurred, 1 << 23);
        
        /// Bit 22 is set when SD overcurrent has occurred
        RTSXDeclareMMIORegisterValue32(kSDOvercurrentOccurred, 1 << 22);
        
        /// Bit 19 is set when SD is write protected
        RTSXDeclareMMIORegisterValue32(kSDWriteProtected, 1 << 19);
        
        /// Bit 18 is set when XD card exists
        RTSXDeclareMMIORegisterValue32(kXDExists, 1 << 18);
        
        /// Bit 17 is set when MS card exists
        RTSXDeclareMMIORegisterValue32(kMSExists, 1 << 17);
        
        /// Bit 16 is set when SD card exists
        RTSXDeclareMMIORegisterValue32(kSDExists, 1 << 16);
    }
}

/// MMIO registers that controls which hardware interrupts are enabled
namespace RTSX::MMIO
{
    /// Address of the register used to enable certain interrupts
    RTSXDeclareMMIORegister(rBIER, 0x18);
    
    /// Special values and flags for the BIER register
    namespace BIER
    {
        /// Bit 31 is set to enable the interrupt when command transfer is done
        RTSXDeclareMMIORegisterValue32(kEnableCommandTransferDone, 1 << 31);
        
        /// Bit 30 is set to enable the interrupt when data transfer is done
        RTSXDeclareMMIORegisterValue32(kEnableDataTransferDone, 1 << 30);
        
        /// Bit 29 is set to enable the interrupt when transfer has succeeded
        RTSXDeclareMMIORegisterValue32(kEnableTransferSuccess, 1 << 29);
        
        /// Bit 28 is set to enable the interrupt when transfer has failed
        RTSXDeclareMMIORegisterValue32(kEnableTransferFailure, 1 << 28);
        
        /// Bit 27 is set to enable XD interrupt
        RTSXDeclareMMIORegisterValue32(kEnableXD, 1 << 27);
        
        /// Bit 26 is set to enable MS interrupt
        RTSXDeclareMMIORegisterValue32(kEnableMS, 1 << 26);
        
        /// Bit 25 is set to enable SD interrupt
        RTSXDeclareMMIORegisterValue32(kEnableSD, 1 << 25);
        
        /// Bit 24 is set to enable GPIO0 interrupt
        RTSXDeclareMMIORegisterValue32(kEnableGPIO0, 1 << 24);
        
        /// Bit 23 is set to enable MS overcurrent interrupt
        RTSXDeclareMMIORegisterValue32(kEnableMSOvercurrent, 1 << 23);
        
        /// Bit 22 is set to enable SD overcurrent interrupt
        RTSXDeclareMMIORegisterValue32(kEnableSDOvercurrent, 1 << 22);
        
        /// Clear all bits to disable all interrupts
        RTSXDeclareMMIORegisterValue32(kDisableAll, 0);
    }
}

//
// MARK: - 2. Chip Registers
//

/// Chip registers that controls the power of SSC and OCP
namespace RTSX::Chip
{
    RTSXDeclareChipRegister(rFPDCTL, 0xFC00);
    
    namespace FPDCTL
    {
        /// Bit 0 is set to power down SSC
        RTSXDeclareChipRegisterValue(kSSCPowerMask, 0x01);
        RTSXDeclareChipRegisterValue(kSSCPowerUpValue, 0x00);
        RTSXDeclareChipRegisterValue(kSSCPowerDownValue, 0x01);
        
        /// Bit 1 is set to power down overcurrent protection
        RTSXDeclareChipRegisterValue(kOCPPowerMask, 0x02);
        RTSXDeclareChipRegisterValue(kOCPPowerUpValue, 0x00);
        RTSXDeclareChipRegisterValue(kOCPPowerDownValue, 0x02);
        
        /// Bit 0 and 1 are set to power down SSC and OCP
        RTSXDeclareChipRegisterValue(kAllPowerMask, 0x03);
        RTSXDeclareChipRegisterValue(kAllPowerUpValue, 0x00);
        RTSXDeclareChipRegisterValue(kAllPowerDownValue, 0x03);
    }
    
    
}

/// Chip registers that control the SSC clock
namespace RTSX::Chip::CLK
{
    RTSXDeclareChipRegister(rCTL, 0xFC02);
    namespace CTL
    {
        RTSXDeclareChipRegisterValue(kLowFrequency, 0x01);
        RTSXDeclareChipRegisterValue(kChangeClock, 0x01);
    }
    
    RTSXDeclareChipRegister(rDIV, 0xFC03);
    namespace DIV
    {
        RTSXDeclareChipRegisterValue(k1, 0x01);
        RTSXDeclareChipRegisterValue(k2, 0x02);
        RTSXDeclareChipRegisterValue(k4, 0x03);
        RTSXDeclareChipRegisterValue(k8, 0x04);
    }
    
    RTSXDeclareChipRegister(rSEL, 0xFC04);
}

/// Chip registers that control the SSC
namespace RTSX::Chip::SSC
{
    RTSXDeclareChipRegister(rDIVN0, 0xFC0F);
    RTSXDeclareChipRegister(rDIVN1, 0xFC10);
    
    RTSXDeclareChipRegister(rCTL1, 0xFC11);
    namespace CTL1
    {
        RTSXDeclareChipRegisterValue(kRSTB, 0x80);
        RTSXDeclareChipRegisterValue(kEnable8x, 0x40);
        RTSXDeclareChipRegisterValue(kFixFraction, 0x20);
        
        RTSXDeclareChipRegisterValue(kSelector1M, 0x00);
        RTSXDeclareChipRegisterValue(kSelector2M, 0x08);
        RTSXDeclareChipRegisterValue(kSelector4M, 0x10);
        RTSXDeclareChipRegisterValue(kSelector8M, 0x18);
    }
    
    RTSXDeclareChipRegister(rCTL2, 0xFC12);
    namespace CTL2
    {
        RTSXDeclareChipRegisterValue(kDepthMask, 0x07);
        RTSXDeclareChipRegisterValue(kDepthDisable, 0x00);
        RTSXDeclareChipRegisterValue(kDepth4M, 0x01);
        RTSXDeclareChipRegisterValue(kDepth2M, 0x02);
        RTSXDeclareChipRegisterValue(kDepth1M, 0x03);
        RTSXDeclareChipRegisterValue(kDepth500K, 0x04);
        RTSXDeclareChipRegisterValue(kDepth250K, 0x05);
        
        /// Double the given SSC depth
        static inline UInt8 doubleDepth(UInt8 depth)
        {
            return depth > 1 ? depth - 1 : depth;
        }
    }
}

namespace RTSX::Chip
{
    RTSXDeclareChipRegister(rRCCTL, 0xFC14);
}
    
/// Chip registers that control the LED behavior
namespace RTSX::Chip::OLT_LED
{
    /// Address of the register that controls the LED
    RTSXDeclareChipRegister(rCTL, 0xFC1E);
    
    /// Special values and flags for the CTL register
    namespace CTL
    {
        /// Bit 3 is set to turn on blinking
        RTSXDeclareChipRegisterValue(kBlinkingMask, 0x08);
        RTSXDeclareChipRegisterValue(kEnableBlinkingValue, 0x08);
        RTSXDeclareChipRegisterValue(kDisableBlinkingValue, 0x00);
    }
}

/// Chip registers that manages GPIO pins
namespace RTSX::Chip::GPIO
{
    /// Address of the register that manages GPIO pins
    RTSXDeclareChipRegister(rCTL, 0xFC1F);
    
    /// Special values and flags for the CTL register
    namespace CTL
    {
        /// Bit 1 is set to turn on the LED
        RTSXDeclareChipRegisterValue(kLEDMask, 0x02);
        RTSXDeclareChipRegisterValue(kTurnOnLEDValue, 0x02);
        RTSXDeclareChipRegisterValue(kTurnOffLEDValue, 0x00);
    }
}

/// Chip registers that manages the card
namespace RTSX::Chip::CARD
{
    RTSXDeclareChipRegister(rCLKSRC, 0xFC2E);
    namespace CLKSRC
    {
        RTSXDeclareChipRegisterValue(kCRCFixClock, 0x00 << 0);
        RTSXDeclareChipRegisterValue(kCRCVarClock0, 0x01 << 0);
        RTSXDeclareChipRegisterValue(kCRCVarClock1, 0x02 << 0);
        
        RTSXDeclareChipRegisterValue(kSD30FixClock, 0x00 << 2);
        RTSXDeclareChipRegisterValue(kSD30VarClock0, 0x01 << 2);
        RTSXDeclareChipRegisterValue(kSD30VarClock1, 0x02 << 2);
        
        RTSXDeclareChipRegisterValue(kSampleFixClock, 0x00 << 4);
        RTSXDeclareChipRegisterValue(kSampleVarClock0, 0x01 << 4);
        RTSXDeclareChipRegisterValue(kSampleVarClock1, 0x02 << 4);
    }
    
    RTSXDeclareChipRegister(rPWRCTRL, 0xFD50);
    namespace PWRCTRL
    {
        RTSXDeclareChipRegisterValue(kSDPowerOn, 0x00);
        RTSXDeclareChipRegisterValue(kSDPartialPowerOn, 0x01);
        RTSXDeclareChipRegisterValue(kSDPowerOff, 0x03);
        
        RTSXDeclareChipRegisterValue(kSDPowerMask, 0x03);
        
        RTSXDeclareChipRegisterValue(kSDVCCPartialPowerOn, 0x02);
        RTSXDeclareChipRegisterValue(kSDVCCPowerOn, 0x00);
    }
    
    RTSXDeclareChipRegister(rSHAREMODE, 0xFD52);
    namespace SHAREMODE
    {
        RTSXDeclareChipRegisterValue(kMask, 0x0F);
        RTSXDeclareChipRegisterValue(kMultiLun, 0x00);
        RTSXDeclareChipRegisterValue(kNormal, 0x00);
        RTSXDeclareChipRegisterValue(k48SD, 0x04);
        RTSXDeclareChipRegisterValue(k48MS, 0x08);
        RTSXDeclareChipRegisterValue(kBarossaSD, 0x01);
        RTSXDeclareChipRegisterValue(kBarossaMS, 0x02);
    }    
    
    /// Address of the register that selects the card drive
    RTSXDeclareChipRegister(rDRVSEL, 0xFD53);
    namespace DRVSEL
    {
        RTSXDeclareChipRegisterValue(kMS8mA, 0x01 << 6);
        RTSXDeclareChipRegisterValue(kMMC8mA, 0x01 << 4);
        RTSXDeclareChipRegisterValue(kXD8mA, 0x01 << 2);
        RTSXDeclareChipRegisterValue(kGPIO8mA, 0x01);
        
        RTSXDeclareChipRegisterValue(kDefault, kMS8mA | kGPIO8mA);
        RTSXDeclareChipRegisterValue(kDefault_5209, kMS8mA | kMMC8mA | kXD8mA);
        RTSXDeclareChipRegisterValue(kDefault_8411, kMS8mA | kMMC8mA | kXD8mA | kGPIO8mA);
    }
    
    RTSXDeclareChipRegister(rSTOP, 0xFD54);
    namespace STOP
    {
        RTSXDeclareChipRegisterValue(kStopSPI, 0x01);
        RTSXDeclareChipRegisterValue(kStopXD, 0x02);
        RTSXDeclareChipRegisterValue(kStopSD, 0x04);
        RTSXDeclareChipRegisterValue(kStopMS, 0x08);
        
        RTSXDeclareChipRegisterValue(kClearSPIError, 0x10);
        RTSXDeclareChipRegisterValue(kClearXDError, 0x20);
        RTSXDeclareChipRegisterValue(kClearSDError, 0x40);
        RTSXDeclareChipRegisterValue(kClearMSError, 0x80);
    }
    
    /// Address of the register that enables the card output
    RTSXDeclareChipRegister(rOUTPUT, 0xFD55);
    
    /// Special values and flags for the OUTPUT register
    namespace OUTPUT
    {
        /// Bit 1 is set to enable XD output
        RTSXDeclareChipRegisterValue(kXDMask, 1 << 1);
        RTSXDeclareChipRegisterValue(kEnableXDValue, 1 << 1);
        RTSXDeclareChipRegisterValue(kDisableXDValue, 0);
        
        /// Bit 2 is set to enable SD output
        RTSXDeclareChipRegisterValue(kSDMask, 1 << 2);
        RTSXDeclareChipRegisterValue(kEnableSDValue, 1 << 2);
        RTSXDeclareChipRegisterValue(kDisableSDValue, 0);
        
        /// Bit 3 is set to enable MS output
        RTSXDeclareChipRegisterValue(kMSMask, 1 << 3);
        RTSXDeclareChipRegisterValue(kEnableMSValue, 1 << 3);
        RTSXDeclareChipRegisterValue(kDisableMSValue, 0);
        
        /// Bit 4 is set to enable SPI output
        RTSXDeclareChipRegisterValue(kSPIMask, 1 << 4);
        RTSXDeclareChipRegisterValue(kEnableSPIValue, 1 << 4);
        RTSXDeclareChipRegisterValue(kDisableSPIValue, 0);
    }
    
    RTSXDeclareChipRegister(rDATASRC, 0xFD5B);
    namespace DATASRC
    {
        RTSXDeclareChipRegisterValue(kPingPongBuffer, 0x01);
        RTSXDeclareChipRegisterValue(kRingBuffer, 0x00);
        RTSXDeclareChipRegisterValue(kMask, 0x01);
    }
    
    RTSXDeclareChipRegister(rSEL, 0xFD5C);
    namespace SEL
    {
        RTSXDeclareChipRegisterValue(kSD, 0x2);
        RTSXDeclareChipRegisterValue(kMS, 0x3);
        RTSXDeclareChipRegisterValue(kMask, 0x7);
    }
    
    namespace SD30::DRVSEL
    {
        RTSXDeclareChipRegister(rCLK, 0xFD5A);
        namespace CLK
        {
            RTSXDeclareChipRegisterValue(kDriverTypeA, 0x05);
            RTSXDeclareChipRegisterValue(kDriverTypeB, 0x03);
            RTSXDeclareChipRegisterValue(kDriverTypeC, 0x02);
            RTSXDeclareChipRegisterValue(kDriverTypeD, 0x01);
        }
        
        RTSXDeclareChipRegister(rCMD, 0xFD5E);
        
        RTSXDeclareChipRegister(rCFG, 0xFD5E);
        namespace CFG
        {
            RTSXDeclareChipRegisterValue(kDriverTypeA, 0x02);
            RTSXDeclareChipRegisterValue(kDriverTypeB, 0x03);
            RTSXDeclareChipRegisterValue(kDriverTypeC, 0x01);
            RTSXDeclareChipRegisterValue(kDriverTypeD, 0x00);
        }
        
        RTSXDeclareChipRegister(rDAT, 0xFD5F);
    }
    
    /// Card pull control registers
    namespace PULL
    {
        RTSXDeclareChipRegister(rCTL1, 0xFD60);
        RTSXDeclareChipRegister(rCTL2, 0xFD61);
        RTSXDeclareChipRegister(rCTL3, 0xFD62);
        RTSXDeclareChipRegister(rCTL4, 0xFD63);
        RTSXDeclareChipRegister(rCTL5, 0xFD64);
        RTSXDeclareChipRegister(rCTL6, 0xFD65);
    }
    
    /// Register to enable the card clock
    RTSXDeclareChipRegister(rCLK, 0xFD69);
    namespace CLK
    {
        RTSXDeclareChipRegisterValue(kDisable, 0x00);
        RTSXDeclareChipRegisterValue(kEnableSD, 0x04);
        RTSXDeclareChipRegisterValue(kEnableMS, 0x08);
        RTSXDeclareChipRegisterValue(kEnableSD40, 0x10);
        RTSXDeclareChipRegisterValue(kMask, 0x1E);
    }
}

/// Chip registers that manages the overcurrent protection
namespace RTSX::Chip::OCP
{
    /// Address of the register that controls the overcurrent protection
    RTSXDeclareChipRegister(rCTL, 0xFD6A);
    
    /// Special values and flags for the CTL register
    namespace CTL
    {
        RTSXDeclareChipRegisterValue(kSDClearStatMask, 0x01);
        RTSXDeclareChipRegisterValue(kSDClearStatValue, 0x01);
        RTSXDeclareChipRegisterValue(kSDClearInterruptMask, 0x02);
        RTSXDeclareChipRegisterValue(kSDClearInterruptValue, 0x02);
        
        RTSXDeclareChipRegisterValue(kSDInterruptMask, 0x04);
        RTSXDeclareChipRegisterValue(kSDInterruptValueEnable, 0x04);
        RTSXDeclareChipRegisterValue(kSDInterruptValueDisable, 0x00);
        
        RTSXDeclareChipRegisterValue(kSDDetectionMask, 0x08);
        RTSXDeclareChipRegisterValue(kSDDetectionValueEnable, 0x08);
        RTSXDeclareChipRegisterValue(kSDDetectionValueDisable, 0x00);
        
        RTSXDeclareChipRegisterValue(kSDDetectionAndInterruptMask, kSDInterruptMask | kSDDetectionMask);
        RTSXDeclareChipRegisterValue(kSDEnableDetectionAndInterruptValue, kSDInterruptValueEnable | kSDDetectionValueEnable)
        RTSXDeclareChipRegisterValue(kSDDisableDetectionAndInterruptValue, kSDInterruptValueDisable | kSDDetectionValueDisable)
        
        
        RTSXDeclareChipRegisterValue(kSDVIOClearStatMask, 0x10);
        RTSXDeclareChipRegisterValue(kSDVIOClearInterruptMask, 0x20);
        RTSXDeclareChipRegisterValue(kSDVIOInterruptMask, 0x40);
        RTSXDeclareChipRegisterValue(kSDVIODetectionMask, 0x80);
    }
    
    /// Address of the register that programs the time parameter
    RTSXDeclareChipRegister(rPARA1, 0xFD6B);
    
    /// Special values and flags for the PARA1 register
    namespace PARA1
    {
        RTSXDeclareChipRegisterValue(kSDVIOTimeMask, 0x70);
        RTSXDeclareChipRegisterValue(kSDVIOTimeValue60, 0x00);
        RTSXDeclareChipRegisterValue(kSDVIOTimeValue100, 0x10);
        RTSXDeclareChipRegisterValue(kSDVIOTimeValue200, 0x20);
        RTSXDeclareChipRegisterValue(kSDVIOTimeValue400, 0x30);
        RTSXDeclareChipRegisterValue(kSDVIOTimeValue600, 0x40);
        RTSXDeclareChipRegisterValue(kSDVIOTimeValue800, 0x50);
        RTSXDeclareChipRegisterValue(kSDVIOTimeValue1100, 0x60);
        
        RTSXDeclareChipRegisterValue(kSDTimeMask, 0x07);
        RTSXDeclareChipRegisterValue(kSDTimeValue60, 0x00);
        RTSXDeclareChipRegisterValue(kSDTimeValue100, 0x01);
        RTSXDeclareChipRegisterValue(kSDTimeValue200, 0x02);
        RTSXDeclareChipRegisterValue(kSDTimeValue400, 0x03);
        RTSXDeclareChipRegisterValue(kSDTimeValue600, 0x04);
        RTSXDeclareChipRegisterValue(kSDTimeValue800, 0x05);
        RTSXDeclareChipRegisterValue(kSDTimeValue1100, 0x06);
    }
    
    /// Address of the register that programs the glitch value
    RTSXDeclareChipRegister(rGLITCH, 0xFD6C);
    
    /// Special values and flags for the GLITCH register
    namespace GLITCH
    {
        RTSXDeclareChipRegisterValue(kSDVIOMask, 0xF0);
        RTSXDeclareChipRegisterValue(kSDVIOValueNone, 0x00);
        RTSXDeclareChipRegisterValue(kSDVIOValue50U, 0x10);
        RTSXDeclareChipRegisterValue(kSDVIOValue100U, 0x20);
        RTSXDeclareChipRegisterValue(kSDVIOValue200U, 0x30);
        RTSXDeclareChipRegisterValue(kSDVIOValue600U, 0x40);
        RTSXDeclareChipRegisterValue(kSDVIOValue800U, 0x50);
        RTSXDeclareChipRegisterValue(kSDVIOValue1M, 0x60);
        RTSXDeclareChipRegisterValue(kSDVIOValue2M, 0x70);
        RTSXDeclareChipRegisterValue(kSDVIOValue3M, 0x80);
        RTSXDeclareChipRegisterValue(kSDVIOValue4M, 0x90);
        RTSXDeclareChipRegisterValue(kSDVIOValue5M, 0xA0);
        RTSXDeclareChipRegisterValue(kSDVIOValue6M, 0xB0);
        RTSXDeclareChipRegisterValue(kSDVIOValue7M, 0xC0);
        RTSXDeclareChipRegisterValue(kSDVIOValue8M, 0xD0);
        RTSXDeclareChipRegisterValue(kSDVIOValue9M, 0xE0);
        RTSXDeclareChipRegisterValue(kSDVIOValue10M, 0xF0);
        
        RTSXDeclareChipRegisterValue(kSDMask, 0x0F);
        RTSXDeclareChipRegisterValue(kSDValueNone, 0x00);
        RTSXDeclareChipRegisterValue(kSDValue50U, 0x01);
        RTSXDeclareChipRegisterValue(kSDValue100U, 0x02);
        RTSXDeclareChipRegisterValue(kSDValue200U, 0x03);
        RTSXDeclareChipRegisterValue(kSDValue600U, 0x04);
        RTSXDeclareChipRegisterValue(kSDValue800U, 0x05);
        RTSXDeclareChipRegisterValue(kSDValue1M, 0x06);
        RTSXDeclareChipRegisterValue(kSDValue2M, 0x07);
        RTSXDeclareChipRegisterValue(kSDValue3M, 0x08);
        RTSXDeclareChipRegisterValue(kSDValue4M, 0x09);
        RTSXDeclareChipRegisterValue(kSDValue5M, 0x0A);
        RTSXDeclareChipRegisterValue(kSDValue6M, 0x0B);
        RTSXDeclareChipRegisterValue(kSDValue7M, 0x0C);
        RTSXDeclareChipRegisterValue(kSDValue8M, 0x0D);
        RTSXDeclareChipRegisterValue(kSDValue9M, 0x0E);
        RTSXDeclareChipRegisterValue(kSDValue10M, 0x0F);
    }
    
    /// Address of the register that programs the total harmonic distortion parameter
    RTSXDeclareChipRegister(rPARA2, 0xFD6D);
    
    /// Special values and flags for the PARA2 register
    namespace PARA2
    {
        RTSXDeclareChipRegisterValue(kSDVIOThdMask, 0x70);
        RTSXDeclareChipRegisterValue(kSDVIOThdValue190, 0x00);
        RTSXDeclareChipRegisterValue(kSDVIOThdValue250, 0x10);
        RTSXDeclareChipRegisterValue(kSDVIOThdValue320, 0x20);
        RTSXDeclareChipRegisterValue(kSDVIOThdValue380, 0x30);
        RTSXDeclareChipRegisterValue(kSDVIOThdValue440, 0x40);
        RTSXDeclareChipRegisterValue(kSDVIOThdValue500, 0x50);
        RTSXDeclareChipRegisterValue(kSDVIOThdValue570, 0x60);
        RTSXDeclareChipRegisterValue(kSDVIOThdValue630, 0x70);
        
        RTSXDeclareChipRegisterValue(kSDThdMask, 0x07);
        RTSXDeclareChipRegisterValue(kSDThdValue450, 0x00);
        RTSXDeclareChipRegisterValue(kSDThdValue550, 0x01);
        RTSXDeclareChipRegisterValue(kSDThdValue650, 0x02);
        RTSXDeclareChipRegisterValue(kSDThdValue750, 0x03);
        RTSXDeclareChipRegisterValue(kSDThdValue850, 0x04);
        RTSXDeclareChipRegisterValue(kSDThdValue950, 0x05);
        RTSXDeclareChipRegisterValue(kSDThdValue1050, 0x06);
        RTSXDeclareChipRegisterValue(kSDThdValue1150, 0x07);
        
        RTSXDeclareChipRegisterValue(kSDThdValue800_524A, 0x04);
        RTSXDeclareChipRegisterValue(kSDThdValue800_525A, 0x05);
        RTSXDeclareChipRegisterValue(kSDThdValue800_522A, 0x06);
    }
    
    /// Address of the register that contains the current status
    RTSXDeclareChipRegister(rSTAT, 0xFD6E);
    
    /// Special values and flags for the STAT register
    namespace STAT
    {
        /// Bit 1 is set to indicate that SD has an overcurrent event before
        RTSXDeclareChipRegisterValue(kSDEver, 1 << 1);
        
        /// Bit 2 is set to indicate that SD has an overcurrent event now
        RTSXDeclareChipRegisterValue(kSDNow, 1 << 2);
        
        /// Bit 5 is set to indicate that SDVIO has an overcurrent event before
        RTSXDeclareChipRegisterValue(kSDVIOEver, 1 << 5);
        
        /// Bit 6 is set to indicate that SDVIO has an overcurrent event now
        RTSXDeclareChipRegisterValue(kSDVIONow, 1 << 6);
    }
}

namespace RTSX::Chip
{
    RTSXDeclareChipRegister(rIRQEN0, 0xFE20);
    namespace IRQEN0
    {
        RTSXDeclareChipRegisterValue(kEnableLinkDownInt, 0x10);
        RTSXDeclareChipRegisterValue(kEnableLinkReadyInt, 0x20);
        RTSXDeclareChipRegisterValue(kEnableSuspendInt, 0x40);
        RTSXDeclareChipRegisterValue(kEnableDMADoneInt, 0x80);
    }
    
    RTSXDeclareChipRegister(rIRQSTAT0, 0xFE21);
    namespace IRQSTAT0
    {
        RTSXDeclareChipRegisterValue(kDMADoneInt, 0x80);
        RTSXDeclareChipRegisterValue(kSuspendInt, 0x40);
        RTSXDeclareChipRegisterValue(kLinkReadyInt, 0x20);
        RTSXDeclareChipRegisterValue(kLinkDownInt, 0x10);
    }
}

/// Chip registers that manages DMA
namespace RTSX::Chip::DMA
{
    /// DMA data length registers
    RTSXDeclareChipRegister(rC0, 0xFE28);
    RTSXDeclareChipRegister(rC1, 0xFE29);
    RTSXDeclareChipRegister(rC2, 0xFE2A);
    RTSXDeclareChipRegister(rC3, 0xFE2B);
    
    /// Address of the register that manages DMA
    RTSXDeclareChipRegister(rCTL, 0xFE2C);
    
    /// Special values and flags for the CTL register
    namespace CTL
    {
        /// Bit 7 is set to reset DMA
        RTSXDeclareChipRegisterValue(kResetMask, 0x80);
        RTSXDeclareChipRegisterValue(kResetValue, 0x80);
        RTSXDeclareChipRegisterValue(kBusy, 0x04);
        
        RTSXDeclareChipRegisterValue(kDirectionToCard, 0x00);
        RTSXDeclareChipRegisterValue(kDirectionFromCard, 0x02);
        RTSXDeclareChipRegisterValue(kDirectionMask, 0x02);
        
        RTSXDeclareChipRegisterValue(kEnable, 0x01);
        
        RTSXDeclareChipRegisterValue(kPackSize128, 0 << 4);
        RTSXDeclareChipRegisterValue(kPackSize256, 1 << 4);
        RTSXDeclareChipRegisterValue(kPackSize512, 2 << 4);
        RTSXDeclareChipRegisterValue(kPackSize1024, 3 << 4);
        RTSXDeclareChipRegisterValue(kPackSizeMask, 0x30);
    }
}

namespace RTSX::Chip::RBUF
{
    RTSXDeclareChipRegister(rCTL, 0xFE34);
    
    /// Special values and flags for the CTL register
    namespace CTL
    {
        /// Bit 7 is set to flush the buffer
        RTSXDeclareChipRegisterValue(kFlushMask, 0x80);
        RTSXDeclareChipRegisterValue(kFlushValue, 0x80);
    }
}

/// Chip registers that can be programmed to access PHY registers
namespace RTSX::Chip::PHY
{
    /// Address of the register used to set the data direction of accessing PHY registers
    RTSXDeclareChipRegister(rRWCTL, 0xFE3C);
    
    /// Special values and flags for the RWCTL register
    namespace RWCTL
    {
        /// The maximum number of attempts
        static constexpr UInt32 kMaximumAttempts = 100000;
        
        /// Bit 0 is cleared to perform a read operation
        static constexpr UInt8 kReadOperation = 0x00;
        
        /// Bit 0 is set to perform a write operation
        static constexpr UInt8 kWriteOperation = 0x01;
        
        /// Bit 7 is set to indicate that the chip is busy
        static constexpr UInt8 kStatusBusy = 0x80;
        
        /// Register value to start a read operation
        static constexpr UInt8 kSetReadOperation = kStatusBusy | kReadOperation;
        
        /// Register value to start a write operation
        static constexpr UInt8 kSetWriteOperation = kStatusBusy | kWriteOperation;
        
        /// Check whether the device is busy by examining its register value
        static inline bool IsBusy(UInt8 value)
        {
            return (value & kStatusBusy) == kStatusBusy;
        }
    }
    
    /// Address of the register that consumes the 1st byte of the data
    RTSXDeclareChipRegister(rDATA0, 0xFE3D);
    
    /// Address of the register that consumes the 2nd byte of the data
    RTSXDeclareChipRegister(rDATA1, 0xFE3E);
    
    /// Address of the register that consumes the address of a PHY register
    RTSXDeclareChipRegister(rADDR, 0xFE3F);
}

namespace RTSX::Chip::MSG
{
    namespace RX
    {
        RTSXDeclareChipRegister(rDATA0, 0xFE40);
        RTSXDeclareChipRegister(rDATA1, 0xFE41);
        RTSXDeclareChipRegister(rDATA2, 0xFE42);
        RTSXDeclareChipRegister(rDATA3, 0xFE43);
    }
    
    namespace TX
    {
        RTSXDeclareChipRegister(rDATA0, 0xFE44);
        RTSXDeclareChipRegister(rDATA1, 0xFE45);
        RTSXDeclareChipRegister(rDATA2, 0xFE46);
        RTSXDeclareChipRegister(rDATA3, 0xFE47);
        RTSXDeclareChipRegister(rCTL, 0xFE48);
    }
}

namespace RTSX::Chip::LTR
{
    RTSXDeclareChipRegister(rCTL, 0xFE4A);
    namespace CTL
    {
        RTSXDeclareChipRegisterValue(kTxMask, 1 << 7);
        RTSXDeclareChipRegisterValue(kEnableTx, 1 << 7);
        RTSXDeclareChipRegisterValue(kDisableTx, 0);
        
        RTSXDeclareChipRegisterValue(kLatencyModeMask, 1 << 6);
        RTSXDeclareChipRegisterValue(kLatencyModeHardware, 0);
        RTSXDeclareChipRegisterValue(kLatencyModeSoftware, 1 << 6);
    }
}

/// Chip registers that control OOBS polling
namespace RTSX::Chip::OOBS
{
    RTSXDeclareChipRegister(rOFFTIMER, 0xFEA6);
    RTSXDeclareChipRegister(rONTIMER, 0xFEA7);
    RTSXDeclareChipRegister(rVCMONTIMER, 0xFEA8)
    RTSXDeclareChipRegister(rPOLLING, 0xFEA9);
    RTSXDeclareChipRegister(rCFG, 0xFF6E);
    namespace CFG
    {
        RTSXDeclareChipRegisterValue(kAutoKDIS, 0x80);
        RTSXDeclareChipRegisterValue(kValMask, 0x1F);
    }
}

/// Chip registers that control the ping pong buffer
namespace RTSX::Chip::PPBUF
{
    RTSXDeclareChipRegister(rBASE1, 0xF800);
    RTSXDeclareChipRegister(rBASE2, 0xFA00);
}

namespace RTSX::Chip
{
    RTSXDeclareChipRegister(rPWRGATECTRL, 0xFE75);
    namespace PWRGATECTRL
    {
        RTSXDeclareChipRegisterValue(kOn, 0x00);
        RTSXDeclareChipRegisterValue(kEnable, 0x01);
        RTSXDeclareChipRegisterValue(kVCC1, 0x02);
        RTSXDeclareChipRegisterValue(kVCC2, 0x04);
        RTSXDeclareChipRegisterValue(kSuspend, 0x04);
        RTSXDeclareChipRegisterValue(kOff, 0x06);
        RTSXDeclareChipRegisterValue(kMask, 0x06);
    }
}

namespace RTSX::Chip::SD
{
    RTSXDeclareChipRegister(rVPCLK0CTL, 0xFC2A);
    RTSXDeclareChipRegister(rVPCLK1CTL, 0xFC2B);
    RTSXDeclareChipRegister(rVPTXCTL, rVPCLK0CTL);
    RTSXDeclareChipRegister(rVPRXCTL, rVPCLK1CTL);
    namespace VPCTL
    {
        RTSXDeclareChipRegisterValue(kPhaseSelectMask, 0x1F);
        RTSXDeclareChipRegisterValue(kPhaseChange, 0x80);
        RTSXDeclareChipRegisterValue(kPhaseNotReset, 0x40);
    }
    
    /// Clock divider, bus mode and width
    RTSXDeclareChipRegister(rCFG1, 0xFDA0);
    namespace CFG1
    {
        RTSXDeclareChipRegisterValue(kClockDivider0, 0x00);
        RTSXDeclareChipRegisterValue(kClockDivider128, 0x80);
        RTSXDeclareChipRegisterValue(kClockDivider256, 0xC0);
        RTSXDeclareChipRegisterValue(kClockDividerMask, 0xC0);
        
        RTSXDeclareChipRegisterValue(kBusWidth1Bit, 0x00);
        RTSXDeclareChipRegisterValue(kBusWidth4Bit, 0x01);
        RTSXDeclareChipRegisterValue(kBusWidth8Bit, 0x02);
        RTSXDeclareChipRegisterValue(kBusWidthMask, 0x03);
        
        RTSXDeclareChipRegisterValue(kModeSD20, 0x00);
        RTSXDeclareChipRegisterValue(kModeSDDDR, 0x04);
        RTSXDeclareChipRegisterValue(kModeSD30, 0x08);
        RTSXDeclareChipRegisterValue(kModeMask, 0x0C);
        
        RTSXDeclareChipRegisterValue(kAsyncFIFONotRST, 0x10);
    }
    
    /// SD command control and response
    RTSXDeclareChipRegister(rCFG2, 0xFDA1);
    namespace CFG2
    {
        RTSXDeclareChipRegisterValue(kCalcCRC7, 0x00);
        RTSXDeclareChipRegisterValue(kNoCalcCRC7, 0x80);
        
        RTSXDeclareChipRegisterValue(kCheckCRC16, 0x00);
        RTSXDeclareChipRegisterValue(kNoCheckCRC16, 0x40);
        RTSXDeclareChipRegisterValue(kNoCheckWaitCRCTo, 0x20);
        
        RTSXDeclareChipRegisterValue(kWaitBusyEnd, 0x08);
        RTSXDeclareChipRegisterValue(kNoWaitBusyEnd, 0x00);
        
        RTSXDeclareChipRegisterValue(kCheckCRC7, 0x00);
        RTSXDeclareChipRegisterValue(kNoCheckCRC7, 0x04);
        
        RTSXDeclareChipRegisterValue(kResponseLength0, 0x00);   // No response
        RTSXDeclareChipRegisterValue(kResponseLength6, 0x01);   // 48-bit response
        RTSXDeclareChipRegisterValue(kResponseLength17, 0x02);  // 136-bit response
        
        RTSXDeclareChipRegisterValue(kResponseTypeR0, 0x04);
        RTSXDeclareChipRegisterValue(kResponseTypeR1, 0x01);
        RTSXDeclareChipRegisterValue(kResponseTypeR1b, 0x09);
        RTSXDeclareChipRegisterValue(kResponseTypeR2, 0x02);
        RTSXDeclareChipRegisterValue(kResponseTypeR3, 0x05);
        RTSXDeclareChipRegisterValue(kResponseTypeR4, 0x05);
        RTSXDeclareChipRegisterValue(kResponseTypeR5, 0x01);
        RTSXDeclareChipRegisterValue(kResponseTypeR6, 0x01);
        RTSXDeclareChipRegisterValue(kResponseTypeR7, 0x01);
    }
    
    /// Used by the RTSX USB driver
    RTSXDeclareChipRegister(rCFG3, 0xFDA2);
    namespace CFG3
    {
        RTSXDeclareChipRegisterValue(kEnableSD30ClockEnd, 0x10);
        RTSXDeclareChipRegisterValue(kEnableResponse80ClockTimeout, 0x01);
    }

    RTSXDeclareChipRegister(rSTAT1, 0xFDA3);
    namespace STAT1
    {
        RTSXDeclareChipRegisterValue(kCRC7Error, 0x80);
        RTSXDeclareChipRegisterValue(kCRC16Error, 0x40);
        RTSXDeclareChipRegisterValue(kCRCWriteError, 0x20);
        RTSXDeclareChipRegisterValue(kCRCWriteErrorMask, 0x1C);
        RTSXDeclareChipRegisterValue(kCRCGetTimeout, 0x02);
        RTSXDeclareChipRegisterValue(kTuningCompareError, 0x01);
    }
    
    /// Used by the RTSX USB driver
    RTSXDeclareChipRegister(rSTAT2, 0xFDA4);
    namespace STAT2
    {
        RTSXDeclareChipRegisterValue(kResponse80ClockTimeout, 0x01);
    }
    
    RTSXDeclareChipRegister(rBUSSTAT, 0xFDA5);
    namespace BUSSTAT
    {
        RTSXDeclareChipRegisterValue(kClockToggleEnable, 0x80);
        RTSXDeclareChipRegisterValue(kClockForceStop, 0x40);
        
        // Data line status
        RTSXDeclareChipRegisterValue(kData3Status, 0x10);
        RTSXDeclareChipRegisterValue(kData2Status, 0x08);
        RTSXDeclareChipRegisterValue(kData1Status, 0x04);
        RTSXDeclareChipRegisterValue(kData0Status, 0x02);
        
        // Command line status
        RTSXDeclareChipRegisterValue(kCommandStatus, 0x01);
        
        RTSXDeclareChipRegisterValue(kAllLinesStatus, kCommandStatus |
                                                      kData0Status   |
                                                      kData1Status   |
                                                      kData2Status   |
                                                      kData3Status);
    }
    
    RTSXDeclareChipRegister(rPADCTL, 0xFDA6);
    namespace PADCTL
    {
        RTSXDeclareChipRegisterValue(kUse1d8V, 0x80);
        RTSXDeclareChipRegisterValue(kUse3d3V, 0x7F);
        RTSXDeclareChipRegisterValue(kUseTypeADriving, 0x00);
        RTSXDeclareChipRegisterValue(kUseTypeBDriving, 0x01);
        RTSXDeclareChipRegisterValue(kUseTypeCDriving, 0x00);
        RTSXDeclareChipRegisterValue(kUseTypeDDriving, 0x01);
    }
    
    // Sample point control
    RTSXDeclareChipRegister(rSPCTL, 0xFDA7);
    namespace SPCTL
    {
        RTSXDeclareChipRegisterValue(kDDRFixRxData, 0x00);
        RTSXDeclareChipRegisterValue(kDDRVarRxData, 0x80);
        RTSXDeclareChipRegisterValue(kDDRFixRxDataEdge, 0x00);
        RTSXDeclareChipRegisterValue(kDDRFixRxData14Delay, 0x40);
        
        RTSXDeclareChipRegisterValue(kDDRFixRxCommand, 0x00);
        RTSXDeclareChipRegisterValue(kDDRVarRxCommand, 0x20);
        RTSXDeclareChipRegisterValue(kDDRFixRxCommandPosEdge, 0x00);
        RTSXDeclareChipRegisterValue(kDDRFixRxCommand14Delay, 0x10);
        
        RTSXDeclareChipRegisterValue(kSD20RxPosEdge, 0x00);
        RTSXDeclareChipRegisterValue(kSD20Rx14Delay, 0x08);
        RTSXDeclareChipRegisterValue(kSD20RxSelMask, 0x08);
    }
    
    // Push point control
    RTSXDeclareChipRegister(rPPCTL, 0xFDA8);
    namespace PPCTL
    {
        RTSXDeclareChipRegisterValue(kDDRFixTxCommandData, 0x00);
        RTSXDeclareChipRegisterValue(kDDRVarTxCommandData, 0x80);
        
        RTSXDeclareChipRegisterValue(kDDRFixTxData14TSU, 0x00);
        RTSXDeclareChipRegisterValue(kDDRFixTxData12TSU, 0x40);
        
        RTSXDeclareChipRegisterValue(kDDRFixTxCommandNegEdge, 0x00);
        RTSXDeclareChipRegisterValue(kDDRFixTxCommand14Ahead, 0x20);
        
        RTSXDeclareChipRegisterValue(kSD20TxNegEdge, 0x00);
        RTSXDeclareChipRegisterValue(kSD20Tx14Ahead, 0x10);
        RTSXDeclareChipRegisterValue(kSD20TxSelMask, 0x10);
        
        RTSXDeclareChipRegisterValue(kSSDRVarSDClockPolSwap, 0x01);
    }
    
    RTSXDeclareChipRegister(rCMD0, 0xFDA9);
    RTSXDeclareChipRegister(rCMD1, 0xFDAA);
    RTSXDeclareChipRegister(rCMD2, 0xFDAB);
    RTSXDeclareChipRegister(rCMD3, 0xFDAC);
    RTSXDeclareChipRegister(rCMD4, 0xFDAD);
    RTSXDeclareChipRegister(rCMD5, 0xFDAE);
    
    RTSXDeclareChipRegister(rBYTECNTL, 0xFDAF);
    RTSXDeclareChipRegister(rBYTECNTH, 0xFDB0);
    RTSXDeclareChipRegister(rBLOCKCNTL, 0xFDB1);
    RTSXDeclareChipRegister(rBLOCKCNTH, 0xFDB2);
    
    RTSXDeclareChipRegister(rTRANSFER, 0xFDB3);
    namespace TRANSFER
    {
        /// Transfer Control
        RTSXDeclareChipRegisterValue(kTransferStart, 0x80);
        RTSXDeclareChipRegisterValue(kTransferEnd, 0x40);
        RTSXDeclareChipRegisterValue(kTransferIdle, 0x20);
        RTSXDeclareChipRegisterValue(kTransferError, 0x10);
        
        /// Write one or two bytes from SD_CMD2 and SD_CMD3 to the card
        RTSXDeclareChipRegisterValue(kTMNormalWrite, 0x00);
        
        /// Write `nblocks * blockSize` bytes from the ring buffer to the card
        RTSXDeclareChipRegisterValue(kTMAutoWrite3, 0x01);
        
        /// Write `nblocks * blockSize` bytes from the ring buffer to the card and send a CMD12 when done.
        /// The response to the CMD12 is written to the `SD_CMD{0-4}` registers.
        RTSXDeclareChipRegisterValue(kTMAutoWrite4, 0x02);
        
        /// Read `nblocks * blockSize` bytes from the card to the ring buffer
        RTSXDeclareChipRegisterValue(kTMAutoRead3, 0x05);
        
        /// Read `nblocks * blockSize` bytes from the card to the ring buffer and send a CMD12 when done.
        /// The response to the CMD12 is written to the `SD_CMD{0-4}` registers.
        RTSXDeclareChipRegisterValue(kTMAutoRead4, 0x06);
        
        /// Send an SD command as specified in the `SD_CMD{0-4}` registers to the card and
        /// put the 48-bit response into those registers as well.
        /// However, the 136-bit response is put into the ping-pond buffer 2 instead.
        RTSXDeclareChipRegisterValue(kTMCmdResp, 0x08);
        
        /// Send a write command, get the response from the card,
        /// write the data from the ring buffer to the card and send a CMD12 when done.
        /// The response to the CMD12 is written to the `SD_CMD{0-4}` registers.
        RTSXDeclareChipRegisterValue(kTMAutoWrite1, 0x09);
        
        /// Same as `kTMAutoWrite1` except that no CMD12 is sent.
        RTSXDeclareChipRegisterValue(kTMAutoWrite2, 0x0A);
        
        /// Send a read command and read up to 512 bytes (`nblocks * blockSize`)
        /// from the card to the ring buffer or the ping-pong buffer 2.
        RTSXDeclareChipRegisterValue(kTMNormalRead, 0x0C);
        
        /// Same as `kTMAutoWrite1` but perform a read operation
        RTSXDeclareChipRegisterValue(kTMAutoRead1, 0x0D);
        
        /// Same as `kTMAutoWrite2` but perform a read operation
        RTSXDeclareChipRegisterValue(kTMAutoRead2, 0x0E);
        
        /// Send a CMD19, receive a response and the tuning pattern from the card and report the result
        RTSXDeclareChipRegisterValue(kTMAutoTuning, 0x0F);
    }
    
    RTSXDeclareChipRegister(rCMDSTATE, 0xFDB5)
    namespace CMDSTATE
    {
        RTSXDeclareChipRegisterValue(kIdle, 0x80);
    }
    
    RTSXDeclareChipRegister(rDATSTATE, 0xFDB6);
    namespace DATSTATE
    {
        RTSXDeclareChipRegisterValue(kIdle, 0x80);
    }
}

namespace RTSX::Chip
{
    RTSXDeclareChipRegister(rDUMMY, 0xFE90);
    namespace DUMMY
    {
        RTSXDeclareChipRegisterValue(kICRevisionMask, 0x0F);
    }
    
    RTSXDeclareChipRegister(rVREF, 0xFE97);
    namespace VREF
    {
        RTSXDeclareChipRegisterValue(kEnablePwdSuspnd, 0x10);
    }
    
    RTSXDeclareChipRegister(rPCLKCTL, 0xFE55);
    namespace PCKLCTL
    {
        RTSXDeclareChipRegisterValue(kModeSelector, 0x20);
    }
    
    RTSXDeclareChipRegister(rPFCTL, 0xFE56);
    RTSXDeclareChipRegister(rPFCTL_52XA, 0xFF78);
    namespace PFCTL_52XA
    {
        RTSXDeclareChipRegisterValue(kEfuseBypass, 0x08);
        RTSXDeclareChipRegisterValue(kEfusePor, 0x04);
        RTSXDeclareChipRegisterValue(kEfusePowerMask, 0x03);
        RTSXDeclareChipRegisterValue(kEfusePowerOn, 0x03);
        RTSXDeclareChipRegisterValue(kEfusePowerOff, 0x00);
    }
    
    RTSXDeclareChipRegister(rAFCTL, 0xFE57);
    namespace AFCTL
    {
        RTSXDeclareChipRegisterValue(kCTL0, 0x10);
        RTSXDeclareChipRegisterValue(kCTL1, 0x20);
        RTSXDeclareChipRegisterValue(kValueMask, 0x03);
        RTSXDeclareChipRegisterValue(kDisableASPM, 0x00);
        RTSXDeclareChipRegisterValue(kEnableASPML0, 0x01);
        RTSXDeclareChipRegisterValue(kEnableASPML1, 0x02);
    }
    
    RTSXDeclareChipRegister(rPMCLKFCTL, 0xFE58);
    namespace PMCLKFCTL
    {
        RTSXDeclareChipRegisterValue(kEnableClockPM, 0x01);
    }
    
    RTSXDeclareChipRegister(rFCTL, 0xFE59);
    namespace FCTL
    {
        RTSXDeclareChipRegisterValue(kUpmeXmtDebug, 0x02);
    }
    
    RTSXDeclareChipRegister(rLINKSTA, 0xFE5B);
    
    RTSXDeclareChipRegister(rPGWIDTH, 0xFE5C);
    
    RTSXDeclareChipRegister(rHSSTA, 0xFE60);
    namespace HSSTA
    {
        RTSXDeclareChipRegisterValue(kHostWakeup, 0);
        RTSXDeclareChipRegisterValue(kHostEnterS1, 1);
        RTSXDeclareChipRegisterValue(kHostEnterS3, 2);
        RTSXDeclareChipRegisterValue(kMask, 3);
    }
    
    RTSXDeclareChipRegister(rPEDBG, 0xFE71);
    namespace PEDBG
    {
        RTSXDeclareChipRegisterValue(kDebug0, 0x08);
    }
    
    RTSXDeclareChipRegister(rNFTSTXCTL, 0xFE72);
}

namespace RTSX::Chip::L1SUB
{
    RTSXDeclareChipRegister(rCFG1, 0xFE8D);
    namespace CFG1
    {
        RTSXDeclareChipRegisterValue(kAuxClockActiveSelectorMask, 0x01);
        RTSXDeclareChipRegisterValue(kMacCkswDone, 0x00);
    }
    
    RTSXDeclareChipRegister(rCFG2, 0xFE8E);
    namespace CFG2
    {
        RTSXDeclareChipRegisterValue(kAutoConfig, 0x02);
    }
    
    RTSXDeclareChipRegister(rCFG3, 0xFE8F);
    namespace CFG3
    {
        RTSXDeclareChipRegisterValue(kMBIAS2Enable_5250, 1 << 7);
    }
}

namespace RTSX::Chip
{
    RTSXDeclareChipRegister(rPETXCFG, 0xFF03);
    namespace PETXCFT
    {
        RTSXDeclareChipRegisterValue(kForceClockRequestDelinkMask, 1 << 7);
        RTSXDeclareChipRegisterValue(kForceClockRequestLow, 0x80);
        RTSXDeclareChipRegisterValue(kForceClockRequestHigh, 0x00);
    }
    
    RTSXDeclareChipRegister(rRREFCFG, 0xFF6C);
    namespace RREFCFG
    {
        RTSXDeclareChipRegisterValue(kVBGSelectorMask, 0x38);
        RTSXDeclareChipRegisterValue(kVBGSelector1V25, 0x28);
    }
}

namespace RTSX::Chip::PM
{
    RTSXDeclareChipRegister(rCTRL1, 0xFF44);
    namespace CTRL1
    {
        RTSXDeclareChipRegisterValue(kCDResumeEnableMask, 0xF0);
    }
    
    RTSXDeclareChipRegister(rCTRL2, 0xFF45);
    
    RTSXDeclareChipRegister(rCTRL3, 0xFF46);
    namespace CTRL3
    {
        RTSXDeclareChipRegisterValue(kEnableSDIOSendPME, 0x80);
        RTSXDeclareChipRegisterValue(kForceRcModeOn, 0x40);
        RTSXDeclareChipRegisterValue(kForceRx50LinkOn, 0x20);
        RTSXDeclareChipRegisterValue(kEnableD3DelinkMode, 0x10);
        RTSXDeclareChipRegisterValue(kUsePesrtbControlDelink, 0x08);
        RTSXDeclareChipRegisterValue(kDelayPinWake, 0x04);
        RTSXDeclareChipRegisterValue(kResetPinWake, 0x02);
        RTSXDeclareChipRegisterValue(kEnablePmWake, 0x01);
    }
    RTSXDeclareChipRegister(rCTRL3_52XA, 0xFF7E); // RTS524A, RTS525A
    
    RTSXDeclareChipRegister(rCTRL4, 0xFF47);
}

namespace RTSX::Chip::LDO
{
    RTSXDeclareChipRegister(rPWRSEL, 0xFE78);
    
    namespace DV18
    {
        RTSXDeclareChipRegister(rCFG, 0xFF70);
        namespace CFG
        {
            RTSXDeclareChipRegisterValue(kSRMask, 0xC0);
            RTSXDeclareChipRegisterValue(kSRDefault, 0x40);
            RTSXDeclareChipRegisterValue(kDV331812Mask, 0x70);
            RTSXDeclareChipRegisterValue(kDV331812_3V3, 0x70);
            RTSXDeclareChipRegisterValue(kDV331812_1V7, 0x30);
        }
    }    
    
    RTSXDeclareChipRegister(rCFG2, 0xFF71);
    namespace CFG2
    {
        RTSXDeclareChipRegisterValue(kD3318Mask, 0x07);
        RTSXDeclareChipRegisterValue(kD33183d3V, 0x07);
        RTSXDeclareChipRegisterValue(kD33181d8V, 0x02);
        
        RTSXDeclareChipRegisterValue(kDV331812VDD1, 0x04);
        RTSXDeclareChipRegisterValue(kDV331812PowerOn, 0x08);
        RTSXDeclareChipRegisterValue(kDV331812PowerOff, 0x00);
    }
    
    namespace VCC
    {
        RTSXDeclareChipRegister(rCFG0, 0xFF72);
        namespace CFG0
        {
            RTSXDeclareChipRegisterValue(kLMTVTHMask, 0x30);
            RTSXDeclareChipRegisterValue(kLMTVTH2A, 0x10);
        }
        
        RTSXDeclareChipRegister(rCFG1, 0xFF73);
        namespace CFG1
        {
            RTSXDeclareChipRegisterValue(kRefTuneMask, 0x30);
            RTSXDeclareChipRegisterValue(kRef1d2V, 0x20);
            RTSXDeclareChipRegisterValue(kTuneMask, 0x07);
            RTSXDeclareChipRegisterValue(k1d8V, 0x04);
            RTSXDeclareChipRegisterValue(k3d3V, 0x07);
            RTSXDeclareChipRegisterValue(kLimitEnable, 0x08);
        }
    }
    
    namespace VIO
    {
        RTSXDeclareChipRegister(rCFG, 0xFF75);
        namespace CFG
        {
            RTSXDeclareChipRegisterValue(kSRMask, 0xC0);
            RTSXDeclareChipRegisterValue(kSRDefault, 0x40);
            RTSXDeclareChipRegisterValue(kRefTuneMask, 0x30);
            RTSXDeclareChipRegisterValue(kRef1d2V, 0x20);
            RTSXDeclareChipRegisterValue(kTuneMask, 0x07);
            RTSXDeclareChipRegisterValue(k1d7V, 0x03);
            RTSXDeclareChipRegisterValue(k1d8V, 0x04);
            RTSXDeclareChipRegisterValue(k3d3V, 0x07);
        }
    }
    
    namespace DV12S
    {
        RTSXDeclareChipRegister(rCFG, 0xFF76);
        namespace CFG
        {
            RTSXDeclareChipRegisterValue(kRef12TuneMask, 0x18);
            RTSXDeclareChipRegisterValue(kRef12TuneDefault, 0x10);
            RTSXDeclareChipRegisterValue(kD12TuneMask, 0x07);
            RTSXDeclareChipRegisterValue(kD12TuneDefault, 0x04);
        }
    }
    
    namespace AV12S
    {
        RTSXDeclareChipRegister(rCFG, 0xFF77);
        namespace CFG
        {
            RTSXDeclareChipRegisterValue(kTuneMask, 0x07);
            RTSXDeclareChipRegisterValue(kTuneDefault, 0x04);
        }
    }
    
    RTSXDeclareChipRegister(rCTL1, 0xFE7D);
    namespace CTL1
    {
        RTSXDeclareChipRegisterValue(kSD40VIOTuneMask, 0x70);
        RTSXDeclareChipRegisterValue(kSD40VIOTune1V7, 0x30);
        RTSXDeclareChipRegisterValue(kSDVIO1V8, 0x40);
        RTSXDeclareChipRegisterValue(kSDVIO3V3, 0x70);
    }    
}

namespace RTSX::Chip
{
    RTSXDeclareChipRegister(rCLKCFG3_525A, 0xFF79);
    namespace CLKCFG3_525A
    {
        RTSXDeclareChipRegisterValue(kMemPD, 0xF0);
    }
}

//
// MARK: - 3. PHY Registers
//

namespace RTSX::PHYS
{
    RTSXDeclarePhysRegister(rPCR, 0x00);
    namespace PCR
    {
        RTSXDeclarePhysRegisterValue(kForceCode, 0xB000);
        RTSXDeclarePhysRegisterValue(kOOBSCali50, 0x0800);
        RTSXDeclarePhysRegisterValue(kOOBSVCM08, 0x0200);
        RTSXDeclarePhysRegisterValue(kOOBSSEN90, 0x0040);
        RTSXDeclarePhysRegisterValue(kEnableRSSI, 0x0002);
        RTSXDeclarePhysRegisterValue(kRx10K, 0x0001);
    }
    
    /// Address of the register that is required to enable or disable OOBS polling
    RTSXDeclarePhysRegister(rRCR0, 0x01);
    namespace RCR0
    {
        /// Bit 9 is set to enable OOBS polling
        static constexpr UInt32 kOOBSControlBitIndex = 9;
    }
    
    RTSXDeclarePhysRegister(rRCR1, 0x02);
    namespace RCR1
    {
        RTSXDeclarePhysRegisterValue(kADPTime4, 0x0400);
        RTSXDeclarePhysRegisterValue(kVCOCoarse, 0x001F);
        RTSXDeclarePhysRegisterValue(kInit27s, 0x0A1F);
    }
    
    RTSXDeclarePhysRegister(rSSCCR2, 0x02);
    namespace SSCCR2
    {
        RTSXDeclarePhysRegisterValue(kPLLNcode, 0x0A00);
        RTSXDeclarePhysRegisterValue(kTime0, 0x001C);
        RTSXDeclarePhysRegisterValue(kTime2Width, 0x0003);
    }
    
    RTSXDeclarePhysRegister(rSSCCR3, 0x03);
    namespace SSCCR3
    {
        RTSXDeclarePhysRegisterValue(kStepIn, 0x2740);
        RTSXDeclarePhysRegisterValue(kCheckDelay, 0x0008);
    }
    
    RTSXDeclarePhysRegister(rANA3, 0x03);
    namespace ANA3
    {
        RTSXDeclarePhysRegisterValue(kTimerMax, 0x2700);
        RTSXDeclarePhysRegisterValue(kOOBSDebugEnable, 0x0040);
        RTSXDeclarePhysRegisterValue(kCMUDebugEnable, 0x0008);
    }
    
    RTSXDeclarePhysRegister(rRCR2, 0x03)
    namespace RCR2
    {
        RTSXDeclarePhysRegisterValue(kEnableEmphase, 0x8000);
        RTSXDeclarePhysRegisterValue(kNADJR, 0x4000);
        RTSXDeclarePhysRegisterValue(kCDRSR2, 0x0100);
        RTSXDeclarePhysRegisterValue(kFreqSel12, 0x0040);
        RTSXDeclarePhysRegisterValue(kCDRSC12P, 0x0010);
        RTSXDeclarePhysRegisterValue(kCalibLate, 0x0002);
        RTSXDeclarePhysRegisterValue(kInit27s, 0xC152);
    }
    
    RTSXDeclarePhysRegister(rRDR, 0x05);
    namespace RDR
    {
        RTSXDeclarePhysRegisterValue(kRxDSEL19, 0x4000);
        RTSXDeclarePhysRegisterValue(kSSCAutoPwd, 0x0600);
    }
    
    RTSXDeclarePhysRegister(rTUNE, 0x08);
    namespace TUNE
    {
        RTSXDeclarePhysRegisterValue(kTuneRef10, 0x4000);
        RTSXDeclarePhysRegisterValue(kVBGSel1252, 0x0C00);
        RTSXDeclarePhysRegisterValue(kSDBus33, 0x0200);
        RTSXDeclarePhysRegisterValue(kTuneD18, 0x01C0);
        RTSXDeclarePhysRegisterValue(kTuneD12, 0x0020);
        RTSXDeclarePhysRegisterValue(kTuneA12, 0x0004);
        RTSXDeclarePhysRegisterValue(kVoltageMask, 0xFC3F);
        RTSXDeclarePhysRegisterValue(kVoltage3V3, 0x03C0);
        RTSXDeclarePhysRegisterValue(kD18_1V8, 0x0100);
        RTSXDeclarePhysRegisterValue(kD18_1V7, 0x0080);
    }
    
    RTSXDeclarePhysRegister(rANA8, 0x08);
    namespace ANA8
    {
        RTSXDeclarePhysRegisterValue(kRxEQDCGain, 0x5000);
        RTSXDeclarePhysRegisterValue(kSelRxEnable, 0x0400);
        RTSXDeclarePhysRegisterValue(kRxEQValue, 0x03C0);
        RTSXDeclarePhysRegisterValue(kSCP, 0x0020);
        RTSXDeclarePhysRegisterValue(kSelIPI, 0x0004);
    }
    
    RTSXDeclarePhysRegister(rBPCR, 0x0A);
    namespace BPCR
    {
        RTSXDeclarePhysRegisterValue(kIBRxSel, 0x0400);
        RTSXDeclarePhysRegisterValue(kIBTxSel, 0x0100);
        RTSXDeclarePhysRegisterValue(kIBFilter, 0x0080);
        RTSXDeclarePhysRegisterValue(kEnableCMirror, 0x0040);
    }
    
    RTSXDeclarePhysRegister(rBACR, 0x11);
    namespace BACR
    {
        RTSXDeclarePhysRegisterValue(kBasicMask, 0xFFF3);
    }
    
    RTSXDeclarePhysRegister(rREV, 0x19);
    namespace REV
    {
        RTSXDeclarePhysRegisterValue(kRESV, 0xE000);
        RTSXDeclarePhysRegisterValue(kRxIdleLatched, 0x1000);
        RTSXDeclarePhysRegisterValue(kEnableP1, 0x0800);
        RTSXDeclarePhysRegisterValue(kEnableRxIdle, 0x0400);
        RTSXDeclarePhysRegisterValue(kEnableTxClockRequest, 0x0200);
        RTSXDeclarePhysRegisterValue(kEnableRxClockRequest, 0x0100);
        RTSXDeclarePhysRegisterValue(kClockRequestDT10, 0x0040);
        RTSXDeclarePhysRegisterValue(kStopClockRD, 0x0020);
        RTSXDeclarePhysRegisterValue(kRxPWST, 0x0008);
        RTSXDeclarePhysRegisterValue(kStopClockWR, 0x0004);
        
        /// Get the value for RTS5249
        static constexpr UInt16 value5249()
        {
            return kRESV | kRxIdleLatched | kEnableP1 | kEnableRxIdle |
                   kEnableTxClockRequest | kClockRequestDT10 | kStopClockRD | kRxPWST | kStopClockWR;
        }
    }
    
    RTSXDeclarePhysRegister(rREV0, 0x19);
    namespace REV0
    {
        RTSXDeclarePhysRegisterValue(kFilterOut, 0x3800);
        RTSXDeclarePhysRegisterValue(kCDRBypassPFD, 0x0100);
        RTSXDeclarePhysRegisterValue(kCDRRxIdleBypass, 0x0002);
    }
    
    RTSXDeclarePhysRegister(rANA1A, 0x1A);
    namespace ANA1A
    {
        RTSXDeclarePhysRegisterValue(kTXRLoopback, 0x2000);
        RTSXDeclarePhysRegisterValue(kRXTBist, 0x0500);
        RTSXDeclarePhysRegisterValue(kTXRBist, 0x0040);
        RTSXDeclarePhysRegisterValue(kRev, 0x0006);
        RTSXDeclarePhysRegisterValue(kInit27s, 0x2546);
    }
    
    RTSXDeclarePhysRegister(rANA1D, 0x1D);
    namespace ANA1D
    {
        RTSXDeclarePhysRegisterValue(kDebugAddress, 0x0004);
    }
    
    RTSXDeclarePhysRegister(rFLD0_525A, 0x1D);
    namespace FLD0_525A
    {
        RTSXDeclarePhysRegisterValue(kClkReq20c, 0x8000);
        RTSXDeclarePhysRegisterValue(kRxIdleEnable, 0x1000);
        RTSXDeclarePhysRegisterValue(kBitErrRstn, 0x0800);
        RTSXDeclarePhysRegisterValue(kBerCount, 0x01E0);
        RTSXDeclarePhysRegisterValue(kBerTimer, 0x001E);
        RTSXDeclarePhysRegisterValue(kCheckEnable, 0x0001);
    }
    
    RTSXDeclarePhysRegister(rFLD3, 0x1D);
    namespace FLD3
    {
        RTSXDeclarePhysRegisterValue(kTimer4, 0x0800);
        RTSXDeclarePhysRegisterValue(kTimer6, 0x0020);
        RTSXDeclarePhysRegisterValue(kRxDelink, 0x0004);
        RTSXDeclarePhysRegisterValue(kInit27s, 0x0004);
    }
    
    RTSXDeclarePhysRegister(rFLD4, 0x1E);
    namespace FLD4
    {
        RTSXDeclarePhysRegisterValue(kFLDENSel, 0x4000);
        RTSXDeclarePhysRegisterValue(kReqRef, 0x2000);
        RTSXDeclarePhysRegisterValue(kRxAmpOff, 0x1000);
        RTSXDeclarePhysRegisterValue(kReqADDA, 0x0800);
        RTSXDeclarePhysRegisterValue(kBerCount, 0x00E0);
        RTSXDeclarePhysRegisterValue(kBerTimer, 0x000A);
        RTSXDeclarePhysRegisterValue(kBerCheckEnable, 0x0001);
        RTSXDeclarePhysRegisterValue(kInit27s, 0x5C7F);
    }
    
    RTSXDeclarePhysRegister(rDIG1E, 0x1E);
    namespace DIG1E
    {
        RTSXDeclarePhysRegisterValue(kRev, 0x4000);
        RTSXDeclarePhysRegisterValue(kD0XD1, 0x1000);
        RTSXDeclarePhysRegisterValue(kRxOnHost, 0x0800);
        RTSXDeclarePhysRegisterValue(kRCLKRefHost, 0x0400);
        RTSXDeclarePhysRegisterValue(kRCLKTxEnableKeep, 0x0040);
        RTSXDeclarePhysRegisterValue(kRCLKTxTermKeep, 0x0020);
        RTSXDeclarePhysRegisterValue(kRCLKRxEIdleOn, 0x0010);
        RTSXDeclarePhysRegisterValue(kTxTermKeep, 0x0008);
        RTSXDeclarePhysRegisterValue(kRxTermKeep, 0x0004);
        RTSXDeclarePhysRegisterValue(kTxEnableKeep, 0x0002);
        RTSXDeclarePhysRegisterValue(kRxEnableKeep, 0x0001);
    }
}

//
// MARK: - 4. PCI Config Registers
//

namespace RTSX::PCR
{
    static constexpr IOByteCount kSREG1 = 0x724;
    static constexpr IOByteCount kSREG2 = 0x814;
    static constexpr IOByteCount kSREG3 = 0x747;
}

// PCI Config L1SS (From Linux)
#define PCI_L1SS_CTL1        0x08    /* Control 1 Register */
#define  PCI_L1SS_CTL1_PCIPM_L1_2    0x00000001  /* PCI-PM L1.2 Enable */
#define  PCI_L1SS_CTL1_PCIPM_L1_1    0x00000002  /* PCI-PM L1.1 Enable */
#define  PCI_L1SS_CTL1_ASPM_L1_2    0x00000004  /* ASPM L1.2 Enable */
#define  PCI_L1SS_CTL1_ASPM_L1_1    0x00000008  /* ASPM L1.1 Enable */
#define  PCI_L1SS_CTL1_L1_2_MASK    0x00000005
#define  PCI_L1SS_CTL1_L1SS_MASK    0x0000000f
#define  PCI_L1SS_CTL1_CM_RESTORE_TIME    0x0000ff00  /* Common_Mode_Restore_Time */
#define  PCI_L1SS_CTL1_LTR_L12_TH_VALUE    0x03ff0000  /* LTR_L1.2_THRESHOLD_Value */
#define  PCI_L1SS_CTL1_LTR_L12_TH_SCALE    0xe0000000  /* LTR_L1.2_THRESHOLD_Scale */

// PCIe Capability (From Linux)
#define PCI_EXP_DEVCTL2        40    /* Device Control 2 */
#define  PCI_EXP_DEVCTL2_COMP_TIMEOUT    0x000f    /* Completion Timeout Value */
#define  PCI_EXP_DEVCTL2_COMP_TMOUT_DIS    0x0010    /* Completion Timeout Disable */
#define  PCI_EXP_DEVCTL2_ARI        0x0020    /* Alternative Routing-ID */
#define  PCI_EXP_DEVCTL2_ATOMIC_REQ    0x0040    /* Set Atomic requests */
#define  PCI_EXP_DEVCTL2_ATOMIC_EGRESS_BLOCK 0x0080 /* Block atomic egress */
#define  PCI_EXP_DEVCTL2_IDO_REQ_EN    0x0100    /* Allow IDO for requests */
#define  PCI_EXP_DEVCTL2_IDO_CMP_EN    0x0200    /* Allow IDO for completions */
#define  PCI_EXP_DEVCTL2_LTR_EN        0x0400    /* Enable LTR mechanism */
#define  PCI_EXP_DEVCTL2_OBFF_MSGA_EN    0x2000    /* Enable OBFF Message type A */
#define  PCI_EXP_DEVCTL2_OBFF_MSGB_EN    0x4000    /* Enable OBFF Message type B */
#define  PCI_EXP_DEVCTL2_OBFF_WAKE_EN    0x6000    /* OBFF using WAKE# signaling */

#endif /* Registers_hpp */
