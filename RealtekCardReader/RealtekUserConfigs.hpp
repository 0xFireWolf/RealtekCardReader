//
//  RealtekUserConfigs.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 7/25/21.
//

#ifndef RealtekUserConfigs_hpp
#define RealtekUserConfigs_hpp

#include "Utilities.hpp"

namespace RealtekUserConfigs::Card
{
    /// `True` if the card should be initialized at 3.3V
    bool InitAt3v3 = BootArgs::contains("-rtsx3v3");
}

#endif /* RealtekUserConfigs_hpp */
