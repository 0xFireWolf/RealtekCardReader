//
//  IOSDBlockRequestEventSource.cpp
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 6/9/21.
//

#include "IOSDBlockRequestEventSource.hpp"
#include "Debug.hpp"

//
// MARK: - Meta Class Definitions
//

OSDefineMetaClassAndStructors(IOSDBlockRequestEventSource, IOEventSource);

///
/// Check whether a SD block request is pending and if so process the request on the workloop
///
/// @return `true` if the work loop should invoke this function again.
///         i.e. One or more requests are pending after the workloop has processed the current one.
///
bool IOSDBlockRequestEventSource::checkForWork()
{
    // Invoked by the processor work loop
    pinfo("The block request event source is invoked by the processor work loop.");
    
    // Guard: No work if the event source is disabled
    // i.e. The host driver stops processing block requests if the card is removed
    if (!this->enabled)
    {
        pinfo("The event has been disabled. Will not process any pending requests.");
        
        return false;
    }
    
    // Guard: Check whether a block request is pending
    if (this->pendingRequests->isEmpty())
    {
        pinfo("The request queue is empty.");
        
        return false;
    }
    
    // Get a pending request
    IOSDBlockRequest* request = this->pendingRequests->dequeueRequest();
    
    // The processor workloop is the only thread that removes a request from the queue
    // Guard: The pending request should be non-null
    if (request == nullptr)
    {
        perr("Detected an inconsistency: The request should not be NULL at this moment.");
        
        return false;
    }
    
    // Process the request
    pinfo("Processing the block request...");
    
    request->service();
    
    pinfo("The block request has been processed.");
    
    // Notify the host driver that a block request has been processed
    // so that the driver can finalize the request and return it to the pool
    passert(this->action != nullptr, "The completion routine should not be NULL.");
    
    (*this->action)(this->owner, request);
    
    // The request has been processed
    // Guard: Check whether the host driver has stopped processing pending requests
    if (!this->enabled)
    {
        pinfo("The event has been disabled. Will not process any pending requests.");
        
        return false;
    }
    
    // Guard: Check whether one or more requests are pending
    if (this->pendingRequests->isEmpty())
    {
        // Later when the host driver adds a request to the queue,
        // it will invoke `enable()` so the processor workloop will call this function again.
        // At that time, the queue will not be empty, so this event source will process the new request.
        pinfo("The request queue is empty. The event source will wait for the next enable() call.");
        
        return false;
    }
    else
    {
        // The processor workloop will invoke this function again,
        // so this event source can process the next pending request.
        pinfo("The request queue is not empty. Will inform the work loop to invoke this function again.");
        
        return true;
    }
}

///
/// Initialize with the given queue
///
/// @param owner Owner of this instance of an event source; the first parameter of the action routine
/// @param action A non-null action that is invoked when a block request has been processed
/// @param queue A list of pending requests
/// @return `true` on success, `false` otherwise.
///
bool IOSDBlockRequestEventSource::initWithQueue(OSObject* owner, Action action, IOSDBlockRequestQueue* queue)
{
    if (action == nullptr)
    {
        perr("The action routine cannot be NULL.");
        
        return false;
    }
    
    if (!super::init(owner, reinterpret_cast<IOEventSource::Action>(action)))
    {
        return false;
    }
    
    this->pendingRequests = queue;
    
    this->pendingRequests->retain();
    
    return true;
}

///
/// Release this event source
///
void IOSDBlockRequestEventSource::free()
{
    OSSafeReleaseNULL(this->pendingRequests);
    
    super::free();
}

///
/// Create a block request event source with the given queue
///
/// @param owner Owner of this instance of an event source; the first parameter of the action routine
/// @param action A non-null action that is invoked when a block request has been processed
/// @return A non-null event source on success, `nullptr` otherwise.
///
IOSDBlockRequestEventSource* IOSDBlockRequestEventSource::createWithQueue(OSObject* owner, Action action, IOSDBlockRequestQueue* queue)
{
    auto instance = OSTypeAlloc(IOSDBlockRequestEventSource);
    
    if (instance == nullptr)
    {
        return nullptr;
    }
    
    if (!instance->initWithQueue(owner, action, queue))
    {
        instance->release();
        
        return nullptr;
    }
    
    return instance;
}
