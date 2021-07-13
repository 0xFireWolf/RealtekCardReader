//
//  RealtekPCIeCardReaderController.hpp
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 2/18/21.
//

#ifndef RealtekPCIeCardReaderController_hpp
#define RealtekPCIeCardReaderController_hpp

#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/IOWorkLoop.h>
#include <IOKit/IOCommandGate.h>
#include <IOKit/IOMemoryDescriptor.h>
#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/IODMACommand.h>
#include <IOKit/IOInterruptEventSource.h>
#include <IOKit/IOFilterInterruptEventSource.h>
#include <IOKit/IOTimerEventSource.h>
#include "RealtekCardReaderController.hpp"
#include "Utilities.hpp"
#include "Registers.hpp"
#include "Debug.hpp"
#include "AppleSDXC.hpp"
#include "BitOptions.hpp"
#include "ClosedRange.hpp"

// TODO: Temp Definitions
// TODO: REMOVE THIS LATER
using ChipRegValuePair = RealtekCardReaderController::ChipRegValuePair;
using ChipRegValuePairs = RealtekCardReaderController::ChipRegValuePairs;
using SimpleRegValuePairs = RealtekCardReaderController::SimpleRegValuePairs;
using ContiguousRegValuePairsForReadAccess = RealtekCardReaderController::ContiguousRegValuePairsForReadAccess;
using ContiguousRegValuePairsForWriteAccess = RealtekCardReaderController::ContiguousRegValuePairsForWriteAccess;

// TODO: RELOCATED THIS
/// TX/RX clock phase
struct ClockPhase
{
    UInt8 sdr104, sdr50, ddr50;
    
    ClockPhase()
        : sdr104(0), sdr50(0), ddr50(0) {}
    
    ClockPhase(UInt8 sdr104, UInt8 sdr50, UInt8 ddr50)
        : sdr104(sdr104), sdr50(sdr50), ddr50(ddr50) {}
};

// TODO: RELOCATE THIS TO BASE CONTROLLER
/// The card output volage
enum class OutputVoltage: UInt32
{
    k3d3V, k1d8V
};

/// Enumerates all possible SSC depth values
enum class SSCDepth: UInt32
{
    k4M = 0x01,
    k2M = 0x02,
    k1M = 0x03,
    k500K = 0x04,
    k250K = 0x05
};

#define LTR_ACTIVE_LATENCY_DEF        0x883C
#define LTR_IDLE_LATENCY_DEF        0x892C
#define LTR_L1OFF_LATENCY_DEF        0x9003
#define L1_SNOOZE_DELAY_DEF        1
#define LTR_L1OFF_SSPWRGATE_522A_DEF        0x7F
#define LTR_L1OFF_SSPWRGATE_5249_DEF        0xAF
#define LTR_L1OFF_SSPWRGATE_5250_DEF        0xFF
#define LTR_L1OFF_SNOOZE_SSPWRGATE_522A_DEF    0x78
#define LTR_L1OFF_SNOOZE_SSPWRGATE_5249_DEF    0xAC
#define LTR_L1OFF_SNOOZE_SSPWRGATE_5250_DEF    0xF8

/// Forward Declaration (Client of the card reader controller)
class RealtekSDXCSlot;

///
/// Represents the abstract PCIe card reader controller
///
/// @note This is the base class of all device-specific controllers.
///
class RealtekPCIeCardReaderController: public AppleSDXC
{
    //
    // MARK: - Constructors & Destructors
    //
    
    OSDeclareAbstractStructors(RealtekPCIeCardReaderController);
    
    using super = AppleSDXC;
    
    //
    // MARK: - Constants
    //

    /// The minimum SSC clock divider value
    static constexpr UInt32 kMinSSCClockDividerN = 80;
    
    /// The maximum SSC clock divider value
    static constexpr UInt32 kMaxSSCClockDividerN = 208;
    
    /// The amount of time in microseconds to wait until the SSC clock becomes stable
    static constexpr UInt32 kWaitStableSSCClock = 130;
    
    //
    // MARK: - Private Data Structures
    //
    
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
    
    // TODO: COMM MOVE
    /// Represents a command in the host command buffer
    struct Command
    {
        /// All supported command types
        enum Type
        {
            kReadRegister = 0,
            kWriteRegister = 1,
            kCheckRegister = 2,
        };
        
        /// The 32-bit value that will be written to the command buffer
        UInt32 value;
        
        ///
        /// Create a command of the given type and arguments
        ///
        /// @param type The command type
        /// @param address The register address
        /// @param mask The register value mask (ignored if `type` is `kReadRegister`)
        /// @param value The register value (ignored if `type` is `kReadRegister`)
        ///
        Command(Type type, UInt16 address, UInt8 mask, UInt8 value)
        {
            this->value = 0;
            
            this->value |= static_cast<UInt32>(type & 0x03) << 30;
            
            this->value |= static_cast<UInt32>(address & 0x3FFF) << 16;
            
            this->value |= static_cast<UInt32>(mask) << 8;
            
            this->value |= static_cast<UInt32>(value);
        }
    };
    
    /// An entry in the driving table
    struct DrivingEntry
    {
        const UInt8 clock, command, data;
        
        DrivingEntry(UInt8 clock, UInt8 command, UInt8 data)
            : clock(clock), command(command), data(data) {}
    };
    
    /// The driving table
    struct DrivingTable
    {
        /// Type D, C, A, B
        const DrivingEntry entries[4];
    };
    
    /// IC Revision
    struct Revision
    {
        /// The underlying revision value
        enum class Value: UInt8
        {
            kA = 0, kB = 1, kC = 2, kD = 3, kUnknown = 0xFF
        } value;
        
        /// Parse the revision from the given value
        static inline Revision parse(UInt8 value)
        {
            switch (value)
            {
                case 0: return { Value::kA };
                case 1: return { Value::kB };
                case 2: return { Value::kC };
                case 3: return { Value::kD };
                default: return { Value::kUnknown };
            }
        }
        
        /// Get the string representation
        inline const char* stringify()
        {
            switch (this->value)
            {
                case Value::kA: return "A";
                case Value::kB: return "B";
                case Value::kC: return "C";
                case Value::kD: return "D";
                default: return "Unknown";
            }
        }
        
        /// Check whether the chip has a revision of A
        inline bool isRevA()
        {
            return this->value == Value::kA;
        }
        
        /// Check whether the chip has a revision of B
        inline bool isRevB()
        {
            return this->value == Value::kB;
        }
        
        /// Check whether the chip has a revision of C
        inline bool isRevC()
        {
            return this->value == Value::kC;
        }
        
        /// Check whether the chip has a revision of D
        inline bool isRevD()
        {
            return this->value == Value::kD;
        }
    };
      
