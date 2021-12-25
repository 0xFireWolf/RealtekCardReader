//
//  IOSDCard-OCR.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 12/24/21.
//

#ifndef IOSDCard_OCR_hpp
#define IOSDCard_OCR_hpp

#include <libkern/OSTypes.h>

/// Operating condition register value
struct OCR
{
    /// Register value as a place holder
    UInt32 value;
    
    /// Bit 24 is set if the host requests the card to switch to 1.8V
    static constexpr UInt32 kRequest1d8V = 1 << 24;
    
    /// Bit 24 is set if the card accepts 1.8V switching
    static constexpr UInt32 kAccepted1d8V = 1 << 24;
    
    /// Bit 28 is set if the host can supply more than 150mA
    static constexpr UInt32 kSDXCPowerControl = 1 << 28;
    
    /// Bit 30 is set if the card is SDHC/SDXC and clear if the card is SDSC
    static constexpr UInt32 kCardCapacityStatus = 1 << 30;
};

#endif /* IOSDCard_OCR_hpp */
