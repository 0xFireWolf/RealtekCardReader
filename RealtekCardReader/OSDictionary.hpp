//
//  OSDictionary.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 8/25/21.
//

#ifndef OSDictionary_hpp
#define OSDictionary_hpp

#include <libkern/c++/OSDictionary.h>

static inline bool OSDictionaryAddStringToDictionary(OSDictionary* dictionary, const char* key, const char* value)
{
    OSString* v = OSString::withCString(value);
    
    if (v == nullptr)
    {
        return false;
    }
    
    bool retVal = dictionary->setObject(key, v);
    
    v->release();
    
    return retVal;
}

static inline bool OSDictionaryAddDataToDictionary(OSDictionary* dictionary, const char* key, const void* bytes, IOByteCount length)
{
    OSData* data = OSData::withBytes(bytes, static_cast<UInt32>(length));
    
    if (data == nullptr)
    {
        return false;
    }
    
    bool retVal = dictionary->setObject(key, data);
    
    data->release();
    
    return retVal;
}

template <size_t N>
static inline bool OSDictionaryAddDataToDictionary(OSDictionary* dictionary, const char* key, const UInt8 (&bytes)[N])
{
    return OSDictionaryAddDataToDictionary(dictionary, key, bytes, N);
}

template <typename T>
static inline bool OSDictionaryAddIntegerToDictionary(OSDictionary* dictionary, const char* key, T interger)
{
    OSNumber* number = OSNumber::withNumber(interger, sizeof(T) * 8);
    
    if (number == nullptr)
    {
        return false;
    }
    
    bool retVal = dictionary->setObject(key, number);
    
    number->release();
    
    return retVal;
}

#endif /* OSDictionary_hpp */