    /// Device-specific parameters
    struct Parameters
    {
        /// IC revision
        Revision revision;
        
        /// The number of slots
        UInt8 numSlots;
     
        /// Host capabilities
        struct Capabilities
        {
            /// Supports SD UHS SDR50 mode
            bool supportsSDSDR50: 1;
            
            /// Supports SD UHS SDR104 mode
            bool supportsSDSDR104: 1;
            
            /// Supports SD UHS DDR50 mode
            bool supportsSDDDR50: 1;
            
            /// Supports MMC DDR 1.8V
            bool supportsMMCDDR1d8V: 1;
            
            /// Supports 8 bit transfers
            bool supportsMMC8BitTransfer: 1;
            
            /// Do not send MMC commands during initialization
            bool noMMCCommandsDuringInitialization: 1;
            
            /// Supports SD Express
            bool supportsSDExpress: 1;
            
            /// Reserved Bits
            bool reserved: 1;
            
        } caps;
        
        /// True if the socket is reversed
        bool isSocketReversed;
        
        /// Card drive selector
        UInt8 cardDriveSelector;
        
        /// SD 3.0 drive selector (1.8V)
        UInt8 sd30DriveSelector1d8V;
        
        /// SD 3.0 drive selector (3.3V)
        UInt8 sd30DriveSelector3d3V;
        
        /// SD 3.0 drive table (1.8V) (Nullable)
        const DrivingTable* sd30DriveTable1d8V;
        
        /// SD 3.0 drive table (3.3V) (Nullable)
        const DrivingTable* sd30DriveTable3d3V;
        
        /// A non-null sequence of chip registers to enable SD pull control
        const SimpleRegValuePairs* sdEnablePullControlTable;
            
        /// A non-null sequence of chip registers to disable SD pull control
        const SimpleRegValuePairs* sdDisablePullControlTable;
        
        /// Initial TX clock phase
        ClockPhase initialTxClockPhase;
        
        /// Initial RX clock phase
        ClockPhase initialRxClockPhase;
        
        /// Overcurrent Protection Parameters
        struct OCP
        {
            /// Set to `true` if overcurrent protection should be enabled
            bool enable;
            
            /// Total harmonic distortion value (400mA)
            UInt8 sdTHD400mA;
            
            /// Total harmonic distortion value (800mA)
            UInt8 sdTHD800mA;
            
            /// Glitch value
            UInt8 sdGlitch;
        } ocp;
        
        /// Power Management
        struct PowerManagement
        {
            /// `True` if ASPM L1 is enabled
            bool isASPML1Enabled;
            
            /// `True` if ASPM is enabled
            bool isASPMEnabled;
            
            /// Enable PCI-PM L1.1
            bool enablePCIPML11;
            
            /// Enable PCI-PM L1.2
            bool enablePCIPML12;
            
            /// Enable ASPM L1.1
            bool enableASPML11;
            
            /// Enable ASPM L1.2
            bool enableASPML12;
            
            /// Enable LTR L1SS power gate
            bool enableLTRL1SSPowerGate;
            
            /// Enable LTR L1SS power gate and card check
            bool enableLTRL1SSPowerGateCheckCard;
            
            /// Enable L1 snooze test
            bool enableL1SnoozeTest;
            
            /// Force clock request
            bool forceClockRequest;
            
            /// Enable LTR mode
            bool enableLTRMode;
            
            /// Is LTR mode enabled
            bool isLTRModeEnabled;
            
            /// Is LTR mode active
            bool isLTRModeActive;
            
            /// LTR mode active latency
            UInt32 ltrActiveLatency;
            
            /// LTR mode idle latency
            UInt32 ltrIdleLatency;
            
            /// LTR mode L1 off latency
            UInt32 ltrL1OffLatency;
            
            /// LTR snooze delay
            UInt32 ltrSnoozeDelay;
            
            /// LTR L1 off substate power gate
            UInt8 ltrL1OffSSPowerGate;
            
            /// LTR L1 off snooze substate power gate
            UInt8 ltrL1OffSnoozeSSPowerGate;
        } pm;
    };
    
    //
    // MARK: - IOKit Basics
    //

    /// The PCIe card reader device (provider)
    IOPCIDevice* device;
    
    // TODO: DEPRECATED
    /// The SD card slot (client)
    RealtekSDXCSlot* slot;
    
    /// A mapping for the device memory
    IOMemoryMap* deviceMemoryMap;
    
    /// Device memory at BARx where x may vary device
    IOMemoryDescriptor* deviceMemoryDescriptor;
    
    // TODO: DEPRECATED
    /// Protect shared resources and serialize operations
    IOWorkLoop* workLoop;
    
    // TODO: DEPRECATED
    ///
    /// A command gate to serialize executions with respect to the work loop
    ///
    /// @note This command gate is not associated with any specific actions.
    ///       The caller should use `runAction` to execute a function in a gated context.
    /// @note The target action is still executed in the context of the calling thread.
    ///
    IOCommandGate* commandGate;
    
    /// An event source that delivers the hardware interrupt
    IOFilterInterruptEventSource* interruptEventSource;
    
    // MARK: - Host Command & Data Buffer
    
    /// The host buffer size (by default 4096 bytes)
    static constexpr IOByteCount kHostBufferSize = RTSX::MMIO::HCBAR::kMaxNumCmds * 4 + RTSX::MMIO::HDBAR::kMaxNumElements * 8;
    
    /// The host command buffer starts at the offset 0 in the host buffer
    static constexpr IOByteCount kHostCommandBufferOffset = 0;
    
    /// The host data buffer starts at the offset 1024 in the host buffer
    static constexpr IOByteCount kHostDatabufferOffset = RTSX::MMIO::HCBAR::kMaxNumCmds * 4;
    
    /// The maximum number of DMA segments supported by the card reader
    static constexpr IOItemCount kMaxNumSegments = 256;
    
    /// The maximum number of DMA transfer failures until the host should reduce the card clock
    static constexpr IOItemCount kMaxDMATransferFailures = 8;
    
    // TODO: DEPRECATED
    ///
    /// A descriptor that allocates the host command and data buffer
    ///
    /// @note The buffer size is 4 KB, in which
    ///       1 KB is reserved for the host command buffer, allowing 256 commands to be queued;
    ///       3 KB is reserved for the host data buffer, allowing 384 scatter/gather list items to be queued.
    /// @note The buffer remains "prepared" until the driver is stopped by the system.
    ///       The lifecycle is managed by `setupHostBuffer()` and `tearDownHostBuffer()`.
    /// @note The buffer direction is set to be `kIODirectionInOut`.
    ///
    IOBufferMemoryDescriptor* hostBufferDescriptor;
    
