//
//  IOSDHostDriver.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 6/1/21.
//

#ifndef IOSDHostDriver_hpp
#define IOSDHostDriver_hpp

#include <IOKit/IOCommandPool.h>
#include <IOKit/IOCommandGate.h>
#include "IOSDHostDevice.hpp"
#include "IOSDBusConfig.hpp"
#include "IOSDBlockRequest.hpp"
#include "IOSDSimpleBlockRequest.hpp"
#include "IOSDComplexBlockRequest.hpp"
#include "IOSDBlockRequestQueue.hpp"
#include "IOSDBlockRequestEventSource.hpp"
#include "IOSDCard.hpp"
#include "IOSDCardEventSource.hpp"
#include "SD.hpp"
#include "Utilities.hpp"

/// Forward declaration (Client of the SD host driver)
class IOSDBlockStorageDevice;

/// Generic SD host device driver
class IOSDHostDriver: public IOService
{
    //
    // MARK: - Constructors & Destructors
    //
    
    OSDeclareDefaultStructors(IOSDHostDriver);
    
    using super = IOService;
    
    //
    // MARK: - Private Properties
    //

    /// The default pool size
    static constexpr IOItemCount kDefaultPoolSize = 32;
    
    /// Preallocated DMA commands
    IODMACommand* pDMACommands[kDefaultPoolSize];
    
    /// Preallocated simple SD block requests
    IOSDSimpleBlockRequest* pSimpleBlockRequests[kDefaultPoolSize];
    
    /// Preallocated complex SD block requests
    IOSDComplexBlockRequest* pComplexBlockRequests[kDefaultPoolSize];
    
    /// The SD host device (provider)
    IOSDHostDevice* host;
    
    /// The SD block storage device (client)
    IOSDBlockStorageDevice* blockStorageDevice;
    
    /// A shared workloop that serializes the access to the pools and the queues
    IOWorkLoop* sharedWorkLoop;
    
    /// A command pool that contains preallocated DMA commands
    IOCommandPool* dmaCommandPool;
    
    /// A request pool that contains preallocated simple SD block requests
    IOCommandPool* simpleBlockRequestPool;
    
    /// A request pool that contains preallocated complex SD block requests
    IOCommandPool* complexBlockRequestPool;
    
    /// A list of pending requests (a subclass of `IOCommandPool` with convenient methods)
    IOSDBlockRequestQueue* pendingRequests;
    
    /// A dedicated workloop that initializes the card and processes the block request
    IOWorkLoop* processorWorkLoop;
    
    /// A command gate to run actions with respect to the processor workloop
    IOCommandGate* processorCommandGate;
    
    ///
    /// An event source to signal the processor workloop to process a pending block request
    ///
    /// @note The event source is enabled when a card is inserted and disabled when the card is removed.
    /// @note The enable status is manipulated by `IOSDHostDriver::onSDCardInsertedGated()` and `IOSDHostDriver::onSDCardRemovedGated()`.
    ///
    IOSDBlockRequestEventSource* queueEventSource;
    
    ///
    /// An event source to signal the processor workloop to attach a SD card
    ///
    /// @note Enable this event source to start to initialize the SD card.
    ///       This event source disables itself automatically to acknowledge the card insertion event.
    ///
    IOSDCardEventSource* attachCardEventSource;
    
    ///
    /// An event source to signal the processor workloop to detach a SD card
    ///
    /// @note Enable this event source to start to clean up the SD card.
    ///       This event source disables itself automatically to acknowledge the card removal event.
    ///
    IOSDCardEventSource* detachCardEventSource;
    
    /// The SD card (NULL if not inserted)
    IOSDCard* card;
    
public:
    
    //
    // MARK: - Pool Management
    //
    
    ///
    /// Allocate a DMA command from the pool
    ///
    /// @return A non-null DMA command.
    ///
    inline IODMACommand* allocateDMACommandFromPool()
    {
        return OSDynamicCast(IODMACommand, this->dmaCommandPool->getCommand());
    }
    
    ///
    /// Return the given DMA command to the pool
    ///
    /// @param command The command returned by `IOSDHostDriver::allocateDMACommandFromPool()`
    ///
    inline void releaseDMACommandToPool(IODMACommand* command)
    {
        this->dmaCommandPool->returnCommand(command);
    }
    
    ///
    /// Allocate a simple block request from the pool
    ///
    /// @return A non-null simple block request.
    ///
    inline IOSDSimpleBlockRequest* allocateSimpleBlockRequestFromPool()
    {
        return OSDynamicCast(IOSDSimpleBlockRequest, this->simpleBlockRequestPool->getCommand());
    }
    
    ///
    /// Return the given simple block request to the pool
    ///
    /// @param request The request returned by `IOSDHostDriver::allocateBlockRequestFromPool()`
    ///
    inline void releaseSimpleBlockRequestToPool(IOSDSimpleBlockRequest* request)
    {
        this->simpleBlockRequestPool->returnCommand(request);
    }
    
    ///
    /// Allocate a complex block request from the pool
    ///
    /// @return A non-null complex block request.
    ///
    inline IOSDComplexBlockRequest* allocateComplexBlockRequestFromPool()
    {
        return OSDynamicCast(IOSDComplexBlockRequest, this->complexBlockRequestPool->getCommand());
    }
    
