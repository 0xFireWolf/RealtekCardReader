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
    
    /// Specify the maximum number of attempts to retry an application command
    UInt32 ACMDMaxNumAttempts = max(BootArgs::get("iosdamna", 2), 1);
}
