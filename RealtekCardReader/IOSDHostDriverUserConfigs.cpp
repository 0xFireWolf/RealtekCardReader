//
//  IOSDHostDriverUserConfigs.cpp
//  RealtekCardReader
//
//  Created by FireWolf on 8/20/21.
//

#include "IOSDHostDriverUserConfigs.hpp"
#include "Utilities.hpp"

/// Boot arguments that customize the card initialization
namespace UserConfigs::Card
{
    /// `True` if the card should be initialized at 3.3V
    bool InitAt3v3 = BootArgs::contains("-iosd3v3");
    
    /// `True` if the card should be initialized at the default speed mode
    bool InitAtDefaultSpeed = BootArgs::contains("-iosddsm");
    
    /// `True` if the card should be initialized at the high speed mode
    bool InitAtHighSpeed = BootArgs::contains("-iosdhsm");
    
    /// `True` if the driver should separate each CMD18/25 request into multiple CMD17/24 ones
    bool SeparateAccessBlocksRequest = BootArgs::contains("-iosdsabr");
    
    /// `True` if the driver should not issue the ACMD23 command when processing CMD25 requests
    bool NoACMD23 = BootArgs::contains("-iosdnoacmd23");

    /// `True` if the driver should use the legacy format of the card characteristics dictionary
    bool UseLegacyCardCharacteristicsDictionary = BootArgs::contains("-iosdlccd");
    
    /// Specify the maximum number of attempts to retry an application command
    UInt32 ACMDMaxNumAttempts = max(BootArgs::get("iosdamna", 2), 1);
    
    /// The amount of time in milliseconds the bus stays powered off before the card is powered on again.
    /// A card that has switched to 1.8V signaling only returns to 3.3V after its supply has fully discharged,
    /// otherwise the retry at a lower speed mode aborts with "the card is already operating at 1.8V".
    UInt32 PowerOffDelay = max(BootArgs::get("iosdpoweroff", 100), 1);
}
