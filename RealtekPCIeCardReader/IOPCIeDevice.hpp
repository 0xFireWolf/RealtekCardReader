//
//  IOPCIeDevice.hpp
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 7/7/21.
//

#ifndef IOPCIeDevice_hpp
#define IOPCIeDevice_hpp

#include <IOKit/pci/IOPCIDevice.h>
#include "Debug.hpp"

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

#endif /* IOPCIeDevice_hpp */
