//
//  RealtekPCICardReaderController.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 2/18/21.
//

#ifndef RealtekPCICardReaderController_hpp
#define RealtekPCICardReaderController_hpp

#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/IOWorkLoop.h>
#include <IOKit/IOCommandGate.h>
#include <IOKit/IOMemoryDescriptor.h>
#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/IODMACommand.h>
#include <IOKit/IOInterruptEventSource.h>
#include <IOKit/IOFilterInterruptEventSource.h>
#include <IOKit/IOTimerEventSource.h>
#include "IOEnhancedCommandPool.hpp"
#include "RealtekCardReaderController.hpp"
#include "RealtekPCIRegisters.hpp"
#include "Utilities.hpp"
#include "Debug.hpp"
#include "WolfsSDXC.hpp"
#include "BitOptions.hpp"
#include "ClosedRange.hpp"

/// Forward Declaration (Client of the card reader controller)
class RealtekSDXCSlot;

///
/// Represents the abstract PCIe card reader controller
///
/// @note This is the base class of all device-specific controllers.
///
class RealtekPCICardReaderController: public RealtekCardReaderController
{
    //
    // MARK: - Constructors & Destructors
    //
    
    OSDeclareAbstractStructors(RealtekPCICardReaderController);
    
    using super = RealtekCardReaderController;
    
    //
    // MARK: - Constants: SSC Clock Properties
    //

    /// The minimum SSC clock divider value
    static constexpr UInt32 kMinSSCClockN = 80;
    
    /// The maximum SSC clock divider value
    static constexpr UInt32 kMaxSSCClockN = 208;
    
    /// The amount of time in microseconds to wait until the SSC clock becomes stable
    static constexpr UInt32 kWaitStableSSCClock = 130;
    
    /// The minimum SSC clock frequency in MHz
    static constexpr UInt32 kMinSSCClockFrequencyMHz = 2;
    
    //
    // MARK: - Constants: Tuning Configurations
    //
    
    /// The number of phases
    static constexpr UInt32 kTuningNumPhases = 32;
    
    /// The controller should enable the 80 clocks timeout
    static constexpr UInt32 kTuningEnable80ClocksTimes = true;
    
    //
    // MARK: - Constants: Bus Timing Tables
    //
    
    /// A sequence of chip registers to switch the bus speed mode to UHS-I SDR50/SDR104
    static const ChipRegValuePair kBusTimingTablePairsSDR50[];
    static const SimpleRegValuePairs kBusTimingTableSDR50;
    
    /// A sequence of chip registers to switch the bus speed mode to UHS-I DDR50
    static const ChipRegValuePair kBusTimingTablePairsDDR50[];
    static const SimpleRegValuePairs kBusTimingTableDDR50;
    
    /// A sequence of chip registers to switch the bus speed mode to High Speed
    static const ChipRegValuePair kBusTimingTablePairsHighSpeed[];
    static const SimpleRegValuePairs kBusTimingTableHighSpeed;
    
    /// A sequence of chip registers to switch the bus speed mode to Default Speed
    static const ChipRegValuePair kBusTimingTablePairsDefaultSpeed[];
    static const SimpleRegValuePairs kBusTimingTableDefaultSpeed;
    
    //
    // MARK: - Constants: LTR Configurations
    //
    
    /// A namespace where LTR-related constants are defined
    struct LTRDefaults
    {
        static constexpr UInt32 kActiveLatency = 0x883C;
        static constexpr UInt32 kIdleLatency = 0x892C;
        static constexpr UInt32 kL1OffLatency = 0x9003;
        static constexpr UInt32 kL1SnoozeDelay = 1;
        static constexpr UInt32 kL1OffSSPowerGate_522A = 0x7F;
        static constexpr UInt32 kL1OffSSPowerGate_5249 = 0xAF;
        static constexpr UInt32 kL1OffSSPowerGate_525A = 0xFF;
        static constexpr UInt32 kL1OffSnoozeSSPowerGate_522A = 0x78;
        static constexpr UInt32 kL1OffSnoozeSSPowerGate_5249 = 0xAC;
        static constexpr UInt32 kL1OffSnoozeSSPowerGate_525A = 0xF8;
    };
    
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
    
