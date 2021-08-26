//
//  Utilities.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 2/27/21.
//

#ifndef Utilities_hpp
#define Utilities_hpp

#include <IOKit/pci/IOPCIDevice.h>
#include "Debug.hpp"

//
// MARK: - Attributes
//

/**
 *  Export function or symbol for linking
 */
#define EXPORT __attribute__((visibility("default")))

/**
 *  Ensure the symbol is not exported
 */
#define PRIVATE __attribute__((visibility("hidden")))

/**
 *  For private fallback symbol definition
 */
#define WEAKFUNC __attribute__((weak))

/**
 *  Remove padding between fields
 */
#define PACKED __attribute__((packed))

/**
 *  Deprecate the interface
 */
#define DEPRECATE(x) __attribute__((deprecated(x)))

/**
 *  Non-null argument
 */
#define NONNULL __attribute__((nonnull))

//
// MARK: - Convenient Helpers
//

static constexpr IOPMPowerFlags kIOPMPowerOff = 0;

/// Get the number of elements in an array
template <typename T, size_t N>
constexpr size_t arrsize(const T (&array)[N])
{
    return N;
}

template <typename T1, typename T2>
struct Pair
{
    T1 first;
    
    T2 second;
    
    Pair(T1 first, T2 second)
        : first(first), second(second) {}
};

template <typename T1, typename T2, typename T3>
struct Triplet
{
    T1 first;
    
    T2 second;
    
    T3 third;
    
    Triplet(T1 first, T2 second, T3 third)
        : first(first), second(second), third(third) {}
};

#define KPTR(ptr) \
    static_cast<UInt32>(reinterpret_cast<UInt64>(ptr) >> 32), \
    static_cast<UInt32>(reinterpret_cast<UInt64>(ptr))

template <typename T>
constexpr T MHz2Hz(T mhz)
{
    return mhz * 1000000;
}

template <typename T>
constexpr T KHz2Hz(T khz)
{
    return khz * 1000;
}

template <typename T>
constexpr T Hz2MHz(T hz)
{
    return hz / 1000000;
}

namespace Value
{
    template <typename T>
    struct Value
    {
        T value;

        explicit Value(T value) : value(value) {}

        template<typename... Ts>
        bool isOneOf(Ts... args)
        {
            return ((this->value == args) || ...);
        }

        template <typename... Ts>
        bool isNotOneOf(Ts... args)
        {
            return ((this->value != args) && ...);
        }
    };

    template <typename T>
    static Value<T> of(T value)
    {
        return Value<T>(value);
    }
}

/// A simplified version of c++17 std::optional<T>
/// T must be default constructable
template <typename T>
struct Optional
{
    T value;
    
    bool exists;
    
    Optional(const T& value) : value(value), exists(true) {}
    
    Optional() : value(), exists(false) {}
    
    inline bool hasValue()
    {
        return this->exists;
    }
    
    T* operator->()
    {
        return &this->value;
    }
    
    T& operator*()
    {
        return this->value;
    }
    
    static Optional<T> nullopt()
    {
        return Optional<T>();
    }
};

namespace BootArgs
{
    static inline bool contains(const char* name)
    {
        int dummy = 0;
        
        return PE_parse_boot_argn(name, &dummy, sizeof(dummy));
    }
    
    template <typename T>
    Optional<T> get(const char* name)
    {
        T value;
        
        if (PE_parse_boot_argn(name, &value, sizeof(T)))
        {
            return Optional<T>(value);
        }
        else
        {
            return Optional<T>::nullopt();
        }
    }
    
    template <typename T>
    T get(const char* name, T fallback)
    {
        T value = {};
        
        return PE_parse_boot_argn(name, &value, sizeof(T)) ? value : fallback;
    }
}

/// Align the given numeric value to the next `boundary` bytes boundary
template <typename T>
T align(T value, T boundary)
{
    T mask = boundary - 1;
    
    return (value + mask) & (~mask);
}

static inline const char* YESNO(bool value)
{
    return value ? "Yes" : "No";
}

/*
 * Find Last Set bit
 */
static inline int myfls(int mask)
{
#if __has_builtin(__builtin_fls)
    return __builtin_fls(mask);
#elif __has_builtin(__builtin_clz)
    if(mask == 0)
        return (0);

    return (sizeof(mask) << 3) - __builtin_clz(mask);
#else
    int bit;

    if(mask == 0)
        return (0);
    for(bit = 1; mask != 1; bit++)
        mask = (unsigned)mask >> 1;
    return (bit);
#endif
}

#endif /* Utilities_hpp */