    ///
    /// Return the given simple block request to the pool
    ///
    /// @param request The request returned by `IOSDHostDriver::allocateBlockRequestFromPool()`
    ///
    inline void releaseComplexBlockRequestToPool(IOSDComplexBlockRequest* request)
    {
        this->complexBlockRequestPool->returnCommand(request);
    }
    
    ///
    /// Return the given block request to the pool where it belongs
    ///
    /// @param request The request returned by one of the allocation functions.
    ///
    inline void releaseBlockRequestToPool(IOSDBlockRequest* request)
    {
        IOSDComplexBlockRequest* creq = OSDynamicCast(IOSDComplexBlockRequest, request);
        
        if (creq != nullptr)
        {
            this->releaseComplexBlockRequestToPool(creq);
            
            return;
        }
        
        IOSDSimpleBlockRequest* sreq = OSDynamicCast(IOSDSimpleBlockRequest, request);
        
        if (sreq != nullptr)
        {
            this->releaseSimpleBlockRequestToPool(sreq);
            
            return;
        }
        
        pfatal("Detected an invalid type of block request.");
    }
    
    ///
    /// Recycle all pending block requests and return them to the pool
    ///
    inline void recyclePendingBlockRequest()
    {
        // Recycle all pending requests
        while (!this->pendingRequests->isEmpty())
        {
            pinfo("Recycling a pending request...");
            
            this->releaseBlockRequestToPool(this->pendingRequests->dequeueRequest());
        }
    }
    
    //
    // MARK: - I/O Requests
    //
    
    ///
    /// Submit the given request to the queue
    ///
    /// @param processor The processor that services the request
    /// @param buffer The data transfer buffer
    /// @param block The starting block number
    /// @param nblocks The number of blocks to transfer
    /// @param attributes Attributes of the data transfer
    /// @param completion The completion routine to call once the data transfer completes
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    ///
    IOReturn submitBlockRequest(IOSDBlockRequest::Processor processor, IOMemoryDescriptor* buffer, UInt64 block, UInt64 nblocks, IOStorageAttributes* attributes, IOStorageCompletion* completion);
    
    ///
    /// Submit a request to read a single block
    ///
    /// @param buffer The data transfer buffer
    /// @param block The starting block number
    /// @param nblocks The number of blocks to transfer
    /// @param attributes Attributes of the data transfer
    /// @param completion The completion routine to call once the data transfer completes
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note The given arguments are guaranteed to be valid.
    ///
    inline IOReturn submitReadBlockRequest(IOMemoryDescriptor* buffer, UInt64 block, UInt64 nblocks, IOStorageAttributes* attributes, IOStorageCompletion* completion)
    {
        auto processor = OSMemberFunctionCast(IOSDBlockRequest::Processor, this, &IOSDHostDriver::processReadBlockRequest);
        
        return this->submitBlockRequest(processor, buffer, block, nblocks, attributes, completion);
    }
    
    ///
    /// Submit a request to write a single block
    ///
    /// @param buffer The data transfer buffer
    /// @param block The starting block number
    /// @param nblocks The number of blocks to transfer
    /// @param attributes Attributes of the data transfer
    /// @param completion The completion routine to call once the data transfer completes
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note The given arguments are guaranteed to be valid.
    ///
    inline IOReturn submitWriteBlockRequest(IOMemoryDescriptor* buffer, UInt64 block, UInt64 nblocks, IOStorageAttributes* attributes, IOStorageCompletion* completion)
    {
        auto processor = OSMemberFunctionCast(IOSDBlockRequest::Processor, this, &IOSDHostDriver::processWriteBlockRequest);
        
        return this->submitBlockRequest(processor, buffer, block, nblocks, attributes, completion);
    }
    
    ///
    /// Submit a request to read multiple blocks
    ///
    /// @param buffer The data transfer buffer
    /// @param block The starting block number
    /// @param nblocks The number of blocks to transfer
    /// @param attributes Attributes of the data transfer
    /// @param completion The completion routine to call once the data transfer completes
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note The given arguments are guaranteed to be valid.
    ///
    inline IOReturn submitReadBlocksRequest(IOMemoryDescriptor* buffer, UInt64 block, UInt64 nblocks, IOStorageAttributes* attributes, IOStorageCompletion* completion)
    {
        auto processor = OSMemberFunctionCast(IOSDBlockRequest::Processor, this, &IOSDHostDriver::processReadBlocksRequest);
        
        return this->submitBlockRequest(processor, buffer, block, nblocks, attributes, completion);
    }
    
    ///
    /// Submit a request to write multiple blocks
    ///
    /// @param buffer The data transfer buffer
    /// @param block The starting block number
    /// @param nblocks The number of blocks to transfer
    /// @param attributes Attributes of the data transfer
    /// @param completion The completion routine to call once the data transfer completes
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note The given arguments are guaranteed to be valid.
    ///
    inline IOReturn submitWriteBlocksRequest(IOMemoryDescriptor* buffer, UInt64 block, UInt64 nblocks, IOStorageAttributes* attributes, IOStorageCompletion* completion)
    {
        auto processor = OSMemberFunctionCast(IOSDBlockRequest::Processor, this, &IOSDHostDriver::processWriteBlocksRequest);
        
        return this->submitBlockRequest(processor, buffer, block, nblocks, attributes, completion);
    }
    