    /// TX/RX clock phase
    struct ClockPhase
    {
        UInt8 sdr104, sdr50, ddr50, reserved;
        
        ClockPhase()
            : sdr104(0), sdr50(0), ddr50(0), reserved(0) {}
        
        ClockPhase(UInt8 sdr104, UInt8 sdr50, UInt8 ddr50)
            : sdr104(sdr104), sdr50(sdr50), ddr50(ddr50), reserved(0) {}
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
            /// Supports SD UHS SDR104 mode
            bool supportsSDSDR104;
            
            /// Supports SD UHS SDR50 mode
            bool supportsSDSDR50;
            
            /// Supports SD UHS DDR50 mode
            bool supportsSDDDR50;
            
            /// Supports SD Express
            bool supportsSDExpress;
            
            /// Supports MMC DDR 1.8V
            bool supportsMMCDDR1d8V;
            
            /// Supports 8 bit transfers
            bool supportsMMC8BitTransfer;
            
            /// Do not send MMC commands during initialization
            bool noMMCCommandsDuringInitialization;
            
        } caps;
        
        /// True if the socket is reversed
        bool isSocketReversed;
        
        /// Card drive selector
        UInt8 cardDriveSelector;
        
        /// SD 3.0 drive selector (1.8V)
        UInt8 sd30DriveSelector1d8V;
        
        /// SD 3.0 drive selector (3.3V)
        UInt8 sd30DriveSelector3d3V;
        
        /// Padding
        UInt8 padding[3];
        
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
    
    /// A mapping for the device memory
    IOMemoryMap* deviceMemoryMap;
    
    /// Device memory at BARx where x may vary device
    IOMemoryDescriptor* deviceMemoryDescriptor;
    
    /// An event source that delivers the hardware interrupt
    IOFilterInterruptEventSource* interruptEventSource;
    
    /// A timer that delays the initialization of the card
    IOTimerEventSource* cardSetupTimer;
    
    //
    // MARK: - Host Command & Data Buffer
    //
    
    ///
    /// The host buffer size (by default 4096 bytes)
    ///
    /// @note 1 KB is reserved for the host command buffer, allowing 256 commands to be queued;
    ///       3 KB is reserved for the host data buffer, allowing 384 scatter/gather list items to be queued.
    ///
    static constexpr IOByteCount kHostBufferSize = RTSX::MMIO::HCBAR::kMaxNumCmds * 4 + RTSX::MMIO::HDBAR::kMaxNumElements * 8;
    
    /// The host command buffer starts at the offset 0 in the host buffer
    static constexpr IOByteCount kHostCommandBufferOffset = 0;
    
    /// The host data buffer starts at the offset 1024 in the host buffer
    static constexpr IOByteCount kHostDataBufferOffset = RTSX::MMIO::HCBAR::kMaxNumCmds * 4;
    
    /// The maximum number of DMA segments supported by the card reader
    static constexpr IOItemCount kMaxNumSegments = 256;
    
    /// The maximum segment size supported by the card reader
    static constexpr IOItemCount kMaxSegmentSize = 65536;
    
    /// The maximum number of DMA transfer failures until the host should reduce the card clock
    static constexpr IOItemCount kMaxDMATransferFailures = 8;
    
    /// The default capacity of the DMA command pool
    static constexpr IOItemCount kDefaultPoolSize = 32;
    
    /// Type of pool of DMA commands
    using IODMACommandPool = IOEnhancedCommandPool<IODMACommand, IODMACommandCreator32<kMaxSegmentSize>>;
    
