//
//  RealtekUserConfigs.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 7/25/21.
//

#ifndef RealtekUserConfigs_hpp
#define RealtekUserConfigs_hpp

#include "Utilities.hpp"

/// Boot arguments that customize the PCIe/USB-based card reader controller
namespace RealtekUserConfigs::COM
{
    /// The amount of time in milliseconds to wait until the SSC clock becomes stable
    /// If the value is too small, ACMD6 may timeout after the driver switches the clock
    extern UInt32 DelayStableSSCClock;
}

/// Boot arguments that customize the PCIe-based card reader controller
namespace RealtekUserConfigs::PCR
{
    /// The amount of time in milliseconds to delay the card initialization
    /// if the card is present when the driver starts
    extern UInt32 DelayCardInitAtBoot;
}

/// Boot arguments that customize the card initialization
namespace RealtekUserConfigs::Card
{
    /// `True` if the card should be initialized at 3.3V
    extern bool InitAt3v3;
    
    /// `True` if the card should be initialized at the default speed mode
    extern bool InitAtDefaultSpeed;
    
    /// `True` if the card should be initialized at the high speed mode
    extern bool InitAtHighSpeed;
    
    /// Specify the maximum number of attempts to retry an application command
    extern UInt32 ACMDMaxNumAttempts;
}

#endif /* RealtekUserConfigs_hpp */
