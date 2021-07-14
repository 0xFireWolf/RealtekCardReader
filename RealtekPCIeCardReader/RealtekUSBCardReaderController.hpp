//
//  RealtekUSBCardReaderController.hpp
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 7/9/21.
//

#ifndef RealtekUSBCardReaderController_hpp
#define RealtekUSBCardReaderController_hpp

#include "RealtekCardReaderController.hpp"
#include "IOUSBHostInterface.hpp"
#include "Utilities.hpp"

///
/// Represents the USB-based card reader controller
///
class RealtekUSBCardReaderController: public RealtekCardReaderController
{
    //
    // MARK: - Constructors & Destructors
    //
    
    OSDeclareDefaultStructors(RealtekUSBCardReaderController);
    
    using super = RealtekCardReaderController;
    
    //
    // MARK: - Constants: SSC Clock Properties
    //

    /// The minimum SSC clock N value
    static constexpr UInt32 kMinSSCClockN = 60;
    
    /// The maximum SSC clock N value
    static constexpr UInt32 kMaxSSCClockN = 120;
    
    /// The amount of time in microseconds to wait until the SSC clock becomes stable
    static constexpr UInt32 kWaitStableSSCClock = 130;
    
    /// The minimum SSC clock frequency in MHz
    static constexpr UInt32 kMinSSCClockFrequencyMHz = 2;
    
    //
    // MARK: - Constants: Tuning Configurations
    //
    
    /// The number of phases
    static constexpr UInt32 kTuningNumPhases = 16;
    
    /// The controller should not enable the 80 clocks timeout
    static constexpr UInt32 kTuningEnable80ClocksTimes = false;
    
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
    // MARK: - Constants: SD Pull Control Tables
    //
    
    /// A sequence of chip registers to enable SD pull control (LQFP48 Package)
    static const ChipRegValuePair kSDEnablePullControlTablePairs_lqfp48[];
    static const SimpleRegValuePairs kSDEnablePullControlTable_lqfp48;
    
    /// A sequence of chip registers to enable SD pull control (QFN24 Package)
    static const ChipRegValuePair kSDEnablePullControlTablePairs_qfn24[];
    static const SimpleRegValuePairs kSDEnablePullControlTable_qfn24;
    
    /// A sequence of chip registers to disable SD pull control (LQFP48 Package)
    static const ChipRegValuePair kSDDisablePullControlTablePairs_lqfp48[];
    static const SimpleRegValuePairs kSDDisablePullControlTable_lqfp48;
    
    /// A sequence of chip registers to disable SD pull control (QFN24 Package)
    static const ChipRegValuePair kSDDisablePullControlTablePairs_qfn24[];
    static const SimpleRegValuePairs kSDDisablePullControlTable_qfn24;
    
    //
    // MARK: - Constants: Host Command & Data Buffer
    //
    
    // TODO: Introduce a namespace `HostBuffer`
    // TODO: HostBuffer::Offset, HostBuffer::kSize, HostBuffer::kMaxNumCommands
    
    /// Special host buffer offset
    enum Offset: IOByteCount
    {
        /// Offset 0: Header tag "R"
        kHeaderTag0 = 0,
        
        /// Offset 1: Header tag "T"
        kHeaderTag1 = 1,
        
        /// Offset 2: Header tag "C"
        kHeaderTag2 = 2,
        
        /// Offset 3: Header tag "R"
        kHeaderTag3 = 3,
        
        ///
        /// Offset 4: Packet type
        ///
        /// @see `enum class PacketType`
        ///
        kPacketType = 4,
        
        ///
        /// Offset 5: Item count (High 8 bits)
        ///
        /// @note If the packet type is `kBatchCommand`,
        ///       the item count is the number of commands in the host buffer,
        ///       otherwise it is the number of registers to access sequentially.
        ///
        kItemCntH8b = 5,
        
        ///
        /// Offset 6: Item count (Low 8 bits)
        ///
        /// @note If the packet type is `kBatchCommand`,
        ///       the item count is the number of commands in the host buffer,
        ///       otherwise it is the number of registers to access sequentially.
        ///
        kItemCntL8b = 6,
        
        ///
        /// Offset 7: The stage flags
        ///
        /// @see `enum class Flag`
        ///
        kStageFlags = 7,
        
        ///
        /// Offset 8: The host command
        ///
        /// @note Each command is 4 bytes long, starting at the offset 0x08.
        ///
        kHostCmdOff = 8,
        
