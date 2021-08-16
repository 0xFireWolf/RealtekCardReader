//
//  RealtekCardReaderController.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 7/10/21.
//

#ifndef RealtekCardReaderController_hpp
#define RealtekCardReaderController_hpp

#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/IOWorkLoop.h>
#include "IOCommandGate.hpp"
#include "WolfsSDXC.hpp"
#include "IOSDCard.hpp"
#include "ClosedRange.hpp"
#include "Utilities.hpp"
#include "Debug.hpp"

/// Forward Declaration (Client of the card reader controller)
class RealtekSDXCSlot;

///
/// Represents the abstract card reader controller
///
/// @note This is the base class of all Realtek PCIe/USB-based card reader controllers
///       and defines the interface for the controller-independent host device.
///
class RealtekCardReaderController: public WolfsSDXC
{
    //
    // MARK: - Constructors & Destructors
    //
    
    OSDeclareAbstractStructors(RealtekCardReaderController);
    
    using super = WolfsSDXC;
    
    //
    // MARK: - Controller-Independent Data Structures (Public)
    //
    
public:
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
        
        /// Create a pair with both mask and value set to 0x00
        ChipRegValuePair(UInt16 address) :
            ChipRegValuePair(address, 0x00, 0x00) {}
        
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
    
    /// Enumerates all possible SSC depth values
    enum class SSCDepth: UInt32
    {
        k4M = 0,        // PCIe  ---
        k2M = 1,        // PCIe  USB
        k1M = 2,        // PCIe  USB
        k512K = 3,      // ----  USB
        k500K = k512K,  // PCIe  ---
        k250K = 4,      // PCIe  ---
    };
    
    /// The card output volage
    enum class OutputVoltage: UInt32
    {
        k3d3V, k1d8V
    };
    
    /// Defines a sequence of registers and their values to change the bus speed mode
    struct BusTimingTables
    {
        /// Bus timing table for SD UHS-I SDR50 and SDR104 mode
        const SimpleRegValuePairs* sdr50;
        
        /// Bus timing table for SD UHS-I DDR50 mode
        const SimpleRegValuePairs* ddr50;
        
        /// Bus timing table for SD High Speed mode
        const SimpleRegValuePairs* highSpeed;
        
        /// Bus timing table for SD Default Speed mode
        const SimpleRegValuePairs* defaultSpeed;
        
        /// Reset all tables to null
        inline void reset()
        {
            this->sdr50 = nullptr;
            
            this->ddr50 = nullptr;
            
            this->highSpeed = nullptr;
            
            this->defaultSpeed = nullptr;
        }
    };
    
    /// Defines the configuration for tunning
    struct TuningConfig
    {
        /// The number of phases
        UInt32 numPhases;
        
        /// True if the controller should enable 80 clocks timeout
        UInt32 enable80ClocksTimeout;
        
        /// Reset all fields to zero
        inline void reset()
        {
            this->numPhases = 0;
            
            this->enable80ClocksTimeout = false;
        }
    };
    
    ///
    /// Defines the value of flags passed to each data transfer operation
    ///
    /// @note USB-based controllers require these flags while PCIe-based ones ignore these values.
    ///
    struct DataTransferFlags
    {
        /// Flags used to issue a SD command and wait for the response
        UInt32 command;
        
        /// Flags used to issue a SD command that involves an inbound data transfer
        UInt32 commandWithInboundDataTransfer;
        
        /// Flags used to issue a SD command that involves an outbound data transfer
        UInt32 commandWithOutboundDataTransfer;
        
        /// Flags used to issue a SD command that involves an inbound DMA transfer
        UInt32 commandWithInboundDMATransfer;
        
        /// Flags used to issue a SD command that involves an inbound DMA transfer
        UInt32 commandWithOutboundDMATransfer;
        
        /// Reset all flags to zero
        inline void reset()
        {
            this->command = 0;
            
            this->commandWithInboundDataTransfer = 0;
            
            this->commandWithOutboundDataTransfer = 0;
            
            this->commandWithInboundDMATransfer = 0;
            
            this->commandWithOutboundDMATransfer = 0;
        }
    };
    
    ///
    /// Type of an action that takes a ussr-defined context as its sole argument and enqueues any host commands
    ///
    /// @see `RealtekCardReaderController::withCustomCommandTransfer()`.
    ///
    using EnqueueAction = IOReturn (*)(void*);
    
    //
    // MARK: - Controller-Independent Data Structures (Private)
    //
    
protected:
    /// Represents a command in the host command buffer
    struct Command
    {
    private:
        /// The 32-bit value that will be written to the command buffer
        UInt32 value;
        
    public:
        /// All supported command types
        enum Type
        {
            kReadRegister = 0,
            kWriteRegister = 1,
            kCheckRegister = 2,
        };
        
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
        
        ///
        /// Create a command for reading from the chip register at the given address
        ///
        /// @param address The register address
        /// @return The host command.
        ///
        static inline Command forReadRegister(UInt16 address)
        {
            return Command(kReadRegister, address, 0, 0);
        }
        