    ///
    /// Translate the host buffer address to I/O bus address
    ///
    /// @note The command remains "prepared" until the driver is stopped by the system.
    ///       The lifecycle is managed by `setupHostBuffer()` and `tearDownHostBuffer()`.
    /// @note The command remains associated with the host buffer descriptor,
    ///       and data is always synchronized with that descriptor.
    /// @note Accesses to the host buffer must be done via `IODMACommand::write/readBytes()`.
    ///
    IODMACommand* hostBufferDMACommand;
    
    /// A timer that runs the timeout handler for the current buffer transfer session
    IOTimerEventSource* hostBufferTimer;
    
    ///
    /// The base address of the host buffer that can be used by the DMA engine
    ///
    /// @note The command buffer starts at offset 0, while the data buffer starts at offset 1024.
    ///
    IOPhysicalAddress32 hostBufferAddress;
    
    // TODO: DEPRECATED
    /// The number of commands in the host command buffer
    IOItemCount hostCommandCount;
    
    ///
    /// Status of the current buffer transfer session
    ///
    /// @note The status is reset by the buffer transfer routines and modified by the timeout handler or the interrupt handler.
    ///       `kIOReturnNotReady` if the transfer has not yet started;
    ///       `kIOReturnSuccess` if the transfer has succeeded;
    ///       `kIOReturnError` if the transfer has failed;
    ///       `kIOReturnTimeout` if the transfer has timed out.
    ///
    IOReturn hostBufferTransferStatus;
    
    //
    // MARK: - Device Specific Properties
    //
    
    /// Device-specific paramters
    Parameters parameters;
    
    //
    // MARK: - Host States
    //
    
    /// The current SSC clock in MHz
    UInt32 currentSSCClock;
    
    /// The number of DMA errors
    UInt32 dmaErrorCounter;
    
    /// `True` if the host is idle
    bool isIdle;
  
    //
    // MARK: - Query Controller Capabilities
    //
    
public:
    /// Check whether the card reader supports the UHS-I SDR50 mode
    inline bool supportsSDR50()
    {
        return this->parameters.caps.supportsSDSDR50;
    }
    
    /// Check whether the card reader supports the UHS-I SDR104 mode
    inline bool supportsSDR104()
    {
        return this->parameters.caps.supportsSDSDR104;
    }
    
    /// Check whether the card reader supports the UHS-I DDR50 mode
    inline bool supportsDDR50()
    {
        return this->parameters.caps.supportsSDDDR50;
    }
    
    //
    // MARK: - Access Memory Mapped Registers (Common, Final)
    //
    
private:
    ///
    /// Read from a memory mapped register at the given address
    ///
    /// @param address The register address
    /// @return The register value.
    ///
    template <typename T>
    T readRegister(UInt32 address)
    {
        T value;
        
        psoftassert(this->deviceMemoryDescriptor->readBytes(address, &value, sizeof(T)) == sizeof(T),
                    "Failed to read %lu bytes from the register at 0x%x.", sizeof(T), address);
        
        return value;
    }
    
    ///
    /// Write to a memory mapped register at the given address
    ///
    /// @param address The register address
    /// @param value The register value
    ///
    template <typename T>
    void writeRegister(UInt32 address, T value)
    {
        psoftassert(this->deviceMemoryDescriptor->writeBytes(address, &value, sizeof(T)) == sizeof(T),
                    "Failed to write %lu bytes to the register at 0x%x.", sizeof(T), address);
    }
    
public:
    ///
    /// Read a byte from a memory mapped register at the given address
    ///
    /// @param address The register address
    /// @return The register value.
    /// @note Port: This function replaces the macro `rtsx_pci_readb()` defined in `rtsx_pci.h`.
    ///
    UInt8 readRegister8(UInt32 address);
    
    ///
    /// Read a word from a memory mapped register at the given address
    ///
    /// @param address The register address
    /// @return The register value.
    /// @note Port: This function replaces the macro `rtsx_pci_readw()` defined in `rtsx_pci.h`.
    ///
    UInt16 readRegister16(UInt32 address);
    
    ///
    /// Read a double word from a memory mapped register at the given address
    ///
    /// @param address The register address
    /// @return The register value.
    /// @note Port: This function replaces the macro `rtsx_pci_readl()` defined in `rtsx_pci.h`.
    ///
    UInt32 readRegister32(UInt32 address);
    
    ///
    /// Write a byte to a memory mapped register at the given address
    ///
    /// @param address The register address
    /// @param value The register value
    /// @note Port: This function replaces the macro `rtsx_pci_writeb()` defined in `rtsx_pci.h`.
    ///
    void writeRegister8(UInt32 address, UInt8 value);
    
    ///
    /// Write a word to a memory mapped register at the given address
    ///
    /// @param address The register address
    /// @param value The register value
    /// @note Port: This function replaces the macro `rtsx_pci_writew()` defined in `rtsx_pci.h`.
    ///
    void writeRegister16(UInt32 address, UInt16 value);
    
    ///
    /// Write a double word to a memory mapped register at the given address
    ///
    /// @param address The register address
    /// @param value The register value
    /// @note Port: This function replaces the macro `rtsx_pci_writel()` defined in `rtsx_pci.h`.
    ///
    void writeRegister32(UInt32 address, UInt32 value);
    
    //
    // MARK: - Access Chip Registers (Common, Final)
    //
    
    ///
    /// Read a byte from the chip register at the given address
    ///
    /// @param address The register address
    /// @param value The register value on return
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out.
    /// @note Port: This function replaces `rtsx_pci_read_register()` defined in `rtsx_psr.c`.
    ///
    IOReturn readChipRegister(UInt16 address, UInt8& value);
    
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
    IOReturn writeChipRegister(UInt16 address, UInt8 mask, UInt8 value);
    
    ///
    /// Write to multiple chip registers conveniently
    ///
    /// @param pairs A sequence of pairs of register address and value
    /// @return `kIOReturnSuccess` on success;
    ///         `kIOReturnTimeout` if timed out;
    ///         `kIOReturnError` if the new register value is not `value`.
    ///
    IOReturn writeChipRegisters(const ChipRegValuePairs& pairs);
    
    // TODO: DEPRECATED
    ///
    /// Dump the chip registers in the given range
    ///
    /// @param range The range of register addresses
    ///
    void dumpChipRegisters(ClosedRange<UInt16> range);
    
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
    virtual IOReturn readPhysRegister(UInt8 address, UInt16& value);
    
    ///
    /// Write a word to the physical layer register at the given address
    ///
    /// @param address The register address
    /// @param value The register value
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    /// @note Port: This function replaces `rtsx_pci_write_phy_register()` defined in `rtsx_psr.c`.
    /// @note Subclasses may override this function to provide a device-specific implementation.
    ///
    virtual IOReturn writePhysRegister(UInt8 address, UInt16 value);
    
