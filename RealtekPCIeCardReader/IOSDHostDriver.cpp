//
//  IOSDHostDriver.cpp
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 6/1/21.
//

#include "IOSDHostDriver.hpp"
#include "BitOptions.hpp"
#include "Debug.hpp"
#include "IOSDBlockStorageDevice.hpp"
#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/IOSubMemoryDescriptor.h>
#include <IOKit/storage/IOBlockStorageDriver.h>
#include <i386/param.h>

//
// MARK: - Meta Class Definitions
//

OSDefineMetaClassAndStructors(IOSDHostDriver, IOService);

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
IOReturn IOSDHostDriver::submitBlockRequest(IOSDBlockRequest::Processor processor, IOMemoryDescriptor* buffer, UInt64 block, UInt64 nblocks, IOStorageAttributes* attributes, IOStorageCompletion* completion)
{
    pinfo("BREQ: Start Block Index = %llu; Number of Blocks = %llu; Number of Bytes = %llu.", block, nblocks, nblocks * 512);
    
    psoftassert(buffer->getLength() == nblocks * 512, "Buffer lengths mismatched.");
    
    // Guard: Ensure that the queue event source is still enabled
    // It is enabled if and only if the card is not ejected or removed.
    if (!this->queueEventSource->isEnabled())
    {
        perr("The queue event source is disabled.");
        
        return kIOReturnNoMedia;
    }
    
    // Guard: The maximum number of blocks in one request is 1024 (for example)
    // Split the incoming request into multiple smaller one if necessary
    IOSDBlockRequest* request = nullptr;

    if (nblocks <= this->host->getDMALimits().maxRequestNumBlocks())
    {
        request = this->allocateSimpleBlockRequestFromPool();
    }
    else
    {
        request = this->allocateComplexBlockRequestFromPool();
    }
    
    passert(request != nullptr, "The block request should not be null at this moment.");
    
    request->init(this, processor, buffer, block, nblocks, attributes, completion);
    
    // It is possible that the user removes the card just before the host driver enqueues the request and signals the processor workloop.
    // In this case, the card removal handler has already disabled the queue event source.
    // Here the host driver notifies the processor workloop that a block request is pending,
    // and the workloop will invoke `IOSDBlockRequestEventSource::checkForWork()`.
    // Since the event source is disabled, it will not process this request.
    // The request remains in the queue, but the host driver will recycle it when a new card is inserted.
    // See `IOSDHostDriver::attachCard(frequency:)` for details.
    this->pendingRequests->enqueueRequest(request);
    
    this->queueEventSource->notify();
    
    return kIOReturnSuccess;
}

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
IOReturn IOSDHostDriver::submitReadBlockRequest(IOMemoryDescriptor* buffer, UInt64 block, UInt64 nblocks, IOStorageAttributes* attributes, IOStorageCompletion* completion)
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
IOReturn IOSDHostDriver::submitWriteBlockRequest(IOMemoryDescriptor* buffer, UInt64 block, UInt64 nblocks, IOStorageAttributes* attributes, IOStorageCompletion* completion)
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
IOReturn IOSDHostDriver::submitReadBlocksRequest(IOMemoryDescriptor* buffer, UInt64 block, UInt64 nblocks, IOStorageAttributes* attributes, IOStorageCompletion* completion)
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
IOReturn IOSDHostDriver::submitWriteBlocksRequest(IOMemoryDescriptor* buffer, UInt64 block, UInt64 nblocks, IOStorageAttributes* attributes, IOStorageCompletion* completion)
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
UInt32 IOSDHostDriver::transformBlockOffsetIfNecessary(UInt64 block)
{
    // SDXC supports up to 2TB
    passert(block <= UINT32_MAX, "The maximum capacity supported is 2TB.");
    
    // This function is invoked by the processor workloop (i.e. the queue event source),
    // so the instance variable `card` is guaranteed to be non-null.
    // 1) It is possible that the user removes the SD card while the workloop is processing this request,
    //    but the card will be released and set to NULL only after the workloop finishes this request.
    // 2) If the user removes the SD card before the workloop starts to process this request,
    //    the queue event source is disabled, so we should never reach at here.
    passert(this->card != nullptr, "The card should be non-null at this moment.");
    
    if (!this->card->getCSD().isBlockAddressed)
    {
        block <<= 9;
        
        pinfo("The card is byte addressed. Adjusted argument = %llu.", block);
    }
    
    return static_cast<UInt32>(block);
}

///
/// Process the given request to read a single block
///
/// @param request A non-null block request
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note The return value will be passed to the storage completion routine.
/// @note When this function is invoked, the DMA command is guaranteed to be non-null and prepared.
///
IOReturn IOSDHostDriver::processReadBlockRequest(IOSDBlockRequest* request)
{
    pinfo("Processing the request that reads a single block...");
    
    auto creq = RealtekSDRequestFactory::CMD17(this->transformBlockOffsetIfNecessary(request->getBlockOffset()), request->getDMACommand());
    
    return this->waitForRequest(creq);
}

///
/// Process the given request to read multiple blocks
///
/// @param request A non-null block request
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note The return value will be passed to the storage completion routine.
/// @note When this function is invoked, the DMA command is guaranteed to be non-null and prepared.
///
IOReturn IOSDHostDriver::processReadBlocksRequest(IOSDBlockRequest* request)
{
    pinfo("Processing the request that reads multiple blocks...");
    
    auto creq = RealtekSDRequestFactory::CMD18(this->transformBlockOffsetIfNecessary(request->getBlockOffset()), request->getDMACommand(), request->getNumBlocks());
    
    return this->waitForRequest(creq);
}

///
/// Process the given request to write a single block
///
/// @param request A non-null block request
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note The return value will be passed to the storage completion routine.
/// @note When this function is invoked, the DMA command is guaranteed to be non-null and prepared.
///
IOReturn IOSDHostDriver::processWriteBlockRequest(IOSDBlockRequest* request)
{
    pinfo("Processing the request that writes a single block...");
    
    auto creq = RealtekSDRequestFactory::CMD24(this->transformBlockOffsetIfNecessary(request->getBlockOffset()), request->getDMACommand());
    
    return this->waitForRequest(creq);
}

///
/// Process the given request to write multiple blocks
///
/// @param request A non-null block request
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note The return value will be passed to the storage completion routine.
/// @note When this function is invoked, the DMA command is guaranteed to be non-null and prepared.
///
IOReturn IOSDHostDriver::processWriteBlocksRequest(IOSDBlockRequest* request)
{
    // Issue the ACMD23 to set the number of pre-erased blocks
    pinfo("Issuing an ACMD23 to set the number of pre-erased blocks...");
    
    passert(request->getNumBlocks() <= ((1 << 23) - 1), "The number of blocks should be less than 2^23 - 1.");
    
    auto preq = RealtekSDRequestFactory::ACMD23(static_cast<UInt32>(request->getNumBlocks()));
    
    IOReturn retVal = this->waitForAppRequest(preq, this->card->getRCA());
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to issue the ACMD23 to set the number of pre-erased blocks. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    pinfo("The ACMD23 has been issued.");
    
    // Process the block request
    pinfo("Processing the request that writes multiple blocks...");
    
    auto creq = RealtekSDRequestFactory::CMD25(this->transformBlockOffsetIfNecessary(request->getBlockOffset()), request->getDMACommand(), request->getNumBlocks());
    
    return this->waitForRequest(creq);
}

///
/// Finalize a request that has been processed
///
/// @param request A non-null block request
/// @note This function is the completion routine registerd with the block request event source.
///       It deinitializes the given request and puts it back to the block request pool.
///
void IOSDHostDriver::finalizeBlockRequest(IOSDBlockRequest* request)
{
    pinfo("The given request has been processed.");
    
    request->deinit();
    
    this->releaseBlockRequestToPool(request);
}

//
// MARK: - Query Host Properties
//

///
/// Check whether the host supports the High Speed mode
///
/// @return `true` if the High Speed mode is supported by the host.
///
bool IOSDHostDriver::hostSupportsHighSpeedMode()
{
    return this->host->getCaps1().contains(MMC_CAP_SD_HIGHSPEED);
}

///
/// Check whether the host supports the Ultra High Speed mode
///
/// @return `true` if the Ultra High Speed mode is supported by the host.
///
bool IOSDHostDriver::hostSupportsUltraHighSpeedMode()
{
    auto caps = this->host->getCaps1();
    
    bool s4b = caps.contains(Capability::k4BitData);
    
    bool uhs = caps.contains(MMC_CAP_UHS_SDR12)  ||
               caps.contains(MMC_CAP_UHS_SDR25)  ||
               caps.contains(MMC_CAP_UHS_SDR50)  ||
               caps.contains(MMC_CAP_UHS_SDR104) ||
               caps.contains(MMC_CAP_UHS_DDR50);
    
    return s4b && uhs;
}

///
/// Check whether the host supports the 4-bit bus width
///
/// @return `true` if the 4-bit bus width is supported by the host.
///
bool IOSDHostDriver::hostSupports4BitBusWidth()
{
    return this->host->getCaps1().contains(Capability::k4BitData);
}