        ///
        /// Create a command for writing to the chip register at the given address
        ///
        /// @param address The register address
        /// @param mask The register value mask
        /// @param value The register value
        /// @return The host command.
        ///
        static inline Command forWriteRegister(UInt16 address, UInt8 mask, UInt8 value)
        {
            return Command(kWriteRegister, address, mask, value);
        }
        
        ///
        /// Create a command for checking the value of the chip register at the given address
        ///
        /// @param address The register address
        /// @param mask The register value mask
        /// @param value The register value
        /// @return The host command.
        ///
        static inline Command forCheckRegister(UInt16 address, UInt8 mask, UInt8 value)
        {
            return Command(kCheckRegister, address, mask, value);
        }
        
        /// Get the command type
        inline Type getType() const
        {
            return static_cast<Type>((this->value >> 30) & 0x03);
        }
        
        /// Get the raw command value
        inline UInt32 getValue() const
        {
            return this->value;
        }
    };
    
    /// ABI Checks
    static_assert(sizeof(Command) == 4, "ABI Error: Host command is not 4 bytes long.");
    
    /// Count the number of commands in the host buffer
    struct HostCommandCounter
    {
        /// Total number of read register commands
        IOItemCount nreads;
        
        /// Total number of write register commands
        IOItemCount nwrites;
        
        /// Total number of check register commands
        IOItemCount nchecks;
        
        /// Total number of commands
        IOItemCount total;
        
        /// Reset the counter
        inline void reset()
        {
            this->nreads = 0;
            
            this->nwrites = 0;
            
            this->nchecks = 0;
            
            this->total = 0;
        }
        
        ///
        /// Increment the counter of the given command type
        ///
        /// @note This function automatically increments the total number of commands.
        /// @warning The caller must ensure that the given type is valid.
        ///
        inline void increment(Command::Type type)
        {
            // Use the given type as the index to the storage array
            passert(type <= Command::Type::kCheckRegister, "The given command type is invalid.");
            
            reinterpret_cast<UInt32*>(this)[type] += 1;
            
            this->total += 1;
        }
        
        /// Get the length of the response to a command transfer session
        inline IOItemCount getResponseLength()
        {
            // The card reader replies to the read and the check register command
            return this->nreads + this->nchecks;
        }
    };
    
    /// Host limitations on the internal SSC clock
    struct SSCClockLimits
    {
        /// The maximum and the minimum SSC clock N values
        ClosedRange<UInt32> rangeN;
        
        /// The maximum and the minimum clock divider register values (CLK_DIV)
        ClosedRange<UInt32> rangeDivider;
        
        /// The minimum SSC clock in MHz
        UInt32 minFrequencyMHz;
        
        /// Reserved field
        UInt32 reserved;
        
        /// Reset the limits to zeros
        inline void reset()
        {
            this->rangeN = {0, 0};
            
            this->rangeDivider = {0, 0};
            
            this->minFrequencyMHz = 0;
            
            this->reserved = 0;
        }
        
        /// Print the limits
        inline void print()
        {
            pinfo("SSC Clock Limits:");
            pinfo("|- N = [%u, %u]", this->rangeN.lowerBound, this->rangeN.upperBound);
            pinfo("|- Divider = [%u, %u]", this->rangeDivider.lowerBound, this->rangeDivider.upperBound);
            pinfo("|- Min Freq = %u MHz", this->minFrequencyMHz);
        }
    };
    
    /// Host UHS-I capabilities
    struct UHSCapabilities
    {
        /// True if the card reader supports UHS-I SDR104
        bool sdr104;
        
        /// True if the card reader supports UHS-I SDR50
        bool sdr50;
        
        /// True if the card reader supports UHS-I DDR50
        bool ddr50;
        
        /// Reserved
        bool reserved;
        
        /// Reset all capabilities to false
        inline void reset()
        {
            this->sdr104 = false;
            
            this->sdr50 = false;
            
            this->ddr50 = false;
            
            this->reserved = false;
        }
    };
    
    //
    // MARK: - IOKit Basics
    //
    
    ///
    /// The SD card slot (client)
    ///
    /// @note The concrete controller is responsible for managing the lifecycle of the card slot.
    ///
    RealtekSDXCSlot* slot;
    
    ///
    /// Protect shared resources and serialize operations
    ///
    /// @note The base controller creates the workloop and manages its lifecycle.
    ///
    IOWorkLoop* workLoop;
    
    ///
    /// A command gate to serialize executions with respect to the work loop
    ///
    /// @note This command gate is not associated with any specific actions.
    ///       The caller should use `runAction` to execute a function in a gated context.
    /// @note The target action is still executed in the context of the calling thread.
    /// @note The base controller creates the command gate and manages its lifecycle.
    ///
    IOCommandGate* commandGate;
    
    //
    // MARK: - Host Command & Data Buffer
    //
    