    /// A pool of DMA commands needed to perform DMA transfers to/from the card
    IODMACommandPool* dmaCommandPool;
    
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
    
    /// The number of DMA errors
    UInt32 dmaErrorCounter;
    
    /// `True` if the host is idle
    bool isIdle;
    
    /// Reserved fields
    UInt8 reserved0, reserved1, reserved2;
    
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
    
protected:
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
    
public:
    ///
    /// Read a byte from the chip register at the given address
    ///
    /// @param address The register address
    /// @param value The register value on return
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out.
    /// @note Port: This function replaces `rtsx_pci_read_register()` defined in `rtsx_psr.c`.
    ///
    IOReturn readChipRegister(UInt16 address, UInt8& value) override final;
    
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
    IOReturn writeChipRegister(UInt16 address, UInt8 mask, UInt8 value) override final;
    
    ///
    /// Write to multiple chip registers conveniently
    ///
    /// @param pairs A sequence of pairs of register address and value
    /// @return `kIOReturnSuccess` on success;
    ///         `kIOReturnTimeout` if timed out;
    ///         `kIOReturnError` if the new register value is not `value`.
    ///
    IOReturn writeChipRegisters(const ChipRegValuePairs& pairs);
    
    //
    // MARK: - Access Physical Layer Registers (Default, Overridable)
    //
    
protected:
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
    
protected:
    ///
    /// Read from the host buffer into the given buffer
    ///
    /// @param offset A byte offset into the host buffer
    /// @param buffer A non-null buffer
    /// @param length The number of bytes to read
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note This function runs in a gated context.
    ///
    IOReturn readHostBufferGated(IOByteCount offset, void* buffer, IOByteCount length) override final;
    
    ///
    /// Write to the host buffer form the given buffer
    ///
    /// @param offset A byte offset into the host buffer
    /// @param buffer A non-null buffer
    /// @param length The number of bytes to write
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note This function runs in a gated context.
    ///
    IOReturn writeHostBufferGated(IOByteCount offset, const void* buffer, IOByteCount length) override final;
    
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
        return this->writeHostBuffer(RealtekPCICardReaderController::kHostDataBufferOffset + offset, buffer, length);
    }
    
    //
    // MARK: - Host Command Management (Core)
    //
    
protected:
    ///
    /// Enqueue a command
    ///
    /// @param command The command
    /// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full, `kIOReturnError` otherwise.
    /// @note Port: This function replaces `rtsx_pci_add_cmd()` defined in `rtsx_psr.c`.
    /// @note This function runs in a gated context.
    ///
    IOReturn enqueueCommandGated(const Command& command) override final;
    
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
    IOReturn endCommandTransferGated(UInt32 timeout, UInt32 flags) override final;
    
    ///
    /// Finish the existing host command transfer session without waiting for the response
    ///
    /// @param flags An optional flag, 0 by default
    /// @return `kIOReturnSuccess` on success, other values if failes to send the command to the device.
    /// @note Port: This function replaces `rtsx_pci_send_cmd_no_wait()` defined in `rtsx_psr.c`.
    /// @note Port: This function replaces `rtsx_usb_send_cmd()` (the original version) defined in `rtsx_usb.c`.
    /// @note This function sends all commands in the queue to the device.
    /// @note This function runs in a gated context.
    ///
    IOReturn endCommandTransferNoWaitGated(UInt32 flags) override final;
    
    ///
    /// Load the response to the existing host command transfer session
    ///
    /// @param timeout Specify the amount of time in milliseconds
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, other values otherwise.
    /// @note Port: This function is a noop and returns `kIOReturnSuccess` for PCIe-based card reader controllers.
    /// @note Port: This function replaces `rtsx_usb_get_rsp()` (the original version) defined in `rtsx_usb.c`.
    /// @note This function runs in a gated context.
    ///
    IOReturn loadCommandTransferResponseGated(UInt32 timeout) override final;
    
    //
    // MARK: - Host Data Management
    //
    
