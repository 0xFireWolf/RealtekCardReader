//
//  IOUSBHostDevice.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 7/16/21.
//

#ifndef IOUSBHostDevice_hpp
#define IOUSBHostDevice_hpp

// Get rid of a bunch of documentation warnings in the USB headers
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#include <IOKit/usb/IOUSBHostFamily.h>
#include <IOKit/usb/IOUSBHostDevice.h>
#include <IOKit/usb/IOUSBHostInterface.h>
#pragma clang diagnostic pop

///
/// Find the first interface of the given device
///
/// @param device A non-null host device
/// @return A non-null retained host interface on success, `nullptr` otherwise.
/// @note Implementation of this function is derived from Apple's IOUSBDevice migration guide.
///
IOUSBHostInterface* IOUSBHostDeviceFindFirstInterface(IOUSBHostDevice* device);

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
IOReturn IOUSBHostDeviceGetConfiguration(IOUSBHostDevice* device, IOService* client, UInt8& config);

///
/// Set the device configuration at the given index
///
/// @param device A non-null host device
/// @param client The client with an open session that issues the request
/// @param config Index of the desired device configuration
/// @return `kIOReturnSuccess` on success, `kIOReturnInvalid` if the given config index is invalid, other values otherwise.
///
IOReturn IOUSBHostDeviceSetConfiguration(IOUSBHostDevice* device, IOService* client, UInt8 config);

#endif /* IOUSBHostDevice_hpp */
