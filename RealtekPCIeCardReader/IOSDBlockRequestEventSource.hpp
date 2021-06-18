//
//  IOSDBlockRequestEventSource.hpp
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 6/9/21.
//

#ifndef IOSDBlockRequestEventSource_hpp
#define IOSDBlockRequestEventSource_hpp

#include <IOKit/IOEventSource.h>
#include "IOSDBlockRequestQueue.hpp"

/// An event source to signal the workloop to process a pending block request
class IOSDBlockRequestEventSource: public IOEventSource
{
    /// Constructors & Destructors
    OSDeclareDefaultStructors(IOSDBlockRequestEventSource);
    
    using super = IOEventSource;
    
public:
    /// The type of the action routine that is invoked when a request has been processed
    using Action = void (*)(OSObject*, IOSDBlockRequest*);
    
private:
    /// A list of pending requests
    IOSDBlockRequestQueue* pendingRequests;
    
    ///
    /// Check whether a SD block request is pending and if so process the request on the workloop
    ///
    /// @return `true` if the work loop should invoke this function again.
    ///         i.e. One or more requests are pending after the workloop has processed the current one.
    ///
    bool checkForWork() override;
    
    ///
    /// Initialize with the given queue
    ///
    /// @param owner Owner of this instance of an event source; the first parameter of the action routine
    /// @param action A non-null action that is invoked when a block request has been processed
    /// @param queue A list of pending requests
    /// @return `true` on success, `false` otherwise.
    ///
    bool initWithQueue(OSObject* owner, Action action, IOSDBlockRequestQueue* queue);
    
    ///
    /// Release this event source
    ///
    void free() override;
    
public:
    ///
    /// Create a block request event source with the given queue
    ///
    /// @param owner Owner of this instance of an event source; the first parameter of the action routine
    /// @param action A non-null action that is invoked when a block request has been processed
    /// @return A non-null event source on success, `nullptr` otherwise.
    ///
    static IOSDBlockRequestEventSource* createWithQueue(OSObject* owner, Action action, IOSDBlockRequestQueue* queue);
};

#endif /* IOSDBlockRequestEventSource_hpp */