    ///
    /// Write to multiple physical layer registers conveniently
    ///
    /// @param pairs An array of registers and their values
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    ///
    IOReturn writePhysRegisters(const PhysRegValuePair* pairs, size_t count);
    
    ///
    /// Write to multiple physical layer registers conveniently
    ///
    /// @param pairs An array of registers and their values
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    ///
    template <size_t N>
    IOReturn writePhysRegisters(const PhysRegValuePair (&pairs)[N])
    {
        return this->writePhysRegisters(pairs, N);
    }
    
    /// The type of a function that modifies the given physical register value
    using PhysRegValueModifier = UInt16 (*)(UInt16, const void*);
    
    ///
    /// Modify a physical layer register conveniently
    ///
    /// @param address The register address
    /// @param modifier A modifier that takes the current register value and return the new value
    /// @param context A nullable context that will be passed to the modifier function
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    /// @note Port: This function replaces `rtsx_pci_update_phy()` defined in `rtsx_pci.h`.
    ///
    IOReturn modifyPhysRegister(UInt8 address, PhysRegValueModifier modifier, const void* context = nullptr);
    
    ///
    /// Set a bit at the given index in a physical layer register conveniently
    ///
    /// @param address The register address
    /// @param index The zero-based bit index
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    ///
    IOReturn setPhysRegisterBitAtIndex(UInt8 address, UInt32 index);
    
    ///
    /// Clear a bit at the given index in a physical layer register conveniently
    ///
    /// @param address The register address
    /// @param index The zero-based bit index
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    ///
    IOReturn clearPhysRegisterBitAtIndex(UInt8 address, UInt32 index);
    
    ///
    /// Append the given value to a physical layer register conveniently
    ///
    /// @param address The register address
    /// @param mask The register value mask
    /// @param append The value to be appended to the current value
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    /// @note Port: This function replaces `rtsx_pci_update_phy()` defined in `rtsx_pci.h`.
    ///
    IOReturn appendPhysRegister(UInt8 address, UInt16 mask, UInt16 append);
    
    ///
    /// Append each given value to the corresponding physical layer register conveniently
    ///
    /// @param pairs An array of registers and their masks and values
    /// @param count The number of elements in the array
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    ///
    IOReturn appendPhysRegisters(const PhysRegMaskValuePair* pairs, size_t count);
    
    ///
    /// Append each given value to the corresponding physical layer register conveniently
    ///
    /// @param pairs An array of registers and their masks and values
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    ///
    template <size_t N>
    IOReturn appendPhysRegisters(const PhysRegMaskValuePair (&pairs)[N])
    {
        return this->appendPhysRegisters(pairs, N);
    }
    
    //
    // MARK: - Host Buffer Management
    //
    
    // TODO: DEPRECATED
    ///
    /// Read from the host buffer into the given buffer
    ///
    /// @param offset A byte offset into the host buffer
    /// @param buffer A non-null buffer
    /// @param length The number of bytes to read
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note This function coordinates all accesses to the host buffer.
    ///
    IOReturn readHostBuffer(IOByteCount offset, void* buffer, IOByteCount length);
    
    // TODO: DEPRECATED
    ///
    /// Write to the host buffer form the given buffer
    ///
    /// @param offset A byte offset into the host buffer
    /// @param buffer A non-null buffer
    /// @param length The number of bytes to write
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note This function coordinates all accesses to the host buffer.
    ///
    IOReturn writeHostBuffer(IOByteCount offset, const void* buffer, IOByteCount length);
    
    ///
    /// Write to the host data buffer form the given buffer conveniently
    ///
    /// @param offset A byte offset into the host data buffer
    /// @param buffer A non-null buffer
    /// @param length The number of bytes to write
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note This function coordinates all accesses to the host data buffer.
    /// @warning The first argument `offset` is relative to the start address of the host data buffer.
    ///          i.e. If `offset` is 5, this function will start to read from the offset `1024 + 5` in the host buffer.
    ///
    inline IOReturn writeHostDataBuffer(IOByteCount offset, const void* buffer, IOByteCount length)
    {
        return this->writeHostBuffer(RealtekPCIeCardReaderController::kHostDatabufferOffset + offset, buffer, length);
    }
    
    // TODO: DEPRECATED
    ///
    /// Peek a numeric value in the host buffer conveniently
    ///
    /// @tparam T Specify a numeric type, e.g., S/UInt{8, 16, 32, 64}
    /// @param offset A byte offset into the host command buffer
    /// @return The numeric value.
    /// @note This function will trigger a kernel panic if the given offset is invalid.
    ///
    template <typename T>
    T peekHostBuffer(IOByteCount offset)
    {
        T value;
        
        passert(this->readHostBuffer(offset, &value, sizeof(T)) == kIOReturnSuccess,
                "Failed to peek %lu bytes at offset %llu.", sizeof(T), offset);
        
        return value;
    }
    
    // TODO: DEPRECATED
    ///
    /// Dump the host buffer contents
    ///
    /// @param offset A byte offset into the host data buffer
    /// @param length The number of bytes to dump
    /// @param column The number of columns to print
    ///
    void dumpHostBuffer(IOByteCount offset, IOByteCount length, IOByteCount column = 8);
    
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
    IOReturn beginCommandTransfer();
    
    ///
    /// Enqueue a command
    ///
    /// @param command The command
    /// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full, `kIOReturnError` otherwise.
    /// @note Port: This function replaces `rtsx_pci_add_cmd()` defined in `rtsx_psr.c`.
    ///
    IOReturn enqueueCommand(const Command& command);
    
    // TODO: DEPRECATED
    ///
    /// Enqueue a command to read the register at the given address conveniently
    ///
    /// @param address The register address
    /// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full.
    /// @note Port: This function replaces `rtsx_pci_add_cmd()` defined in `rtsx_psr.c`.
    ///
    IOReturn enqueueReadRegisterCommand(UInt16 address);
    
    // TODO: DEPRECATED
    ///
    /// Enqueue a command to write a value to the register at the given address conveniently
    ///
    /// @param address The register address
    /// @param mask The register value mask
    /// @param value The register value
    /// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full.
    /// @note Port: This function replaces `rtsx_pci_add_cmd()` defined in `rtsx_psr.c`.
    ///
    IOReturn enqueueWriteRegisterCommand(UInt16 address, UInt8 mask, UInt8 value);
    
    // TODO: DEPRECATED
    ///
    /// Enqueue a command to check the value of the register at the given address conveniently
    ///
    /// @param address The register address
    /// @param mask The register value mask
    /// @param value The register value
    /// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full.
    /// @note Port: This function replaces `rtsx_pci_add_cmd()` defined in `rtsx_psr.c`.
    ///
    IOReturn enqueueCheckRegisterCommand(UInt16 address, UInt8 mask, UInt8 value);
    
