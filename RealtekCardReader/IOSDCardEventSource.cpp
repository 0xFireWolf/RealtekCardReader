//
//  IOSDCardEventSource.cpp
//  RealtekCardReader
//
//  Created by FireWolf on 6/9/21.
//

#include "IOSDCardEventSource.hpp"

#include "Debug.hpp"

//
// MARK: - Meta Class Definitions
//

OSDefineMetaClassAndStructors(IOSDCardEventSource, IOEventSource);

///
/// Check whether an action needs to perform to handle the card event
///
/// @return `false` always. The owner must explictly invoke `IOSDCardEventSource::enable()` to trigger another action.
///
bool IOSDCardEventSource::checkForWork()
{
    pinfo("The card event source is invoked by the processor work loop.");
    
    if (this->action == nullptr)
    {
        perr("Detected inconsistency: The action routine should never be null.");
        
        return false;
    }
    
    if (!this->enabled)
    {
        pinfo("The event source is disabled.");
        
        return false;
    }
    
    // Acknowledge the card event
    this->disable();
    
    pinfo("Processing the card event...");
    
    (*this->action)(this->owner);
    
    pinfo("The card event has been processed.");
    
    return false;
}

///
/// Create a card event source with the given action
///
/// @param owner Owner of the returned instance; the first parameter of the action routine
/// @param action A non-null action to process the card insertion or removal event
/// @return A non-null event source on success, `nullptr` otherwise.
///
IOSDCardEventSource* IOSDCardEventSource::createWithAction(OSObject* owner, IOEventSource::Action action)
{
    if (action == nullptr)
    {
        perr("The action routine must be non-null.");
        
        return nullptr;
    }
    
    auto instance = OSTypeAlloc(IOSDCardEventSource);
    
    if (instance == nullptr)
    {
        return nullptr;
    }
    
    if (!instance->init(owner, action))
    {
        instance->release();
        
        return nullptr;
    }
    
    return instance;
}
