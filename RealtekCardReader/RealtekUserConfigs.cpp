//
//  RealtekUserConfigs.cpp
//  RealtekCardReader
//
//  Created by FireWolf on 8/6/21.
//

#include "RealtekUserConfigs.hpp"

/// Boot arguments that customize the PCIe/USB-based card reader controller
namespace RealtekUserConfigs::COM
{
    /// The amount of time in milliseconds to wait until the SSC clock becomes stable
    /// If the value is too small, ACMD6 may timeout after the driver switches the clock
    UInt32 DelayStableSSCClock = BootArgs::get("rtsxdssc", 10);
}

/// Boot arguments that customize the PCIe-based card reader controller
namespace RealtekUserConfigs::PCR
{
    /// The amount of time in milliseconds to delay the card initialization
    /// if the card is present when the driver starts
    UInt32 DelayCardInitAtBoot = BootArgs::get("rtsxdcib", 100);
}

/// Boot arguments that customize the card initialization
namespace RealtekUserConfigs::Card
{
    /// `True` if the card should be initialized at 3.3V
    bool InitAt3v3 = BootArgs::contains("-rtsx3v3");
    
    /// `True` if the card should be initialized at the default speed mode
    bool InitAtDefaultSpeed = BootArgs::contains("-rtsxdsm");
    
    /// `True` if the card should be initialized at the high speed mode
    bool InitAtHighSpeed = BootArgs::contains("-rtsxhsm");
    
    /// `True` if the driver should separate each CMD18/25 request into multiple CMD17/24 ones
    bool SeparateAccessBlocksRequest = BootArgs::contains("-rtsxsabr");
    
    /// Specify the maximum number of attempts to retry an application command
    UInt32 ACMDMaxNumAttempts = max(BootArgs::get("rtsxamna", 2), 1);
}