    ///
    /// Finish the existing host command transfer session
    ///
    /// @param timeout Specify the amount of time in milliseconds
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    /// @note Port: This function replaces `rtsx_pci_send_cmd()` defined in `rtsx_psr.c`.
    /// @note This function sends all commands in the queue to the device.
    ///
    IOReturn endCommandTransfer(UInt32 timeout = 100);
    
    ///
    /// Finish the existing host command transfer session without waiting for the completion interrupt
    ///
    /// @note Port: This function replaces `rtsx_pci_send_cmd_no_wait()` defined in `rtsx_psr.c`.
    /// @note This function sends all commands in the queue to the device.
    ///
    void endCommandTransferNoWait();
    
    // TODO: DEPRECATED
    ///
    /// Enqueue a sequence of commands to read registers conveniently
    ///
    /// @param pairs A sequence of pairs of register address and value
    /// @return `kIOReturnSuccess` on success, `kIOReturnError` otherwise.
    /// @note This function provides an elegant way to enqueue multiple commands and handle errors.
    ///
    IOReturn enqueueReadRegisterCommands(const ChipRegValuePairs& pairs);
    
    // TODO: DEPRECATED
    ///
    /// Enqueue a sequence of commands to write registers conveniently
    ///
    /// @param pairs A sequence of pairs of register address and value
    /// @return `kIOReturnSuccess` on success, `kIOReturnError` otherwise.
    /// @note This function provides an elegant way to enqueue multiple commands and handle errors.
    ///
    IOReturn enqueueWriteRegisterCommands(const ChipRegValuePairs& pairs);
    
    // TODO: DEPRECATED
    ///
    /// Transfer a sequence of commands to read registers conveniently
    ///
    /// @param pairs A sequence of pairs of register address and value
    /// @param timeout Specify the amount of time in milliseconds
    /// @return `kIOReturnSuccess` on success, `kIOReturnError` otherwise.
    /// @note This function provides an elegant way to start a command transfer session and handle errors.
    ///       Same as calling `startCommandTransfer`, a sequence of `enqueueReadRegisterCommand` and `endCommandTransfer`.
    ///
    IOReturn transferReadRegisterCommands(const ChipRegValuePairs& pairs, UInt32 timeout = 100);
    
    // TODO: DEPRECATED
    ///
    /// Transfer a sequence of commands to write registers conveniently
    ///
    /// @param pairs A sequence of pairs of register address and value
    /// @param timeout Specify the amount of time in milliseconds
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    /// @note This function provides an elegant way to start a command transfer session and handle errors.
    ///       Same as calling `startCommandTransfer`, a sequence of `enqueueWriteRegisterCommand` and `endCommandTransfer`.
    ///
    IOReturn transferWriteRegisterCommands(const ChipRegValuePairs& pairs, UInt32 timeout = 100);
    
    ///
    /// Tell the device to stop transferring commands
    ///
    /// @note Port: This function replaces `rtsx_pci_stop_cmd()` defined in `rtsx_psr.c`.
    /// @note Subclasses may override this function to provide device-specific implementation.
    ///
    virtual IOReturn stopTransfer();
    
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
    virtual UInt64 transformIOVMSegment(IODMACommand::Segment32 segment);
    
    ///
    /// [Helper] Generate a physical scather/gather list from the given DMA command and enqueue all entries into the host data buffer
    ///
    /// @param command A non-null, perpared DMA command
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This helper function replaces `rtsx_pci_add_sg_tbl()` defined in `rtsx_psr.c`.
    /// @warning The caller must ensure that the given instance of `IODMACommand` is prepared.
    ///
    IOReturn enqueueDMACommand(IODMACommand* command);
    
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
    IOReturn performDMATransfer(IODMACommand* command, UInt32 timeout, UInt32 control);
    
    ///
    /// Perform a DMA read operation
    ///
    /// @param command A non-null, perpared DMA command
    /// @param timeout Specify the amount of time in milliseconds
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    ///
    IOReturn performDMARead(IODMACommand* command, UInt32 timeout);
    
    ///
    /// Perform a DMA write operation
    ///
    /// @param command A non-null, perpared DMA command
    /// @param timeout Specify the amount of time in milliseconds
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    ///
    IOReturn performDMAWrite(IODMACommand* command, UInt32 timeout);
    
    //
    // MARK: - LED Management
    //
    
    ///
    /// ------------------------------------------------------------
    /// | Controller | LED Toggle Register | LED Blinking Register |
    /// ------------------------------------------------------------
    /// |    5209    |      CARD_GPIO      |    CARD_AUTO_BLINK    |
    /// |    5227    |      GPIO_CTL       |      OLT_LED_CTL      |
    /// |    522A    |      GPIO_CTL       |      OLT_LED_CTL      |
    /// |    5228    |      GPIO_CTL       |      OLT_LED_CTL      |
    /// |    5229    |      GPIO_CTL       |      OLT_LED_CTL      |
    /// |    5249    |      GPIO_CTL       |      OLT_LED_CTL      |
    /// |    524A    |      GPIO_CTL       |      OLT_LED_CTL      |
    /// |    525A    |      GPIO_CTL       |      OLT_LED_CTL      |
    /// |    5260    |  RTS5260_GPIO_CTL0  |      OLT_LED_CTL      |
    /// |    5261    |      GPIO_CTL       |      OLT_LED_CTL      |
    /// |    5286    |      CARD_GPIO      |    CARD_AUTO_BLINK    |
    /// |    5287    |      CARD_GPIO      |    CARD_AUTO_BLINK    |
    /// |    5289    |      CARD_GPIO      |    CARD_AUTO_BLINK    |
    /// ------------------------------------------------------------
    ///
    
    ///
    /// Turn on the LED
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    /// @note Port: This function replaces `turn_on_led()` defined in `struct pcr_ops`.
    ///             The base controller class implements this function by changing the value of `GPIO_CTL`.
    ///             RTS5209, 5260, 5286, 5287 and 5289 controllers must override this function.
    ///
    virtual IOReturn turnOnLED();
    
    ///
    /// Turn off the LED
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    /// @note Port: This function replaces `turn_off_led()` defined in `struct pcr_ops`.
    ///             The base controller class implements this function by changing the value of `GPIO_CTL`.
    ///             RTS5209, 5286, 5287 and 5289 controllers must override this function.
    ///
    virtual IOReturn turnOffLED();
    
    ///
    /// Enable LED blinking
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    /// @note Port: This function replaces `enable_auto_blink()` defined in `struct pcr_ops`.
    ///             The base controller class implements this function by changing the value of `OLT_LED_CTL`.
    ///             RTS5209, 5286, 5287 and 5289 controllers must override this function.
    ///
    virtual IOReturn enableLEDBlinking();
    