    ///
    /// A descriptor that allocates the host command and data buffer
    ///
    /// @note The buffer remains "prepared" until the driver is stopped by the system.
    ///       The lifecycle is managed by `setupHostBuffer()` and `tearDownHostBuffer()`.
    /// @note The buffer direction is set to be `kIODirectionInOut`.
    /// @note The actual buffer content and format are determined by the concrete controller.
    /// @note The concrete controller is responsible for managing the lifecycle of the host buffer.
    ///
    IOBufferMemoryDescriptor* hostBufferDescriptor;
    
    ///
    /// The number of commands in the host command buffer
    ///
    /// @note The counter is reset automatically each time the caller initiates a new transfer session.
    /// @see `RealtekCardReaderController::beginCommandTransfer()`.
    ///
    HostCommandCounter hostCommandCounter;
    
    //
    // MARK: - Host Capabilities
    //
    
    ///
    /// Host limitations on the internal SSC clock
    ///
    /// @note The concrete controller is responsbile for initializing its content, otherwise clock switches will fail.
    /// @see `RealtekCardReaderController::switchCardClock()`.
    ///
    SSCClockLimits sscClockLimits;
    
    /// Host UHS-I capabilities
    UHSCapabilities uhsCaps;
    
    //
    // MARK: - Host States
    //
    
    ///
    /// The current SSC clock in MHz
    ///
    /// @note The cached SSC clock is 0 at the beginning and is updated each time the caller requests to switch the clock.
    /// @see `RealtekCardReaderController::switchCardClock()`.
    ///
    UInt32 currentSSCClock;
    
    /// Bus timing tables
    BusTimingTables busTimingTables;
    
    /// Tuning configurations
    TuningConfig tuningConfig;
    
    /// Flags passed to each data transfer operation
    DataTransferFlags dataTransferFlags;
    
    //
    // MARK: - Query UHS-I Capabilities
    //
    
public:
    /// Check whether the card reader supports the UHS-I SDR104 mode
    inline bool supportsSDR104()
    {
        return this->uhsCaps.sdr104;
    }
    
    /// Check whether the card reader supports the UHS-I SDR50 mode
    inline bool supportsSDR50()
    {
        return this->uhsCaps.sdr50;
    }
    
    /// Check whether the card reader supports the UHS-I DDR50 mode
    inline bool supportsDDR50()
    {
        return this->uhsCaps.ddr50;
    }
    
    /// Get the bus timing tables
    inline const BusTimingTables& getBusTimingTables()
    {
        return this->busTimingTables;
    }
    
    /// Get the tuning configurations
    inline const TuningConfig& getTuningConfig()
    {
        return this->tuningConfig;
    }
    
    //
    // MARK: - Query Data Transfer Properties
    //
    
public:
    /// Get the flags passed to each data transfer operation
    inline const DataTransferFlags& getDataTransferFlags()
    {
        return this->dataTransferFlags;
    }
    
    //
    // MARK: - Access Chip Registers
    //
    
public:
    ///
    /// Read a byte from the chip register at the given address
    ///
    /// @param address The register address
    /// @param value The register value on return
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out.
    ///
    virtual IOReturn readChipRegister(UInt16 address, UInt8& value) = 0;
    
    ///
    /// Write a byte to the chip register at the given address
    ///
    /// @param address The register address
    /// @param mask The register value mask
    /// @param value The register value
    /// @return `kIOReturnSuccess` on success;
    ///         `kIOReturnTimeout` if timed out;
    ///         `kIOReturnError` if the new register value is not `value`.
    ///
    virtual IOReturn writeChipRegister(UInt16 address, UInt8 mask, UInt8 value) = 0;
    
    ///
    /// Dump the chip registers in the given range
    ///
    /// @param range The range of register addresses
    ///
    void dumpChipRegisters(ClosedRange<UInt16> range);
    
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
    virtual IOReturn readHostBufferGated(IOByteCount offset, void* buffer, IOByteCount length) = 0;
    
    ///
    /// Write to the host buffer form the given buffer
    ///
    /// @param offset A byte offset into the host buffer
    /// @param buffer A non-null buffer
    /// @param length The number of bytes to write
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note This function runs in a gated context.
    ///
    virtual IOReturn writeHostBufferGated(IOByteCount offset, const void* buffer, IOByteCount length) = 0;
    
    ///
    /// Read a value from the host buffer conveniently
    ///
    /// @tparam T Specify a value type, e.g., S/UInt{8, 16, 32, 64}
    /// @param offset A byte offset into the host command buffer
    /// @return The value.
    /// @note This function will trigger a kernel panic if the given offset is invalid.
    /// @note This function runs in a gated context.
    ///
    template <typename T>
    T readHostBufferValueGated(IOByteCount offset)
    {
        T value;
        
        passert(this->readHostBufferGated(offset, &value, sizeof(T)) == kIOReturnSuccess,
                "Failed to peek %lu bytes at offset %llu.", sizeof(T), offset);
        
        return value;
    }
    