    ///
    /// Transform the given starting block number to the argument of CMD17/18/24/25 if necessary
    ///
    /// @param block The starting block number
    /// @return The argument to be passed to CMD17/18/24/25.
    ///         i.e. The given block number if the card is block addressed (SDHC/XC),
    ///              or the corresponding byte offset if the card is byte addressed (SDSC).
    ///
    UInt32 transformBlockOffsetIfNecessary(UInt64 block);
    
    ///
    /// Process the given request to read a single block
    ///
    /// @param request A non-null block request
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note The return value will be passed to the storage completion routine.
    /// @note When this function is invoked, the DMA command is guaranteed to be non-null and prepared.
    ///
    IOReturn processReadBlockRequest(IOSDBlockRequest* request);
    
    ///
    /// Process the given request to read multiple blocks
    ///
    /// @param request A non-null block request
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note The return value will be passed to the storage completion routine.
    /// @note When this function is invoked, the DMA command is guaranteed to be non-null and prepared.
    ///
    IOReturn processReadBlocksRequest(IOSDBlockRequest* request);
    
    ///
    /// Process the given request to write a single block
    ///
    /// @param request A non-null block request
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note The return value will be passed to the storage completion routine.
    /// @note When this function is invoked, the DMA command is guaranteed to be non-null and prepared.
    ///
    IOReturn processWriteBlockRequest(IOSDBlockRequest* request);
    
    ///
    /// Process the given request to write multiple blocks
    ///
    /// @param request A non-null block request
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note The return value will be passed to the storage completion routine.
    /// @note When this function is invoked, the DMA command is guaranteed to be non-null and prepared.
    ///
    IOReturn processWriteBlocksRequest(IOSDBlockRequest* request);
    
    ///
    /// Finalize a request that has been processed
    ///
    /// @param request A non-null block request
    /// @note This function is the completion routine registerd with the block request event source.
    ///       It deinitializes the given request and puts it back to the block request pool.
    ///
    void finalizeBlockRequest(IOSDBlockRequest* request);
    
    //
    // MARK: - Query Host Properties
    //
    
    ///
    /// Check whether the host supports the High Speed mode
    ///
    /// @return `true` if the High Speed mode is supported by the host.
    ///
    bool hostSupportsHighSpeedMode();
    
    ///
    /// Check whether the host supports the Ultra High Speed mode
    ///
    /// @return `true` if the Ultra High Speed mode is supported by the host.
    ///
    bool hostSupportsUltraHighSpeedMode();
    
    ///
    /// Check whether the host supports the 4-bit bus width
    ///
    /// @return `true` if the 4-bit bus width is supported by the host.
    ///
    bool hostSupports4BitBusWidth();
    
    ///
    /// Get the maximum current setting at its current voltage
    ///
    /// @return The maximum current in mA.
    /// @note Port: This function replaces `sd_get_host_max_current()` defined in `sd.c`.
    ///
    UInt32 getHostMaxCurrent();
    
    ///
    /// Select voltage levels mutually supported by the host and the card
    ///
    /// @param ocr The card OCR value
    /// @return The OCR value that contains voltage levels supported by both parties.
    /// @note Port: This function replaces `mmc_select_voltage()` defined in `core.c`.
    ///
    UInt32 selectMutualVoltageLevels(UInt32 ocr);
    
    ///
    /// Get the host device
    ///
    /// @return The non-null host device.
    ///
    inline IOSDHostDevice* getHostDevice()
    {
        return this->host;
    }
    
    //
    // MARK: - Adjust Host Bus Settings
    //
    
    // TODO: RETUNE ENABLE/DISABLE????
    
    ///
    /// [Shared] Set the bus config
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `mmc_set_ios()` defined in `core.c`.
    ///
    IOReturn setBusConfig();
    
    ///
    /// Set the initial bus config
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `mmc_set_initial_state()` defined in `core.c`.
    ///
    IOReturn setInitialBusConfig();
    
    ///
    /// Set the SPI chip select mode
    ///
    /// @param chipSelect The target mode
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `mmc_set_chip_select()` defined in `core.c`.
    ///
    IOReturn setChipSelect(IOSDBusConfig::ChipSelect chipSelect);
    
    ///
    /// Set the bus speed mode
    ///
    /// @param timing The target bus speed mode
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `mmc_set_timing()` defined in `core.c`.
    ///
    IOReturn setBusTiming(IOSDBusConfig::BusTiming timing);
    
    ///
    /// Set the bus clock
    ///
    /// @param clock The target clock frequency in Hz
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `mmc_set_clock()` defined in `core.c`.
    /// @warning If the given clock frequency is beyond the range of supported clock frequencies,
    ///          this function will adjust the final clock appropriately.
    ///
    IOReturn setBusClock(UInt32 clock);
    
    ///
    /// Set the bus width
    ///
    /// @param width The target bus width
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `mmc_set_bus_width()` defined in `core.c`.
    ///
    IOReturn setBusWidth(IOSDBusConfig::BusWidth width);
    
    ///
    /// Set the signal voltage
    ///
    /// @param voltage The target signal voltage
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `mmc_set_signal_voltage()` defined in `core.c`.
    ///
    IOReturn setSignalVoltage(IOSDBusConfig::SignalVoltage voltage);
    
