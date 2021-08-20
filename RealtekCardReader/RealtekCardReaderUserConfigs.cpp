//
//  RealtekCardReaderUserConfigs.cpp
//  RealtekCardReader
//
//  Created by FireWolf on 8/20/21.
//

#include "RealtekCardReaderUserConfigs.hpp"
#include "Utilities.hpp"

/// Boot arguments that customize the PCIe/USB-based card reader controller
namespace UserConfigs::COM
{
    /// The amount of time in milliseconds to wait until the SSC clock becomes stable
    /// If the value is too small, ACMD6 may timeout after the driver switches the clock
    UInt32 DelayStableSSCClock = max(BootArgs::get("rtsxdssc", 10), 1);
}

/// Boot arguments that customize the PCIe-based card reader controller
namespace UserConfigs::PCR
{
    /// The amount of time in milliseconds to delay the card initialization
    /// if the card is present when the driver starts
    UInt32 DelayCardInitAtBoot = BootArgs::get("rtsxdcib", 100);
}

/// Boot arguments that customize the USB-based card reader controller
namespace UserConfigs::UCR
{
    /// The amount of time in milliseconds to poll for the device status
    UInt32 DeviceStatusPollingInterval = max(BootArgs::get("rtsxdspi", 500), 100);
}
