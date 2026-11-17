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
    
    /// Keep the card reader in the worker state forever (the behavior of v0.9.7)
    bool DisableIdleState = BootArgs::contains("-rtsxnoidle");
    
    /// The amount of time in milliseconds to wait after a card insertion interrupt before the card is set up
    UInt32 CardInsertionDebounce = BootArgs::get("rtsxdebounce", 200);
    
    /// The amount of time in milliseconds to wait after the controller is re-initialized on wake before the card presence is read
    /// (on some chips, e.g. RTS5227, the card detect bit reports the opposite of the slot state for up to ~200 ms)
    UInt32 WakeCardDetectSettle = BootArgs::get("rtsxwakesettle", 250);
    
    /// Publish the power-management state ("RTSX Power") to the I/O Registry; always on in debug builds
#ifdef DEBUG
    bool PowerDiagnostics = true;
#else
    bool PowerDiagnostics = BootArgs::contains("-rtsxdiag");
#endif
}

/// Boot arguments that customize the USB-based card reader controller
namespace UserConfigs::UCR
{
    /// The amount of time in milliseconds to poll for the device status
    UInt32 DeviceStatusPollingInterval = max(BootArgs::get("rtsxdspi", 500), 100);
}