///
/// Get the maximum current setting at its current voltage
///
/// @return The maximum current in mA.
/// @note Port: This function replaces `sd_get_host_max_current()` defined in `sd.c`.
///
UInt32 IOSDHostDriver::getHostMaxCurrent()
{
    IOSDHostDevice::MaxCurrents maxCurrents = this->host->getHostMaxCurrents();

    const IOSDBusConfig& config = this->host->getHostBusConfig();

    UInt32 maxCurrent = 0;

    switch (1 << config.vdd)
    {
        case MMC_VDD_165_195:
        {
            maxCurrent = maxCurrents.v18;

            break;
        }

        case MMC_VDD_29_30:
        case MMC_VDD_30_31:
        {
            maxCurrent = maxCurrents.v30;

            break;
        }

        case MMC_VDD_32_33:
        case MMC_VDD_33_34:
        {
            maxCurrent = maxCurrents.v33;

            break;
        }
        default:
        {
            perr("Unsupported host signal voltage level.");

            break;
        }
    }

    return maxCurrent;
}

///
/// Select voltage levels mutually supported by the host and the card
///
/// @param ocr The card OCR value
/// @return The OCR value that contains voltage levels supported by both parties.
/// @note Port: This function replaces `mmc_select_voltage()` defined in `core.c`.
///
UInt32 IOSDHostDriver::selectMutualVoltageLevels(UInt32 ocr)
{
    pinfo("Selecting voltage levels that are supported by both sides...");
    
    // Sanitize the card OCR value
    // The low 7 bits should be zero
    if ((ocr & 0x7F) != 0)
    {
        pwarning("The card OCR value 0x%08x reports to support undefined voltage levels.", ocr);

        ocr &= ~0x7F;
    }
    
    pinfo("[OCR] Card = 0x%08x.", ocr);

    // Filter out voltage levels unsupported by both sides
    ocr &= this->host->getHostSupportedVoltageRanges();

    pinfo("[OCR] Both = 0x%08x.", ocr);
    
    if (ocr == 0)
    {
        pwarning("No voltage levels supported by the host and the card.");

        return 0;
    }

    // TODO: CHANGE THIS TO GETTER FUNC?
    if (this->host->getCaps2().contains(MMC_CAP2_FULL_PWR_CYCLE))
    {
        pinfo("The host device supports a full power cycle.");
        
        ocr &= 3 << (ffs(ocr) - 1);
        
        pinfo("Restarting the host device with the new OCR value 0x%08x.", ocr);

        psoftassert(this->powerCycle(ocr) == kIOReturnSuccess,
                    "Failed to restart the power of the card with the new OCR value 0x%08x.", ocr);
    }
    else
    {
        ocr &= 3 << (myfls(ocr) - 1);

        psoftassert(myfls(ocr) - 1 == this->host->getHostBusConfig().vdd,
                    "The host voltage supply exceeds the card's supported value.");
    }
    
    pinfo("Selected mutual voltage levels = 0x%x.", ocr);

    return ocr;
}

//
// MARK: - Adjust Host Bus Settings
//

///
/// [Shared] Set the bus config
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `mmc_set_ios()` defined in `core.c`.
///
IOReturn IOSDHostDriver::setBusConfig()
{
    const IOSDBusConfig& config = this->host->getHostBusConfig();

    config.print();

    return this->host->setBusConfig(config);
}

///
/// Set the initial bus config
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `mmc_set_initial_state()` defined in `core.c`.
///
IOReturn IOSDHostDriver::setInitialBusConfig()
{
    pinfo("Setting the initial bus config...");
    
    IOSDBusConfig& config = this->host->getHostBusConfig();

    config.chipSelect = IOSDBusConfig::ChipSelect::kDoNotCare;

    config.busMode = IOSDBusConfig::BusMode::kPushPull;

    config.busWidth = IOSDBusConfig::BusWidth::k1Bit;

    config.busTiming = IOSDBusConfig::BusTiming::kLegacy;

    config.driverType = IOSDBusConfig::DriverType::kTypeB;

    return this->setBusConfig();
}

///
/// Set the SPI chip select mode
///
/// @param chipSelect The target mode
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `mmc_set_chip_select()` defined in `core.c`.
///
IOReturn IOSDHostDriver::setChipSelect(IOSDBusConfig::ChipSelect chipSelect)
{
    // TODO: Optimize this: Realtek's hardware seems to put the chip select to High by default
//    IOSDBusConfig& config = this->host->getHostBusConfig();
//
//    config.chipSelect = chipSelect;
//
//    return this->setBusConfig();
    
    pinfo("Optimization: Realtek does not support the chip select.");
  
    return kIOReturnSuccess;
}

///
/// Set the bus speed mode
///
/// @param timing The target bus speed mode
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `mmc_set_timing()` defined in `core.c`.
///
IOReturn IOSDHostDriver::setBusTiming(IOSDBusConfig::BusTiming timing)
{
    IOSDBusConfig& config = this->host->getHostBusConfig();

    config.busTiming = timing;

    return this->setBusConfig();
}

///
/// Set the bus clock
///
/// @param clock The target clock frequency in Hz
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `mmc_set_clock()` defined in `core.c`.
/// @warning If the given clock frequency is beyond the range of supported clock frequencies,
///          this function will adjust the final clock appropriately.
///
IOReturn IOSDHostDriver::setBusClock(UInt32 clock)
{
    ClosedRange<UInt32> range = this->host->getHostClockRange();

    IOSDBusConfig& config = this->host->getHostBusConfig();

    psoftassert(range.contains(clock), "The given clock %u Hz is beyond the range.", clock);

    config.clock = min(range.upperBound, clock);

    return this->setBusConfig();
}

///
/// Set the bus width
///
/// @param width The target bus width
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `mmc_set_bus_width()` defined in `core.c`.
///
IOReturn IOSDHostDriver::setBusWidth(IOSDBusConfig::BusWidth width)
{
    IOSDBusConfig& config = this->host->getHostBusConfig();

    config.busWidth = width;

    return this->setBusConfig();
}

///
/// Set the signal voltage
///
/// @param voltage The target signal voltage
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `mmc_set_signal_voltage()` defined in `core.c`.
///
IOReturn IOSDHostDriver::setSignalVoltage(IOSDBusConfig::SignalVoltage voltage)
{
    IOSDBusConfig& config = this->host->getHostBusConfig();

    auto oldVoltage = config.signalVoltage;

    config.signalVoltage = voltage;

    IOReturn retVal = this->host->switchSignalVoltage(config);

    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to switch to the signal voltage %hhu. Error = 0x%x.", voltage, retVal);

        config.signalVoltage = oldVoltage;
    }

    return retVal;
}

///
/// Set the initial signal voltage
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `mmc_set_initial_signal_voltage()` defined in `core.c`.
///
IOReturn IOSDHostDriver::setInitialSignalVoltage()
{
    // Try to set the signal voltage to 3.3V
    pinfo("Attempt to set the signal voltage to 3.3V.");

    if (this->setSignalVoltage(IOSDBusConfig::SignalVoltage::k3d3V) == kIOReturnSuccess)
    {
        pinfo("The initial signal voltage has been set to 3.3V.");

        return kIOReturnSuccess;
    }

    perr("Failed to set the signal voltage to 3.3V. Will try to set the signal voltage to 1.8V.");

    if (this->setSignalVoltage(IOSDBusConfig::SignalVoltage::k1d8V) == kIOReturnSuccess)
    {
        pinfo("The initial signal voltage has been set to 1.8V.");

        return kIOReturnSuccess;
    }

    perr("Failed to set the signal voltage to 1.8V. Will try to set the signal voltage to 1.2V.");

    if (this->setSignalVoltage(IOSDBusConfig::SignalVoltage::k1d2V) == kIOReturnSuccess)
    {
        pinfo("The initial signal voltage has been set to 1.2V.");

        return kIOReturnSuccess;
    }

    perr("Cannot set the initial signal voltage to one of supported values.");

    return kIOReturnError;
}

///
/// Set the signal voltage for the Ultra High Speed mode
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `mmc_host_set_uhs_voltage()` defined in `core.c`.
///
IOReturn IOSDHostDriver::setUltraHighSpeedSignalVoltage()
{
    // The clock must be gated for 5ms during a signal voltage level switch
    IOSDBusConfig& config = this->host->getHostBusConfig();

    UInt32 clock = config.clock;

    config.clock = 0;

    IOReturn retVal = this->setBusConfig();

    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to gate the clock. Error = 0x%x.", retVal);

        return retVal;
    }

    // Set the signal voltage to 1.8V
    retVal = this->setSignalVoltage(IOSDBusConfig::SignalVoltage::k1d8V);

    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to switch the signal voltage to 1.8V. Error = 0x%x.", retVal);

        return retVal;
    }

    // Keep the clock gated for at least 10ms
    IOSleep(10);

    // Restore the clock
    config.clock = clock;

    retVal = this->setBusConfig();

    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to restore the clock. Error = 0x%x.", retVal);

        return retVal;
    }

    return kIOReturnSuccess;
}

///
/// Execute the tuning algorithm
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `mmc_execute_tuning()` defined in `core.c`.
///
IOReturn IOSDHostDriver::executeTuning()
{
    // TODO: RETUNE ENABLE???
    return this->host->executeTuning(this->host->getHostBusConfig());
}

