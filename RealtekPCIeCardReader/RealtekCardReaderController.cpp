//
//  RealtekCardReaderController.cpp
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 7/10/21.
//

#include "RealtekCardReaderController.hpp"
#include "RealtekCommonRegisters.hpp"
#include "Utilities.hpp"

//
// MARK: - Meta Class Definitions
//

OSDefineMetaClassAndAbstractStructors(RealtekCardReaderController, AppleSDXC);

//
// MARK: - Access Chip Registers
//

///
/// Dump the chip registers in the given range
///
/// @param range The range of register addresses
///
void RealtekCardReaderController::dumpChipRegisters(ClosedRange<UInt16> range)
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
IOReturn RealtekCardReaderController::readHostBuffer(IOByteCount offset, void* buffer, IOByteCount length)
{
    // Guard: The given buffer must be non-null
    if (buffer == nullptr)
    {
        perr("The given buffer is NULL.");
        
        return kIOReturnBadArgument;
    }
    
    // Guard: The number of bytes to read must not exceed the host buffer length
    if (offset + length > this->hostBufferDescriptor->getLength())
    {
        perr("Detected an out-of-bounds read: Request to read %llu bytes at offset %llu.", length, offset);
        
        return kIOReturnBadArgument;
    }
    
    // The read routine will run in a gated context
    auto action = [](OSObject* controller, void* offset, void* buffer, void* length, void*) -> IOReturn
    {
        // Retrieve the controller instance
        auto instance = OSDynamicCast(RealtekCardReaderController, controller);
        
        passert(instance != nullptr, "The controller instance is invalid.");
        
        return instance->readHostBufferGated(*reinterpret_cast<IOByteCount*>(offset), buffer, *reinterpret_cast<IOByteCount*>(length));
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
IOReturn RealtekCardReaderController::writeHostBuffer(IOByteCount offset, const void* buffer, IOByteCount length)
{
    // Guard: The given buffer must be non-null
    if (buffer == nullptr)
    {
        perr("The given buffer is NULL.");
        
        return kIOReturnBadArgument;
    }
    
    // Guard: The number of bytes to read must not exceed the host buffer length
    if (offset + length > this->hostBufferDescriptor->getLength())
    {
        perr("Detected an out-of-bounds write: Request to write %llu bytes at offset %llu.", length, offset);
        
        return kIOReturnBadArgument;
    }
    
    // The write routine will run in a gated context
    auto action = [](OSObject* controller, void* offset, void* buffer, void* length, void*) -> IOReturn
    {
        // Retrieve the controller instance
        auto instance = OSDynamicCast(RealtekCardReaderController, controller);
        
        passert(instance != nullptr, "The controller instance is invalid.");
        
        return instance->writeHostBufferGated(*reinterpret_cast<IOByteCount*>(offset), buffer, *reinterpret_cast<IOByteCount*>(length));
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
void RealtekCardReaderController::dumpHostBuffer(IOByteCount offset, IOByteCount length, IOByteCount column)
{
    UInt8* buffer = reinterpret_cast<UInt8*>(IOMalloc(length));
    
    this->readHostBuffer(offset, buffer, length);
    
    pbuf(buffer, length, column);
    
    IOFree(buffer, length);
}

//
// MARK: - Host Command Management (Core)
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
IOReturn RealtekCardReaderController::beginCommandTransfer()
{
    // The reset counter routine will run in a gated context
    auto action = [](OSObject* controller, void*, void*, void*, void*) -> IOReturn
    {
        // Retrieve the controller instance
        auto instance = OSDynamicCast(RealtekCardReaderController, controller);
        
        passert(instance != nullptr, "The controller instance is invalid.");
        
        // Reset the counter
        instance->hostCommandCounter.reset();
        
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
/// @note Port: This function replaces `rtsx_pci/usb_add_cmd()` defined in `rtsx_pcr/usb.c`.
///
IOReturn RealtekCardReaderController::enqueueCommand(const Command& command)
{
    // The enqueue routine will run in a gated context
    auto action = [](OSObject* controller, void* command, void*, void*, void*) -> IOReturn
    {
        // Retrieve the controller instance
        auto instance = OSDynamicCast(RealtekCardReaderController, controller);
        
        passert(instance != nullptr, "The controller instance is invalid.");
        
        return instance->enqueueCommandGated(*reinterpret_cast<Command*>(command));
    };
    
    return this->commandGate->runAction(action, const_cast<void*>(reinterpret_cast<const void*>(&command)));
}

///
/// Finish the existing host command transfer session
///
/// @param timeout Specify the amount of time in milliseconds
/// @param flags An optional flag, 0 by default
/// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
/// @note Port: This function replaces `rtsx_pci/usb_send_cmd()` defined in `rtsx_pcr/usb.c`.
/// @note This function sends all commands in the queue to the device.
///
IOReturn RealtekCardReaderController::endCommandTransfer(UInt32 timeout, UInt32 flags)
{
    // The transfer routine will run in a gated context
    auto action = [](OSObject* controller, void* timeout, void* flags, void*, void*) -> IOReturn
    {
        // Retrieve the controller instance
        auto instance = OSDynamicCast(RealtekCardReaderController, controller);
        
        passert(instance != nullptr, "The controller instance is invalid.");
        
        return instance->endCommandTransferGated(*reinterpret_cast<UInt32*>(timeout), *reinterpret_cast<UInt32*>(flags));
    };
    
    IOReturn retVal = this->commandGate->runAction(action, &timeout, &flags);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to transfer host commands. Error = 0x%x.", retVal);
        
        this->clearError();
    }
    
    pinfo("Finished a command transfer session. Returns 0x%x.", retVal);
    
    return retVal;
}

//
// MARK: - Host Command Management (Convenient)
//

///
/// Enqueue a sequence of commands to read registers conveniently
///
/// @param pairs A sequence of pairs of register address and value
/// @return `kIOReturnSuccess` on success, `kIOReturnError` otherwise.
/// @note This function provides an elegant way to enqueue multiple commands and handle errors.
///
IOReturn RealtekCardReaderController::enqueueReadRegisterCommands(const ChipRegValuePairs& pairs)
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
/// @note This function provides an elegant way to enqueue multiple commands and handle errors.
///
IOReturn RealtekCardReaderController::enqueueWriteRegisterCommands(const ChipRegValuePairs& pairs)
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
/// @param flags An optional flag, 0 by default
/// @return `kIOReturnSuccess` on success, `kIOReturnError` otherwise.
/// @note This function provides an elegant way to start a command transfer session and handle errors.
///       Same as calling `startCommandTransfer`, a sequence of `enqueueReadRegisterCommand` and `endCommandTransfer`.
///
IOReturn RealtekCardReaderController::transferReadRegisterCommands(const ChipRegValuePairs& pairs, UInt32 timeout, UInt32 flags)
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
    return this->endCommandTransfer(timeout, flags);
}

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

IOReturn RealtekCardReaderController::transferWriteRegisterCommands(const ChipRegValuePairs& pairs, UInt32 timeout, UInt32 flags)
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
    return this->endCommandTransfer(timeout, flags);
}

///
/// Launch a custom command transfer session conveniently
///
/// @param action A non-null action that enqueues any host commands
/// @param timeout Specify the amount of time in milliseconds
/// @param flags An optional flag, 0 by default
/// @param context An optional user-defined context that will be passed to the given action
/// @return `kIOReturnSuccess` on success, `kIOReturnBadArgument` if the given action is null,
///         `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
/// @note This function provides an elegant way to start a command transfer session and handle errors.
///       Same as calling `startCommandTransfer`, a sequence of enqueue invocations and `endCommandTransfer`.
///
IOReturn RealtekCardReaderController::withCustomCommandTransfer(EnqueueAction action, UInt32 timeout, UInt32 flags, const void* context)
{
    IOReturn retVal;
    
    // Guard: The action cannot be null
    if (action == nullptr)
    {
        perr("The given action is null.");
        
        return kIOReturnBadArgument;
    }
    
    // Guard: Begin command transfer
    retVal = this->beginCommandTransfer();
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to initiate a new command transfer session. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // Guard: Enqueue commands
    retVal = (*action)(context);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to enqueue the given sequence of commands. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // Guard: Transfer commands
    return this->endCommandTransfer(timeout, flags);
}

//
// MARK: - Clear Error
//

///
/// Clear any transfere error on the both sides
///
/// @note This function invokes `clearCardError()` and `clearHostError()`.
/// @note Port: This function replaces `sd_clear_error()` defined in `rtsx_pci/usb_sdmmc.c`.
///
void RealtekCardReaderController::clearError()
{
    this->clearCardError();
    
    this->clearHostError();
}

//
// MARK: - Card Clock Configurations
//

///
/// Get the card clock and the divider for the initial mode
///
/// @return A pair of card clock in Hz (first) and the clock divider register value (second).
/// @note This function uses a 30 MHz carc clock and a divider of 128 as the default values.
///       RTS5261 controller must override this function to set a higher initial card clock
///       and divider for chips whose revision is higher than C.
///
Pair<UInt32, UInt32> RealtekCardReaderController::getInitialCardClockAndDivider()
{
    using namespace RTSX::COM::Chip;
    
    return { MHz2Hz(30), SD::CFG1::kClockDivider128 };
}

///
/// Adjust the card clock if DMA transfer errors occurred
///
/// @param cardClock The current card clock in Hz
/// @return The adjusted card clock.
/// @note Port: This function replaces the code block that reduces the card clock in `rtsx_pci_switch_clock()` defined in `rtsx_psr.c`.
///             By default, this function does not adjust the card clock and return the given clock.
///             RTS5227 controller must override this function.
///
UInt32 RealtekCardReaderController::getAdjustedCardClockOnDMAError(UInt32 cardClock)
{
    return cardClock;
}

///
/// Convert the given SSC clock to the divider N
///
/// @param clock The SSC clock in MHz
/// @return The divider N.
/// @note Port: This function replaces `conv_clk_and_div_n()` defined in `rtsx_psr.c` and the code block in `rtsx_usb_switch_clock()` defined in `rtsx_usb.c`.
///             RTL8411 series controllers must override this function.
///
UInt32 RealtekCardReaderController::sscClock2DividerN(UInt32 clock)
{
    return clock - 2;
}

///
/// Convert the given divider N back to the SSC clock
///
/// @param n The divider N
/// @return The SSC clock in MHz.
/// @note Port: This function replaces `conv_clk_and_div_n()` defined in `rtsx_psr.c` and the code block in `rtsx_usb_switch_clock()` defined in `rtsx_usb.c`.
///             RTL8411 series controllers must override this function.
///
UInt32 RealtekCardReaderController::dividerN2SSCClock(UInt32 n)
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
/// @note Port: This function replaces `rtsx_pci/usb_switch_clock()` defined in `rtsx_psr/usb.c`.
///
IOReturn RealtekCardReaderController::switchCardClock(UInt32 cardClock, SSCDepth sscDepth, bool initialMode, bool doubleClock, bool vpclock)
{
    using namespace RTSX::COM::Chip;
 
    pinfo("Switching the clock to %u Hz with SSC depth = %u, Initial Mode = %s, Double Clock = %s, Use VPClock = %s.",
          cardClock, sscDepth, YESNO(initialMode), YESNO(doubleClock), YESNO(vpclock));
    
    this->sscClockLimits.print();
    
    // Adjust the clock divider and the frequency if the card is at its initial stage
    UInt8 clockDivider = SD::CFG1::kClockDivider0;
    
    if (initialMode)
    {
        Pair<UInt32, UInt32> pair = this->getInitialCardClockAndDivider();
        
        cardClock = pair.first;
        
        clockDivider = pair.second;
        
        pinfo("Use the initial card clock %u Hz and clock divider %u.", cardClock, clockDivider);
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
    pinfo("Requested SSC Clock = %d MHz; Current SSC Clock = %d MHz.", sscClock, this->currentSSCClock);
    
    if (sscClock == this->currentSSCClock)
    {
        pinfo("No need to switch the clock.");
        
        return kIOReturnSuccess;
    }
    
    // Calculate the SSC clock divider N
    UInt32 n = this->sscClock2DividerN(sscClock);
    
    if (sscClock <= this->sscClockLimits.minFrequencyMHz)
    {
        perr("The SSC clock frequency %u MHz is too low.", sscClock);
        
        return kIOReturnInvalid;
    }
    
    if (n > this->sscClockLimits.rangeN.upperBound)
    {
        perr("The SSC clock N = %d derived from the clock %d MHz is too large.", n, sscClock);
        
        return kIOReturnInvalid;
    }
    
    // Calculate the MCU count
    UInt32 mcus = this->sscClock2MCUCount(sscClock);
    
    pinfo("MCU Count = %u.", mcus);
    
    // Ensure that the SSC clock divider N is not too small
    UInt8 divider;
    
    for (divider = this->sscClockLimits.rangeDivider.lowerBound;
         divider < this->sscClockLimits.rangeDivider.upperBound;
         divider += 1)
    {
        if (n >= this->sscClockLimits.rangeN.lowerBound)
        {
            break;
        }
        
        n = this->sscClock2DividerN(this->dividerN2SSCClock(n) * 2);
    }
    
    pinfo("SSC clock N = %d; Divider register value = %d.", n, divider);
    
    // Retrieve the register value that corresponds to the requested SSC depth
    UInt8 sscDepthRegValue = this->sscDepth2RegValue(sscDepth);
    
    pinfo("SSC depth register value = %u.", sscDepthRegValue);
    
    // Double the SSC depth if necessary
    if (doubleClock && sscDepthRegValue > 1)
    {
        sscDepthRegValue -= 1;
        
        pinfo("SSC depth has been doubled. Register value = %u.", sscDepthRegValue);
    }
    
    // Revise the SSC depth
    sscDepthRegValue = this->reviseSSCDepthRegValue(sscDepthRegValue, divider);
    
    pinfo("Revised SSC depth register value = %u.", sscDepthRegValue);
    
    // Switch the SSC clock
    retVal = this->switchCardClock(sscDepthRegValue, static_cast<UInt8>(n), divider, mcus, vpclock);
    
    if (retVal == kIOReturnSuccess)
    {
        this->currentSSCClock = sscClock;
        
        pinfo("The SSC clock has been switched to %u Hz.", sscClock);
    }
    else
    {
        perr("Failed to switch the SSC clock. Error = 0x%x.", retVal);
    }
    
    return retVal;
}

//
// MARK: - Power Management
//

///
/// Adjust the power state in response to system-wide power events
///
/// @param powerStateOrdinal The number in the power state array of the state the driver is being instructed to switch to
/// @param whatDevice A pointer to the power management object which registered to manage power for this device
/// @return `kIOPMAckImplied` always.
///
IOReturn RealtekCardReaderController::setPowerState(unsigned long powerStateOrdinal, IOService* whatDevice)
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
/// @param provider The provider
/// @return `true` on success, `false` otherwise.
///
bool RealtekCardReaderController::setupPowerManagement(IOService* provider)
{
    static IOPMPowerState kPowerStates[] =
    {
        { kIOPMPowerStateVersion1, kIOPMPowerOff, kIOPMPowerOff, kIOPMPowerOff, 0, 0, 0, 0, 0, 0, 0, 0 },
        { kIOPMPowerStateVersion1, kIOPMPowerOn | kIOPMDeviceUsable | kIOPMInitialDeviceState, kIOPMPowerOn, kIOPMPowerOn, 0, 0, 0, 0, 0, 0, 0, 0 },
    };
    
    this->PMinit();
    
    provider->joinPMtree(this);
    
    return this->registerPowerDriver(this, kPowerStates, arrsize(kPowerStates)) == kIOPMNoErr;
}

///
/// Setup the workloop
///
/// @return `true` on success, `false` otherwise.
///
bool RealtekCardReaderController::setupWorkLoop()
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

//
// MARK: - Teardown Routines
//

///
/// Tear down the power management
///
void RealtekCardReaderController::tearDownPowerManagement()
{
    this->PMstop();
}

///
/// Tear down the workloop
///
void RealtekCardReaderController::tearDownWorkLoop()
{
    if (this->commandGate != nullptr)
    {
        this->commandGate->disable();
        
        this->workLoop->removeEventSource(this->commandGate);
        
        this->commandGate->release();
        
        this->commandGate = nullptr;
    }
    
    OSSafeReleaseNULL(this->workLoop);
}

//
// MARK: - IOService Implementations
//

///
/// Initialize the controller
///
/// @return `true` on success, `false` otherwise.
///
bool RealtekCardReaderController::init(OSDictionary* dictionary)
{
    if (!super::init(dictionary))
    {
        perr("Failed to initialize the super class.");
        
        return false;
    }
    
    this->slot = nullptr;
    
    this->workLoop = nullptr;
    
    this->commandGate = nullptr;
    
    this->hostBufferDescriptor = nullptr;
    
    this->hostCommandCounter.reset();
    
    this->sscClockLimits.reset();
    
    this->currentSSCClock = 0;
    
    return true;
}

///
/// Start the controller
///
/// @param provider The provider
/// @return `true` on success, `false` otherwise.
///
bool RealtekCardReaderController::start(IOService* provider)
{
    pinfo("=======================================================");
    pinfo("Starting the Realtek PCIe/USB card reader controller...");
    pinfo("=======================================================");
    
    // Start the super class
    if (!super::start(provider))
    {
        perr("Failed to start the super class.");
        
        return false;
    }
    
    // Setup the power management
    if (!this->setupPowerManagement(provider))
    {
        perr("Failed to setup the power management.");
        
        return false;
    }
    
    // Set up the work loop
    if (!this->setupWorkLoop())
    {
        perr("Failed to set up the work loop.");
        
        this->tearDownPowerManagement();
        
        return false;
    }
    
    pinfo("=======================================================");
    pinfo("The base card reader controller started successfully...");
    pinfo("=======================================================");
    
    return true;
}

///
/// Stop the controller
///
/// @param provider The provider
///
void RealtekCardReaderController::stop(IOService* provider)
{
    this->tearDownWorkLoop();
    
    this->tearDownPowerManagement();
    
    super::stop(provider);
}
