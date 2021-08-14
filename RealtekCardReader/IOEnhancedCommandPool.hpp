//
//  IOEnhancedCommandPool.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 8/14/21.
//

#ifndef IOEnhancedCommandPool_hpp
#define IOEnhancedCommandPool_hpp

#include <IOKit/IOCommandPool.h>
#include "Utilities.hpp"

/// The default creator that allocates a command
template <typename Command>
struct IOCommandCreator
{
    Command* operator()()
    {
        return OSTypeAlloc(Command);
    }
};

/// The default deleter that releases the given command
struct IOCommandDeleter
{
    void operator()(IOCommand* command)
    {
        OSSafeReleaseNULL(command);
    }
};

///
/// Represents an enhanced command pool that provides one-stop services
///
/// @tparam Command Specify the type of commands in the pool (must be a subclass of `IOCommand`).
/// @tparam CommandCreator Specify the type of a callable object that creates a command dynamically;
///                        The creator should overload the operator `()` which takes no arguments and returns a `Command` pointer;
///                        By default, the command pool uses the `IOCommandCreator` that allocates an instance of `Command`.
/// @tparam CommandDeleter Specify the type of a callable object that deletes a dynamically allocated command;
///                        The deleter should overload the operator `()` which takes a non-null `Command` pointer and returns void;
///                        By default, the command pool uses the `IOCommandDeleter` that releases any instances of `IOCommand`.
///
template <typename Command, typename CommandCreator = IOCommandCreator<Command>, typename CommandDeleter = IOCommandDeleter>
class IOEnhancedCommandPool
{
private:
    /// Storage for preallocated commands
    Command** commands;
    
    /// The number of preallocated commands
    IOItemCount capacity;
    
    /// The command pool
    IOCommandPool* pool;
    
    //
    // MARK: Startup Routines
    //
    
private:
    ///
    /// Setup the storage for preallocated commands
    ///
    /// @param capacity The number of preallocated commands
    /// @return `true` on success, `false` otherwise.
    ///
    bool setupStorage(IOItemCount capacity)
    {
        this->commands = IONewZero(Command*, capacity);
        
        this->capacity = capacity;
        
        return this->commands != nullptr;
    }
    
    ///
    /// Preallocate an array of commands
    ///
    /// @return `true` on success, `false` otherwise.
    ///
    bool setupCommands()
    {
        for (auto index = 0; index < this->capacity; index += 1)
        {
            this->commands[index] = CommandCreator{}();
            
            if (this->commands[index] == nullptr)
            {
                perr("[%02d] Failed to preallocate a command.", index);
                
                this->tearDownCommands();
                
                return false;
            }
        }
        
        return true;
    }
    
    ///
    /// Setup the command pool
    ///
    /// @param workloop The workloop with which the pool should synchronize
    /// @return `true` on success, `false` otherwise.
    ///
    bool setupPool(IOWorkLoop* workloop)
    {
        this->pool = IOCommandPool::withWorkLoop(workloop);
        
        if (this->pool == nullptr)
        {
            perr("Failed to create the command pool.");
            
            return false;
        }
        
        for (auto index = 0; index < this->capacity; index += 1)
        {
            this->pool->returnCommand(this->commands[index]);
        }
        
        return true;
    }
    
    //
    // MARK: Tear Down Routines
    //
    
private:
    ///
    /// Tear down the storage for preallocated commands
    ///
    void tearDownStorage()
    {
        IOSafeDeleteNULL(this->commands, Command*, this->capacity);
    }
    
    ///
    /// Tear down the array of preallocated commands
    ///
    void tearDownCommands()
    {
        for (auto index = 0; index < this->capacity; index += 1)
        {
            CommandDeleter{}(this->commands[index]);
        }
    }
    
    ///
    /// Tear down the command pool
    ///
    void tearDownPool()
    {
        OSSafeReleaseNULL(this->pool);
    }
    
    //
    // MARK: Initializer & Finalizer
    //
    
public:
    ///
    /// Initialize a command pool with the given capacity
    ///
    /// @param workloop The workloop with which the pool should synchronize
    /// @param capacity The number of preallocated commands
    /// @return `true` on success, `false` otherwise.
    ///
    bool init(IOWorkLoop* workloop, IOItemCount capacity)
    {
        // Guard: Allocate the storage for preallocated commands
        if (!this->setupStorage(capacity))
        {
            perr("Failed to allocate the storage for preallocated commands.");
            
            return false;
        }
        
        // Guard: Preallocate an array of commands
        if (!this->setupCommands())
        {
            perr("Failed to preallocate an array of commands.");
            
            goto error1;
        }
        
        // Guard: Create the internal command pool
        if (!this->setupPool(workloop))
        {
            perr("Failed to create the internal command pool.");
            
            goto error2;
        }
        
        return true;
        
    error2:
        this->tearDownCommands();
        
    error1:
        this->tearDownStorage();
        
        return false;
    }
    
    ///
    /// Tear down the command pool
    ///
    void free()
    {
        this->tearDownPool();
        
        this->tearDownCommands();
        
        this->tearDownStorage();
    }
    
    //
    // MARK: Command Pool Factory
    //
    
public:
    ///
    /// Create a command pool with the given capacity
    ///
    /// @param workloop The workloop with which the pool should synchronize
    /// @param capacity The number of preallocated commands
    /// @return A non-null command pool on success, `nullptr` otherwise.
    ///
    static IOEnhancedCommandPool* createWithCapacity(IOWorkLoop* workloop, IOItemCount capacity)
    {
        auto instance = new IOEnhancedCommandPool;
        
        if (instance == nullptr)
        {
            return nullptr;
        }
        
        if (!instance->init(workloop, capacity))
        {
            delete instance;
            
            return nullptr;
        }
        
        return instance;
    }
    
    ///
    /// Destory the given command pool
    ///
    /// @param pool A non-null command pool returned by `IOEnhancedCommandPool::createWithCapacity()`.
    ///
    static void destory(IOEnhancedCommandPool* pool NONNULL)
    {
        pool->free();
        
        delete pool;
    }
    
    ///
    /// Destory a command pool safely
    ///
    /// @param pool A nullable command pool returned by `IOEnhancedCommandPool::createWithCapacity()`.
    /// @note This function mimics the macro `OSSafeReleaseNULL()`.
    ///
    static void safeDestory(IOEnhancedCommandPool*& pool)
    {
        if (pool != nullptr)
        {
            IOEnhancedCommandPool::destory(pool);
            
            pool = nullptr;
        }
    }
    
    //
    // MARK: Manage Commands
    //
    
public:
    ///
    /// Get a command from the pool
    ///
    /// @param blockForCommand Pass `true` if the caller should be blocked waiting for a command
    /// @return A non-null command if `blockForCommand` is `true`, otherwise a nullable command.
    /// @note The semantics is identical to `IOCommandPool::getCommand()`.
    ///
    Command* getCommand(bool blockForCommand = true)
    {
        return OSDynamicCast(Command, this->pool->getCommand(blockForCommand));
    }
    
    ///
    /// Return a command to the pool
    ///
    /// @param command A non-null command that is previously returned by `IOEnhancedCommandPool::getCommand()`
    ///
    void returnCommand(Command* command NONNULL)
    {
        this->pool->returnCommand(command);
    }
};

#endif /* IOEnhancedCommandPool_hpp */