///
/// Power up the SD stack
///
/// @param ocr The OCR value from which to select the VDD value
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `mmc_power_up()` defined in `core.c`.
///
IOReturn IOSDHostDriver::powerUp(UInt32 ocr)
{
    // Check whether the bus power is already on
    IOSDBusConfig& config = this->host->getHostBusConfig();

    if (config.powerMode == IOSDBusConfig::PowerMode::kPowerOn)
    {
        pinfo("The bus power is already on.");

        return kIOReturnSuccess;
    }

    // Power up the bus
    // Set the initial bus config without the clock running
    pinfo("Powering up the bus...");
    
    config.vdd = myfls(ocr) - 1;

    config.powerMode = IOSDBusConfig::PowerMode::kPowerUp;

    IOReturn retVal = this->setInitialBusConfig();

    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to set the initial bus state. Error = 0x%x.", retVal);

        return retVal;
    }
    
    pinfo("The bus power is now up.");

    // Set the initial signal voltage level
    pinfo("Setting the initial signal voltage...");
    
    retVal = this->setInitialSignalVoltage();

    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to set the initial signal voltage level. Error = 0x%x.", retVal);

        return retVal;
    }

    // Wait for a while until the power supply becomes stable
    IOSleep(config.powerDelay);

    pinfo("The initial signal voltage has been set.");
    
    // Power on the bus with the clock running
    pinfo("Powering on the bus...");
    
    config.clock = this->host->getHostInitialClock();

    config.powerMode = IOSDBusConfig::PowerMode::kPowerOn;

    retVal = this->setBusConfig();

    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to power up the bus with the clock running. Error = 0x%x.", retVal);
    }

    // Wait for a while until the voltage level becomes stable
    IOSleep(config.powerDelay);

    pinfo("The bus power is now on.");
    
    return retVal;
}

///
/// Power off the SD stack
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `mmc_power_up()` defined in `core.c`.
///
IOReturn IOSDHostDriver::powerOff()
{
    // Check whether the bus power is already off
    IOSDBusConfig& config = this->host->getHostBusConfig();

    if (config.powerMode == IOSDBusConfig::PowerMode::kPowerOff)
    {
        pinfo("The bus power is already off.");

        return kIOReturnSuccess;
    }
    
    pinfo("Powering off the bus...");

    config.clock = 0;

    config.vdd = 0;

    config.powerMode = IOSDBusConfig::PowerMode::kPowerOff;

    IOReturn retVal = this->setInitialBusConfig();

    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to power off the bus. Error = 0x%x.", retVal);
    }

    // Some cards require a short delay after powered off before turned on again
    IOSleep(1);

    return retVal;
}

///
/// Reboot the SD stack
///
/// @param ocr The OCR value from which to select the VDD value
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `mmc_power_cycle()` defined in `core.c`.
///
IOReturn IOSDHostDriver::powerCycle(UInt32 ocr)
{
    // Power off the stack
    IOReturn retVal = this->powerOff();

    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to power off the stack. Error = 0x%x.", retVal);

        return retVal;
    }

    // Wait at least 1 ms according to the SD specification
    IOSleep(1);

    // Power on the stack
    retVal = this->powerUp(ocr);

    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to power on the stack. Error = 0x%x.", retVal);

        return retVal;
    }

    return kIOReturnSuccess;
}

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
IODMACommand* IOSDHostDriver::allocateDMABuffer(IOByteCount size)
{
    // Guard: Allocate a buffer of the given size
    IOMemoryDescriptor* descriptor = IOBufferMemoryDescriptor::withCapacity(size, kIODirectionInOut);

    if (descriptor == nullptr)
    {
        perr("Failed to allocate a %llu-byte buffer.", size);

        return nullptr;
    }

    // Guard: Page in and wire down the buffer
    if (descriptor->prepare() != kIOReturnSuccess)
    {
        perr("Failed to page in and wire down the buffer.");

        descriptor->release();

        return nullptr;
    }

    // Get a preallocated DMA command from the pool
    // Note that the calling thread will be blocked until a command is available
    IODMACommand* command = this->allocateDMACommandFromPool();

    // Guard: Associate the memory descriptor with the DMA command
    // Note that the DMA command is prepared automatically
    if (command->setMemoryDescriptor(descriptor) != kIOReturnSuccess)
    {
        perr("Failed to associate the memory descriptor with the DMA command.");

        this->releaseDMACommandToPool(command);

        descriptor->complete();

        descriptor->release();

        return nullptr;
    }

    // Note that `IODMACommand::setMemoryDescriptor()` retains the memory descriptor
    // We can release it here, so that the reference count becomes 1 again
    descriptor->release();

    return command;
}

///
/// [Convenient] Release the given DMA capable buffer
///
/// @param command A non-null IODMACommand instance previously returned by `IOSDHostDriver::allocateDMABuffer()`.
///
void IOSDHostDriver::releaseDMABuffer(IODMACommand* command)
{
    // Clear the memory descriptor
    // Note that the DMA command is completed automatically
    // This method also releases the memory descriptor previously associated with the DMA command
    // @see https://opensource.apple.com/source/xnu/xnu-7195.101.1/iokit/Kernel/IODMACommand.cpp.auto.html
    // We do not need to `complete` the memory descriptor manually,
    // because its free method checks whether it is wired and if so `completes` it.
    // @see https://opensource.apple.com/source/xnu/xnu-7195.101.1/iokit/Kernel/IOMemoryDescriptor.cpp.auto.html
    psoftassert(command->clearMemoryDescriptor() == kIOReturnSuccess,
                "Failed to deassociate the memory descriptor with the given DMA command.");

    // Return the command back to the pool
    this->releaseDMACommandToPool(command);
}

//
// MARK: - SD Request Center
//

///
/// [Helper] Send the given SD command request and wait for the response
///
/// @param request A SD command request
/// @return `kIOReturnSuccess` on success, other values otherwise.
///
IOReturn IOSDHostDriver::waitForRequest(RealtekSDRequest& request)
{
    // TODO: CHECK THIS & IMP THIS
    return this->host->processRequest(request);
}

///
/// [Helper] Send the given SD application command request and wait for the response
///
/// @param request A SD application command request
/// @param rca The card relative address
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `mmc_wait_for_app_cmd()` defined in `sd_ops.c`.
/// @note This function issues a CMD55 before sending the given request.
///
IOReturn IOSDHostDriver::waitForAppRequest(RealtekSDRequest& request, UInt32 rca)
{
    static constexpr int kMaxAttempts = 10;
    
    IOReturn retVal = kIOReturnSuccess;
    
    for (int attempt = 0; attempt < kMaxAttempts; attempt += 1)
    {
        pinfo("[%02d] Sending the application command...", attempt);
        
        // Guard: Send the CMD55
        retVal = this->CMD55(rca);

        if (retVal != kIOReturnSuccess)
        {
            perr("Failed to issue a CMD55. Error = 0x%x.", retVal);

            continue;
        }

        // Send the application command
        retVal = this->waitForRequest(request);
        
        if (retVal != kIOReturnSuccess)
        {
            perr("Failed to issue the application command. Error = 0x%x.", retVal);
            
            continue;
        }
        
        pinfo("[%02d] The application command has been sent successfully.", attempt);
        
        return kIOReturnSuccess;
    }
    
    perr("Failed to send the application command. Error = 0x%x.", retVal);
    
    return retVal;
}

///
/// CMD0: Reset all cards to the idle state
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `mmc_go_idle()` defined in `mmc_ops.c`.
///
IOReturn IOSDHostDriver::CMD0()
{
    pinfo("Sending CMD0...");
    
    // Guard: Ensure that chip select is high to prevent the chip entering into the SPI mode
    pinfo("Setting the chip select to high to prevent the SPI mode...");
    
    IOReturn retVal = this->setChipSelect(IOSDBusConfig::ChipSelect::kHigh);

    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to set the chip select to high. Error = 0x%x.", retVal);

        return retVal;
    }

    IOSleep(1);
    
    pinfo("The chip select is now set to high.");

    // Issue the CMD0
    pinfo("Sending CMD0 to the card...");
    
    auto request = RealtekSDRequestFactory::CMD0();

    retVal = this->waitForRequest(request);

    IOSleep(1);

    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to issue the CMD0. Error = 0x%x.", retVal);

        psoftassert(this->setChipSelect(IOSDBusConfig::ChipSelect::kDoNotCare) == kIOReturnSuccess, "Failed to reset the chip select.");

        return retVal;
    }
    
    pinfo("The card is now in the idle state.");

    // Reset the chip select value
    pinfo("Setting the chip select to do-not-care...");
    
    retVal = this->setChipSelect(IOSDBusConfig::ChipSelect::kDoNotCare);

    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to reset the chip select. Error = 0x%x.", retVal);

        return retVal;
    }
    
    pinfo("CMD0 has been sent.");
    
    return kIOReturnSuccess;
}

///
/// CMD2: Ask any card to send the card identification data
///
/// @param buffer A non-null that stores the raw card identification data on return
/// @param length Specify the number of bytes read from the card identification data (must not exceed 16 bytes)
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `mmc_send_cid()` defined in `mmc_ops.c`.
/// @note Upon a successful return, the given buffer contains the response data as is.
///       The caller is responsible for dealing with the endianness and parsing the data.
/// @note It is recommended to use `IOSDHostDriver::CMD2(cid:)` to fetch and parse the data in one function call.
///
IOReturn IOSDHostDriver::CMD2(UInt8* buffer, IOByteCount length)
{
    auto request = RealtekSDRequestFactory::CMD2();

    IOReturn retVal = this->waitForRequest(request);

    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to issue the CMD2. Error = 0x%x.", retVal);

        return retVal;
    }
    
    length = min(sizeof(CID), length);

    memcpy(buffer, request.command.reinterpretResponseAs<SDResponse2>()->value, length);

    return kIOReturnSuccess;
}