    ///
    /// Write a value to the host buffer conveniently
    ///
    /// @param offset A byte offset into the host buffer
    /// @param value A value
    /// @note This function will trigger a kernel panic if the given offset is invalid.
    /// @note This function runs in a gated context.
    ///
    template <typename T>
    void writeHostBufferValueGated(IOByteCount offset, T value)
    {
        passert(this->writeHostBufferGated(offset, &value, sizeof(T)) == kIOReturnSuccess,
                "Failed to write %lu bytes at offset %llu.", sizeof(T), offset);
    }
    
public:
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
    /// Read a value from the host buffer conveniently
    ///
    /// @tparam T Specify a value type, e.g., S/UInt{8, 16, 32, 64}
    /// @param offset A byte offset into the host command buffer
    /// @return The value.
    /// @note This function will trigger a kernel panic if the given offset is invalid.
    ///
    template <typename T>
    T readHostBufferValue(IOByteCount offset)
    {
        T value;
        
        passert(this->readHostBuffer(offset, &value, sizeof(T)) == kIOReturnSuccess,
                "Failed to peek %lu bytes at offset %llu.", sizeof(T), offset);
        
        return value;
    }
    
    ///
    /// Write a value to the host buffer conveniently
    ///
    /// @tparam T Specify a value type, e.g., S/UInt{8, 16, 32, 64}
    /// @param offset A byte offset into the host buffer
    /// @param value A value
    /// @note This function will trigger a kernel panic if the given offset is invalid.
    ///
    template <typename T>
    void writeHostBufferValue(IOByteCount offset, T value)
    {
        passert(this->writeHostBuffer(offset, &value, sizeof(T)) == kIOReturnSuccess,
                "Failed to write %lu bytes at offset %llu.", sizeof(T), offset);
    }
    
    ///
    /// Dump the host buffer contents
    ///
    /// @param offset A byte offset into the host data buffer
    /// @param length The number of bytes to dump
    /// @param column The number of columns to print
    ///
    void dumpHostBuffer(IOByteCount offset, IOByteCount length, IOByteCount column = 8);
    
    //
    // MARK: - Host Command Management (Core)
    //
    
public:
    ///
    /// Start a new host command transfer session
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note The caller must invoke this function before any subsequent calls to `enqueue*Command()`.
    ///       Any commands enqueued before this function call will be overwritten.
    ///       Once all commands are enqueued, the caller should invoke `endCommandTransfer()` to send commands to the device.
    /// @note Port: This function replaces `rtsx_pci/usb_init_cmd()` defined in `rtsx_pci/usb.h`.
    ///
    IOReturn beginCommandTransfer();
    
protected:
    ///
    /// Enqueue a command
    ///
    /// @param command The command
    /// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full, `kIOReturnError` otherwise.
    /// @note Port: This function replaces `rtsx_pci/usb_add_cmd()` defined in `rtsx_pcr/usb.c`.
    /// @note This function runs in a gated context.
    ///
    virtual IOReturn enqueueCommandGated(const Command& command) = 0;
    
    ///
    /// Finish the existing host command transfer session
    ///
    /// @param timeout Specify the amount of time in milliseconds
    /// @param flags An optional flag, 0 by default
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    /// @note Port: This function replaces `rtsx_pci/usb_send_cmd()` defined in `rtsx_pcr/usb.c`.
    /// @note This function sends all commands in the queue to the device.
    /// @note This function runs in a gated context.
    ///
    virtual IOReturn endCommandTransferGated(UInt32 timeout, UInt32 flags) = 0;
    
public:
    ///
    /// Enqueue a command
    ///
    /// @param command The command
    /// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full, `kIOReturnError` otherwise.
    /// @note Port: This function replaces `rtsx_pci/usb_add_cmd()` defined in `rtsx_pcr/usb.c`.
    ///
    IOReturn enqueueCommand(const Command& command);
    
    ///
    /// Finish the existing host command transfer session
    ///
    /// @param timeout Specify the amount of time in milliseconds
    /// @param flags An optional flag, 0 by default
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    /// @note Port: This function replaces `rtsx_pci/usb_send_cmd()` defined in `rtsx_pcr/usb.c`.
    /// @note This function sends all commands in the queue to the device.
    ///
    IOReturn endCommandTransfer(UInt32 timeout = 100, UInt32 flags = 0);
    
    //
    // MARK: - Host Command Management (Convenient)
    //
    
public:
    ///
    /// Enqueue a command to read the register at the given address conveniently
    ///
    /// @param address The register address
    /// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full.
    /// @note Port: This function replaces `rtsx_pci/usb_add_cmd()` defined in `rtsx_psr/usb.c`.
    ///
    inline IOReturn enqueueReadRegisterCommand(UInt16 address)
    {
        pinfo("Enqueue: Address = 0x%04x; Mask = 0x%02x; Value = 0x%02x.", address, 0, 0);
        
        return this->enqueueCommand(Command::forReadRegister(address));
    }
    