    ///
    /// Disable LED blinking
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    /// @note Port: This function replaces `enable_auto_blink()` defined in `struct pcr_ops`.
    ///             The base controller class implements this function by changing the value of `OLT_LED_CTL`.
    ///             RTS5209, 5286, 5287 and 5289 controllers must override this function.
    ///
    virtual IOReturn disableLEDBlinking();
    
    //
    // MARK: - Card Power Management
    //
    
    ///
    /// Power on the SD card
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_pci_card_power_on()` defined in `rtsx_psr.c`.
    ///
    virtual IOReturn powerOnCard() = 0;
    
    ///
    /// Power off the SD card
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_pci_card_power_off()` defined in `rtsx_psr.c`.
    ///
    virtual IOReturn powerOffCard() = 0;
    
    ///
    /// Switch to the given output voltage
    ///
    /// @param outputVoltage The new output voltage
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_pci_switch_output_voltage()` defined in `rtsx_psr.c`.
    ///
    virtual IOReturn switchOutputVoltage(OutputVoltage outputVoltage) = 0;
    
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
    virtual IOReturn setDrivingForOutputVoltage(OutputVoltage outputVoltage, bool intermediate, UInt32 timeout);
    
    //
    // MARK: - Card Clock Configurations
    //
    
    // TODO: COMM
    ///
    /// Adjust the card clock if DMA transfer errors occurred
    ///
    /// @param cardClock The current card clock
    /// @return The adjusted card clock.
    /// @note Port: This function replaces the code block that reduces the card clock in `rtsx_pci_switch_clock()` defined in `rtsx_psr.c`.
    ///             By default, this function does not adjust the card clock and return the given clock.
    ///             RTS5227 controller must override this function.
    ///
    virtual UInt32 getAdjustedCardClockOnDMAError(UInt32 cardClock);
    
    ///
    /// Convert the given SSC clock to the divider N
    ///
    /// @param clock The SSC clock in MHz
    /// @return The divider N.
    /// @note Port: This function replaces `conv_clk_and_div_n()` defined in `rtsx_psr.c`.
    ///             RTL8411 series controllers must override this function.
    ///
    virtual UInt32 sscClock2DividerN(UInt32 clock);
    
    ///
    /// Convert the given divider N back to the SSC clock
    ///
    /// @param n The divider N
    /// @return The SSC clock in MHz.
    /// @note Port: This function replaces `conv_clk_and_div_n()` defined in `rtsx_psr.c`.
    ///             RTL8411 series controllers must override this function.
    ///
    virtual UInt32 dividerN2SSCClock(UInt32 n);
    
    // TODO: COMM
    ///
    /// Switch to the given card clock
    ///
    /// @param cardClock The card clock in Hz
    /// @param sscDepth The SSC depth value
    /// @param initialMode Pass `true` if the card is at its initial stage
    /// @param doubleClock Pass `true` if the SSC clock should be doubled
    /// @param vpclock Pass `true` if VPCLOCK is used
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_pci_switch_clock()` defined in `rtsx_psr.c`.
    ///             RTS5261 and RTS5228 controllers must override this function.
    ///
    virtual IOReturn switchCardClock(UInt32 cardClock, SSCDepth sscDepth, bool initialMode, bool doubleClock, bool vpclock);
    
    //
    // MARK: - Card Detection and Write Protection
    //
    
    // TODO: COMM
    ///
    /// Check whether the card has write protection enabled
    ///
    /// @return `true` if the card is write protected, `false` otherwise.
    ///
    bool isCardWriteProtected();
    
    // TODO: COMM
    ///
    /// Check whether a card is present
    ///
    /// @return `true` if a card exists, `false` otherwise.
    /// @note Port: This function replaces `rtsx_pci_card_exist()` and `cd_deglitch()` defined in `rtsx_psr.c`.
    /// @warning: This function supports SD cards only.
    ///
    virtual bool isCardPresent();
    
    //
    // MARK: - Overcurrent Protection Support
    //
    
    ///
    /// Initialize and enable overcurrent protection
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_pci_init_ocp()` defined in `rtsx_psr.c`.
    ///
    virtual IOReturn initOvercurrentProtection();
    
    ///
    /// Enable overcurrent protection
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_pci_enable_ocp()` defined in `rtsx_psr.c`.
    ///
    virtual IOReturn enableOvercurrentProtection();
    
    ///
    /// Disable overcurrent protection
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_pci_disable_ocp()` defined in `rtsx_psr.c`.
    ///
    virtual IOReturn disableOvercurrentProtection();
    
    ///
    /// Get the overcurrent protection status
    ///
    /// @param status The status on return
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_pci_get_ocpstat()` defined in `rtsx_psr.c`.
    ///
    virtual IOReturn getOvercurrentProtectionStatus(UInt8& status);
    
    ///
    /// Clear the overcurrent protection status
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_pci_clear_ocpstat()` defined in `rtsx_psr.c`.
    ///
    virtual IOReturn clearOvercurrentProtectionStatus();
    
    //
    // MARK: - Card Pull Control Management
    //
    
    ///
    /// Enable pull control for the SD card
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_pci_card_pull_ctl_enable()` defined in `rtsx_psr.c`.
    ///
    IOReturn enablePullControlForSDCard();
    
    ///
    /// Disable pull control for the SD card
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_pci_card_pull_ctl_disable()` defined in `rtsx_psr.c`.
    ///
    IOReturn disablePullControlForSDCard();
    
    //
    // MARK: - OOBS Polling
    //
    
    ///
    /// Check whether the driver needs to modify the PHY::RCR0 register to enable or disable OOBS polling
    ///
    /// @return `true` if the controller is not RTS525A nor RTS5260.
    /// @note RTS525A and RTS5260 controllers should override this function and return `false`.
    ///
    virtual bool oobsPollingRequiresPhyRCR0Access();
    
    ///
    /// Check whether the hardware supports OOBS polling
    ///
    /// @return `true` if supported, `false` otherwise.
    /// @note By default, this function returns `false`.
    /// @note e.g., RTS522A, RTS524A and RTS525A should override this function and return `true`.
    ///
    virtual bool supportsOOBSPolling();
    
    ///
    /// Enable OOBS polling
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    ///
    IOReturn enableOOBSPolling();
    
    ///
    /// Disable OOBS polling
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    ///
    IOReturn disableOOBSPolling();
    
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
    IOReturn readPingPongBuffer(UInt8* destination, IOByteCount length);
    
    ///
    /// Write to the ping pong buffer
    ///
    /// @param source The buffer to write
    /// @param length The number of bytes to write (cannot exceed 512 bytes)
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_pci_write_ppbuf()` defined in `rtsx_psr.c`.
    ///
    IOReturn writePingPongBuffer(const UInt8* source, IOByteCount length);
    