///
/// CMD2: Ask any card to send the card identification data and parse the returned data
///
/// @param cid The **parsed** card identification data on return
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `mmc_send_cid()` defined in `mmc_ops.c`.
///
IOReturn IOSDHostDriver::CMD2(CID& cid)
{
    UInt8 buffer[sizeof(CID)] = {};
    
    IOReturn retVal = this->CMD2(buffer);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to fetch the raw card identification data. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // FIXME: Compatibility layer: REMOVE THIS ONCE THE DECODE FUNC IS REWRITTEN
    // FIXME: The decode function uses Linux's UNSTUFF_BITS macro to extract bits from the response
    // FIXME: This macro assumes that the data is an array of four UInt32 intergers encoded in big endian
    UInt32* data = reinterpret_cast<UInt32*>(buffer);
    
    for (auto index = 0; index < sizeof(CID) / sizeof(UInt32); index += 1)
    {
        data[index] = OSSwapInt32(data[index]);
    }
    
    if (!CID::decode(data, cid))
    {
        perr("Failed to decode the raw card identification data.");
        
        return kIOReturnInvalid;
    }
    
    pinfo("Fetched and decoded the card identification data successfully.");
    
    return kIOReturnSuccess;
}

///
/// CMD3: Ask the card to publish its relative address
///
/// @param rca The card relative address on return
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `mmc_send_relative_addr()` defined in `sd_ops.c`.
///
IOReturn IOSDHostDriver::CMD3(UInt32& rca)
{
    auto request = RealtekSDRequestFactory::CMD3();

    IOReturn retVal = this->waitForRequest(request);

    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to issue the CMD3. Error = 0x%x.", retVal);

        return retVal;
    }

    rca = request.command.reinterpretResponseAs<SDResponse6>()->getRCA();

    return kIOReturnSuccess;
}

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
IOReturn IOSDHostDriver::CMD6(UInt32 mode, UInt32 group, UInt8 value, UInt8* response, IOByteCount length)
{
    // Sanitize the given `mode` and `value`
    pinfo("CMD6: [ORG] Mode = %d; Group = %d; Value = %d.", mode, group, value);

    mode = !!mode;

    value &= 0x0F;

    pinfo("CMD6: [SAN] Mode = %d; Group = %d; Value = %d.", mode, group, value);

    // Allocate a DMA buffer
    IODMACommand* dma = this->allocateDMABuffer(64);

    if (dma == nullptr)
    {
        perr("Failed to allocate a 64-byte DMA buffer.");

        return kIOReturnNoMemory;
    }

    // Send the command
    // TODO: SET DATA TIMEOUT????
    auto request = RealtekSDRequestFactory::CMD6(mode, group, value, dma);

    IOReturn retVal = this->waitForRequest(request);

    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to issue the CMD6. Error = 0x%x.", retVal);

        this->releaseDMABuffer(dma);

        return retVal;
    }

    // Copy the SD status from the DMA buffer
    length = min(length, 64);

    retVal = dma->readBytes(0, response, length) == length ? kIOReturnSuccess : kIOReturnError;

    this->releaseDMABuffer(dma);

    return retVal;
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
IOReturn IOSDHostDriver::CMD6(UInt32 mode, UInt32 group, UInt8 value, IOMemoryDescriptor* response)
{
    // Guard: Verify the given response buffer
    if (response == nullptr)
    {
        perr("The given response buffer cannot be NULL.");

        return kIOReturnBadArgument;
    }

    // Sanitize the given `mode` and `value`
    pinfo("CMD6: [ORG] Mode = %d; Group = %d; Value = %d.", mode, group, value);

    mode = !!mode;

    value &= 0x0F;

    pinfo("CMD6: [SAN] Mode = %d; Group = %d; Value = %d.", mode, group, value);

    // Guard: Associate the given response buffer with the DMA command
    // Note that the DMA command is prepared automatically
    IODMACommand* dma = this->allocateDMACommandFromPool();

    IOReturn retVal = dma->setMemoryDescriptor(response);

    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to associate the given response with the DMA command. Error = 0x%x.", retVal);

        this->releaseDMACommandToPool(dma);
        
        return retVal;
    }

    // Generate the SD command request
    auto request = RealtekSDRequestFactory::CMD6(mode, group, value, dma);

    // TODO: Set the data timeout as Linux???
    // TODO: Realtek's driver seems to ignore the data timeout in the mmc_data struct
    retVal = this->waitForRequest(request);

    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to issue the CMD6. Error = 0x%x.", retVal);
    }

    // Deassociate the given response buffer with the DMA command
    // Note that the DMA command is completed automatically
    psoftassert(dma->clearMemoryDescriptor() == kIOReturnSuccess,
                "Failed to deassociate the given response with the DMA command.");

    // Return the DMA command back
    this->releaseDMACommandToPool(dma);

    return retVal;
}

///
/// CMD7: Select a card
///
/// @param rca The relative address of the card to be selected;
///            Pass 0 to deselect all cards
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `_mmc_select_card()` defined in `sd_ops.c`.
///
IOReturn IOSDHostDriver::CMD7(UInt32 rca)
{
    auto request = RealtekSDRequestFactory::CMD7(rca);

    return this->waitForRequest(request);
}

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
IOReturn IOSDHostDriver::CMD8(UInt8 vhs, SDResponse7& response)
{
    static constexpr UInt8 kCheckPattern = 0xAA;

    // Guard: Send the command
    auto request = RealtekSDRequestFactory::CMD8(vhs, kCheckPattern);

    IOReturn retVal = this->waitForRequest(request);

    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to issue the CMD8. Error = 0x%x.", retVal);

        return retVal;
    }

    // Guard: Verify the check pattern in the response
    auto res = request.command.reinterpretResponseAs<SDResponse7>();

    if (res->checkPattern != kCheckPattern)
    {
        perr("The check pattern in the response is invalid.");

        return kIOReturnInvalid;
    }

    response = *res;

    return kIOReturnSuccess;
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
IOReturn IOSDHostDriver::CMD9(UInt32 rca, UInt8* buffer, IOByteCount length)
{
    auto request = RealtekSDRequestFactory::CMD9(rca);

    IOReturn retVal = this->waitForRequest(request);

    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to issue the CMD9. Error = 0x%x.", retVal);

        return retVal;
    }
    
    length = min(sizeof(CSDVX), length);

    memcpy(buffer, request.command.reinterpretResponseAs<SDResponse2>()->value, length);

    return kIOReturnSuccess;
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
IOReturn IOSDHostDriver::CMD9(UInt32 rca, CSD& csd)
{
    UInt8 buffer[sizeof(CSDVX)] = {};
    
    IOReturn retVal = this->CMD9(rca, buffer);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to fetch the card specific data. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // FIXME: Compatibility layer: REMOVE THIS ONCE THE DECODE FUNC IS REWRITTEN
    // FIXME: The decode function uses Linux's UNSTUFF_BITS macro to extract bits from the response
    // FIXME: This macro assumes that the data is an array of four UInt32 intergers encoded in big endian
    UInt32* data = reinterpret_cast<UInt32*>(buffer);
    
    for (auto index = 0; index < sizeof(CSDVX) / sizeof(UInt32); index += 1)
    {
        data[index] = OSSwapInt32(data[index]);
    }
    
    if (!CSD::decode(data, csd))
    {
        perr("Failed to decode the SD configuration register value.");
        
        return kIOReturnInvalid;
    }
    
    pinfo("Fetched and decoded SD configuration register value successfully.");
    
    return kIOReturnSuccess;
}

///
/// CMD11: Switch to 1.8V bus signaling level
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces a portion of `mmc_set_uhs_voltage()` defined in `mmc_ops.c`.
///
IOReturn IOSDHostDriver::CMD11()
{
    auto request = RealtekSDRequestFactory::CMD11();

    IOReturn retVal = this->waitForRequest(request);

    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to initiate the CMD11. Error = 0x%x.", retVal);

        return retVal;
    }

    if (BitOptions(request.command.reinterpretResponseAs<SDResponse1>()->getStatus()).contains(R1_ERROR))
    {
        perr("The response to the CMD11 has the error bit set.");

        return kIOReturnInvalid;
    }

    return kIOReturnSuccess;
}

///
/// CMD13: Send the card status
///
/// @param rca The card relative address
/// @param status The card status on return
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `__mmc_send_status()` defined in `mmc_ops.c`.
///
IOReturn IOSDHostDriver::CMD13(UInt32 rca, UInt32& status)
{
    auto request = RealtekSDRequestFactory::CMD13(rca);

    IOReturn retVal = this->waitForRequest(request);

    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to initiate the CMD13. Error = 0x%x.", retVal);

        return retVal;
    }

    status = request.command.reinterpretResponseAs<SDResponse1>()->getStatus();

    return kIOReturnSuccess;
}