    ///
    /// Set the initial signal voltage
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `mmc_set_initial_signal_voltage()` defined in `core.c`.
    ///
    IOReturn setInitialSignalVoltage();
    
    ///
    /// Set the signal voltage for the Ultra High Speed mode
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `mmc_host_set_uhs_voltage()` defined in `core.c`.
    ///
    IOReturn setUltraHighSpeedSignalVoltage();
    
    ///
    /// Execute the tuning algorithm
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `mmc_execute_tuning()` defined in `core.c`.
    ///
    IOReturn executeTuning();
    
    ///
    /// Power up the SD stack
    ///
    /// @param ocr The OCR value from which to select the VDD value
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `mmc_power_up()` defined in `core.c`.
    ///
    IOReturn powerUp(UInt32 ocr);
    
    ///
    /// Power off the SD stack
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `mmc_power_up()` defined in `core.c`.
    ///
    IOReturn powerOff();
    
    ///
    /// Reboot the SD stack
    ///
    /// @param ocr The OCR value from which to select the VDD value
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `mmc_power_cycle()` defined in `core.c`.
    ///
    IOReturn powerCycle(UInt32 ocr);
    
    //
    // MARK: - DMA Utility
    //
    
    ///
    /// [Convenient] Allocate a DMA capable buffer
    ///
    /// @param size The number of bytes
    /// @return A non-null IODMACommand instance on success, `nullptr` otherwise.
    /// @note The calling thread can be blocked.
    ///
    IODMACommand* allocateDMABuffer(IOByteCount size);
    
    ///
    /// [Convenient] Release the given DMA capable buffer
    ///
    /// @param command A non-null IODMACommand instance previously returned by `IOSDHostDriver::allocateDMABuffer()`.
    ///
    void releaseDMABuffer(IODMACommand* command);
    
    //
    // MARK: - SD Request Center
    //
    
    ///
    /// [Helper] Send the given SD command request and wait for the response
    ///
    /// @param request A SD command request
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    ///
    IOReturn waitForRequest(RealtekSDRequest& request);
    
    ///
    /// [Helper] Send the given SD application command request and wait for the response
    ///
    /// @param request A SD application command request
    /// @param rca The card relative address
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `mmc_wait_for_app_cmd()` defined in `sd_ops.c`.
    /// @note This function issues a CMD55 before sending the given request.
    ///
    IOReturn waitForAppRequest(RealtekSDRequest& request, UInt32 rca);
    
    ///
    /// CMD0: Reset all cards to the idle state
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `mmc_go_idle()` defined in `mmc_ops.c`.
    ///
    IOReturn CMD0();
    
    ///
    /// CMD2: Ask any card to send the card identification data
    ///
    /// @param buffer A non-null that stores the **raw** card identification data on return
    /// @param length Specify the number of bytes read from the card identification data (must not exceed 16 bytes)
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `mmc_send_cid()` defined in `mmc_ops.c`.
    /// @note Upon a successful return, the given buffer contains the response data as is.
    ///       The caller is responsible for dealing with the endianness and parsing the data.
    /// @note It is recommended to use `IOSDHostDriver::CMD2(cid:)` to fetch and parse the data in one function call.
    ///
    IOReturn CMD2(UInt8* buffer, IOByteCount length);
    
    ///
    /// CMD2: Ask any card to send the card identification data
    ///
    /// @param buffer A non-null that stores the **raw** card identification data on return
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `mmc_send_cid()` defined in `mmc_ops.c`.
    /// @note Upon a successful return, the given buffer contains the response data as is.
    ///       The caller is responsible for dealing with the endianness and parsing the data.
    /// @note It is recommended to use `IOSDHostDriver::CMD2(cid:)` to fetch and parse the data in one function call.
    ///
    template <size_t N>
    IOReturn CMD2(UInt8 (&buffer)[N])
    {
        return this->CMD2(buffer, N);
    }
    
    ///
    /// CMD2: Ask any card to send the card identification data and parse the returned data
    ///
    /// @param cid The **parsed** card identification data on return
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `mmc_send_cid()` defined in `mmc_ops.c`.
    ///
    IOReturn CMD2(CID& cid);
    
    ///
    /// CMD3: Ask the card to publish its relative address
    ///
    /// @param rca The card relative address on return
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `mmc_send_relative_addr()` defined in `sd_ops.c`.
    ///
    IOReturn CMD3(UInt32& rca);
    
    ///
    /// CMD6: Check switchable function or switch the card function
    ///
    /// @param mode Pass 0 to check switchable function or 1 to switch the card function
    /// @param group The function group
    /// @param value The function value
    /// @param response A non-null buffer that stores the response on return
    /// @param length Specify the number of bytes read from the response (must not exceed 64 bytes)
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `mmc_sd_switch()` defined in `sd_ops.c`.
    /// @note This function allocates an internal 64-byte DMA capable buffer to store the response from the card.
    ///       The response is then copied to the given `response` buffer.
    /// @seealso `IOSDHostDriver::CMD6(mode:group:value:response)` if the caller desires to reuse an existing buffer.
    ///
    IOReturn CMD6(UInt32 mode, UInt32 group, UInt8 value, UInt8* response, IOByteCount length);
    