    //
    // MARK: - Rx/Tx Phases
    //
    
    ///
    /// Get the clock phase for Rx
    ///
    /// @return The clock phase.
    ///
    ClockPhase getRxClockPhase();
    
    ///
    /// Get the clock phase for Tx
    ///
    /// @return The clock phase.
    ///
    ClockPhase getTxClockPhase();
    
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
    virtual IOReturn setASPM(bool enable);
    
    ///
    /// Enable the active state power management
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces ``
    ///
    inline IOReturn enableASPM()
    {
        return this->setASPM(true);
    }
    
    ///
    /// Disable the active state power management
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_disable_aspm()` defined in `rtsx_psr.c`.
    ///
    inline IOReturn disableASPM()
    {
        return this->setASPM(false);
    }
    
    ///
    /// Check whether the active state power management is active
    ///
    /// @return `true` if ASPM is enabled, `false` otherwise.
    ///
    inline bool isASPMEnabled()
    {
        using namespace RTSX::Chip;
        
        UInt8 value;
        
        if (this->readChipRegister(rAFCTL, value) != kIOReturnSuccess)
        {
            return true;
        }
        
        return !BitOptions(value).contains(AFCTL::kCTL0 | AFCTL::kCTL1);
    }
    
    ///
    /// [Helper] Set the L1 off substates configuration
    ///
    /// @param value The register value of `L1SUB_CONFIG3`
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_set_l1off_sub()` defined in `rtsx_pcr.c`.
    ///
    inline IOReturn setL1OffSubConfig(UInt8 value)
    {
        return this->writeChipRegister(RTSX::Chip::L1SUB::rCFG3, 0xFF, value);
    }
    
    ///
    /// Set the L1 substates configuration
    ///
    /// @param active Pass `true` to set the active state
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_set_l1off_sub_cfg_d0()` and `set_l1off_cfg_sub_d0()` defined in `rtsx_pcr.c`.
    /// @note The base controller provides a default implementation that simply returns `kIOReturnSuccess`.
    ///
    virtual IOReturn setL1OffSubConfigD0(bool active);
    
    ///
    /// Set the LTR latency
    ///
    /// @param latency The latency
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_comm_set_ltr_latency()` and `rtsx_set_ltr_latency()` defined in `rtsx_psr.c`.
    ///
    IOReturn setLTRLatency(UInt32 latency);
    
    ///
    /// Notify the card reader to enter into the worker state
    ///
    /// @note Port: This function replaces `rtsx_pci_start_run()`, `rtsx_pm_full_on()` and `rtsx_comm_pm_full_on()` defined in `rtsx_psr.c`.
    ///
    void enterWorkerState();
    
    //
    // MARK: - Power Management
    //
    
    ///
    /// Power down the controller forcedly
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_base_force_power_down()` defined in `rtsx_psr.c` and `*_force_power_down()` defined in each controller file.
    ///
    virtual IOReturn forcePowerDown() = 0;
    
    ///
    /// Prepare to enter the sleep state
    ///
    /// @note Port: This function replaces `rtsx_pci_suspend()` defined in `rtsx_psr.c`.
    ///
    void prepareToSleep(); // TODO: RELOCATE VIRTUAL
    
    ///
    /// Prepare to wake up from sleep
    ///
    /// @note Port: This function replaces `rtsx_pci_resume()` defined in `rtsx_psr.c`.
    ///
    void prepareToWakeUp(); // TODO: RELOCATE VIRTUAL
    
    ///
    /// Adjust the power state in response to system-wide power events
    ///
    /// @param powerStateOrdinal The number in the power state array of the state the driver is being instructed to switch to
    /// @param whatDevice A pointer to the power management object which registered to manage power for this device
    /// @return `kIOPMAckImplied` always.
    ///
    IOReturn setPowerState(unsigned long powerStateOrdinal, IOService* whatDevice) override; // TODO: RELOCATE BASE IMP
    
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
    bool interruptEventSourceFilter(IOFilterInterruptEventSource* source);
    
    ///
    /// Main interrupt service routine
    ///
    /// @param sender The interrupt event source
    /// @param count The number of interrupts seen before delivery
    /// @note The interrupt service routine runs in a gated context.
    ///       It checks hardware registers to identify the reason and invoke helper handlers.
    /// @note The interrupt event source filter guarantees that there is a pending interrupt when the handler is invoked.
    ///
    void interruptHandlerGated(IOInterruptEventSource* sender, int count);
    
    ///
    /// Timeout interrupt service routine
    ///
    /// @param sender The timer event source
    /// @note The timeout handler runs in a gated context.
    ///
    void bufferTransferTimeoutHandlerGated(IOTimerEventSource* sender);
    
    ///
    /// Helper interrupt service routine when a host command or data transfer is done
    ///
    /// @param succeeded `true` if the transfer has succeeded. `false` otherwise.
    /// @note This interrupt service routine runs in a gated context.
    ///
    void onTransferDoneGated(bool succeeded);
    
    ///
    /// Helper interrupt service routine when an overcurrent is detected
    ///
    /// @note This interrupt service routine runs in a gated context.
    /// @note Port: This function replaces `rtsx_pci_process_ocp_interrupt()` and `rtsx_pci_process_ocp()` defined in `rtsx_psr.c`.
    /// @note Unlike the Linux driver, this function is invoked if and only if OCP is enabled.
    ///
    virtual void onSDCardOvercurrentOccurredGated();
    
    ///
    /// Helper interrupt service routine when a SD card is inserted
    ///
    /// @note This interrupt service routine runs in a gated context.
    /// @note Port: This function replaces `rtsx_pci_card_detect()` defined in `rtsx_psr.c` but has a completely different design and implementation.
    ///
    void onSDCardInsertedGated();
    
    ///
    /// Helper interrupt service routine when a SD card is removed
    ///
    /// @note This interrupt service routine runs in a gated context.
    /// @note Port: This function replaces `rtsx_pci_card_detect()` defined in `rtsx_psr.c` but has a completely different design and implementation.
    ///
    void onSDCardRemovedGated();
    
    //
    // MARK: - Hardware Initialization and Configuration
    //
    
    ///
    /// Get the IC revision
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `*_get_ic_version()` defined in each controller file.
    ///
    virtual IOReturn getRevision(Revision& revision);
    
    ///
    /// Optimize the physical layer
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `optimize_phy()` defined in the the `pcr->ops`.
    ///
    virtual IOReturn optimizePhys();
    
    ///
    /// Initialize hardware-specific parameters
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `*_init_params()` defined in each controller file.
    ///
    virtual IOReturn initParameters() = 0;
    
    ///
    /// Initialize vendor-specific parameters
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `fetch_vendor_settings()` defined in the `pcr->ops`.
    ///
    virtual IOReturn initVendorSpecificParameters() = 0;
    
    ///
    /// [Helper] Check whether the vendor settings register contains a valid value
    ///
    /// @param regVal The register value
    /// @return `true` if valid, `false` otherwise.
    ///
    inline bool vsIsRegisterValueValid(UInt32 regVal)
    {
        return !(regVal & 0x1000000);
    }
    
    ///
    /// [Helper] Check whether the driver should send MMC commands during initialization
    ///
    /// @param regVal The register value
    /// @return `true` if needed, `false` otherwise.
    ///
    inline bool vsIsMMCCommandsNeededDuringInitialiation(UInt32 regVal)
    {
        return !(regVal & 0x10);
    }
    
    ///
    /// [Helper] Extract the SD 3.0 card drive selector (1.8V) from the register value
    ///
    /// @param regVal The register value
    /// @return The SD 3.0 card drive selector (1.8V).
    ///
    virtual UInt8 vsGetSD30DriveSelector1d8V(UInt32 regVal)
    {
        return (regVal >> 26) & 0x03;
    }
    
    ///
    /// [Helper] Extract the SD 3.0 card drive selector (3.3V) from the register value
    ///
    /// @param regVal The register value
    /// @return The SD 3.0 card drive selector (3.3V).
    ///
    virtual UInt8 vsGetSD30DriveSelector3d3V(UInt32 regVal)
    {
        return (regVal >> 5) & 0x03;
    }
    
    ///
    /// [Helper] Map the given selector index to the selector value
    ///
    /// @param index The index extracted from `vsGetSD30DriveSelector*()`
    /// @return The actual selector value.
    ///
    inline UInt8 vsMapDriveSelector(UInt8 index)
    {
        static constexpr UInt8 kMap[] =
        {
            0x01, // Type D
            0x02, // Type C
            0x05, // Type A
            0x03, // Type B
        };
        
        return kMap[index];
    }
    
    ///
    /// [Helper] Extract the card drive selector from the register value
    ///
    /// @param regVal The register value
    /// @return The card drive selector.
    ///
    virtual UInt8 vsGetCardDriveSelector(UInt32 regVal)
    {
        return ((regVal >> 25) & 0x01) << 6;
    }
    
    ///
    /// [Helper] Check whether the socket is reversed
    ///
    /// @param regVal The register value
    /// @return `true` if the socket is reversed, `false` otherwise.
    ///
    inline bool vsIsSocketReversed(UInt32 regVal)
    {
        return regVal & 0x4000;
    }
    
    ///
    /// [Helper] Check whether the ASPM L1 is already enabled
    ///
    /// @param regVal The register value
    /// @return `true` if L1 is enabled, `false` otherwise.
    /// @note Port: This function replaces `rtsx_reg_to_aspm()` defined in `rtsx_pcr.h`.
    ///
    virtual bool vsIsASPML1Enabled(UInt32 regVal)
    {
        return ((regVal >> 28) & 0x02) == 0x02;
    }
        
    ///
    /// Initialize the hardware (Extra Part)
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `extra_init_hw()` defined in defined in the `pcr->ops`.
    ///
    virtual IOReturn initHardwareExtra() = 0;
    
    ///
    /// Initialize the hardware (Common Part)
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_pci_init_hw()` defined in `rtsx_psr.c`.
    ///
    IOReturn initHardwareCommon();
    