///
/// CMD55: Tell the card that the next command is an application command
///
/// @param rca The card relative address
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `mmc_app_cmd()` defined in `sd_ops.c`.
///
IOReturn IOSDHostDriver::CMD55(UInt32 rca)
{
    // Send the command
    auto request = RealtekSDRequestFactory::CMD55(rca);

    IOReturn retVal = this->waitForRequest(request);

    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to initiate the CMD55. Error = 0x%x.", retVal);

        return retVal;
    }

    // Check whether the card supports application commands
    if (!BitOptions(request.command.reinterpretResponseAs<SDResponse1>()->getStatus()).contains(R1_APP_CMD))
    {
        perr("The card does not support application commands.");

        return kIOReturnUnsupported;
    }

    return kIOReturnSuccess;
}

///
/// ACMD6: Set the data bus width
///
/// @param rca The card relative address
/// @param busWidth The data bus width (1 bit or 4 bit)
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `mmc_app_set_bus_width()` defined in `sd_ops.c`.
///
IOReturn IOSDHostDriver::ACMD6(UInt32 rca, IOSDBusConfig::BusWidth busWidth)
{
    // Bus width value
    static constexpr UInt32 kSDBusWidth1Bit = 0b00;
    static constexpr UInt32 kSDBusWidth4Bit = 0b10;

    // Verify the given bus width
    UInt32 busWidthValue;

    switch (busWidth)
    {
        case IOSDBusConfig::BusWidth::k1Bit:
        {
            busWidthValue = kSDBusWidth1Bit;

            break;
        }

        case IOSDBusConfig::BusWidth::k4Bit:
        {
            busWidthValue = kSDBusWidth4Bit;

            break;
        }

        default:
        {
            perr("SD does not support the 8-bit bus.");

            return kIOReturnBadArgument;
        }
    }

    auto request = RealtekSDRequestFactory::ACMD6(busWidthValue);

    return this->waitForAppRequest(request, rca);
}

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
IOReturn IOSDHostDriver::ACMD13(UInt32 rca, UInt8* status, IOByteCount length)
{
    // Allocate a DMA buffer
    IODMACommand* dma = this->allocateDMABuffer(64);

    if (dma == nullptr)
    {
        perr("Failed to allocate a 64-byte DMA buffer.");

        return kIOReturnNoMemory;
    }

    // Send the command
    // TODO: SET DATA TIMEOUT????
    auto request = RealtekSDRequestFactory::ACMD13(dma);

    IOReturn retVal = this->waitForAppRequest(request, rca);

    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to issue the ACMD13. Error = 0x%x.", retVal);

        this->releaseDMABuffer(dma);

        return retVal;
    }

    // Copy the SD status from the DMA buffer
    length = min(length, 64);

    retVal = dma->readBytes(0, status, length) == length ? kIOReturnSuccess : kIOReturnError;

    this->releaseDMABuffer(dma);

    return retVal;
}

///
/// ACMD41: Send the operating condition register (OCR) value at the probe stage
///
/// @param rocr The OCR value returned by the card
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `mmc_send_app_op_cond()` defined in `sd_ops.c`.
///
IOReturn IOSDHostDriver::ACMD41(UInt32& rocr)
{
    auto request = RealtekSDRequestFactory::ACMD41(0);

    IOReturn retVal = this->waitForAppRequest(request, 0);

    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to issue the ACMD41. Error = 0x%x.", retVal);

        return retVal;
    }

    rocr = request.command.reinterpretResponseAs<SDResponse3>()->getValue();

    return kIOReturnSuccess;
}

///
/// ACMD41: Send the operating condition register (OCR) value
///
/// @param ocr The OCR value that contains requested settings
/// @param rocr The OCR value returned by the card
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `mmc_send_app_op_cond()` defined in `sd_ops.c`.
///
IOReturn IOSDHostDriver::ACMD41(UInt32 ocr, UInt32& rocr)
{
    auto request = RealtekSDRequestFactory::ACMD41(ocr);

    for (int attempt = 0; attempt < 100; attempt += 1)
    {
        // Send the command
        IOReturn retVal = this->waitForAppRequest(request, 0);

        if (retVal != kIOReturnSuccess)
        {
            perr("Failed to issue the ACMD41. Error = 0x%x.", retVal);

            return retVal;
        }

        // Retrieve the returned OCR value
        rocr = request.command.reinterpretResponseAs<SDResponse3>()->getValue();

        // Check the busy bit
        if (BitOptions(rocr).containsBit(31))
        {
            return kIOReturnSuccess;
        }
        
        IOSleep(20);
    }

    return kIOReturnTimeout;
}

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
IOReturn IOSDHostDriver::ACMD51(UInt32 rca, UInt8* buffer, IOByteCount length)
{
    // Allocate a DMA buffer
    IODMACommand* dma = this->allocateDMABuffer(8);

    if (dma == nullptr)
    {
        perr("Failed to allocate a 8-byte DMA buffer.");

        return kIOReturnNoMemory;
    }

    // Send the command
    // TODO: SET DATA TIMEOUT????
    auto request = RealtekSDRequestFactory::ACMD51(dma);

    IOReturn retVal = this->waitForAppRequest(request, rca);

    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to issue the ACMD51. Error = 0x%x.", retVal);

        this->releaseDMABuffer(dma);

        return retVal;
    }

    // Copy the SD status from the DMA buffer
    length = min(length, 8);

    retVal = dma->readBytes(0, buffer, length) == length ? kIOReturnSuccess : kIOReturnError;

    this->releaseDMABuffer(dma);

    return retVal;
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
IOReturn IOSDHostDriver::ACMD51(UInt32 rca, SCR& scr)
{
    UInt8 buffer[sizeof(SCR)] = {};
    
    IOReturn retVal = this->ACMD51(rca, buffer);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to fetch the SD configuration register value. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    if (!SCR::decode(buffer, scr))
    {
        perr("Failed to decode the SD configuration register value.");
        
        return kIOReturnInvalid;
    }
    
    pinfo("Fetched and decoded SD configuration register value successfully.");
    
    return kIOReturnSuccess;
}

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
bool IOSDHostDriver::attachCard(UInt32 frequency)
{
    // Start to attach the card
    pinfo("Trying to initialize the card at %u Hz.", frequency);

    psoftassert(this->card == nullptr, "this->card should be null at this moment.");

    // Initial clock and voltages
    this->host->setHostInitialClock(frequency);

    UInt32 ocr = this->host->getHostSupportedVoltageRanges();
    
    pinfo("Voltage ranges supported by the host: 0x%08x.", ocr);
    
    // OCR value returned by the card
    UInt32 rocr = 0;
    
    // Power up the SD bus
    pinfo("Powering up the host bus...");
    
    if (this->powerUp(ocr) != kIOReturnSuccess)
    {
        perr("Failed to power up the host bus.");

        return false;
    }
    
    pinfo("The host bus is now powered up.");

    // Tell the card to go to the idle state
    pinfo("Asking the card to go to the idle state...");
    
    if (this->CMD0() != kIOReturnSuccess)
    {
        perr("Failed to tell the card to go to the idle state.");

        goto error;
    }
    
    pinfo("The card is now in the idle state.");

    // Check whether a SD card is inserted
    if (this->CMD8((ocr & 0xFF8000) != 0) != kIOReturnSuccess)
    {
        perr("The card does not respond to the CMD8.");
    }

    if (this->ACMD41(rocr) != kIOReturnSuccess)
    {
        perr("The card does not respond to the ACMD41.");

        goto error;
    }

    // Filter out unsupported voltage levels
    rocr &= ~0x7FFF;

    rocr = this->selectMutualVoltageLevels(rocr);

    if (rocr == 0)
    {
        perr("Failed to find a voltage level supported by both the host and the card.");

        return false;
    }
    
    pinfo("Voltage levels supported by both sides = 0x%08x (OCR).", rocr);
    
    // Start the card initialization sequence
    pinfo("Creating the card with the OCR = 0x%08x.", rocr);
    
    this->card = IOSDCard::createWithOCR(this, rocr);

    if (this->card == nullptr)
    {
        perr("Failed to complete the card initialization sequence.");

        goto error;
    }
    
    if (!this->card->attach(this))
    {
        perr("Failed to attach the SD card device.");
        
        OSSafeReleaseNULL(this->card);
        
        goto error;
    }
    
    if (!this->card->start(this))
    {
        perr("Failed to start the SD card device.");
        
        this->card->detach(this);
        
        OSSafeReleaseNULL(this->card);
        
        goto error;
    }
    
    // Fetch and publish the card characteristics
    // so that the System Profiler can recognize the card and show related information
    this->publishCardCharacteristics();
    
    // Sanitize the pending request queue
    // It is possible that one or two requests are left in the queue
    // even after the card has been removed from the system.
    // See `IOSDHostDriver::submitBlockRequest()` for details.
    this->recyclePendingBlockRequest();
    
    pinfo("The card has been created and initialized.");
    
    return true;
    
error:
    psoftassert(this->powerOff() == kIOReturnSuccess, "Failed to power off the bus.");
    
    return false;
}

///
/// [Helper] Publish the block storage device
///
/// @return `true` on success, `false` otherwise.
/// @note This function is invoked by `IOSDHostDriver::attachCard()`,
///       so it runs synchronously with respect to the processor workloop.
///
bool IOSDHostDriver::publishBlockStorageDevice()
{
    // Initialize and publish the block storage device
    pinfo("Publishing the block storage device...");
    
    IOSDBlockStorageDevice* device = OSTypeAlloc(IOSDBlockStorageDevice);

    if (device == nullptr)
    {
        perr("Failed to allocate the block storage device.");

        return false;
    }

    if (!device->init(nullptr))
    {
        perr("Failed to initialize the block storage device.");

        device->release();

        return false;
    }

    if (!device->attach(this))
    {
        perr("Failed to attach the block storage device.");

        device->release();

        return false;
    }

    if (!device->start(this))
    {
        perr("Failed to start the block storage device.");

        device->detach(this);

        device->release();

        return false;
    }

    this->blockStorageDevice = device;
    
    pinfo("The block storage device has been published.");
    
    return true;
}