    ///
    /// CMD6: Check switchable function or switch the card function
    ///
    /// @param mode Pass 0 to check switchable function or 1 to switch the card function
    /// @param group The function group
    /// @param value The function value
    /// @param response A non-null buffer that stores the response on return
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `mmc_sd_switch()` defined in `sd_ops.c`.
    /// @note This function allocates an internal 64-byte DMA capable buffer to store the response from the card.
    ///       The response is then copied to the given `response` buffer.
    /// @seealso `IOSDHostDriver::CMD6(mode:group:value:response)` if the caller desires to reuse an existing buffer.
    ///
    template <size_t N>
    IOReturn CMD6(UInt32 mode, UInt32 group, UInt8 value, UInt8 (&response)[N])
    {
        return this->CMD6(mode, group, value, response, N);
    }
    
    ///
    /// CMD6: Check switchable function or switch the card function
    ///
    /// @param mode Pass 0 to check switchable function or 1 to switch the card function
    /// @param group The function group
    /// @param value The function value
    /// @param response A non-null and prepared memory descriptor that stores the response on return
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `mmc_sd_switch()` defined in `sd_ops.c`.
    /// @note This function uses the given response buffer to initiate a DMA read operation.
    ///       The caller is responsbile for managing the life cycle of the given buffer.
    ///
    DEPRECATE("Use other CMD6 methods")
    IOReturn CMD6(UInt32 mode, UInt32 group, UInt8 value, IOMemoryDescriptor* response);
    
    ///
    /// CMD7: Select a card
    ///
    /// @param rca The relative address of the card to be selected;
    ///            Pass 0 to deselect all cards
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `_mmc_select_card()` defined in `sd_ops.c`.
    ///
    IOReturn CMD7(UInt32 rca);
    
    ///
    /// CMD8: Send the interface condition and the voltage supply information
    ///
    /// @param vhs The voltage supply information
    /// @param response The response value on return
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `__mmc_send_if_cond()` defined in `sd_ops.c`,
    ///             the caller is responsbile for set the VHS value from the OCR register value.
    /// @seealso `IOSDHostDriver::CMD8(vhs:)` if the response can be ignored.
    ///
    IOReturn CMD8(UInt8 vhs, SDResponse7& response);
    
    ///
    /// CMD8: Send the interface condition and the voltage supply information
    ///
    /// @param vhs The voltage supply information
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `mmc_send_if_cond()` defined in `sd_ops.c`,
    ///             but the caller is responsbile for set the VHS value from the OCR register value.
    ///
    inline IOReturn CMD8(UInt8 vhs)
    {
        SDResponse7 response;
        
        return this->CMD8(vhs, response);
    }
    
    ///
    /// CMD8: Send the interface condition and the voltage supply information based on the given operation condition value
    ///
    /// @param ocr The operation condition register value
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    ///
    inline IOReturn CMD8(UInt32 ocr)
    {
        return this->CMD8(static_cast<UInt8>((ocr & 0xFF8000) != 0));
    }
    
    ///
    /// CMD9: Ask the card specified by the given relative address to send the card specific data
    ///
    /// @param rca The card relative address
    /// @param buffer A non-null that stores the **raw** card specific data on return
    /// @param length Specify the number of bytes read from the card specific data (must not exceed 16 bytes)
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `mmc_send_csd()` defined in `mmc_ops.c`.
    /// @note Upon a successful return, the given buffer contains the response data as is.
    ///       The caller is responsible for dealing with the endianness and parsing the data.
    /// @note It is recommended to use `IOSDHostDriver::CMD9(rca:csd:)` to fetch and parse the data in one function call.
    ///
    IOReturn CMD9(UInt32 rca, UInt8* buffer, IOByteCount length);
    
    ///
    /// CMD9: Ask the card specified by the given relative address to send the card specific data
    ///
    /// @param rca The card relative address
    /// @param buffer A non-null that stores the **raw** card specific data on return
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `mmc_send_csd()` defined in `mmc_ops.c`.
    /// @note Upon a successful return, the given buffer contains the response data as is.
    ///       The caller is responsible for dealing with the endianness and parsing the data.
    /// @note It is recommended to use `IOSDHostDriver::CMD9(rca:csd:)` to fetch and parse the data in one function call.
    ///
    template <size_t N>
    IOReturn CMD9(UInt32 rca, UInt8 (&buffer)[N])
    {
        return this->CMD9(rca, buffer, N);
    }
    
    ///
    /// CMD9: Ask the card specified by the given relative address to send the card specific data and parse the returned data
    ///
    /// @param rca The card relative address
    /// @param csd The card specific data on return
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `mmc_send_csd()` defined in `mmc_ops.c`.
    /// @note Upon a successful return, the given buffer contains the response data as is.
    ///       The caller is responsible for dealing with the endianness and parsing the data.
    /// @note It is recommended to use `IOSDHostDriver::CMD9(cid:)` to fetch and parse the data in one function call.
    ///
    IOReturn CMD9(UInt32 rca, CSD& csd);
    
    ///
    /// CMD11: Switch to 1.8V bus signaling level
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces a portion of `mmc_set_uhs_voltage()` defined in `mmc_ops.c`.
    ///
    IOReturn CMD11();
    