protected:
    ///
    /// [Helper] Convert a physical scatter/gather entry to a value to be stored in the host data buffer
    ///
    /// @param segment An entry in the scatter/gather list
    /// @return The value to be written to the host data buffer.
    /// @note Port: This helper function is refactored from `rtsx_pci_add_sg_tbl()` defined in `rtsx_psr.c`.
    ///             RTS5261 and RTS5228 controllers must override this function but should not include the `option` part in the returned value.
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
    /// [Helper] Perform a DMA transfer
    ///
    /// @param descriptor A non-null, perpared memory descriptor
    /// @param timeout Specify the amount of time in milliseconds
    /// @param control Specify the value that will be written to the register `HDBCTLR` to customize the DMA transfer
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    /// @note Port: This function replaces `rtsx_pci_dma_transfer()` defined in `rtsx_psr.c`.
    /// @note This helper function is invoked by both `performDMARead()` and `performDMAWrite()`.
    ///       The caller should avoid calling this function directly.
    /// @warning The caller must ensure that the given memory descriptor is prepared.
    ///
    IOReturn performDMATransfer(IOMemoryDescriptor* descriptor, UInt32 timeout, UInt32 control);
    
public:    
    ///
    /// Perform a DMA read operation
    ///
    /// @param descriptor A non-null, perpared memory descriptor
    /// @param timeout Specify the amount of time in milliseconds
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    ///
    IOReturn performDMARead(IOMemoryDescriptor* descriptor, UInt32 timeout) override final;
    
    ///
    /// Perform a DMA write operation
    ///
    /// @param descriptor A non-null, perpared memory descriptor
    /// @param timeout Specify the amount of time in milliseconds
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    ///
    IOReturn performDMAWrite(IOMemoryDescriptor* descriptor, UInt32 timeout) override final;
    
    //
    // MARK: - Clear Error
    //
    
public:
    ///
    /// Clear any transfer error on the card side
    ///
    /// @note Port: This function replaces the code block that stops the card and clears the card errorin `sd_clear_error()` defined in `rtsx_pci_sdmmc.c`.
    ///
    void clearCardError() override final;
    
    ///
    /// Clear any transfer error on the host side
    ///
    /// @note This function is invoked when a command or data transfer has failed.
    /// @note Port: This function replaces `rtsx_pci_stop_cmd()` defined in `rtsx_psr.c`.
    ///             RTS5228, RTS5260 and RTS5261 controllers must override this function.
    ///
    void clearHostError() override;
    
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
    
public:
    ///
    /// Turn on the LED
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    /// @note Port: This function replaces `turn_on_led()` defined in `struct pcr_ops`.
    ///             The base controller class implements this function by changing the value of `GPIO_CTL`.
    ///             RTS5209, 5260, 5286, 5287 and 5289 controllers must override this function.
    ///
    IOReturn turnOnLED() override;
    
    ///
    /// Turn off the LED
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    /// @note Port: This function replaces `turn_off_led()` defined in `struct pcr_ops`.
    ///             The base controller class implements this function by changing the value of `GPIO_CTL`.
    ///             RTS5209, 5260, 5286, 5287 and 5289 controllers must override this function.
    ///
    IOReturn turnOffLED() override;
    
    ///
    /// Enable LED blinking
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    /// @note Port: This function replaces `enable_auto_blink()` defined in `struct pcr_ops`.
    ///             The base controller class implements this function by changing the value of `OLT_LED_CTL`.
    ///             RTS5209, 5286, 5287 and 5289 controllers must override this function.
    ///
    IOReturn enableLEDBlinking() override;
    
    ///
    /// Disable LED blinking
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    /// @note Port: This function replaces `enable_auto_blink()` defined in `struct pcr_ops`.
    ///             The base controller class implements this function by changing the value of `OLT_LED_CTL`.
    ///             RTS5209, 5286, 5287 and 5289 controllers must override this function.
    ///
    IOReturn disableLEDBlinking() override;
    
    //
    // MARK: - Card Selection, Share Mode and Transfer Properties
    //
    
