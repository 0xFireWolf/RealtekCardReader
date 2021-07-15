//
//  IOSDCardEventSource.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 6/9/21.
//

#ifndef IOSDCardEventSource_hpp
#define IOSDCardEventSource_hpp

#include <IOKit/IOEventSource.h>

/// An event source to signal the workloop to process the card insertion or removal event
class IOSDCardEventSource: public IOEventSource
{
    /// Constructors & Destructors
    OSDeclareDefaultStructors(IOSDCardEventSource);
    
    using super = IOEventSource;
    
    ///
    /// Check whether an action needs to perform to handle the card event
    ///
    /// @return `false` always. The owner must explictly invoke `IOSDCardEventSource::enable()` to trigger another action.
    ///
    bool checkForWork() override;
    
public:
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