        ///
        /// Offset 12: A sequence of register values if the packet type is `kSeqWriteCommand`
        ///
        /// @note The sequence write command resides at the offset 0x08.
        ///       The list of register values start at the offset 0x0C.
        ///
        kSeqRegsVal = 12,
    };
    
    /// The host buffer size (by default 1024 bytes)
    static constexpr IOByteCount kHostBufferSize = 1024;
    
    /// The host buffer can hold up to 254 commands
    static constexpr IOItemCount kMaxNumCommands = (kHostBufferSize - Offset::kHostCmdOff) / 4;
    
    //
    // MARK: - Data Structures (Private)
    //
    
    /// Represents a packet in the host buffer
    struct PACKED Packet
    {
        /// The header value
        static constexpr UInt32 kHeader = 0x52435452;
        
        /// Enumerates all possible packet types
        enum class Type: UInt8
        {
            /// The packet contains a batch of commands to access chip registers
            kBatchCommand = 0,
            
            /// The packet contains a command to read from a contiguous sequence of registers
            kSeqReadCommand = 1,
            
            /// The packet contains a command to write to a contiguous sequence of registers
            kSeqWriteCommand = 2,
        };
        
        /// Stage bits in the host command buffer
        enum Stage: UInt8
        {
            kRead = 0x01,
            kDirectionIn = 0x02,
            kDirectionOut = 0x04,
            kStatusMS = 0x08,
            kStatusXD = 0x10,
        };
        
        /// Flags value passed to `endCommandTransfer()`
        enum Flags: UInt8
        {
            /// Send out a sequence of commands
            kC = 0x00,
            
            /// Send out a sequence of commands and expect the response
            kCR = Stage::kRead,
            
            /// Send out a sequence of commands, initiate an inbound transfer and expect the response
            kCDIR = Stage::kRead | Stage::kDirectionIn,
            
            /// Send out a sequence of commands, initiate an outbound transfer and expect the response
            kCDOR = Stage::kRead | Stage::kDirectionOut,
        };
        
        /// The header "RTCR"
        UInt32 header;
        
        /// The packet type
        Type type;
        
        /// The item count (encoded in big endian)
        UInt16 itemCount;
        
        /// The stage flags
        Flags flags;
        
        /// Create a packet
        Packet(Type type, UInt16 itemCount, Flags flags)
            : header(kHeader), type(type), itemCount(OSSwapInt16(itemCount)), flags(flags) {}
        
        ///
        /// [Convenient] Create a packet for issuing a batch of commands to access chip registers
        ///
        /// @param ncmds The number of commands in the host buffer
        /// @param flags The stage flags
        /// @note Port: This function partially replaces `rtsx_usb_init_cmd()` defined in `rtsx_usb.h`.
        ///
        static inline Packet forBatchCommand(UInt16 ncmds, Flags flags)
        {
            return Packet(Type::kBatchCommand, ncmds, flags);
        }
        
        ///
        /// [Convenient] Create a packet for issuing a command to read a contiguous sequence of registers
        ///
        /// @param nregs The number of registers to read
        /// @note Port: This function partially replaces `rtsx_usb_seq_cmd_hdr()` defined in `rtsx_usb.c`.
        ///
        static inline Packet forSeqReadCommand(UInt16 nregs)
        {
            return Packet(Type::kSeqReadCommand, nregs, Flags::kCR);
        }
        
        ///
        /// [Convenient] Create a packet for issuing a command to write a contiguous sequence of registers
        ///
        /// @param nregs The number of registers to write
        /// @note Port: This function partially replaces `rtsx_usb_seq_cmd_hdr()` defined in `rtsx_usb.c`.
        ///
        static inline Packet forSeqWriteCommand(UInt16 nregs)
        {
            return Packet(Type::kSeqWriteCommand, nregs, Flags::kC);
        }
    };
    
    /// ABI Checks
    static_assert(sizeof(Packet) == Offset::kHostCmdOff, "ABI Error: The packet is not 8 bytes long.");
    static_assert(offsetof(Packet, header) == Offset::kHeaderTag0, "ABI Error: Header offset is not 0");
    static_assert(offsetof(Packet, type) == Offset::kPacketType, "ABI Error: Packet type offset is not 5.");
    static_assert(offsetof(Packet, itemCount) == Offset::kItemCntH8b, "ABI Error: Item count offset is not 6.");
    static_assert(offsetof(Packet, flags) == Offset::kStageFlags, "ABI Error: Flags offset is not 7.");
    
