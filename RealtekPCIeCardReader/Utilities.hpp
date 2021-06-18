//
//  Utilities.hpp
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 2/27/21.
//

#ifndef Utilities_hpp
#define Utilities_hpp

#include <IOKit/pci/IOPCIDevice.h>
#include "Debug.hpp"

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

static inline UInt16 IOPCIeDeviceConfigRead16(IOPCIDevice* device, IOByteCount offset)
{
    UInt32 base = device->findPCICapability(kIOPCICapabilityIDPCIExpress);
    
    passert(base != 0, "The given device is not a PCIe device.");
    
    return device->configRead16(base + offset);
}

static inline void IOPCIeDeviceConfigWrite16(IOPCIDevice* device, IOByteCount offset, UInt16 value)
{
    UInt32 base = device->findPCICapability(kIOPCICapabilityIDPCIExpress);
    
    passert(base != 0, "The given device is not a PCIe device.");
    
    device->configWrite16(base + offset, value);
}

static inline void IOPCIeDeviceConfigSet16(IOPCIDevice* device, IOByteCount offset, UInt16 set)
{
    UInt32 base = device->findPCICapability(kIOPCICapabilityIDPCIExpress);
    
    passert(base != 0, "The given device is not a PCIe device.");
    
    UInt16 value = device->configRead16(base + offset) | set;
    
    device->configWrite16(base + offset, value);
}

static inline void IOPCIeDeviceConfigClear16(IOPCIDevice* device, IOByteCount offset, UInt16 clear)
{
    UInt32 base = device->findPCICapability(kIOPCICapabilityIDPCIExpress);
    
    passert(base != 0, "The given device is not a PCIe device.");
    
    UInt16 value = device->configRead16(base + offset) & (~clear);
    
    device->configWrite16(base + offset, value);
}

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

template <typename T, typename... Ts>
bool isOneOf(T value, Ts... args)
{
    return ((value == args) || ...);
}

template <typename T, typename... Ts>
bool isNotOneOf(T value, Ts... args)
{
    return ((value != args) && ...);
}

static inline const char* YESNO(bool value)
{
    return value ? "Yes" : "No";
}

template <typename T>
T MIN(T lhs, T rhs)
{
    return lhs < rhs ? lhs : rhs;
}

template <typename T>
T MAX(T lhs, T rhs)
{
    return lhs < rhs ? rhs : lhs;
}

static inline bool OSDictionaryAddStringToDictionary(OSDictionary* dictionary, const char* key, const char* value)
{
    OSString* v = OSString::withCString(value);
    
    if (v == nullptr)
    {
        return false;
    }
    
    dictionary->setObject(key, v);
    
    v->release();
    
    return true;
}

static inline bool OSDictionaryAddDataToDictionary(OSDictionary* dictionary, const char* key, const void* bytes, IOByteCount length)
{
    OSData* data = OSData::withBytes(bytes, static_cast<UInt32>(length));
    
    if (data == nullptr)
    {
        return false;
    }
    
    dictionary->setObject(key, data);
    
    data->release();
    
    return true;
}

template <size_t N>
static inline bool OSDictionaryAddDataToDictionary(OSDictionary* dictionary, const char* key, const UInt8 (&bytes)[N])
{
    return OSDictionaryAddDataToDictionary(dictionary, key, bytes, N);
}

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