///
/// [Helper] Publish the card characteristics
///
/// @note System Information needs this to present card information.
///
void IOSDHostDriver::publishCardCharacteristics()
{
    OSDictionary* characteristics = this->card->getCardCharacteristics();
    
    if (characteristics != nullptr)
    {
        this->getHostDevice()->setProperty("Card Characteristics", characteristics);
        
        OSSafeReleaseNULL(characteristics);
    }
    else
    {
        perr("Failed to fetch the card characteristics.");
    }
}

///
/// [Helper] Remove the published card characteristics
///
void IOSDHostDriver::removeCardCharacteristics()
{
    this->host->removeProperty("Card Characteristics");
}

///
/// Attach the SD card
///
/// @note This function is invoked on the processor workloop thread when a SD card is inserted.
///
void IOSDHostDriver::attachCard()
{
    /// Initial card frequencies in Hz
    static constexpr UInt32 frequencies[] = { KHz2Hz(400), KHz2Hz(300), KHz2Hz(200), KHz2Hz(100) };

    pinfo("Attaching the SD card...");
    
    ClosedRange<UInt32> range = this->host->getHostClockRange();
    
    // Try each default frequency
    for (auto frequency : frequencies)
    {
        pinfo("---------------------------------------------------------------------------");
        
        // Guard: Ensure that the default frequency is supported
        if (!range.contains(frequency))
        {
            perr("Default frequency %d Hz is not supported by the host device.", frequency);
            
            continue;
        }
        
        // Guard: Attempt to initialize the card
        if (!this->attachCard(frequency))
        {
            perr("Failed to initialize the card at %u Hz.", frequency);
            
            continue;
        }
        
        // The card has been initialized
        pinfo("The card has been initialized at %u Hz.", frequency);
        
        psoftassert(this->publishBlockStorageDevice(), "Failed to publish the block storage device");
        
        pinfo("The SD card has been attached.");
        
        return;
    }
    
    // The default frequencies are not suitable for the host
    perr("Failed to initialize the card at the default frequencies.");
}

///
/// Detach the SD card
///
/// @note This function is invoked on the processor workloop thread when a SD card is removed.
///
void IOSDHostDriver::detachCard()
{
    pinfo("Detaching the SD card...");
    
    // Stop the block storage device
    if (this->blockStorageDevice != nullptr)
    {
        pinfo("Stopping the block storage device...");
        
        this->blockStorageDevice->terminate();
        
        this->blockStorageDevice->stop(this);

        this->blockStorageDevice->detach(this);
        
        this->blockStorageDevice->release();
        
        this->blockStorageDevice = nullptr;
        
        pinfo("The block storage device has been stopped.");
    }
    
    // Stop the card device
    if (this->card != nullptr)
    {
        pinfo("Stopping the card device...");
        
        this->card->stop(this);
        
        this->card->detach(this);
        
        this->card->release();
        
        this->card = nullptr;
        
        pinfo("The card device has been stopped.");
    }
    
    // Remove published card characteristics
    this->removeCardCharacteristics();
    
    // Recycle all pending requests
    this->recyclePendingBlockRequest();
    
    // Power off the bus
    psoftassert(this->powerOff() == kIOReturnSuccess, "Failed to power off the bus.");
    
    pinfo("The SD card has been detached.");
}

//
// MARK: - Card Events Callbacks
//

///
/// [UPCALL] Notify the host driver when a SD card is inserted
///
/// @note This callback function runs in a gated context provided by the underlying card reader controller.
///       The host device should implement this function without any blocking operations.
///
void IOSDHostDriver::onSDCardInsertedGated()
{
    // Make sure that the detach event source is disabled
    this->detachCardEventSource->disable();
    
    // Notify the processor work loop to attach the card
    this->attachCardEventSource->enable();
    
    // Enable the queue event source to accept incoming block requests
    this->queueEventSource->enable();
}

///
/// [UPCALL] Notify the host driver when a SD card is removed
///
/// @note This callback function runs in a gated context provided by the underlying card reader controller.
///       The host device should implement this function without any blocking operations.
///
void IOSDHostDriver::onSDCardRemovedGated()
{
    // Disable the queue event source so that the processor work loop will stop processing requests
    this->queueEventSource->disable();
    
    // Make sure that the attach event source is disabled
    this->attachCardEventSource->disable();
    
    // Notify the processor work loop to detach the card
    this->detachCardEventSource->enable();
}

//
// MARK: - Query Card Information
//

///
/// Check whether the card has write protection enabled
///
/// @param result Set `true` if the card is write protected, `false` otherwise.
/// @return `kIOReturnSuccess` on success, other values otherwise.
///
IOReturn IOSDHostDriver::isCardWriteProtected(bool& result)
{
    return this->host->isCardWriteProtected(result);
}

///
/// Check whether the card exists
///
/// @param result Set `true` if the card exists, `false` otherwise.
/// @return `kIOReturnSuccess` always.
///
IOReturn IOSDHostDriver::isCardPresent(bool& result)
{
    return this->host->isCardPresent(result);
}

///
/// Get the card capacity in number of blocks
///
/// @param nblocks The number of blocks on return
/// @return `kIOReturnSuccess` on success, `kIOReturnNoMedia` if the card is not present.
///
IOReturn IOSDHostDriver::getCardNumBlocks(UInt64& nblocks)
{
    if (this->card == nullptr)
    {
        perr("The card is not present.");
        
        return kIOReturnNoMedia;
    }
    
    nblocks = this->card->getCSD().capacity;
    
    // Treat the SDSC card as a block device whose block size is 512 bytes
    // SDHC/XC cards always have a read block length of 9
    // SDSC cards may have a read block length of 9, 10, or 11
    // Adjust the total number of blocks accordingly
    nblocks <<= (this->card->getCSD().readBlockLength - 9);
    
    return kIOReturnSuccess;
}

///
/// Get the index of the maximum block of the card
///
/// @param index The index of the last accessible block on the card on return
/// @return `kIOReturnSuccess` on success, `kIOReturnNoMedia` if the card is not present.
///
IOReturn IOSDHostDriver::getCardMaxBlockIndex(UInt64& index)
{
    IOReturn retVal = this->getCardNumBlocks(index);
    
    index -= 1;
    
    return retVal;
}

///
/// Get the size of a block
///
/// @param length The block length in bytes on return
/// @return `kIOReturnSuccess` on success, `kIOReturnNoMedia` if the card is not present.
///
IOReturn IOSDHostDriver::getCardBlockLength(UInt64& length)
{
    if (this->card == nullptr)
    {
        perr("The card is not present.");
        
        return kIOReturnNoMedia;
    }
    
    length = 512;
    
    return kIOReturnSuccess;
}

///
/// Get the card vendor name
///
/// @return A non-null static vendor string.
/// @note This function returns "<null>" if the card is not present.
///
const char* IOSDHostDriver::getCardVendor()
{
    if (this->card == nullptr)
    {
        perr("The card is not present.");
        
        return "Realtek";
    }
    
    return this->card->getCID().getVendorString();
}

///
/// Get the card product name
///
/// @param name A non-null buffer that can hold at least 8 bytes.
/// @param length The buffer length
/// @return `kIOReturnSuccess` on success,
///         `kIOReturnNoMedia` if the card is not present,
///         `kIOReturnBadArgument` if the buffer is NULL or too small.
///
IOReturn IOSDHostDriver::getCardName(char* name, IOByteCount length)
{
    if (this->card == nullptr)
    {
        perr("The card is not present.");
        
        return kIOReturnNoMedia;
    }
    
    if (!this->card->getCardName(name, length))
    {
        perr("The given buffer/length is invalid.");
        
        return kIOReturnBadArgument;
    }
    
    return kIOReturnSuccess;
}

///
/// Get the card revision value
///
/// @param revision A non-null buffer that can hold at least 8 bytes.
/// @param length The buffer length
/// @return `kIOReturnSuccess` on success,
///         `kIOReturnNoMedia` if the card is not present,
///         `kIOReturnBadArgument` if the buffer is NULL or too small.
///
IOReturn IOSDHostDriver::getCardRevision(char* revision, IOByteCount length)
{
    if (this->card == nullptr)
    {
        perr("The card is not present.");
        
        return kIOReturnNoMedia;
    }
    
    if (!this->card->getCardRevision(revision, length))
    {
        perr("The given buffer/length is invalid.");
        
        return kIOReturnBadArgument;
    }
    
    return kIOReturnSuccess;
}

///
/// Get the card manufacture date
///
/// @param date A non-null buffer that can hold at least 8 bytes.
/// @param length The buffer length
/// @return `kIOReturnSuccess` on success,
///         `kIOReturnNoMedia` if the card is not present,
///         `kIOReturnBadArgument` if the buffer is NULL or too small.
///
IOReturn IOSDHostDriver::getCardProductionDate(char* date, IOByteCount length)
{
    if (this->card == nullptr)
    {
        perr("The card is not present.");
        
        return kIOReturnNoMedia;
    }
    
    if (!this->card->getCardProductionDate(date, length))
    {
        perr("The given buffer/length is invalid.");
        
        return kIOReturnBadArgument;
    }
    
    return kIOReturnSuccess;
}