    ///
    /// A temporary quartet for bulk transfer
    ///
    /// @see `RealtekUSBCardReaderController::performInboundBulkTransfer(buffer:length:timeout:)`
    /// @see `RealtekUSBCardReaderController::performOutboundBulkTransfer(buffer:length:timeout:)`
    ///
    struct BulkTransferContext
    {
        /// The card reader controller
        RealtekUSBCardReaderController* controller;
        
        /// The buffer involved in the bulk transfer
        void* buffer;
        
        /// The buffer length
        IOByteCount length;
        
        /// The timeout value in milliseconds
        UInt32 timeout;
        
        /// Padding
        UInt32 reserved;
        
        /// Create the context
        BulkTransferContext(RealtekUSBCardReaderController* controller, void* buffer, IOByteCount length, UInt32 timeout)
            : controller(controller), buffer(buffer), length(length), timeout(timeout), reserved(0) {}
    };
    
    /// A namespace that defines constants and utilities related to USB endpoints
    struct Endpoints
    {
        /// The control endpoint
        struct Control
        {
            /// Constants
            static constexpr UInt16 kOperationShift = 14;
            static constexpr UInt16 kReadRegister = 2;
            static constexpr UInt16 kWriteRegister = 3;
            
            /// USB vendor request (DeviceRequest::bRequest value)
            enum VendorRequest: UInt8
            {
                /// Register operation
                kRegister = 0x00,
                
                /// Poll operation
                kPoll = 0x02,
            };
            
            ///
            /// Get the value for a control request to read a chip register
            ///
            /// @param address The register address
            /// @return The value of `DeviceRequest::wValue`.
            ///
            static inline UInt16 deviceRequestValueForReadRegisterCommand(UInt16 address)
            {
                address |= (kReadRegister << kOperationShift);
                
                return OSSwapInt16(address);
            }
            
            ///
            /// Get the value for a control request to write a chip register
            ///
            /// @param address The register address
            /// @return The value of `DeviceRequest::wValue`.
            ///
            static inline UInt16 deviceRequestValueForWriteRegisterCommand(UInt16 address)
            {
                address |= (kWriteRegister << kOperationShift);
                
                return OSSwapInt16(address);
            }
        };
        
        /// The bulk endpoints
        struct Bulk
        {
            /// Endpoint address of the input bulk endpoint
            static constexpr UInt8 kInput = 1;
            
            /// Endpoint address of the output bulk endpoint
            static constexpr UInt8 kOutput = 2;
        };
        
        /// The interrupt endpoint
        struct Interrupt
        {
            /// Endpoint address of the interrupt endpoint
            static constexpr UInt8 kInterrupt = 3;
        };
    };
    
    ///
    /// Defines bits in the card status
    ///
    /// @see `RealtekUSBCardReaderController::getCardStatus()`
    ///
    enum CardStatus: UInt16
    {
        /// A SD card is present
        kSDPresent = 0x01,
        
        /// A MS card is present
        kMSPresent = 0x02,
        
        /// A XD card is present
        kXDPresent = 0x04,
        
        /// A card is present
        kCardPresent = kSDPresent | kMSPresent | kXDPresent,
        
        /// The SD card is write protected
        kSDWriteProtected = 0x08,
    };
    
    //
    // MARK: - IOKit Basics
    //
    
    /// The USB host interface (provider)
    IOUSBHostInterface* interface;
    
    /// The input bulk pipe
    IOUSBHostPipe* inputPipe;
    
    /// The output bulk pipe
    IOUSBHostPipe* outputPipe;
    
    //
    // MARK: - Host States
    //
    
    /// True if the packet is LQFT48
    bool isLQFT48;
    
    /// True if the chip is RTS5179
    bool isRTS5179;
    
    /// Chip revision
    UInt8 revision;
    
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
    IOReturn readChipRegister(UInt16 address, UInt8& value) override final;
    
    ///
    /// Write a byte to the chip register at the given address
    ///
    /// @param address The register address
    /// @param mask The register value mask
    /// @param value The register value
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out.
    /// @note Port: This function replaces `rtsx_usb_write_register()` defined in `rtsx_usb.c`.
    ///
    IOReturn writeChipRegister(UInt16 address, UInt8 mask, UInt8 value) override final;
    
    ///
    /// Read from a contiguous sequence of chip registers
    ///
    /// @param address The starting register address
    /// @param count The number of registers to read
    /// @param destination A non-null buffer that stores the registers value on return
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out.
    /// @note Port: This function replaces `rtsx_usb_seq_read_register()` defined in `rtsx_usb.c`.
    ///
    IOReturn readChipRegistersSequentially(UInt16 address, UInt16 count, UInt8* destination);
    