    ///
    /// Enqueue a command to write a value to the register at the given address conveniently
    ///
    /// @param address The register address
    /// @param mask The register value mask
    /// @param value The register value
    /// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full.
    /// @note Port: This function replaces `rtsx_pci/usb_add_cmd()` defined in `rtsx_psr/usb.c`.
    ///
    inline IOReturn enqueueWriteRegisterCommand(UInt16 address, UInt8 mask, UInt8 value)
    {
        pinfo("Enqueue: Address = 0x%04x; Mask = 0x%02x; Value = 0x%02x.", address, mask, value);
        
        return this->enqueueCommand(Command::forWriteRegister(address, mask, value));
    }
    
    ///
    /// Enqueue a command to check the value of the register at the given address conveniently
    ///
    /// @param address The register address
    /// @param mask The register value mask
    /// @param value The register value
    /// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full.
    /// @note Port: This function replaces `rtsx_pci/usb_add_cmd()` defined in `rtsx_psr/usb.c`.
    ///
    inline IOReturn enqueueCheckRegisterCommand(UInt16 address, UInt8 mask, UInt8 value)
    {
        pinfo("Enqueue: Address = 0x%04x; Mask = 0x%02x; Value = 0x%02x.", address, mask, value);
        
        return this->enqueueCommand(Command::forCheckRegister(address, mask, value));
    }
    
    ///
    /// Enqueue a sequence of commands to read registers conveniently
    ///
    /// @param pairs A sequence of pairs of register address and value
    /// @return `kIOReturnSuccess` on success, `kIOReturnError` otherwise.
    /// @note This function provides an elegant way to enqueue multiple commands and handle errors.
    ///
    IOReturn enqueueReadRegisterCommands(const ChipRegValuePairs& pairs);
    
    ///
    /// Enqueue a sequence of commands to write registers conveniently
    ///
    /// @param pairs A sequence of pairs of register address and value
    /// @return `kIOReturnSuccess` on success, `kIOReturnError` otherwise.
    /// @note This function provides an elegant way to enqueue multiple commands and handle errors.
    ///
    IOReturn enqueueWriteRegisterCommands(const ChipRegValuePairs& pairs);
    
    ///
    /// Transfer a sequence of commands to read registers conveniently
    ///
    /// @param pairs A sequence of pairs of register address and value
    /// @param timeout Specify the amount of time in milliseconds
    /// @param flags An optional flag, 0 by default
    /// @return `kIOReturnSuccess` on success, `kIOReturnError` otherwise.
    /// @note This function provides an elegant way to start a command transfer session and handle errors.
    ///       Same as calling `startCommandTransfer`, a sequence of `enqueueReadRegisterCommand` and `endCommandTransfer`.
    ///
    IOReturn transferReadRegisterCommands(const ChipRegValuePairs& pairs, UInt32 timeout = 100, UInt32 flags = 0);
    
    ///
    /// Transfer a sequence of commands to write registers conveniently
    ///
    /// @param pairs A sequence of pairs of register address and value
    /// @param timeout Specify the amount of time in milliseconds
    /// @param flags An optional flag, 0 by default
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    /// @note This function provides an elegant way to start a command transfer session and handle errors.
    ///       Same as calling `startCommandTransfer`, a sequence of `enqueueWriteRegisterCommand` and `endCommandTransfer`.
    ///
    IOReturn transferWriteRegisterCommands(const ChipRegValuePairs& pairs, UInt32 timeout = 100, UInt32 flags = 0);
    
    ///
    /// Launch a custom command transfer session conveniently
    ///
    /// @param action A non-null action that enqueues any host commands
    /// @param context An optional user-defined context that will be passed to the given action
    /// @param timeout Specify the amount of time in milliseconds
    /// @param flags An optional flag, 0 by default
    /// @return `kIOReturnSuccess` on success, `kIOReturnBadArgument` if the given action is null,
    ///         `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    /// @note This function provides an elegant way to start a command transfer session and handle errors.
    ///       Same as calling `startCommandTransfer`, a sequence of enqueue invocations and `endCommandTransfer`.
    ///
    IOReturn withCustomCommandTransfer(EnqueueAction action, void* context = nullptr, UInt32 timeout = 100, UInt32 flags = 0);
    
    //
    // MARK: - Host Data Management
    //
    
public:
    ///
    /// Perform a DMA read operation
    ///
    /// @param descriptor A non-null, perpared memory descriptor
    /// @param timeout Specify the amount of time in milliseconds
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    ///
    virtual IOReturn performDMARead(IOMemoryDescriptor* descriptor, UInt32 timeout) = 0;
    
    ///
    /// Perform a DMA write operation
    ///
    /// @param descriptor A non-null, perpared memory descriptor
    /// @param timeout Specify the amount of time in milliseconds
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    ///
    virtual IOReturn performDMAWrite(IOMemoryDescriptor* descriptor, UInt32 timeout) = 0;
    
