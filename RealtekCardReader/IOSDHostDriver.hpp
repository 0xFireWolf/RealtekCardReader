//
//  IOSDHostDriver.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 6/1/21.
//

#ifndef IOSDHostDriver_hpp
#define IOSDHostDriver_hpp

#include <IOKit/IOCommandGate.h>
#include <IOKit/IOSubMemoryDescriptor.h>
#include <IOKit/storage/IOBlockStorageDriver.h>
#include "IOEnhancedCommandPool.hpp"
#include "IOSDHostDevice.hpp"
#include "IOSDHostRequest.hpp"
#include "IOSDBusConfig.hpp"
#include "IOSDBlockRequest.hpp"
#include "IOSDSimpleBlockRequest.hpp"
#include "IOSDComplexBlockRequest.hpp"
#include "IOSDBlockRequestQueue.hpp"
#include "IOSDBlockRequestEventSource.hpp"
#include "IOSDCard.hpp"
#include "IOSDCardEventSource.hpp"
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
    // MARK: - Type Definitions
    //
    
    /// Type of the pool of simple block requests
    using IOSDSimpleBlockRequestPool = IOEnhancedCommandPool<IOSDSimpleBlockRequest>;
    
    /// Type of the pool of complex block requests
    using IOSDComplexBlockRequestPool = IOEnhancedCommandPool<IOSDComplexBlockRequest>;
    
    //
    // MARK: - Private Properties
    //

    /// The default pool size
    static constexpr IOItemCount kDefaultPoolSize = 32;
    
    /// The SD host device (provider)
    IOSDHostDevice* host;
    
    /// The SD block storage device (client)
    IOSDBlockStorageDevice* blockStorageDevice;
    
    /// A shared workloop that serializes the access to the pools and the queues
    IOWorkLoop* sharedWorkLoop;
    
    /// A request pool that contains preallocated simple SD block requests
    IOSDSimpleBlockRequestPool* simpleBlockRequestPool;
    
    /// A request pool that contains preallocated complex SD block requests
    IOSDComplexBlockRequestPool* complexBlockRequestPool;
    
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
    
    ///
    /// The identification data of the card inserted before the computer sleeps
    ///
    /// @note `pcid` is zeroed if no card is inserted when the computer sleeps
    ///
    CID pcid;
    
    //
    // MARK: - Pool Management
    //
    