    //
    // MARK: - Startup Routines
    //
    
    ///
    /// Setup the power management
    ///
    /// @return `true` on success, `false` otherwise.
    ///
    bool setupPowerManagement(); // TODO: RELOCATED TO BASE
    
    ///
    /// [Helper] Get the 8-bit base address register to map the device memory
    ///
    /// @return The BAR value 0x10 for all controllers other than RTS525A.
    /// @note RTS525A controller must override this function and return 0x14 instead.
    /// @note This helper function is invoked by `RealtekPCIeCardReaderController::mapDeviceMemory()`.
    ///
    virtual UInt8 getDeviceMemoryMapBaseAddressRegister() { return kIOPCIConfigBaseAddress0; }
    
    ///
    /// Map the device memory
    ///
    /// @return `true` on success, `false` otherwise.
    ///
    bool mapDeviceMemory();
    
    ///
    /// Setup the workloop
    ///
    /// @return `true` on success, `false` otherwise.
    ///
    bool setupWorkLoop(); // TODO: RELOCATED TO BASE
    
    ///
    /// [Helper] Probe the index of the message signaled interrupt
    ///
    /// @return The index on success, `0` otherwise.
    /// @note This helper function is invoked by `RealtekPCIeCardReaderController::setupInterrupts()`.
    ///
    int probeMSIIndex();
    
    ///
    /// Setup the interrupt management module
    ///
    /// @return `true` on success, `false` otherwise.
    ///
    bool setupInterrupts();
    
    ///
    /// Setup the host command and buffer management module
    ///
    /// @return `true` on success, `false` otherwise.
    ///
    bool setupHostBuffer();
    
    ///
    /// Setup the SD card reader
    ///
    /// @return `true` on success, `false` otherwise.
    /// @note Port: This function replaces `rtsx_pci_init_chip()` defined in `rtsx_pci.c`.
    ///
    bool setupCardReader();
    
    ///
    /// Create the card slot and publish it
    ///
    /// @return `true` on success, `false` otherwise.
    ///
    bool createCardSlot();
    
    //
    // MARK: - Teardown Routines
    //
    
    ///
    /// Tear down the power management
    ///
    void tearDownPowerManagement(); // TODO: RELOCATED TO BASE
    
    ///
    /// Unmap the device memory
    ///
    void unmapDeviceMemory();
    
    ///
    /// Tear down the workloop
    ///
    void tearDownWorkLoop(); // TODO: RELOCATED TO BASE
    
    ///
    /// Tear down the interrupt event source
    ///
    void tearDownInterrupts();
    
    ///
    /// Tear down the host command and buffer management module
    ///
    void tearDownHostBuffer();
    
    ///
    /// Destroy the card slot
    ///
    void destroyCardSlot();
    
public:
    //
    // MARK: - IOService Implementations
    //
    
    ///
    /// Start the controller
    ///
    /// @param provider An instance of PCI device that represents the card reader
    /// @return `true` on success, `false` otherwise.
    ///
    bool start(IOService* provider) override;
    
    ///
    /// Stop the controller
    ///
    /// @param provider An instance of PCI device that represents the card reader
    ///
    void stop(IOService* provider) override;
};

#endif /* RealtekPCIeCardReaderController_hpp */