    ///
    /// CMD13: Send the card status
    ///
    /// @param rca The card relative address
    /// @param status The card status on return
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `__mmc_send_status()` defined in `mmc_ops.c`.
    ///
    IOReturn CMD13(UInt32 rca, UInt32& status);
    
    ///
    /// CMD55: Tell the card that the next command is an application command
    ///
    /// @param rca The card relative address
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `mmc_app_cmd()` defined in `sd_ops.c`.
    ///
    IOReturn CMD55(UInt32 rca);
    
    ///
    /// ACMD6: Set the data bus width
    ///
    /// @param rca The card relative address
    /// @param busWidth The data bus width (1 bit or 4 bit)
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `mmc_app_set_bus_width()` defined in `sd_ops.c`.
    ///
    IOReturn ACMD6(UInt32 rca, IOSDBusConfig::BusWidth busWidth);
    
    ///
    /// ACMD13: Send the SD status
    ///
    /// @param rca The card relative address
    /// @param status A non-null buffer that stores the SD status on return
    /// @param length Specify the number of bytes read from the response (must not exceed 64 bytes)
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `mmc_app_sd_status()` defined in `sd_ops.c`.
    /// @note This function allocates an internal 64-byte DMA capable buffer to store the status sent by the card.
    ///       The status is then copied to the given `status` buffer.
    ///
    IOReturn ACMD13(UInt32 rca, UInt8* status, IOByteCount length);
    
    ///
    /// ACMD13: Send the SD status
    ///
    /// @param rca The card relative address
    /// @param status A non-null buffer that stores the SD status on return
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `mmc_app_sd_status()` defined in `sd_ops.c`.
    /// @note This function allocates an internal 64-byte DMA capable buffer to store the status sent by the card.
    ///       The status is then copied to the given `status` buffer.
    ///
    template <size_t N>
    IOReturn ACMD13(UInt32 rca, UInt8 (&status)[N])
    {
        return this->ACMD13(rca, status, N);
    }
    
    ///
    /// ACMD13: Send the SD status
    ///
    /// @param rca The card relative address
    /// @param status The SD status on return
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `mmc_app_sd_status()` defined in `sd_ops.c`.
    /// @note This function allocates an internal 64-byte DMA capable buffer to store the status sent by the card.
    ///       The status is then copied to the given `status` buffer.
    ///
    inline IOReturn ACMD13(UInt32 rca, SSR& status)
    {
        // TODO: PARSE SSR
        // TODO: The Linux driver calculates the allocation unit size, erase timeout and erase offset values from the SSR
        // TODO: Erase function is not used by the macOS driver at this moment.
        return this->ACMD13(rca, reinterpret_cast<UInt8*>(&status), sizeof(SSR));
    }
    
    ///
    /// ACMD41: Send the operating condition register (OCR) value at the probe stage
    ///
    /// @param rocr The OCR value returned by the card
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `mmc_send_app_op_cond()` defined in `sd_ops.c`.
    ///
    IOReturn ACMD41(UInt32& rocr);
    
    ///
    /// ACMD41: Send the operating condition register (OCR) value
    ///
    /// @param ocr The OCR value that contains requested settings
    /// @param rocr The OCR value returned by the card
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `mmc_send_app_op_cond()` defined in `sd_ops.c`.
    ///
    IOReturn ACMD41(UInt32 ocr, UInt32& rocr);
    
    ///
    /// ACMD51: Ask the card to send the SD configuration register (SCR) value
    ///
    /// @param rca The card relative address
    /// @param buffer A non-null buffer that stores the **raw** SD configuration register value on return
    /// @param length Specify the number of bytes read from the response (must not exceed 8 bytes)
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `mmc_app_send_scr()` defined in `sd_ops.c`.
    /// @note This function allocates an internal 8-byte DMA capable buffer to store the register value sent by the card.
    ///       The value is then copied to the given `scr` buffer.
    /// @note Upon a successful return, the given buffer contains the response data as is.
    ///       The caller is responsible for dealing with the endianness and parsing the data.
    /// @note It is recommended to use `IOSDHostDriver::ACMD51(cid:)` to fetch and parse the data in one function call.
    ///
    IOReturn ACMD51(UInt32 rca, UInt8* buffer, IOByteCount length);
    
    ///
    /// ACMD51: Ask the card to send the SD configuration register (SCR) value
    ///
    /// @param rca The card relative address
    /// @param buffer The **raw** SD configuration register value on return
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `mmc_app_send_scr()` defined in `sd_ops.c`.
    /// @note This function allocates an internal 8-byte DMA capable buffer to store the register value sent by the card.
    ///       The value is then copied to the given `scr` buffer.
    /// @note Upon a successful return, the given buffer contains the response data as is.
    ///       The caller is responsible for dealing with the endianness and parsing the data.
    /// @note It is recommended to use `IOSDHostDriver::ACMD51(cid:)` to fetch and parse the data in one function call.
    ///
    template <size_t N>
    IOReturn ACMD51(UInt32 rca, UInt8 (&buffer)[N])
    {
        return this->ACMD51(rca, buffer, N);
    }
    
