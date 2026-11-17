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
    
    // Take the completion and the options under the lock so that the thread that raises the event
    // can neither replace them nor reset them while the action is running.
    // The completion is one-shot: A stale completion (that points to the stack of a caller
    // that has long returned) must never be invoked by a later event.
    IOSDCard::Completion completion;
    
    completion.reset();
    
    IOSDCard::EventOptions options;
    
    IOLockLock(this->lock);
    
    if (!this->enabled)
    {
        IOLockUnlock(this->lock);
        
        pinfo("The event source is disabled.");
        
        return false;
    }
    
    // Acknowledge the card event
    super::disable();
    
    completion = this->completion;
    
    options = this->options;
    
    this->completion.reset();
    
    IOLockUnlock(this->lock);
    
    pinfo("Processing the card event...");
    
    (*reinterpret_cast<Action>(this->action))(this->owner, &completion, options);
    
    pinfo("The card event has been processed.");
    
    return false;
}

///
/// Enable the event source
///
/// @param completion A nullable completion routine to be invoked when the card event has been processed
/// @param options An optional value passed to the host driver
/// @note The completion and the options are stored *before* the event source is signaled.
///       Otherwise the processor work loop may run the action with a missing completion
///       and the synchronous caller would wait forever.
///
void IOSDCardEventSource::enable(IOSDCard::Completion* completion, IOSDCard::EventOptions options)
{
    // A completion that is still waiting and gets replaced by this event (see `abortDroppedCompletion()`)
    IOSDCard::Completion dropped;
    
    dropped.reset();
    
    IOLockLock(this->lock);
    
    if (completion != nullptr)
    {
        if (this->completion.isSet())
        {
            dropped = this->completion;
        }
        
        this->completion = *completion;
    }
    
    this->options = options;
    
    // Mark the source as enabled while holding the lock; this signals the work loop
    super::enable();
    
    IOLockUnlock(this->lock);
    
    this->abortDroppedCompletion(dropped);
}

///
/// Disable the event source
///
/// @note A completion that is waiting to be processed is completed with `kIOReturnAborted` (see `abortDroppedCompletion()`);
///       a completion that is being processed is not affected.
///
void IOSDCardEventSource::disable()
{
    IOSDCard::Completion dropped;
    
    dropped.reset();
    
    IOLockLock(this->lock);
    
    super::disable();
    
    dropped = this->completion;
    
    this->completion.reset();
    
    IOLockUnlock(this->lock);
    
    this->abortDroppedCompletion(dropped);
}

///
/// [Helper] Complete a completion that will never be processed
///
/// @param completion The completion taken out of the event source (may be unset)
/// @note v0.9.7 discarded it. If it belonged to a synchronous waiter (the sleep/wake path), the waiter ran into its
///       timeout and its context was never freed. Completing it with `kIOReturnAborted` ends the wait at once and
///       releases the context; the completion is called exactly once because it has already been removed from the source.
///       Called without the lock held, since the completion may take the controller's gate.
///
void IOSDCardEventSource::abortDroppedCompletion(IOSDCard::Completion& completion)
{
    if (!completion.isSet())
    {
        return;
    }
    
    pinfo("A pending card event has been dropped. Its completion is invoked with kIOReturnAborted.");
    
    completion.invoke(kIOReturnAborted, nullptr);
}

///
/// Release the lock
///
void IOSDCardEventSource::free()
{
    if (this->lock != nullptr)
    {
        IOLockFree(this->lock);
        
        this->lock = nullptr;
    }
    
    super::free();
}

///
/// Create a card event source with the given action
///
/// @param owner Owner of the returned instance; the first parameter of the action routine
/// @param action A non-null action to process the card insertion or removal event
/// @return A non-null event source on success, `nullptr` otherwise.
///
IOSDCardEventSource* IOSDCardEventSource::createWithAction(OSObject* owner, Action action)
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
    
    if (!instance->init(owner, reinterpret_cast<IOEventSource::Action>(action)))
    {
        instance->release();
        
        return nullptr;
    }
    
    instance->lock = IOLockAlloc();
    
    if (instance->lock == nullptr)
    {
        instance->release();
        
        return nullptr;
    }
    
    return instance;
}
