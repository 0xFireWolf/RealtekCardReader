//
//  IOSDBlockRequestQueue.cpp
//  RealtekCardReader
//
//  Created by FireWolf on 6/9/21.
//

#include "IOSDBlockRequestQueue.hpp"
#include "Debug.hpp"
#include "IOCommandGate.hpp"
#include <kern/queue.h>

//
// MARK: - Meta Class Definitions
//

OSDefineMetaClassAndStructors(IOSDBlockRequestQueue, IOCommandPool);

///
/// Create an empty queue protected by the given work loop
///
/// @param workLoop The workloop that the queue should synchronize with
/// @return A non-null queue on success, `nullptr` otherwise.
///
IOSDBlockRequestQueue* IOSDBlockRequestQueue::create(IOWorkLoop* workLoop)
{
    auto queue = OSTypeAlloc(IOSDBlockRequestQueue);
    
    if (queue == nullptr)
    {
        return nullptr;
    }
    
    if (!queue->initWithWorkLoop(workLoop))
    {
        queue->release();
        
        return nullptr;
    }
    
    return queue;
}

///
/// Check whether the queue is empty
///
/// @return `true` if the queue is empty, `false` otherwise.
///
bool IOSDBlockRequestQueue::isEmpty()
{
    bool empty = false;
    
    auto action = [&]() -> IOReturn
    {
        empty = queue_empty(&this->fQueueHead);
        
        return kIOReturnSuccess;
    };
    
    IOCommandGateRunAction(this->fSerializer, action);
    
    return empty;
}
