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
    pinfo("The card insertion event has been processed. Result = 0x%08x.", status);
    
    // Publish the card characteristics
    if (status == kIOReturnSuccess && characteristics != nullptr)
    {
        this->setProperty(kIOSDCardCharacteristics, characteristics);
        
        pinfo("The card characteristics have been published.");
    }
    
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
    pinfo("The card removal event has been processed. Result = 0x%08x.", status);
    
    // Remove the card characteristics
    this->removeProperty(kIOSDCardCharacteristics);
    
    // Invoke the completion routine supplied by the controller
    IOSDCard::complete(reinterpret_cast<IOSDCard::Completion*>(parameter), status, characteristics);
}

///
/// [UPCALL] Notify the host device when a SD card is inserted
///
/// @param completion A nullable completion routine to be invoked when the card is attached
/// @param options An optional value passed to the host driver
/// @note This callback function runs in a gated context provided by the underlying card reader controller.
///       The host device should implement this function without any blocking operations.
///       A default implementation that notifies the host driver is provided.
///
void IOSDHostDevice::onSDCardInsertedGated(IOSDCard::Completion* completion, IOSDCard::EventOptions options)
{
    auto chainedCompletion = IOSDCard::Completion::withMemberFunction(this, &IOSDHostDevice::onSDCardInsertedCompletion, completion);
    
    passert(this->driver != nullptr, "The host driver should not be null.");
    
    this->driver->onSDCardInsertedGated(&chainedCompletion, options);
    
    this->setProperty(kIOSDCardPresent, true);
}

///
/// [UPCALL] Notify the host device when a SD card is removed
///
/// @param completion A nullable completion routine to be invoked when the card is attached
/// @param options An optional value passed to the host driver
/// @note This callback function runs in a gated context provided by the underlying card reader controller.
///       The host device should implement this function without any blocking operations.
///       A default implementation that notifies the host driver is provided.
///
void IOSDHostDevice::onSDCardRemovedGated(IOSDCard::Completion* completion, IOSDCard::EventOptions options)
{
    auto chainedCompletion = IOSDCard::Completion::withMemberFunction(this, &IOSDHostDevice::onSDCardRemovedCompletion, completion);
    
    passert(this->driver != nullptr, "The host driver should not be null.");
    
    this->driver->onSDCardRemovedGated(&chainedCompletion, options);
    
    this->setProperty(kIOSDCardPresent, false);
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
