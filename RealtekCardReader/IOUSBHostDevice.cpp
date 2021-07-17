//
//  IOUSBHostDevice.cpp
//  RealtekCardReader
//
//  Created by FireWolf on 7/16/21.
//

#include "IOUSBHostDevice.hpp"
#include "Debug.hpp"

///
/// Find the first interface of the given device
///
/// @param device A non-null host device
/// @return A non-null retained host interface on success, `nullptr` otherwise.
/// @note Implementation of this function is derived from Apple's IOUSBDevice migration guide.
///
IOUSBHostInterface* IOUSBHostDeviceFindFirstInterface(IOUSBHostDevice* device)
{
    OSIterator* iterator = device->getChildIterator(gIOServicePlane);
    
    if (iterator == nullptr)
    {
        perr("Failed to fetch the child iterator.");
        
        return nullptr;
    }
    
    OSObject* candidate = nullptr;
    
    IOUSBHostInterface* interface = nullptr;
    
    while (true)
    {
        candidate = iterator->getNextObject();
        
        if (candidate == nullptr)
        {
            pinfo("Reach the end of the iterator.");
            
            break;
        }
        
        interface = OSDynamicCast(IOUSBHostInterface, candidate);
        
        if (interface != nullptr)
        {
            pinfo("Found the host interface.");
            
            interface->retain();
            
            break;
        }
    }
    
    OSSafeReleaseNULL(iterator);
    
    return interface;
}

///
/// Get the current device configuration
///
/// @param device A non-null host device
/// @param client The client with an open session that issues the request
/// @param config The current device configuration on return;
///               0 if the device is not configured, other values otherwise
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Implementation of this function is derived from Apple's IOUSBDevice migration guide.
///
IOReturn IOUSBHostDeviceGetConfiguration(IOUSBHostDevice* device, IOService* client, UInt8& config)
{
    StandardUSB::DeviceRequest request =
    {
        // The request type
        makeDeviceRequestbmRequestType(kRequestDirectionIn, kRequestTypeStandard, kRequestRecipientDevice),
        
        // The request
        kDeviceRequestGetConfiguration,
        
        // The request value
        0,
        
        // The request index
        0,
        
        // The request length
        sizeof(UInt8)
    };
    
    UInt32 bytesTransferred = 0;
    
    return device->deviceRequest(client, request, &config, bytesTransferred);
}

///
/// Set the device configuration at the given index
///
/// @param device A non-null host device
/// @param client The client with an open session that issues the request
/// @param config Index of the desired device configuration
/// @return `kIOReturnSuccess` on success, `kIOReturnInvalid` if the given config index is invalid, other values otherwise.
///
IOReturn IOUSBHostDeviceSetConfiguration(IOUSBHostDevice* device, IOService* client, UInt8 config)
{
    // Guard: Check whether the given config index is valid
    UInt8 numConfigs = device->getDeviceDescriptor()->bNumConfigurations;
    
    if (config >= numConfigs)
    {
        perr("The given config index %u is invalid. Total number of configs = %u.", config, numConfigs);
        
        return kIOReturnInvalid;
    }
    
    // Guard: Find the target configuration descriptor
    const StandardUSB::ConfigurationDescriptor* targetDescriptor = device->getConfigurationDescriptor(config);
    
    if (targetDescriptor == nullptr)
    {
        perr("Cannot find the descriptor for the target configuration %u.", config)
        
        return kIOReturnNotFound;
    }
    
    // Guard: Find the current configuration descriptor
    const StandardUSB::ConfigurationDescriptor* currentDescriptor = device->getConfigurationDescriptor();
    
    if (currentDescriptor == nullptr)
    {
        pinfo("The device is not configured. Will set the configuration to %u.", config);
    }
    else if (currentDescriptor->bConfigurationValue != targetDescriptor->bConfigurationValue)
    {
        pinfo("The current configuration is not the requested one. Will set the configuration to %u.", config);
    }
    else
    {
        pinfo("The device has already been configured to %u. No need to change the configuration.", config);
        
        return kIOReturnSuccess;
    }
    
    // Set the new configuration
    return device->setConfiguration(targetDescriptor->bConfigurationValue);
}