    ///
    /// ACMD51: Ask the card to send the SD configuration register (SCR) value and parse the value
    ///
    /// @param rca The card relative address
    /// @param scr The **parsed** SD configuration register value on return
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `mmc_app_send_scr()` defined in `sd_ops.c`.
    /// @note This function allocates an internal 8-byte DMA capable buffer to store the register value sent by the card.
    ///       The value is then copied to the given `scr` buffer.
    ///
    IOReturn ACMD51(UInt32 rca, SCR& scr);
    
    //
    // MARK: - Card Management
    //
    
    ///
    /// [Helper] Use the given frequency to communicate with the card and try to attach it
    ///
    /// @param frequency The initial frequency in Hz
    /// @return `true` on success, `false` otherwise.
    /// @note Port: This function replaces `mmc_rescan_try_freq()` defined in `core.c` and `mmc_attach_sd()` in `sd.c`.
    /// @note This function is invoked by `IOSDHostDriver::attachCard()`,
    ///       so it runs synchronously with respect to the processor workloop.
    ///
    bool attachCardAtFrequency(UInt32 frequency);
    
    ///
    /// [Helper] Publish the block storage device
    ///
    /// @return `true` on success, `false` otherwise.
    /// @note This function is invoked by `IOSDHostDriver::attachCard()`,
    ///       so it runs synchronously with respect to the processor workloop.
    ///
    bool publishBlockStorageDevice();
    
    ///
    /// [Helper] Publish the card characteristics
    ///
    /// @note System Information needs this to present card information.
    ///
    void publishCardCharacteristics();
    
    ///
    /// [Helper] Remove the published card characteristics
    ///
    void removeCardCharacteristics();
    
    ///
    /// Attach the SD card
    ///
    /// @return `true` if the card has been initialized and attached successfully.
    /// @note This function is invoked on the processor workloop thread when a SD card is inserted.
    ///
    bool attachCard();
    
    ///
    /// Detach the SD card
    ///
    /// @return `true` if the card has been removed from the system successfully.
    /// @note This function is invoked on the processor workloop thread when a SD card is removed.
    ///
    bool detachCard();
    
    //
    // MARK: - Card Events Callbacks
    //
    
    ///
    /// [UPCALL] Notify the host driver when a SD card is inserted
    ///
    /// @param completion A nullable completion routine to be invoked when the card is attached
    /// @note This callback function runs in a gated context provided by the underlying card reader controller.
    ///       The host device should implement this function without any blocking operations.
    ///
    void onSDCardInsertedGated(IOSDCard::Completion* completion = nullptr);
    
    ///
    /// [UPCALL] Notify the host driver when a SD card is removed
    ///
    /// @param completion A nullable completion routine to be invoked when the card is detached
    /// @note This callback function runs in a gated context provided by the underlying card reader controller.
    ///       The host device should implement this function without any blocking operations.
    ///
    void onSDCardRemovedGated(IOSDCard::Completion* completion = nullptr);
    
    //
    // MARK: - Query Card Information and Status
    //
    
    ///
    /// Check whether the card has write protection enabled
    ///
    /// @param result Set `true` if the card is write protected, `false` otherwise.
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    ///
    IOReturn isCardWriteProtected(bool& result);
    
    ///
    /// Check whether the card exists
    ///
    /// @param result Set `true` if the card exists, `false` otherwise.
    /// @return `kIOReturnSuccess` always.
    ///
    IOReturn isCardPresent(bool& result);
    
    ///
    /// Get the card capacity in number of blocks
    ///
    /// @param nblocks The number of blocks on return
    /// @return `kIOReturnSuccess` on success, `kIOReturnNoMedia` if the card is not present.
    ///
    IOReturn getCardNumBlocks(UInt64& nblocks);
    
    ///
    /// Get the index of the maximum block of the card
    ///
    /// @param index The index of the last accessible block on the card on return
    /// @return `kIOReturnSuccess` on success, `kIOReturnNoMedia` if the card is not present.
    ///
    IOReturn getCardMaxBlockIndex(UInt64& index);
    
    ///
    /// Get the size of a block
    ///
    /// @param length The block length in bytes on return
    /// @return `kIOReturnSuccess` on success, `kIOReturnNoMedia` if the card is not present.
    ///
    IOReturn getCardBlockLength(UInt64& length);
    
    ///
    /// Get the card vendor name
    ///
    /// @return A non-null static vendor string.
    /// @note This function returns "Realtek" if the card is not present.
    ///
    const char* getCardVendor();
    
    ///
    /// Get the card product name
    ///
    /// @param name A non-null buffer that can hold at least 8 bytes.
    /// @param length The buffer length
    /// @return `kIOReturnSuccess` on success,
    ///         `kIOReturnNoMedia` if the card is not present,
    ///         `kIOReturnBadArgument` if the buffer is NULL or too small.
    ///
    IOReturn getCardName(char* name, IOByteCount length);
    
    ///
    /// Get the card revision value
    ///
    /// @param revision A non-null buffer that can hold at least 8 bytes.
    /// @param length The buffer length
    /// @return `kIOReturnSuccess` on success,
    ///         `kIOReturnNoMedia` if the card is not present,
    ///         `kIOReturnBadArgument` if the buffer is NULL or too small.
    ///
    IOReturn getCardRevision(char* revision, IOByteCount length);
    