///
/// Get the card serial number
///
/// @param serial The card serial number on return
/// @return `kIOReturnSuccess` on success, `kIOReturnNoMedia` if the card is not present.
///
IOReturn IOSDHostDriver::getCardSerialNumber(UInt32& serial)
{
    if (this->card == nullptr)
    {
        perr("The card is not present.");
        
        return kIOReturnNoMedia;
    }
    
    serial = this->card->getCID().serial;
    
    return kIOReturnSuccess;
}

//
// MARK: - Power Management
//

///
/// Prepare to enter the sleep state
///
void IOSDHostDriver::prepareToSleep()
{
    pinfo("Prepare to sleep...");
    
    // Disable the queue event source so that the processor work loop will stop processing requests
    this->queueEventSource->disable();
    
    // Detach the card
    this->detachCard();
    
    // All done
    pinfo("The host driver is ready to sleep.");
}

///
/// Prepare to wake up from sleep
///
void IOSDHostDriver::prepareToWakeUp()
{
    pinfo("Prepare to wake up...");
    
    bool present = false;
    
    this->isCardPresent(present);
    
    if (present)
    {
        // Attach the card
        pinfo("The card is present when the computer wakes up.");
        
        this->queueEventSource->enable();
        
        this->attachCard();
    }
    else
    {
        pinfo("The card is not present when the computer wakes up.");
    }
    
    // All done
    pinfo("The host driver is ready.");
}

///
/// Adjust the power state in response to system-wide power events
///
/// @param powerStateOrdinal The number in the power state array of the state the driver is being instructed to switch to
/// @param whatDevice A pointer to the power management object which registered to manage power for this device
/// @return `kIOPMAckImplied` always.
///
IOReturn IOSDHostDriver::setPowerState(unsigned long powerStateOrdinal, IOService* whatDevice)
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
// MARK: - Startup Routines
//

///
/// Setup the power management
///
/// @return `true` on success, `false` otherwise.
///
bool IOSDHostDriver::setupPowerManagement()
{
    static IOPMPowerState kPowerStates[] =
    {
        { kIOPMPowerStateVersion1, kIOPMPowerOff, kIOPMPowerOff, kIOPMPowerOff, 0, 0, 0, 0, 0, 0, 0, 0 },
        { kIOPMPowerStateVersion1, kIOPMPowerOn | kIOPMDeviceUsable | kIOPMInitialDeviceState, kIOPMPowerOn, kIOPMPowerOn, 0, 0, 0, 0, 0, 0, 0, 0 },
    };
    
    this->PMinit();
    
    this->host->joinPMtree(this);
    
    return this->registerPowerDriver(this, kPowerStates, arrsize(kPowerStates)) == kIOPMNoErr;
}

///
/// Setup the array of preallocated DMA commands
///
/// @return `true` on success, `false` otherwise.
/// @note Upon an unsuccessful return, all resources allocated by this function are released.
///
bool IOSDHostDriver::setupPreallocatedDMACommands()
{
    pinfo("Preallocating an array of DMA commands...");
    
    bzero(this->pDMACommands, sizeof(this->pDMACommands));
    
    UInt64 maxSegmentSize = this->host->getDMALimits().maxSegmentSize;
    
    pinfo("Maximum segment size suppported by the host device = %llu bytes.", maxSegmentSize);
    
    for (auto index = 0; index < IOSDHostDriver::kDefaultPoolSize; index += 1)
    {
        this->pDMACommands[index] = IODMACommand::withSpecification(kIODMACommandOutputHost32, 32, maxSegmentSize, IODMACommand::kMapped, 0, 1);
        
        if (this->pDMACommands[index] == nullptr)
        {
            perr("[%02d] Failed to preallocate the DMA command.", index);
            
            this->tearDownPreallocatedDMACommands();
            
            return false;
        }
    }
    
    pinfo("Preallocated %u DMA commands successfully.", IOSDHostDriver::kDefaultPoolSize);
    
    return true;
}

///
/// Setup the array of preallocated SD block requests
///
/// @return `true` on success, `false` otherwise.
/// @note Upon an unsuccessful return, all resources allocated by this function are released.
///
bool IOSDHostDriver::setupPreallocatedBlockRequests()
{
    pinfo("Preallocating an array of block requests...");
    
    bzero(this->pSimpleBlockRequests, sizeof(this->pSimpleBlockRequests));
    
    bzero(this->pComplexBlockRequests, sizeof(this->pComplexBlockRequests));
    
    for (auto index = 0; index < IOSDHostDriver::kDefaultPoolSize; index += 1)
    {
        this->pSimpleBlockRequests[index] = OSTypeAlloc(IOSDSimpleBlockRequest);
        
        this->pComplexBlockRequests[index] = OSTypeAlloc(IOSDComplexBlockRequest);
        
        if (this->pSimpleBlockRequests[index] == nullptr ||
            this->pComplexBlockRequests[index] == nullptr)
        {
            perr("[%02d] Failed to preallocate the block request.", index);
            
            this->tearDownPreallocatedBlockRequests();
            
            return false;
        }
    }
    
    pinfo("Preallocated %u SD block requests successfully.", IOSDHostDriver::kDefaultPoolSize);
    
    return true;
}

///
/// Setup the shared work loop to protect the pool and the queue
///
/// @return `true` on success, `false` otherwise.
/// @note Upon an unsuccessful return, all resources allocated by this function are released.
///
bool IOSDHostDriver::setupSharedWorkLoop()
{
    pinfo("Creating the shared work loop...");
    
    this->sharedWorkLoop = IOWorkLoop::workLoop();
    
    if (this->sharedWorkLoop == nullptr)
    {
        perr("Failed to create the shared work loop.");
        
        return false;
    }
    
    pinfo("The shared work loop has been created.");
    
    return true;
}

///
/// Setup the DMA command pool
///
/// @return `true` on success, `false` otherwise.
/// @note Upon an unsuccessful return, all resources allocated by this function are released.
///
bool IOSDHostDriver::setupDMACommandPool()
{
    pinfo("Creating the DMA command pool...");
    
    this->dmaCommandPool = IOCommandPool::withWorkLoop(this->sharedWorkLoop);
    
    if (this->dmaCommandPool == nullptr)
    {
        perr("Failed to create the DMA command pool.");
        
        return false;
    }
    
    pinfo("Populating the pool with preallocated DMA commands...");
    
    for (auto index = 0; index < IOSDHostDriver::kDefaultPoolSize; index += 1)
    {
        this->releaseDMACommandToPool(this->pDMACommands[index]);
    }
    
    pinfo("The DMA command pool has been created.");
    
    return true;
}

///
/// Setup the SD block request pool
///
/// @return `true` on success, `false` otherwise.
/// @note Upon an unsuccessful return, all resources allocated by this function are released.
///
bool IOSDHostDriver::setupBlockRequestPool()
{
    pinfo("Creating the block request pool...");
    
    this->simpleBlockRequestPool = IOCommandPool::withWorkLoop(this->sharedWorkLoop);
    
    if (this->simpleBlockRequestPool == nullptr)
    {
        perr("Failed to create the simple block request pool.");
        
        return false;
    }
    
    this->complexBlockRequestPool = IOCommandPool::withWorkLoop(this->sharedWorkLoop);
    
    if (this->complexBlockRequestPool == nullptr)
    {
        perr("Failed to create the complex block request pool.");
        
        this->simpleBlockRequestPool->release();
        
        this->simpleBlockRequestPool = nullptr;
        
        return false;
    }
    
    pinfo("Populating the pool with preallocated block requests...");
    
    for (auto index = 0; index < IOSDHostDriver::kDefaultPoolSize; index += 1)
    {
        this->releaseSimpleBlockRequestToPool(this->pSimpleBlockRequests[index]);
        
        this->releaseComplexBlockRequestToPool(this->pComplexBlockRequests[index]);
    }
    
    pinfo("The block request pool has been created.");
    
    return true;
}

///
/// Setup the SD block request queue
///
/// @return `true` on success, `false` otherwise.
/// @note Upon an unsuccessful return, all resources allocated by this function are released.
///
bool IOSDHostDriver::setupBlockRequestQueue()
{
    pinfo("Creating the block request queue...");
    
    this->pendingRequests = IOSDBlockRequestQueue::create(this->sharedWorkLoop);
    
    if (this->pendingRequests == nullptr)
    {
        perr("Failed to create the request queue.");
        
        return false;
    }
    
    pinfo("The block request queue has been created.");
    
    return true;
}

///
/// Setup the shared work loop to process card events and block requests
///
/// @return `true` on success, `false` otherwise.
/// @note Upon an unsuccessful return, all resources allocated by this function are released.
///
bool IOSDHostDriver::setupProcessorWorkLoop()
{
    pinfo("Creating the dedicated processor work loop...");
    
    this->processorWorkLoop = IOWorkLoop::workLoop();
    
    if (this->processorWorkLoop == nullptr)
    {
        perr("Failed to create the dedicated processor work loop.");
        
        return false;
    }
    
    pinfo("The dedicated processor work loop has been created.");
    
    return true;
}

