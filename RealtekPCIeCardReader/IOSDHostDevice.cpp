//
//  IOSDHostDevice.cpp
//  RealtekPCIeCardReader
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
// MARK: - Card Events Callbacks
//

///
/// [UPCALL] Notify the host device when a SD card is inserted
///
/// @note This callback function runs in a gated context provided by the underlying card reader controller.
///       The host device should implement this function without any blocking operations.
///       A default implementation that notifies the host driver is provided.
///
void IOSDHostDevice::onSDCardInsertedGated()
{
    this->driver->onSDCardInsertedGated();
    
    this->setProperty("Card Present", true);
}

///
/// [UPCALL] Notify the host device when a SD card is removed
///
/// @note This callback function runs in a gated context provided by the underlying card reader controller.
///       The host device should implement this function without any blocking operations.
///       A default implementation that notifies the host driver is provided.
///
void IOSDHostDevice::onSDCardRemovedGated()
{
    this->driver->onSDCardRemovedGated();
    
    this->setProperty("Card Present", false);
    
    this->removeProperty("Card Characteristics");
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
    
    this->setProperty("Card Present", false);
    
    return true;
}
