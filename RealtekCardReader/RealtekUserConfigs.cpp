//
//  RealtekUserConfigs.cpp
//  RealtekCardReader
//
//  Created by FireWolf on 8/6/21.
//

#include "RealtekUserConfigs.hpp"

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
}
