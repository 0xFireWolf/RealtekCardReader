//
//  IOSDHostDevice.cpp
//  RealtekCardReader
//
//  Created by FireWolf on 6/5/21.
//

#include "IOSDHostDevice.hpp"
#include "IOSDHostDriver.hpp"

//
// MARK: - Meta Class Definitions
//

OSDefineMetaClassAndAbstractStructors(IOSDHostDevice, IOService);

//
// MARK: - SD Request Processors
//

///
/// Preprocess the given SD command request
///
/// @param request A SD command request
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `sdmmc_pre_req()` defined in `rtsx_pci_sdmmc.c`.
///
IOReturn IOSDHostDevice::preprocessRequest(IOSDHostRequest& request)
{
    return kIOReturnSuccess;
}

///
/// Postprocess the given SD command request
///
/// @param request A SD command request
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `sdmmc_post_req()` defined in `rtsx_pci_sdmmc.c`.
///
IOReturn IOSDHostDevice::postprocessRequest(IOSDHostRequest& request)
{
    return kIOReturnSuccess;
}

//
// MARK: - Card Events Callbacks
//

///
/// [Completion] Notify the host device when the host driver has processed a card insertion event
///
/// @param parameter An opaque client-supplied parameter pointer
/// @param status `kIOReturnSuccess` if the card event has been processed without errors, other values otherwise.
/// @param characteristics A non-null dictionary that contains characteristics of the card inserted and initialized successfully,
///                        `nullptr` if the card inserted by users cannot be initialized or has been removed from the card slot.
///
void IOSDHostDevice::onSDCardInsertedCompletion(void* parameter, IOReturn status, OSDictionary* characteristics)
{
    // Publish the card characteristics
    this->setProperty(kIOSDCardCharacteristics, characteristics);
    
    // Invoke the completion routine supplied by the controller
    IOSDCard::complete(reinterpret_cast<IOSDCard::Completion*>(parameter), status, characteristics);
}

///
/// [Completion] Notify the host device when the host driver has processed a card removal event
///
/// @param parameter An opaque client-supplied parameter pointer
/// @param status `kIOReturnSuccess` if the card event has been processed without errors, other values otherwise.
/// @param characteristics A non-null dictionary that contains characteristics of the card inserted and initialized successfully,
///                        `nullptr` if the card inserted by users cannot be initialized or has been removed from the card slot.
///
void IOSDHostDevice::onSDCardRemovedCompletion(void* parameter, IOReturn status, OSDictionary* characteristics)
{
    // Remove the card characteristics
    this->removeProperty(kIOSDCardCharacteristics);
    
    // Invoke the completion routine supplied by the controller
    IOSDCard::complete(reinterpret_cast<IOSDCard::Completion*>(parameter), status, characteristics);
}

///
/// [UPCALL] Notify the host device when a SD card is inserted
///
/// @param completion A nullable completion routine to be invoked when the card is attached
/// @note This callback function runs in a gated context provided by the underlying card reader controller.
///       The host device should implement this function without any blocking operations.
///       A default implementation that notifies the host driver is provided.
///
void IOSDHostDevice::onSDCardInsertedGated(IOSDCard::Completion* completion)
{
    this->cardInsertionCompletion = IOSDCard::Completion::withMemberFunction(this, &IOSDHostDevice::onSDCardInsertedCompletion, completion);
    
    this->driver->onSDCardInsertedGated(&this->cardInsertionCompletion);
    
    this->setProperty(kIOSDCardPresent, true);
}

///
/// [UPCALL] Notify the host device when a SD card is removed
///
/// @param completion A nullable completion routine to be invoked when the card is attached
/// @note This callback function runs in a gated context provided by the underlying card reader controller.
///       The host device should implement this function without any blocking operations.
///       A default implementation that notifies the host driver is provided.
///
void IOSDHostDevice::onSDCardRemovedGated(IOSDCard::Completion* completion)
{
    this->cardRemovalCompletion = IOSDCard::Completion::withMemberFunction(this, &IOSDHostDevice::onSDCardRemovedCompletion, completion);
    
    this->driver->onSDCardRemovedGated(&this->cardRemovalCompletion);
    
    this->setProperty(kIOSDCardPresent, false);
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
IOReturn IOSDHostDevice::setPowerState(unsigned long powerStateOrdinal, IOService* whatDevice)
{
    return kIOPMAckImplied;
}

//
// MARK: - IOService Implementation
//

///
/// Initialize the host device
///
/// @param dictionary A nullable matching dictionary
/// @return `true` on success, `false` otherwise.
///
bool IOSDHostDevice::init(OSDictionary* dictionary)
{
    if (!super::init(dictionary))
    {
        return false;
    }
    
    this->setProperty(kIOSDCardPresent, false);
    
    return true;
}

///
/// Start the host device
///
/// @param provider An instance of card reader controller
/// @return `true` on success, `false` otherwise.
///
bool IOSDHostDevice::start(IOService* provider)
{
    pinfo("Starting the host device...");
    
    if (!super::start(provider))
    {
        return false;
    }
    
    pinfo("Setting up the power management...");
    
    this->PMinit();
    
    provider->joinPMtree(this);
    
    static IOPMPowerState kPowerStates[] =
    {
        { kIOPMPowerStateVersion1, kIOPMPowerOff, kIOPMPowerOff, kIOPMPowerOff, 0, 0, 0, 0, 0, 0, 0, 0 },
        { kIOPMPowerStateVersion1, kIOPMPowerOn | kIOPMDeviceUsable | kIOPMInitialDeviceState, kIOPMPowerOn, kIOPMPowerOn, 0, 0, 0, 0, 0, 0, 0, 0 },
    };
    
    return this->registerPowerDriver(this, kPowerStates, arrsize(kPowerStates)) == kIOPMNoErr;
}

///
/// Stop the host device
///
/// @param provider An instance of card reader controller
///
void IOSDHostDevice::stop(IOService* provider)
{
    this->PMstop();
    
    super::stop(provider);
}