public:
    ///
    /// Select the SD card
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full, `kIOReturnError` otherwise.
    /// @note This function invokes `enqueueWriteRegisterCommand()` thus must be invoked between `beginCommandTransfer()` and `endCommandTransfer()`.
    /// @note The caller may use `withCustomCommandTransfer()` to combine this operation with other ones.
    ///
    IOReturn selectCard() override final;
    
    ///
    /// Select the data source for the SD card
    ///
    /// @param ppbuf `True` if the data source should be set to the ping pong buffer;
    ///              `False` if the data source should be the ring buffer instead
    /// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full, `kIOReturnError` otherwise.
    /// @note This function invokes `enqueueWriteRegisterCommand()` thus must be invoked between `beginCommandTransfer()` and `endCommandTransfer()`.
    /// @note The caller may use `withCustomCommandTransfer()` to combine this operation with other ones.
    ///
    IOReturn selectCardDataSource(bool ppbuf) override final;
    
    ///
    /// Configure the card share mode
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full, `kIOReturnError` otherwise.
    /// @note This function invokes `enqueueWriteRegisterCommand()` thus must be invoked between `beginCommandTransfer()` and `endCommandTransfer()`.
    /// @note The caller may use `withCustomCommandTransfer()` to combine this operation with other ones.
    ///
    IOReturn configureCardShareMode() override final;
    
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
    IOReturn setupCardDMATransferProperties(UInt32 length, IODirection direction) override final;
    
    //
    // MARK: - Card Power Management
    //
    
