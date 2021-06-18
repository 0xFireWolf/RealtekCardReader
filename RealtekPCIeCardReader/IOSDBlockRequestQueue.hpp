//
//  IOSDBlockRequestQueue.hpp
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 6/9/21.
//

#ifndef IOSDBlockRequestQueue_hpp
#define IOSDBlockRequestQueue_hpp

#include <IOKit/IOCommandPool.h>
#include "IOSDBlockRequest.hpp"

/// Represents a thread-safe queue of SD block requests
class IOSDBlockRequestQueue: public IOCommandPool
{
    /// Constructors & Destructors
    OSDeclareDefaultStructors(IOSDBlockRequestQueue);
    
    using super = IOCommand;
    
public:
    ///
    /// Create an empty queue protected by the given work loop
    ///
    /// @param workLoop The workloop that the queue should synchronize with
    /// @return A non-null queue on success, `nullptr` otherwise.
    ///
    static IOSDBlockRequestQueue* create(IOWorkLoop* workLoop);
    
    ///
    /// Check whether the queue is empty
    ///
    /// @return `true` if the queue is empty, `false` otherwise.
    ///
    bool isEmpty();
    
    ///
    /// Enqueue the given block request
    ///
    /// @param request A block request
    ///
    inline void enqueueRequest(IOSDBlockRequest* request)
    {
        this->returnCommand(request);
    }
    
    ///
    /// Dequeue a block request
    ///
    /// @return A non-null block request.
    /// @warning The calling thread will not be blocked waiting for a request.
    ///
    inline IOSDBlockRequest* dequeueRequest()
    {
        return OSRequiredCast(IOSDBlockRequest, this->getCommand(false));
    }
};

#endif /* IOSDBlockRequestQueue_hpp */