    ///
    /// Get the card manufacture date
    ///
    /// @param date A non-null buffer that can hold at least 8 bytes.
    /// @param length The buffer length
    /// @return `kIOReturnSuccess` on success,
    ///         `kIOReturnNoMedia` if the card is not present,
    ///         `kIOReturnBadArgument` if the buffer is NULL or too small.
    ///
    IOReturn getCardProductionDate(char* date, IOByteCount length);
    
    ///
    /// Get the card serial number
    ///
    /// @param serial The card serial number on return
    /// @return `kIOReturnSuccess` on success, `kIOReturnNoMedia` if the card is not present.
    ///
    IOReturn getCardSerialNumber(UInt32& serial);
    
    //
    // MARK: - Power Management
    //
    
    ///
    /// Prepare to enter the sleep state
    ///
    void prepareToSleep();
    
    ///
    /// Prepare to wake up from sleep
    ///
    void prepareToWakeUp();
    
    ///
    /// Adjust the power state in response to system-wide power events
    ///
    /// @param powerStateOrdinal The number in the power state array of the state the driver is being instructed to switch to
    /// @param whatDevice A pointer to the power management object which registered to manage power for this device
    /// @return `kIOPMAckImplied` always.
    ///
    IOReturn setPowerState(unsigned long powerStateOrdinal, IOService* whatDevice) override;
    
    //
    // MARK: - Startup Routines
    //
    
    ///
    /// Setup the power management
    ///
    /// @return `true` on success, `false` otherwise.
    ///
    bool setupPowerManagement();
    
    ///
    /// Setup the array of preallocated DMA commands
    ///
    /// @return `true` on success, `false` otherwise.
    /// @note Upon an unsuccessful return, all resources allocated by this function are released.
    ///
    bool setupPreallocatedDMACommands();
    
    ///
    /// Setup the array of preallocated SD block requests
    ///
    /// @return `true` on success, `false` otherwise.
    /// @note Upon an unsuccessful return, all resources allocated by this function are released.
    ///
    bool setupPreallocatedBlockRequests();
    
    ///
    /// Setup the shared work loop to protect the pool and the queue
    ///
    /// @return `true` on success, `false` otherwise.
    /// @note Upon an unsuccessful return, all resources allocated by this function are released.
    ///
    bool setupSharedWorkLoop();
    
    ///
    /// Setup the DMA command pool
    ///
    /// @return `true` on success, `false` otherwise.
    /// @note Upon an unsuccessful return, all resources allocated by this function are released.
    ///
    bool setupDMACommandPool();
    
    ///
    /// Setup the SD block request pool
    ///
    /// @return `true` on success, `false` otherwise.
    /// @note Upon an unsuccessful return, all resources allocated by this function are released.
    ///
    bool setupBlockRequestPool();
    
    ///
    /// Setup the SD block request queue
    ///
    /// @return `true` on success, `false` otherwise.
    /// @note Upon an unsuccessful return, all resources allocated by this function are released.
    ///
    bool setupBlockRequestQueue();
    
    ///
    /// Setup the shared work loop to process card events and block requests
    ///
    /// @return `true` on success, `false` otherwise.
    /// @note Upon an unsuccessful return, all resources allocated by this function are released.
    ///
    bool setupProcessorWorkLoop();
    
    ///
    /// Setup the event source that signals the processor work loop to process the block request
    ///
    /// @return `true` on success, `false` otherwise.
    /// @note Upon an unsuccessful return, all resources allocated by this function are released.
    ///
    bool setupBlockRequestEventSource();
    
    ///
    /// Setup the event sources that signal the processor work loop to handle card insertion and removal events
    ///
    /// @return `true` on success, `false` otherwise.
    /// @note Upon an unsuccessful return, all resources allocated by this function are released.
    ///
    bool setupCardEventSources();
    
    //
    // MARK: - Teardown Routines
    //
    
    ///
    /// Tear down the power management
    ///
    void tearDownPowerManagement();
    
    ///
    /// Tear down the array of preallocated DMA commands
    ///
    void tearDownPreallocatedDMACommands();
    
    ///
    /// Tear down the array of preallocated SD block requests
    ///
    void tearDownPreallocatedBlockRequests();
    
    ///
    /// Tear down the shared workloop
    ///
    void tearDownSharedWorkLoop();
    
    ///
    /// Tear down the DMA command pool
    ///
    void tearDownDMACommandPool();
    
    ///
    /// Tear down the SD block request pool
    ///
    void tearDownBlockRequestPool();
    
    ///
    /// Tear down the SD block request pool
    ///
    void tearDownBlockRequestQueue();
    
    ///
    /// Tear down the processor workloop
    ///
    void tearDownProcessorWorkLoop();
    
    ///
    /// Tear down the SD block request event source
    ///
    void tearDownBlockRequestEventSource();
    
    ///
    /// Tear down the card event sources
    ///
    void tearDownCardEventSources();
    
    //
    // MARK: - IOService Implementations
    //
    
    ///
    /// Start the host driver
    ///
    /// @param provider An instance of the host device
    /// @return `true` on success, `false` otherwise.
    ///
    bool start(IOService* provider) override;
    
    ///
    /// Stop the host driver
    ///
    /// @param provider An instance of the host device
    ///
    void stop(IOService* provider) override;
};

#endif /* IOSDHostDriver_hpp */