public:
    ///
    /// Enable the card clock
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full, `kIOReturnError` otherwise.
    /// @note This function invokes `enqueueWriteRegisterCommand()` thus must be invoked between `beginCommandTransfer()` and `endCommandTransfer()`.
    /// @note The caller may use `withCustomCommandTransfer()` to combine this operation with other ones.
    ///
    IOReturn enableCardClock() override final;
    
    ///
    /// Disable the card clock
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full, `kIOReturnError` otherwise.
    /// @note This function invokes `enqueueWriteRegisterCommand()` thus must be invoked between `beginCommandTransfer()` and `endCommandTransfer()`.
    /// @note The caller may use `withCustomCommandTransfer()` to combine this operation with other ones.
    ///
    IOReturn disableCardClock() override final;
    
    ///
    /// Enable the card output
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full, `kIOReturnError` otherwise.
    /// @note This function invokes `enqueueWriteRegisterCommand()` thus must be invoked between `beginCommandTransfer()` and `endCommandTransfer()`.
    /// @note The caller may use `withCustomCommandTransfer()` to combine this operation with other ones.
    ///
    IOReturn enableCardOutput() override final;
    
    ///
    /// Disable the card output
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full, `kIOReturnError` otherwise.
    /// @note This function invokes `enqueueWriteRegisterCommand()` thus must be invoked between `beginCommandTransfer()` and `endCommandTransfer()`.
    /// @note The caller may use `withCustomCommandTransfer()` to combine this operation with other ones.
    ///
    IOReturn disableCardOutput() override final;
    
    ///
    /// Power on the SD card
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_pci_card_power_on()` defined in `rtsx_psr.c`.
    ///
    virtual IOReturn powerOnCard() override = 0;
    
    ///
    /// Power off the SD card
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_pci_card_power_off()` defined in `rtsx_psr.c`.
    ///
    virtual IOReturn powerOffCard() override = 0;
    
    ///
    /// Switch to the given output voltage
    ///
    /// @param outputVoltage The new output voltage
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_pci_switch_output_voltage()` defined in `rtsx_psr.c`.
    ///
    virtual IOReturn switchOutputVoltage(OutputVoltage outputVoltage) override = 0;
    
protected:
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
    IOReturn setDrivingForOutputVoltage(OutputVoltage outputVoltage, bool intermediate, UInt32 timeout);
    
    //
    // MARK: - Card Clock Configurations
    //
    
protected:
    ///
    /// Calculate the number of MCUs from the given SSC clock
    ///
    /// @param clock The SSC clock in MHz
    /// @return The MCU count.
    /// @note Port: This function replaces the code block that calculates the MCU count in `rtsx_pci_switch_clock()` defined in `rtsx_psr.c`.
    /// @note Concrete controllers must ensure that the returned MCU count is less than or equal to 15.
    ///
    UInt32 sscClock2MCUCount(UInt32 clock) override final;
    
    ///
    /// Convert the given SSC depth to the actual register value
    ///
    /// @param depth The SSC depth
    /// @return The register value.
    /// @note Port: RTS5261 and RTS5228 controllers must override this function.
    ///
    UInt8 sscDepth2RegValue(SSCDepth depth) override;
    
    ///
    /// Revise the given SSC depth register value
    ///
    /// @param depth The SSC depth register value
    /// @param divider The SSC clock divider value
    /// @return The revised SSC depth register value.
    /// @note Port: RTS5261 and RTS5228 controllers must override this function.
    ///
    UInt8 reviseSSCDepthRegValue(UInt8 depth, UInt8 divider) override;
    
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
    IOReturn switchCardClock(UInt8 depth, UInt8 n, UInt8 divider, UInt8 mcus, bool vpclock) override;
    
    //
    // MARK: - Card Detection and Write Protection
    //
    
public:
    ///
    /// Check whether the card has write protection enabled
    ///
    /// @return `true` if the card is write protected, `false` otherwise.
    ///
    bool isCardWriteProtected() override final;
    
    ///
    /// Check whether a card is present
    ///
    /// @return `true` if a card exists, `false` otherwise.
    /// @note Port: This function replaces `rtsx_pci_card_exist()` and `cd_deglitch()` defined in `rtsx_psr.c`.
    /// @warning: This function supports SD cards only.
    ///
    bool isCardPresent() override;
    
    ///
    /// Check whether the command line of the card is busy
    ///
    /// @return `true` if the card CMD line is busy, `false` otherwise.
    /// @warning This function returns `true` if failed to read the register.
    ///
    bool isCardCommandLineBusy() override final;
    
    ///
    /// Check whether the data line of the card is busy
    ///
    /// @return `true` if the card DAT lines are busy, `false` otherwise.
    /// @warning This function returns `true` if failed to read the register.
    ///
    bool isCardDataLineBusy() override final;
    
    //
    // MARK: - Overcurrent Protection Support
    //
    
protected:
    ///
    /// Initialize and enable overcurrent protection
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_pci_init_ocp()` defined in `rtsx_psr.c`.
    ///             RTS5228, RTS5260 and RTS5261 controllers must override this function.
    ///
    virtual IOReturn initOvercurrentProtection();
    
    ///
    /// Enable overcurrent protection
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_pci_enable_ocp()` defined in `rtsx_psr.c`.
    ///             RTS5228, RTS5260 and RTS5261 controllers must override this function.
    ///
    virtual IOReturn enableOvercurrentProtection();
    
    ///
    /// Disable overcurrent protection
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_pci_disable_ocp()` defined in `rtsx_psr.c`.
    ///             RTS5228, RTS5260 and RTS5261 controllers must override this function.
    ///
    virtual IOReturn disableOvercurrentProtection();
    
    ///
    /// Get the overcurrent protection status
    ///
    /// @param status The status on return
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_pci_get_ocpstat()` defined in `rtsx_psr.c`.
    ///
    IOReturn getOvercurrentProtectionStatus(UInt8& status);
    
    ///
    /// Clear the overcurrent protection status
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_pci_clear_ocpstat()` defined in `rtsx_psr.c`.
    ///             RTS5228, RTS5260 and RTS5261 controllers must override this function.
    ///
    virtual IOReturn clearOvercurrentProtectionStatus();
    
    ///
    /// Check whether the controller should power off the card when it receives an OCP interrupt
    ///
    /// @return `true` if the controller should take necessary actions to protect the card, `false` otherwise.
    /// @note Port: This function replaces the code block that examines the OCP status in `rtsx_pci_process_ocp()` defined in `rtsx_psr.c`.
    ///             RTS5260 controller must override this function.
    /// @note This function runs in a gated context and is invoked by the OCP interrupt service routine.
    ///
    virtual bool shouldPowerOffCardOnOvercurrentInterruptGated();
    
    //
    // MARK: - Card Pull Control Management
    //
    