    ///
    /// Write to a contiguous sequence of chip registers
    ///
    /// @param address The starting register address
    /// @param count The number of registers to write
    /// @param source A non-null buffer that contains the registers value
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out.
    /// @note Port: This function replaces `rtsx_usb_seq_read_register()` defined in `rtsx_usb.c`.
    ///
    IOReturn writeChipRegistersSequentially(UInt16 address, UInt16 count, const UInt8* source);
    
    ///
    /// Read a byte from the chip register at the given address via the control endpoint
    ///
    /// @param address The register address
    /// @param value The register value on return
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out.
    /// @note Port: This function replaces `rtsx_usb_ep0_write_register()` defined in `rtsx_usb.c`.
    ///
    IOReturn readChipRegisterViaControlEndpoint(UInt16 address, UInt8& value);
    
    ///
    /// Write a byte to the chip register at the given address via the control endpoint
    ///
    /// @param address The register address
    /// @param mask The register value mask
    /// @param value The register value
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out.
    /// @note Port: This function replaces `rtsx_usb_ep0_read_register()` defined in `rtsx_usb.c`.
    ///
    IOReturn writeChipRegisterViaControlEndpoint(UInt16 address, UInt8 mask, UInt8 value);
    
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
    IOReturn writePhysRegister(UInt8 address, UInt8 value);
    
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
    /// Write the given packet to the host buffer
    ///
    /// @param packet The packet to write
    /// @note This function runs in a gated context.
    ///
    inline void writePacketToHostBufferGated(const Packet& packet)
    {
        this->writeHostBufferValueGated(Offset::kHeaderTag0, packet);
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
    IOReturn enqueueCommandGated(const Command& command) override final;
    
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
    IOReturn endCommandTransferGated(UInt32 timeout, UInt32 flags) override final;
    
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
    IOReturn performDMARead(IODMACommand* command, UInt32 timeout) override;
    
    ///
    /// Perform a DMA write operation
    ///
    /// @param command A non-null, perpared DMA command
    /// @param timeout Specify the amount of time in milliseconds
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    ///
    IOReturn performDMAWrite(IODMACommand* command, UInt32 timeout) override;
    
    //
    // MARK: - Clear Error
    //
    
    ///
    /// Clear any transfer error on the card side
    ///
    /// @note Port: This function replaces the code block that stops the card and clears the card errorin `sd_clear_error()` defined in `rtsx_pci/usb_sdmmc.c`.
    ///
    void clearCardError() override final;
    
    ///
    /// Clear any transfer error on the host side
    ///
    /// @note This function is invoked when a command or data transfer has failed.
    /// @note Port: This function replaces `rtsx_usb_clear_fsm_err()` and `rtsx_usb_clear_dma_err()` defined in `rtsx_usb.h`.
    ///
    void clearHostError() override final;
    
    //
    // MARK: - LED Management
    //
    
    ///
    /// Turn on the LED
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    /// @note Port: This function replaces `rtsx_usb_turn_on_led()` defined in `rtsx_usb.h`.
    ///
    IOReturn turnOnLED() override;
    
    ///
    /// Turn off the LED
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    /// @note Port: This function replaces `rtsx_usb_turn_off_led()` defined in `rtsx_usb.h`.
    ///
    IOReturn turnOffLED() override;
    
    ///
    /// Enable LED blinking
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    /// @note Port: USB-based card reader controllers do not support this feature,
    ///             so this function simply returns `kIOReturnSuccess`.
    ///
    IOReturn enableLEDBlinking() override;
    
    ///
    /// Disable LED blinking
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    /// @note Port: USB-based card reader controllers do not support this feature,
    ///             so this function simply returns `kIOReturnSuccess`.
    ///
    IOReturn disableLEDBlinking() override;
    
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
    IOReturn selectCard() override;
    
    ///
    /// Select the data source for the SD card
    ///
    /// @param ppbuf `True` if the data source should be set to the ping pong buffer;
    ///              `False` if the data source should be the ring buffer instead
    /// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full, `kIOReturnError` otherwise.
    /// @note This function invokes `enqueueWriteRegisterCommand()` thus must be invoked between `beginCommandTransfer()` and `endCommandTransfer()`.
    /// @note The caller may use `withCustomCommandTransfer()` to combine this operation with other ones.
    ///
    IOReturn selectCardDataSource(bool ppbuf) override;
    
    ///
    /// Configure the card share mode
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full, `kIOReturnError` otherwise.
    /// @note This function invokes `enqueueWriteRegisterCommand()` thus must be invoked between `beginCommandTransfer()` and `endCommandTransfer()`.
    /// @note The caller may use `withCustomCommandTransfer()` to combine this operation with other ones.
    ///
    IOReturn configureCardShareMode() override;
    
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
    IOReturn setupCardDMATransferProperties(UInt32 length, IODirection direction) override;
    
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
    IOReturn enableCardClock() override;
    
    ///
    /// Disable the card clock
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full, `kIOReturnError` otherwise.
    /// @note This function invokes `enqueueWriteRegisterCommand()` thus must be invoked between `beginCommandTransfer()` and `endCommandTransfer()`.
    /// @note The caller may use `withCustomCommandTransfer()` to combine this operation with other ones.
    ///
    IOReturn disableCardClock() override;
    
    ///
    /// Enable the card output
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full, `kIOReturnError` otherwise.
    /// @note This function invokes `enqueueWriteRegisterCommand()` thus must be invoked between `beginCommandTransfer()` and `endCommandTransfer()`.
    /// @note The caller may use `withCustomCommandTransfer()` to combine this operation with other ones.
    ///
    IOReturn enableCardOutput() override;
    
    ///
    /// Disable the card output
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnBusy` if the command buffer is full, `kIOReturnError` otherwise.
    /// @note This function invokes `enqueueWriteRegisterCommand()` thus must be invoked between `beginCommandTransfer()` and `endCommandTransfer()`.
    /// @note The caller may use `withCustomCommandTransfer()` to combine this operation with other ones.
    ///
    IOReturn disableCardOutput() override;
    
    ///
    /// Power on the SD card
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces the code block that powers on the card in `sd_power_on()` defined in `rtsx_usb_sdmmc.c`.
    ///
    IOReturn powerOnCard() override;
    
    ///
    /// Power off the SD card
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces the code block that powers off the card in `sd_power_off()` defined in `rtsx_usb_sdmmc.c`.
    ///
    IOReturn powerOffCard() override;
    
    ///
    /// Switch to the given output voltage
    ///
    /// @param outputVoltage The new output voltage
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces the code block that changes the voltage in `sdmmc_switch_voltage()` defined in `rtsx_usb_sdmmc.c`.
    ///
    IOReturn switchOutputVoltage(OutputVoltage outputVoltage) override;
    
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
    UInt32 sscClock2MCUCount(UInt32 clock) override;
    
    ///
    /// Convert the given SSC depth to the actual register value
    ///
    /// @param depth The SSC depth
    /// @return The register value.
    ///
    UInt8 sscDepth2RegValue(SSCDepth depth) override;
    
    ///
    /// Revise the given SSC depth register value
    ///
    /// @param depth The SSC depth register value
    /// @param divider The SSC clock divider value
    /// @return The revised SSC depth register value.
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
    
    ///
    /// Get the card status
    ///
    /// @param status The card status on return
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_usb_get_card_status()` and `rtsx_usb_get_status_with_bulk` defined in `rtsx_usb.c`.
    ///
    IOReturn getCardStatus(UInt16& status);
    
    ///
    /// Check whether the card has write protection enabled
    ///
    /// @return `true` if the card is write protected, `false` otherwise.
    ///
    bool isCardWriteProtected() override;
    
    ///
    /// Check whether a card is present
    ///
    /// @return `true` if a card exists, `false` otherwise.
    /// @note Port: This function replaces `rtsx_pci_card_exist()` and `cd_deglitch()` defined in `rtsx_psr.c`.
    /// @warning: This function supports SD cards only.
    ///
    bool isCardPresent() override;
    
    //
    // MARK: - Card Pull Control Management
    //
    
    ///
    /// Enable pull control for the SD card
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `sd_pull_ctl_enable_*()` defined in `rtsx_usb_sdmmc.c`.
    ///
    IOReturn enablePullControlForSDCard() override;
    
    ///
    /// Disable pull control for the SD card
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `sd_pull_ctl_disable_*()` defined in `rtsx_usb_sdmmc.c`.
    ///
    IOReturn disablePullControlForSDCard() override;
    
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
    IOReturn changeRxPhase(UInt8 samplePoint) override;
    
    ///
    /// Change the Tx phase
    ///
    /// @param samplePoint The sample point value
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces the Tx portion of `sd_change_phase()` defined in `rtsx_usb_sdmmc.c`.
    ///
    IOReturn changeTxPhase(UInt8 samplePoint) override;
    
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
    IOReturn readPingPongBuffer(UInt8* destination, IOByteCount length) override final;
    
    ///
    /// Write to the ping pong buffer
    ///
    /// @param source The buffer to write
    /// @param length The number of bytes to write (cannot exceed 512 bytes)
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_usb_write_ppbuf()` defined in `rtsx_usb.c`.
    ///
    IOReturn writePingPongBuffer(const UInt8* source, IOByteCount length) override final;
    
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
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note This function increases the timeout to 600 ms if the given timeout is less than 600 ms.
    /// @note This function returns an error if the actual number of bytes transferred is not identical to the given `length`.
    /// @note Port: This function in a sense replaces `rtsx_usb_transfer_data()` and `rtsx_usb_bulk_transfer_sglist()` defined in `rtsx_usb.c`.
    ///
    IOReturn performBulkTransfer(IOUSBHostPipe* pipe, IOMemoryDescriptor* buffer, IOByteCount length, UInt32 timeout);
    
    ///
    /// Perform an inbound transfer on the bulk endpoint
    ///
    /// @param buffer A memory descriptor that stores the data transferred
    /// @param length The total number of bytes to transfer
    /// @param timeout Specify the amount of time in milliseconds
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note This function increases the timeout to 600 ms if the given timeout is less than 600 ms.
    /// @note This function returns an error if the actual number of bytes transferred is not identical to the given `length`.
    /// @note Port: This function in a sense replaces `rtsx_usb_transfer_data()` and `rtsx_usb_bulk_transfer_sglist()` defined in `rtsx_usb.c`.
    ///
    inline IOReturn performInboundBulkTransfer(IOMemoryDescriptor* buffer, IOByteCount length, UInt32 timeout)
    {
        pinfo("Initiating an inbound bulk transfer with length = %llu bytes and timeout = %u ms...", length, timeout);
        
        return this->performBulkTransfer(this->inputPipe, buffer, length, timeout);
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
    inline IOReturn performInboundBulkTransfer(void* buffer, IOByteCount length, UInt32 timeout);
    
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
    inline IOReturn performOutboundBulkTransfer(IOMemoryDescriptor* buffer, IOByteCount length, UInt32 timeout)
    {
        pinfo("Initiating an outbound bulk transfer with length = %llu bytes and timeout = %u ms...", length, timeout);
        
        return this->performBulkTransfer(this->outputPipe, buffer, length, timeout);
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
    IOReturn performOutboundBulkTransfer(const void* buffer, IOByteCount length, UInt32 timeout);
    
    //
    // MARK: - Power Management
    //
    
    ///
    /// Prepare to enter the sleep state
    ///
    /// @note Port: This function replaces `rtsx_usb_suspend()` defined in `rtsx_usb.c`.
    ///
    void prepareToSleep() override;
    
    ///
    /// Prepare to wake up from sleep
    ///
    /// @note Port: This function replaces `rtsx_usb_resume()` defined in `rtsx_usb.c`.
    ///
    void prepareToWakeUp() override;
    
    //
    // MARK: - Hardware Initialization and Configuration
    //
    
    ///
    /// Reset the card reader chip
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_usb_reset_chip()` defined in `rtsx_usb.c`.
    ///
    IOReturn resetHardware();
    
    ///
    /// Initialize the card reader chip
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_usb_init_chip()` defined in `rtsx_usb.c`.
    ///
    IOReturn initHardware();
    
    //
    // MARK: - Startup Routines
    //
    
    ///
    /// Setup the USB interface and endpoints
    ///
    /// @return `true` on success, `false` otherwise.
    ///
    bool setupUSBInterface();
    
    ///
    /// Setup the host command and buffer management module
    ///
    /// @return `true` on success, `false` otherwise.
    ///
    bool setupHostBuffer();
    
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
    /// Tear down the USB interface and endpoints
    ///
    void tearDownUSBInterface();
    
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
    /// Initialize the controller
    ///
    /// @return `true` on success, `false` otherwise.
    ///
    bool init(OSDictionary* dictionary = nullptr) override;
    
    ///
    /// Start the controller
    ///
    /// @param provider An instance of USB host interface that represents the card reader
    /// @return `true` on success, `false` otherwise.
    ///
    bool start(IOService* provider) override;
    
    ///
    /// Stop the controller
    ///
    /// @param provider An instance of USB host interface that represents the card reader
    ///
    void stop(IOService* provider) override;
};

#endif /* RealtekUSBCardReaderController_hpp */