    //
    // MARK: - Clear Error
    //
    
protected:
    ///
    /// Clear any transfer error on the card side
    ///
    /// @note Port: This function replaces the code block that stops the card and clears the card errorin `sd_clear_error()` defined in `rtsx_pci/usb_sdmmc.c`.
    ///
    virtual void clearCardError() = 0;
    
    ///
    /// Clear any transfer error on the host side
    ///
    /// @note This function is invoked when a command or data transfer has failed.
    ///
    virtual void clearHostError() = 0;
    
public:
    ///
    /// Clear any transfere error on the both sides
    ///
    /// @note This function invokes `clearCardError()` and `clearHostError()`.
    /// @note Port: This function replaces `sd_clear_error()` defined in `rtsx_pci/usb_sdmmc.c`.
    ///
    void clearError();
    
    //
    // MARK: - LED Management
    //
    
public:
    ///
    /// Turn on the LED
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    ///
    virtual IOReturn turnOnLED() = 0;
    
    ///
    /// Turn off the LED
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    ///
    virtual IOReturn turnOffLED() = 0;
    
    ///
    /// Enable LED blinking
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    ///
    virtual IOReturn enableLEDBlinking() = 0;
    
    ///
    /// Disable LED blinking
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    ///
    virtual IOReturn disableLEDBlinking() = 0;
    
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
    virtual IOReturn selectCard() = 0;
    
    ///
    /// Select the data source for the SD card
    ///
    /// @param ppbuf `True` if the data source should be set to the ping pong buffer;
    ///              `False` if the data source should be the ring buffer instead
    /// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full, `kIOReturnError` otherwise.
    /// @note This function invokes `enqueueWriteRegisterCommand()` thus must be invoked between `beginCommandTransfer()` and `endCommandTransfer()`.
    /// @note The caller may use `withCustomCommandTransfer()` to combine this operation with other ones.
    ///
    virtual IOReturn selectCardDataSource(bool ppbuf) = 0;
    
    ///
    /// Select the data source to the ping pong buffer for the SD card conveniently
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full, `kIOReturnError` otherwise.
    /// @note This function invokes `enqueueWriteRegisterCommand()` thus must be invoked between `beginCommandTransfer()` and `endCommandTransfer()`.
    /// @note The caller may use `withCustomCommandTransfer()` to combine this operation with other ones.
    ///
    inline IOReturn selectCardDataSourceToPingPongBuffer()
    {
        return this->selectCardDataSource(true);
    }

    ///
    /// Select the data source to the ring buffer for the SD card conveniently
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full, `kIOReturnError` otherwise.
    /// @note This function invokes `enqueueWriteRegisterCommand()` thus must be invoked between `beginCommandTransfer()` and `endCommandTransfer()`.
    /// @note The caller may use `withCustomCommandTransfer()` to combine this operation with other ones.
    ///
    inline IOReturn selectCardDataSourceToRingBuffer()
    {
        return this->selectCardDataSource(false);
    }
    
    ///
    /// Configure the card share mode
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full, `kIOReturnError` otherwise.
    /// @note This function invokes `enqueueWriteRegisterCommand()` thus must be invoked between `beginCommandTransfer()` and `endCommandTransfer()`.
    /// @note The caller may use `withCustomCommandTransfer()` to combine this operation with other ones.
    ///
    virtual IOReturn configureCardShareMode() = 0;
    
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
    virtual IOReturn setupCardDMATransferProperties(UInt32 length, IODirection direction) = 0;
    
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
    virtual IOReturn enableCardClock() = 0;
    
    ///
    /// Disable the card clock
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full, `kIOReturnError` otherwise.
    /// @note This function invokes `enqueueWriteRegisterCommand()` thus must be invoked between `beginCommandTransfer()` and `endCommandTransfer()`.
    /// @note The caller may use `withCustomCommandTransfer()` to combine this operation with other ones.
    ///
    virtual IOReturn disableCardClock() = 0;
    
    ///
    /// Enable the card output
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full, `kIOReturnError` otherwise.
    /// @note This function invokes `enqueueWriteRegisterCommand()` thus must be invoked between `beginCommandTransfer()` and `endCommandTransfer()`.
    /// @note The caller may use `withCustomCommandTransfer()` to combine this operation with other ones.
    ///
    virtual IOReturn enableCardOutput() = 0;
    
    ///
    /// Disable the card output
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full, `kIOReturnError` otherwise.
    /// @note This function invokes `enqueueWriteRegisterCommand()` thus must be invoked between `beginCommandTransfer()` and `endCommandTransfer()`.
    /// @note The caller may use `withCustomCommandTransfer()` to combine this operation with other ones.
    ///
    virtual IOReturn disableCardOutput() = 0;
    
    ///
    /// Power on the SD card
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    ///
    virtual IOReturn powerOnCard() = 0;
    
    ///
    /// Power off the SD card
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    ///
    virtual IOReturn powerOffCard() = 0;
    