private:
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
            this->complexBlockRequestPool->returnCommand(creq);
            
            return;
        }
        
        IOSDSimpleBlockRequest* sreq = OSDynamicCast(IOSDSimpleBlockRequest, request);
        
        if (sreq != nullptr)
        {
            this->simpleBlockRequestPool->returnCommand(sreq);
            
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
    // MARK: - Submit Block I/O Requests
    //
    
private:
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
    
public:
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
    
    //
    // MARK: - Process Block I/O Requests
    //
    
private:
    ///
    /// [Helper] Transform the given starting block number to the argument of CMD17/18/24/25 if necessary
    ///
    /// @param block The starting block number
    /// @return The argument to be passed to CMD17/18/24/25.
    ///         i.e. The given block number if the card is block addressed (SDHC/XC),
    ///              or the corresponding byte offset if the card is byte addressed (SDSC).
    ///
    UInt32 transformBlockOffsetIfNecessary(UInt64 block);
    
    ///
    /// [Helper] Process the given request to access multiple blocks separately
    ///
    /// @param request A non-null block request
    /// @param requestBuilder A callable object that takes the block offset and the transfer data as input and returns a single block transfer request
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note The return value will be passed to the storage completion routine.
    /// @note When this function is invoked, the memory descriptor is guaranteed to be non-null and prepared.
    /// @note This function separates a CMD18/25 request into multiple CMD17/24 ones.
    /// @note Signature of the request builder: `IOSDSingleBlockRequest operator()(UInt32, IOMemoryDescriptor*)`.
    ///
    template <typename RequestBuilder>
    IOReturn processAccessBlocksRequestSeparately(IOSDBlockRequest* request, RequestBuilder requestBuilder)
    {
        pinfo("Processing the request that reads multiple blocks separately...");
        
        IOReturn retVal = kIOReturnSuccess;
        
        // Fetching the properties of the original request
        UInt64 blockOffset = request->getBlockOffset();
        
        UInt64 nblocks = request->getNumBlocks();
        
        IOMemoryDescriptor* data = request->getMemoryDescriptor();
        
        pinfo("Request: Block Offset = %llu; NumBlocks = %llu.", blockOffset, nblocks);
        
        // The portion of the data buffer
        IOSubMemoryDescriptor* pdata = OSTypeAlloc(IOSubMemoryDescriptor);
        
        if (pdata == nullptr)
        {
            perr("Failed to allocate the sub-memory descriptor.");
            
            return kIOReturnNoMemory;
        }
        
        // Process each CMD17/24 subrequest
        for (UInt64 index = 0; index < nblocks; index += 1)
        {
            // Calculate the properties for the current subrequest
            UInt64 pBlockOffset = blockOffset + index;
            
            UInt64 pDataOffset = index * 512;
            
            pinfo("[%02llu] Subrequest: Block Offset = %llu; Data Offset = %llu.", index, pBlockOffset, pDataOffset);
            
            // Specify the portion of data to be transferred
            if (!pdata->initSubRange(data, pDataOffset, 512, data->getDirection()))
            {
                perr("[%02llu] Failed to initialize the sub-memory descriptor.", index);
                
                retVal = kIOReturnError;
                
                break;
            }
            
            // Prepare the data
            if (pdata->prepare() != kIOReturnSuccess)
            {
                perr("[%02llu] Failed to page in and wire down the portion of data.", index);
                
                break;
            }
            
            // Process the subrequest
            auto prequest = requestBuilder(this->transformBlockOffsetIfNecessary(pBlockOffset), pdata);
            
            retVal = this->waitForRequest(prequest);
            
            psoftassert(pdata->complete() == kIOReturnSuccess, "[%02llu] Failed to complete the portion of data.", index);
            
            if (retVal != kIOReturnSuccess)
            {
                perr("[%02llu] Failed to complete the subrequest. Error = 0x%08x.", index, retVal);
                
                break;
            }
        }
        
        // Cleanup
        pdata->release();
        
        return retVal;
    }
    
    ///
    /// Process the given request to read a single block
    ///
    /// @param request A non-null block request
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note The return value will be passed to the storage completion routine.
    /// @note When this function is invoked, the memory descriptor is guaranteed to be non-null and prepared.
    ///
    IOReturn processReadBlockRequest(IOSDBlockRequest* request);
    
    ///
    /// Process the given request to read multiple blocks
    ///
    /// @param request A non-null block request
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note The return value will be passed to the storage completion routine.
    /// @note When this function is invoked, the memory descriptor is guaranteed to be non-null and prepared.
    ///
    IOReturn processReadBlocksRequest(IOSDBlockRequest* request);
    
    ///
    /// Process the given request to read multiple blocks separately
    ///
    /// @param request A non-null block request
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note The return value will be passed to the storage completion routine.
    /// @note When this function is invoked, the memory descriptor is guaranteed to be non-null and prepared.
    /// @note This function separates a CMD18 request into multiple CMD17 ones.
    ///
    IOReturn processReadBlocksRequestSeparately(IOSDBlockRequest* request);
    
    ///
    /// Process the given request to write a single block
    ///
    /// @param request A non-null block request
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note The return value will be passed to the storage completion routine.
    /// @note When this function is invoked, the memory descriptor is guaranteed to be non-null and prepared.
    ///
    IOReturn processWriteBlockRequest(IOSDBlockRequest* request);
    
    ///
    /// Process the given request to write multiple blocks
    ///
    /// @param request A non-null block request
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note The return value will be passed to the storage completion routine.
    /// @note When this function is invoked, the memory descriptor is guaranteed to be non-null and prepared.
    ///
    IOReturn processWriteBlocksRequest(IOSDBlockRequest* request);
    
    ///
    /// Process the given request to write multiple blocks separately
    ///
    /// @param request A non-null block request
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note The return value will be passed to the storage completion routine.
    /// @note When this function is invoked, the memory descriptor is guaranteed to be non-null and prepared.
    /// @note This function separates a CMD25 request into multiple CMD24 ones.
    ///
    IOReturn processWriteBlocksRequestSeparately(IOSDBlockRequest* request);
    
    ///
    /// Finalize a request that has been processed
    ///
    /// @param request A non-null block request
    /// @note This function is the completion routine registered with the block request event source.
    ///       It deinitializes the given request and puts it back to the block request pool.
    ///
    void finalizeBlockRequest(IOSDBlockRequest* request);
    
    //
    // MARK: - Query Host Properties
    //
    
public:
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
    
private:
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
    
public:
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
    
private:
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
    
public:
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
    // MARK: - SD Request Center
    //
    
private:
    ///
    /// [Helper] Send the given SD command request and wait for the response
    ///
    /// @param request A SD command request
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    ///
    IOReturn waitForRequest(IOSDHostRequest& request);
    
    ///
    /// [Helper] Send the given SD application command request and wait for the response
    ///
    /// @param request A SD application command request
    /// @param rca The card relative address
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `mmc_wait_for_app_cmd()` defined in `sd_ops.c`.
    /// @note This function issues a CMD55 before sending the given request.
    ///
    IOReturn waitForAppRequest(IOSDHostRequest& request, UInt32 rca);
    
public:
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
    IOReturn CMD8(UInt8 vhs, IOSDHostResponse7& response);
    
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
        IOSDHostResponse7 response;
        
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
    IOReturn ACMD13(UInt32 rca, SSR& status);
    
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
    /// @param configuration A non-null buffer that stores the **raw** SD configuration register value on return
    /// @param length Specify the number of bytes read from the response (must not exceed 8 bytes)
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `mmc_app_send_scr()` defined in `sd_ops.c`.
    /// @note This function allocates an internal 8-byte DMA capable buffer to store the register value sent by the card.
    ///       The value is then copied to the given `scr` buffer.
    /// @note Upon a successful return, the given buffer contains the response data as is.
    ///       The caller is responsible for dealing with the endianness and parsing the data.
    /// @note It is recommended to use `IOSDHostDriver::ACMD51(cid:)` to fetch and parse the data in one function call.
    ///
    IOReturn ACMD51(UInt32 rca, UInt8* configuration, IOByteCount length);
    
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
    
private:
    ///
    /// [Helper] Notify the block storage device that the media state has changed
    ///
    /// @param state The new state of the media
    /// @return The status returned by the block storage device.
    ///
    IOReturn notifyBlockStorageDevice(IOMediaState state);
    
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
    /// [Helper] Use the given frequency to communicate with the card and try to attach it
    ///
    /// @param frequency The initial frequency in Hz
    /// @return `true` on success, `false` otherwise.
    /// @note Port: This function replaces `mmc_rescan_try_freq()` defined in `core.c` and `mmc_attach_sd()` in `sd.c`.
    /// @note This function is invoked by `IOSDHostDriver::attachCard()`,
    ///       so it runs synchronously with respect to the processor workloop.
    ///
    bool attachCardAtFrequencyV2(UInt32 frequency);
    
    ///
    /// Attach the SD card
    ///
    /// @param completion The completion routine to call once the card insertion event has been processed
    /// @param options An optional value passed to the card event handler
    /// @note This function is invoked on the processor workloop thread when a SD card is inserted.
    ///
    void attachCard(IOSDCard::Completion* completion = nullptr, IOSDCard::EventOptions options = 0);
    
    ///
    /// Detach the SD card
    ///
    /// @param completion The completion routine to call once the card removal event has been processed
    /// @param options An optional value passed to the card event handler
    /// @note This function is invoked on the processor workloop thread when a SD card is removed.
    ///
    void detachCard(IOSDCard::Completion* completion = nullptr, IOSDCard::EventOptions options = 0);
    
    //
    // MARK: - Card Events Callbacks
    //
    
public:
    ///
    /// [UPCALL] Notify the host driver when a SD card is inserted
    ///
    /// @param completion A nullable completion routine to be invoked when the card is attached
    /// @param options An optional value passed to the host driver
    /// @note This callback function runs in a gated context provided by the underlying card reader controller.
    ///       The host device should implement this function without any blocking operations.
    ///
    void onSDCardInsertedGated(IOSDCard::Completion* completion = nullptr, IOSDCard::EventOptions options = 0);
    
    ///
    /// [UPCALL] Notify the host driver when a SD card is removed
    ///
    /// @param completion A nullable completion routine to be invoked when the card is detached
    /// @param options An optional value passed to the host driver
    /// @note This callback function runs in a gated context provided by the underlying card reader controller.
    ///       The host device should implement this function without any blocking operations.
    ///
    void onSDCardRemovedGated(IOSDCard::Completion* completion = nullptr, IOSDCard::EventOptions options = 0);
    
    //
    // MARK: - Query Card Information and Status
    //
    
public:
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
    // MARK: - Startup Routines
    //
    
private:
    ///
    /// Setup the shared work loop to protect the pool and the queue
    ///
    /// @return `true` on success, `false` otherwise.
    /// @note Upon an unsuccessful return, all resources allocated by this function are released.
    ///
    bool setupSharedWorkLoop();
    
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
    
    ///
    /// Wait until the block storage device is published
    ///
    /// @return `true` on success, `false` otherwise.
    /// @note Upon an unsuccessful return, all resources allocated by this function are released.
    ///
    bool setupBlockStorageDevice();
    
    ///
    /// Setup the SD card instance
    ///
    /// @return `true` on success, `false` otherwise.
    /// @note Upon an unsuccessful return, all resources allocated by this function are released.
    /// @note This startup routine is used by `IOSDHostDriver::attachCardAtFrequency()`.
    ///
    bool setupCard();
    
    //
    // MARK: - Teardown Routines
    //
    
private:    
    ///
    /// Tear down the shared workloop
    ///
    void tearDownSharedWorkLoop();
    
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
    
    ///
    /// Tear down the block storage device
    ///
    void tearDownBlockStorageDevice();
    
    ///
    /// Tear down the SD card instance
    ///
    /// @note This startup routine is used by `IOSDHostDriver::attachCardAtFrequency()`.
    ///
    void tearDownCard();
    
    //
    // MARK: - IOService Implementations
    //
    
public:
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