///
/// Setup the event source that signals the processor work loop to process the block request
///
/// @return `true` on success, `false` otherwise.
/// @note Upon an unsuccessful return, all resources allocated by this function are released.
///
bool IOSDHostDriver::setupBlockRequestEventSource()
{
    pinfo("Creating the block request event source...");
    
    auto finalizer = OSMemberFunctionCast(IOSDBlockRequestEventSource::Action, this, &IOSDHostDriver::finalizeBlockRequest);
    
    this->queueEventSource = IOSDBlockRequestEventSource::createWithQueue(this, finalizer, this->pendingRequests);
    
    if (this->queueEventSource == nullptr)
    {
        perr("Failed to create the block request event source.");
        
        return false;
    }
    
    this->queueEventSource->disable();
    
    this->processorWorkLoop->addEventSource(this->queueEventSource);
    
    pinfo("The block request event source has been created and registerd with the processor work loop.");
    
    return true;
}

///
/// Setup the event sources that signal the processor work loop to handle card insertion and removal events
///
/// @return `true` on success, `false` otherwise.
/// @note Upon an unsuccessful return, all resources allocated by this function are released.
///
bool IOSDHostDriver::setupCardEventSources()
{
    // Card Insertion Event
    pinfo("Creating the card insertion event source...");
    
    this->attachCardEventSource = IOSDCardEventSource::createWithAction(this, OSMemberFunctionCast(IOSDCardEventSource::Action, this, &IOSDHostDriver::attachCard));
    
    if (this->attachCardEventSource == nullptr)
    {
        perr("Failed to create the card insertion event source.");
        
        return false;
    }
    
    // The card insertion event source will be enabled by the notification handler
    // @see `IOSDHostDriver::onSDCardInsertedGated()` for details
    this->attachCardEventSource->disable();
    
    pinfo("The card insertion event source has been created.");
    
    // Card Removal Event
    pinfo("Creating the card removal event source...");
    
    this->detachCardEventSource = IOSDCardEventSource::createWithAction(this, OSMemberFunctionCast(IOSDCardEventSource::Action, this, &IOSDHostDriver::detachCard));
    
    if (this->detachCardEventSource == nullptr)
    {
        perr("Failed to create the card insertion event source.");
        
        this->attachCardEventSource->release();
        
        this->attachCardEventSource = nullptr;
        
        return false;
    }
    
    // The card removal event source will be enabled by the notification handler
    // @see `IOSDHostDriver::onSDCardRemovedGated()` for details
    this->detachCardEventSource->disable();
    
    pinfo("The card insertion event source has been created.");
    
    // Register with the processor work loop
    this->processorWorkLoop->addEventSource(this->attachCardEventSource);
    
    this->processorWorkLoop->addEventSource(this->detachCardEventSource);
    
    pinfo("Card event sources have been registered with the processor work loop.");
    
    return true;
}

//
// MARK: - Teardown Routines
//

///
/// Tear down the power management
///
void IOSDHostDriver::tearDownPowerManagement()
{
    this->PMstop();
}

///
/// Tear down the array of preallocated DMA commands
///
void IOSDHostDriver::tearDownPreallocatedDMACommands()
{
    for (auto index = 0; index < IOSDHostDriver::kDefaultPoolSize; index += 1)
    {
        OSSafeReleaseNULL(this->pDMACommands[index]);
    }
}

///
/// Tear down the array of preallocated SD block requests
///
void IOSDHostDriver::tearDownPreallocatedBlockRequests()
{
    for (auto index = 0; index < IOSDHostDriver::kDefaultPoolSize; index += 1)
    {
        OSSafeReleaseNULL(this->pSimpleBlockRequests[index]);
        
        OSSafeReleaseNULL(this->pComplexBlockRequests[index]);
    }
}

///
/// Tear down the shared workloop
///
void IOSDHostDriver::tearDownSharedWorkLoop()
{
    OSSafeReleaseNULL(this->sharedWorkLoop);
}

///
/// Tear down the DMA command pool
///
void IOSDHostDriver::tearDownDMACommandPool()
{
    // Commands are released by `IOSDHostDriver::tearDownPreallocatedDMACommands()`
    OSSafeReleaseNULL(this->dmaCommandPool);
}

///
/// Tear down the SD block request pool
///
void IOSDHostDriver::tearDownBlockRequestPool()
{
    // Block requests are released by `IOSDHostDriver::tearDownPreallocatedBlockRequests()`
    OSSafeReleaseNULL(this->simpleBlockRequestPool);
    
    OSSafeReleaseNULL(this->complexBlockRequestPool);
}

///
/// Tear down the SD block request pool
///
void IOSDHostDriver::tearDownBlockRequestQueue()
{
    OSSafeReleaseNULL(this->pendingRequests);
}

///
/// Tear down the processor workloop
///
void IOSDHostDriver::tearDownProcessorWorkLoop()
{
    OSSafeReleaseNULL(this->processorWorkLoop);
}

///
/// Tear down the SD block request event source
///
void IOSDHostDriver::tearDownBlockRequestEventSource()
{
    if (this->queueEventSource != nullptr)
    {
        this->queueEventSource->disable();
        
        this->processorWorkLoop->removeEventSource(this->queueEventSource);
        
        this->queueEventSource->release();
        
        this->queueEventSource = nullptr;
    }
}

///
/// Tear down the card event sources
///
void IOSDHostDriver::tearDownCardEventSources()
{
    if (this->detachCardEventSource != nullptr)
    {
        this->detachCardEventSource->disable();
        
        this->processorWorkLoop->removeEventSource(this->detachCardEventSource);
        
        this->detachCardEventSource->release();
        
        this->detachCardEventSource = nullptr;
    }
    
    if (this->attachCardEventSource != nullptr)
    {
        this->attachCardEventSource->disable();
        
        this->processorWorkLoop->removeEventSource(this->attachCardEventSource);
        
        this->attachCardEventSource->release();
        
        this->attachCardEventSource = nullptr;
    }
}

//
// MARK: - IOService Implementations
//

///
/// Start the host driver
///
/// @param provider An instance of the host device
/// @return `true` on success, `false` otherwise.
///
bool IOSDHostDriver::start(IOService* provider)
{
    pinfo("====================================================================");
    pinfo("Starting the SD host driver with the device at 0x%08x%08x...", KPTR(provider));
    pinfo("====================================================================");
    
    // Start the super class
    if (!super::start(provider))
    {
        perr("Failed to start the super class.");
        
        return false;
    }
    
    // Get the host device
    this->host = OSDynamicCast(IOSDHostDevice, provider);
    
    if (this->host == nullptr)
    {
        perr("The provider is not a valid host device.");
        
        return false;
    }
    
    this->host->retain();
    
    // Setup the power managememt
    if (!this->setupPowerManagement())
    {
        goto error;
    }
    
    // Preallocate DMA commands
    if (!this->setupPreallocatedDMACommands())
    {
        goto error;
    }
    
    // Preallocate SD block requests
    if (!this->setupPreallocatedBlockRequests())
    {
        goto error;
    }
    
    // Setup the shared work loop
    if (!this->setupSharedWorkLoop())
    {
        goto error;
    }
    
    // Create the DMA command pool
    if (!this->setupDMACommandPool())
    {
        goto error;
    }
    
    // Create the block request pool
    if (!this->setupBlockRequestPool())
    {
        goto error;
    }
    
    // Create the request queue
    if (!this->setupBlockRequestQueue())
    {
        goto error;
    }
    
    // Create the processor work loop
    if (!this->setupProcessorWorkLoop())
    {
        goto error;
    }
    
    // Create the block request event source
    if (!this->setupBlockRequestEventSource())
    {
        goto error;
    }
    
    // Create the card insertion and removal event sources
    if (!this->setupCardEventSources())
    {
        goto error;
    }
    
    pinfo("========================================");
    pinfo("The SD host driver started successfully.");
    pinfo("========================================");
    
    return true;
    
error:
    this->tearDownCardEventSources();
    
    this->tearDownBlockRequestEventSource();
    
    this->tearDownProcessorWorkLoop();
    
    this->tearDownBlockRequestQueue();
    
    this->tearDownBlockRequestPool();
    
    this->tearDownDMACommandPool();
    
    this->tearDownSharedWorkLoop();
    
    this->tearDownPreallocatedBlockRequests();
    
    this->tearDownPreallocatedDMACommands();
    
    this->tearDownPowerManagement();
    
    OSSafeReleaseNULL(this->host);
    
    pinfo("===================================");
    perr("Failed to start the SD host driver.");
    pinfo("===================================");
    
    return false;
}

///
/// Stop the host driver
///
/// @param provider An instance of the host device
///
void IOSDHostDriver::stop(IOService* provider)
{
    this->tearDownCardEventSources();
    
    this->tearDownBlockRequestEventSource();
    
    this->tearDownProcessorWorkLoop();
    
    this->tearDownBlockRequestQueue();
    
    this->tearDownBlockRequestPool();
    
    this->tearDownDMACommandPool();
    
    this->tearDownSharedWorkLoop();
    
    this->tearDownPreallocatedBlockRequests();
    
    this->tearDownPreallocatedDMACommands();
    
    this->tearDownPowerManagement();
    
    OSSafeReleaseNULL(this->host);
    
    super::stop(provider);
}