public:
    ///
    /// Enable pull control for the SD card
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_pci_card_pull_ctl_enable()` defined in `rtsx_psr.c`.
    ///
    IOReturn enablePullControlForSDCard() override final;
    
    ///
    /// Disable pull control for the SD card
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_pci_card_pull_ctl_disable()` defined in `rtsx_psr.c`.
    ///
    IOReturn disablePullControlForSDCard() override final;
    
    //
    // MARK: - Card Tuning & Phase Management
    //
    
public:
    ///
    /// Change the Rx phase
    ///
    /// @param samplePoint The sample point value
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces the Rx portion of `sd_change_phase()` defined in `rtsx_pci_sdmmc.c`.
    ///
    IOReturn changeRxPhase(UInt8 samplePoint) override final;
    
    ///
    /// Change the Tx phase
    ///
    /// @param samplePoint The sample point value
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces the Tx portion of `sd_change_phase()` defined in `rtsx_pci_sdmmc.c`.
    ///
    IOReturn changeTxPhase(UInt8 samplePoint) override final;
    
    //
    // MARK: - OOBS Polling
    //
    
protected:
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
    
public:
    ///
    /// Read from the ping pong buffer
    ///
    /// @param destination The buffer to store bytes
    /// @param length The number of bytes to read (cannot exceed 512 bytes)
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_pci_read_ppbuf()` defined in `rtsx_psr.c`.
    ///
    IOReturn readPingPongBuffer(UInt8* destination, IOByteCount length) override final;
    
    ///
    /// Write to the ping pong buffer
    ///
    /// @param source The buffer to write
    /// @param length The number of bytes to write (cannot exceed 512 bytes)
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_pci_write_ppbuf()` defined in `rtsx_psr.c`.
    ///
    IOReturn writePingPongBuffer(const UInt8* source, IOByteCount length) override final;
    
    ///
    /// Read from the ping pong buffer
    ///
    /// @param destination The buffer to store bytes
    /// @param length The number of bytes to read (cannot exceed 512 bytes)
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_pci_read_ppbuf()` defined in `rtsx_psr.c`.
    ///
    IOReturn readPingPongBuffer(IOMemoryDescriptor* destination, IOByteCount length) override final;
    
    ///
    /// Write to the ping pong buffer
    ///
    /// @param source The buffer to write
    /// @param length The number of bytes to write (cannot exceed 512 bytes)
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_pci_write_ppbuf()` defined in `rtsx_psr.c`.
    ///
    IOReturn writePingPongBuffer(IOMemoryDescriptor* source, IOByteCount length) override final;
    
    //
    // MARK: - Rx/Tx Phases
    //
    
public:
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
    
protected:
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
    /// @note Port: This function replaces `rtsx_enable_aspm()` defined in `rtsx_psr.c`.
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
        using namespace RTSX::PCR::Chip;
        
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
        return this->writeChipRegister(RTSX::PCR::Chip::L1SUB::rCFG3, 0xFF, value);
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
    