    ///
    /// Switch to the given output voltage
    ///
    /// @param outputVoltage The new output voltage
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    ///
    virtual IOReturn switchOutputVoltage(OutputVoltage outputVoltage) = 0;
    
    //
    // MARK: - Card Clock Configurations
    //
    
protected:
    ///
    /// Get the card clock and the divider for the initial mode
    ///
    /// @return A pair of card clock in Hz (first) and the clock divider register value (second).
    /// @note This function uses a 30 MHz carc clock and a divider of 128 as the default values.
    ///       RTS5261 controller must override this function to set a higher initial card clock
    ///       and divider for chips whose revision is higher than C.
    ///
    virtual Pair<UInt32, UInt32> getInitialCardClockAndDivider();
    
    ///
    /// Adjust the card clock if DMA transfer errors occurred
    ///
    /// @param cardClock The current card clock in Hz
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
    /// @note Port: This function replaces `conv_clk_and_div_n()` defined in `rtsx_psr.c` and the code block in `rtsx_usb_switch_clock()` defined in `rtsx_usb.c`.
    ///             RTL8411 series controllers must override this function.
    ///
    virtual UInt32 sscClock2DividerN(UInt32 clock);
    
    ///
    /// Convert the given divider N back to the SSC clock
    ///
    /// @param n The divider N
    /// @return The SSC clock in MHz.
    /// @note Port: This function replaces `conv_clk_and_div_n()` defined in `rtsx_psr.c` and the code block in `rtsx_usb_switch_clock()` defined in `rtsx_usb.c`.
    ///             RTL8411 series controllers must override this function.
    ///
    virtual UInt32 dividerN2SSCClock(UInt32 n);
    
    ///
    /// Calculate the number of MCUs from the given SSC clock
    ///
    /// @param clock The SSC clock in MHz
    /// @return The MCU count.
    /// @note Port: This function replaces the code block that calculates the MCU count in `rtsx_pci/usb_switch_clock()` defined in `rtsx_psr/usb.c`.
    /// @note Concrete controllers must ensure that the returned MCU count is less than or equal to 15.
    ///
    virtual UInt32 sscClock2MCUCount(UInt32 clock) = 0;
    
    ///
    /// Convert the given SSC depth to the actual register value
    ///
    /// @param depth The SSC depth
    /// @return The register value.
    ///
    virtual UInt8 sscDepth2RegValue(SSCDepth depth) = 0;
    
