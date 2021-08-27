//
//  IOSDCardEventSource.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 6/9/21.
//

#ifndef IOSDCardEventSource_hpp
#define IOSDCardEventSource_hpp

#include <IOKit/IOEventSource.h>
#include "IOSDCard.hpp"

/// An event source to signal the workloop to process the card insertion or removal event
class IOSDCardEventSource: public IOEventSource
{
    /// Constructors & Destructors
    OSDeclareDefaultStructors(IOSDCardEventSource);
    
    using super = IOEventSource;
    
    /// The completion routine
    IOSDCard::Completion completion;
    
    ///
    /// Check whether an action needs to perform to handle the card event
    ///
    /// @return `false` always. The owner must explictly invoke `IOSDCardEventSource::enable()` to trigger another action.
    ///
    bool checkForWork() override;
    
public:
    ///
    /// Type of the function that processes a card event
    ///
    /// @param owner The owner of the event source
    /// @param completion A non-null completion routine to be invoked when the card event has been processed
    ///
    using Action = void (*)(OSObject* owner, IOSDCard::Completion* completion);
    
    ///
    /// Enable the event source
    ///
    /// @param completion A nullable completion routine to be invoked when the card event has been processed
    ///
    void enable(IOSDCard::Completion* completion);

    ///
    /// Disable the event source
    ///
    void disable() override;
    
    ///
    /// Create a card event source with the given action
    ///
    /// @param owner Owner of the returned instance; the first parameter of the action routine
    /// @param action A non-null action to process the card insertion or removal event
    /// @return A non-null event source on success, `nullptr` otherwise.
    ///
    static IOSDCardEventSource* createWithAction(OSObject* owner, Action action);
};

#endif /* IOSDCardEventSource_hpp */
