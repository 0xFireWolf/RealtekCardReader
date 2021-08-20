//
//  RealtekCardReaderUserConfigs.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 8/20/21.
//

#ifndef RealtekCardReaderUserConfigs_hpp
#define RealtekCardReaderUserConfigs_hpp

#include <IOKit/IOTypes.h>

/// Boot arguments that customize the PCIe/USB-based card reader controller
namespace UserConfigs::COM
{
    /// The amount of time in milliseconds to wait until the SSC clock becomes stable
    /// If the value is too small, ACMD6 may timeout after the driver switches the clock
    extern UInt32 DelayStableSSCClock;
}

/// Boot arguments that customize the PCIe-based card reader controller
namespace UserConfigs::PCR
{
    /// The amount of time in milliseconds to delay the card initialization
    /// if the card is present when the driver starts
    extern UInt32 DelayCardInitAtBoot;
}

/// Boot arguments that customize the USB-based card reader controller
namespace UserConfigs::UCR
{
    /// The amount of time in milliseconds to poll for the device status
    extern UInt32 DeviceStatusPollingInterval;
}

#endif /* RealtekCardReaderUserConfigs_hpp */