    ///
    /// Revise the given SSC depth register value
    ///
    /// @param depth The SSC depth register value
    /// @param divider The SSC clock divider value
    /// @return The revised SSC depth register value.
    ///
    virtual UInt8 reviseSSCDepthRegValue(UInt8 depth, UInt8 divider) = 0;
    
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
    virtual IOReturn switchCardClock(UInt8 depth, UInt8 n, UInt8 divider, UInt8 mcus, bool vpclock) = 0;
    
public:
    ///
    /// Switch to the given card clock
    ///
    /// @param cardClock The card clock in Hz
    /// @param sscDepth The SSC depth value
    /// @param initialMode Pass `true` if the card is at its initial stage
    /// @param doubleClock Pass `true` if the SSC clock should be doubled
    /// @param vpclock Pass `true` if VPCLOCK is used
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_pci/usb_switch_clock()` defined in `rtsx_psr/usb.c`.
    ///
    IOReturn switchCardClock(UInt32 cardClock, SSCDepth sscDepth, bool initialMode, bool doubleClock, bool vpclock);
    
    //
    // MARK: - Card Detection and Write Protection
    //
    
public:
    ///
    /// Check whether the card has write protection enabled
    ///
    /// @return `true` if the card is write protected, `false` otherwise.
    ///
    virtual bool isCardWriteProtected() = 0;
    
    ///
    /// Check whether a card is present
    ///
    /// @return `true` if a card exists, `false` otherwise.
    /// @warning: This function supports SD cards only.
    ///
    virtual bool isCardPresent() = 0;
    
    ///
    /// Check whether the command line of the card is busy
    ///
    /// @return `true` if the card CMD line is busy, `false` otherwise.
    /// @warning This function returns `true` if failed to read the register.
    ///
    virtual bool isCardCommandLineBusy() = 0;
    
    ///
    /// Check whether the data line of the card is busy
    ///
    /// @return `true` if the card DAT lines are busy, `false` otherwise.
    /// @warning This function returns `true` if failed to read the register.
    ///
    virtual bool isCardDataLineBusy() = 0;
    
    //
    // MARK: - Card Pull Control Management
    //
    
public:
    ///
    /// Enable pull control for the SD card
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    ///
    virtual IOReturn enablePullControlForSDCard() = 0;
    
    ///
    /// Disable pull control for the SD card
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    ///
    virtual IOReturn disablePullControlForSDCard() = 0;
    
    //
    // MARK: - Card Tuning & Phase Management
    //
    
public:
    ///
    /// Change the Rx phase
    ///
    /// @param samplePoint The sample point value
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces the Rx portion of `sd_change_phase()` defined in `rtsx_pci/usb_sdmmc.c`.
    ///
    virtual IOReturn changeRxPhase(UInt8 samplePoint) = 0;
    
    ///
    /// Change the Tx phase
    ///
    /// @param samplePoint The sample point value
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces the Tx portion of `sd_change_phase()` defined in `rtsx_pci/usb_sdmmc.c`.
    ///
    virtual IOReturn changeTxPhase(UInt8 samplePoint) = 0;
    
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
    ///
    DEPRECATE("Use readPingPongBuffer(IOIOMemoryDescriptor*, IOByteCount) to avoid unnecessary buffer copies.")
    virtual IOReturn readPingPongBuffer(UInt8* destination, IOByteCount length) = 0;
    
    ///
    /// Write to the ping pong buffer
    ///
    /// @param source The buffer to write
    /// @param length The number of bytes to write (cannot exceed 512 bytes)
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    ///
    DEPRECATE("Use readPingPongBuffer(IOIOMemoryDescriptor*, IOByteCount) to avoid unnecessary buffer copies.")
    virtual IOReturn writePingPongBuffer(const UInt8* source, IOByteCount length) = 0;
    
    ///
    /// Read from the ping pong buffer
    ///
    /// @param destination The buffer to store bytes
    /// @param length The number of bytes to read (cannot exceed 512 bytes)
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    ///
    virtual IOReturn readPingPongBuffer(IOMemoryDescriptor* destination, IOByteCount length) = 0;
    
    ///
    /// Write to the ping pong buffer
    ///
    /// @param source The buffer to write
    /// @param length The number of bytes to write (cannot exceed 512 bytes)
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    ///
    virtual IOReturn writePingPongBuffer(IOMemoryDescriptor* source, IOByteCount length) = 0;
    
    //
    // MARK: - Active State Power Management
    //
    
public:
    ///
    /// Notify the card reader to enter into the worker state
    ///
    virtual void enterWorkerState() {};
    
    //
    // MARK: - Power Management
    //
    
protected:
    ///
    /// Prepare to enter the sleep state
    ///
    virtual void prepareToSleep() = 0;
    
    ///
    /// Prepare to wake up from sleep
    ///
    virtual void prepareToWakeUp() = 0;
    
public:
    ///
    /// Adjust the power state in response to system-wide power events
    ///
    /// @param powerStateOrdinal The number in the power state array of the state the driver is being instructed to switch to
    /// @param whatDevice A pointer to the power management object which registered to manage power for this device
    /// @return `kIOPMAckImplied` always.
    ///
    IOReturn setPowerState(unsigned long powerStateOrdinal, IOService* whatDevice) override;
    
    //
    // MARK: - Interrupt Service Routines
    //
    
protected:
    ///
    /// Helper interrupt service routine when a SD card is inserted
    ///
    /// @param completion A nullable completion routine to be invoked when the card is attached
    /// @note This interrupt service routine runs in a gated context.
    /// @note Port: This function replaces `rtsx_pci_card_detect()` defined in `rtsx_psr.c` but has a completely different design and implementation.
    ///             This function is invoked by the polling thread when a SD card has been inserted to the USB card reader.
    ///
    void onSDCardInsertedGated(IOSDCard::Completion* completion = nullptr);
    
    ///
    /// Helper interrupt service routine when a SD card is removed
    ///
    /// @param completion A nullable completion routine to be invoked when the card is detached
    /// @note This interrupt service routine runs in a gated context.
    /// @note Port: This function replaces `rtsx_pci_card_detect()` defined in `rtsx_psr.c` but has a completely different design and implementation.
    ///             This function is invoked by the polling thread when a SD card has been removed from the USB card reader.
    ///
    void onSDCardRemovedGated(IOSDCard::Completion* completion = nullptr);
    
    //
    // MARK: - Startup Routines
    //
    
private:
    ///
    /// Setup the power management
    ///
    /// @param provider The provider
    /// @return `true` on success, `false` otherwise.
    ///
    bool setupPowerManagement(IOService* provider);
    
    ///
    /// Setup the workloop
    ///
    /// @return `true` on success, `false` otherwise.
    ///
    bool setupWorkLoop();
    
    //
    // MARK: - Teardown Routines
    //
    
    ///
    /// Tear down the power management
    ///
    void tearDownPowerManagement();
    
    ///
    /// Tear down the workloop
    ///
    void tearDownWorkLoop();
    
public:
    //
    // MARK: - IOService Implementations
    //
    
    ///
    /// Initialize the controller
    ///
    /// @return `true` on success, `false` otherwise.
    ///
    bool init(OSDictionary* dictionary = nullptr) override;
    
    ///
    /// Start the controller
    ///
    /// @param provider The provider
    /// @return `true` on success, `false` otherwise.
    ///
    bool start(IOService* provider) override;
    
    ///
    /// Stop the controller
    ///
    /// @param provider The provider
    ///
    void stop(IOService* provider) override;
};

#endif /* RealtekCardReaderController_hpp */