public:
    ///
    /// Notify the card reader to enter into the worker state
    ///
    /// @note Port: This function replaces `rtsx_pci_start_run()`, `rtsx_pm_full_on()` and `rtsx_comm_pm_full_on()` defined in `rtsx_psr.c`.
    ///
    void enterWorkerState() override final;
    
    //
    // MARK: - Power Management
    //
    
protected:
    ///
    /// Check if the controller should enable the clock power management
    ///
    /// @return `true` if the driver should write 0x01 to `PM_CLK_FORCE_CTL`, `false` otherwise.
    /// @note Port: This function replaces the code block that checks the device ID and writes to the register in `rtsx_pci_init_hw()` defined in `rtsx_psr.c`.
    ///             The base controller returns `false` by default.
    ///             RTS524A, RTS525A, RTS5260, RTS5261 and RTS5228 controllers should override this function and return `true`.
    ///
    virtual bool shouldEnableClockPowerManagement();
    
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
    void prepareToSleep() override final;
    
    ///
    /// Prepare to wake up from sleep
    ///
    /// @note Port: This function replaces `rtsx_pci_resume()` defined in `rtsx_psr.c`.
    ///
    void prepareToWakeUp() override final;
    
    //
    // MARK: - Hardware Interrupt Management
    //
    
private:
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
    void onSDCardOvercurrentOccurredGated();
    
    //
    // MARK: - Hardware Initialization and Configuration
    //
    
protected:
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
    
private:
    ///
    /// [Helper] Get the 8-bit base address register to map the device memory
    ///
    /// @return The BAR value 0x10 for all controllers other than RTS525A.
    /// @note RTS525A controller must override this function and return 0x14 instead.
    /// @note This helper function is invoked by `RealtekPCICardReaderController::mapDeviceMemory()`.
    ///
    virtual UInt8 getDeviceMemoryMapBaseAddressRegister() { return kIOPCIConfigBaseAddress0; }
    
    ///
    /// Map the device memory
    ///
    /// @return `true` on success, `false` otherwise.
    ///
    bool mapDeviceMemory();
    
    ///
    /// [Helper] Probe the index of the message signaled interrupt
    ///
    /// @return The index on success, `0` otherwise.
    /// @note This helper function is invoked by `RealtekPCICardReaderController::setupInterrupts()`.
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
    /// Create the timer that delays the initialization of a card
    ///
    /// @return `true` on success, `false` otherwise.
    ///
    bool setupCardInitTimer();
    
    ///
    /// Setup the card if it is present when the driver starts
    ///
    /// @param sender The timer event source that sends the event.
    /// @note The setup routine runs in a gated context.
    ///
    void setupCardIfPresentGated(IOTimerEventSource* sender);
    
    //
    // MARK: - Teardown Routines
    //
    
private:
    ///
    /// Unmap the device memory
    ///
    void unmapDeviceMemory();
    
    ///
    /// Tear down the interrupt event source
    ///
    void tearDownInterrupts();
    
    ///
    /// Tear down the host command and buffer management module
    ///
    void tearDownHostBuffer();
    
    ///
    /// Destory the timer that delays the initialization of a card
    ///
    void destroyCardInitTimer();
    
    //
    // MARK: - IOService Implementations
    //
    
public:
    ///
    /// Initialize the controller
    ///
    /// @return `true` on success, `false` otherwise.
    /// @note RTS5228 and RTS5261 controllers must override this function to adjust the minimum SSC clock frequency.
    ///
    bool init(OSDictionary* dictionary = nullptr) override;
    
    ///
    /// Start the controller
    ///
    /// @param provider An instance of PCI device that represents the card reader
    /// @return `true` on success, `false` otherwise.
    ///
    bool start(IOService* provider) override final;
    
    ///
    /// Stop the controller
    ///
    /// @param provider An instance of PCI device that represents the card reader
    ///
    void stop(IOService* provider) override final;
};

#endif /* RealtekPCICardReaderController_hpp */
