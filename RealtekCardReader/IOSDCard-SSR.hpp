//
//  IOSDCard-SSR.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 12/24/21.
//

#ifndef IOSDCard_SSR_hpp
#define IOSDCard_SSR_hpp

#include "Utilities.hpp"

/// SD status register value (64 bytes)
struct PACKED SSR
{
    /// Bus width (0: 1 bit, 2: 4 bits)
    UInt8 busWidth: 2;
    
    /// `True` if the card is in secured mode
    bool securedMode: 1;
    
    /// Reserved 7 bits for security functions
    UInt8 reserved0: 7;
    
    /// Reserved 6 bits
    UInt8 reserved1: 6;
    
    /// SD card type
    UInt16 cardType;
    
    /// Size of protected area
    UInt32 protectedAreaSize;
    
    /// Speed class of the card
    UInt8 speedClass;
    
    /// Performance of move indicatedf by 1MB/s step
    UInt8 movePerformance;
    
    /// Size of an allocation unit
    UInt8 auSize: 4;
    
    /// Reserved 4 bits
    UInt8 reserved2: 4;
    
    /// Number of AUs to be erased at a time
    UInt16 eraseSize;
    
    /// Timeout value for erasing areaa
    UInt8 eraseTimeout: 6;
    
    /// Fixed offset value added to erase time
    UInt8 eraseOffset: 2;
    
    /// Speed grade for UHS mode
    UInt8 uhsSpeedGrade: 4;
    
    /// Size of AU for UHS mode
    UInt8 uhsAuSize: 4;
    
    /// Video speed class
    UInt8 videoSpeedClass;
    
    /// Reserved 6 bits
    UInt16 reserved3: 6;
    
    /// Size of AU for video speed class
    UInt16 vscAuSize: 10;
    
    /// Suspension address
    UInt32 suspensionAddress: 22;
    
    /// Reserved 6 bits
    UInt32 reserved4: 6;
    
    /// Application performance class value
    UInt32 appPerformanceClass: 4;
    
    /// Support for Performance Enhancement functionalities
    UInt8 performanceEnhance;
    
    /// Reserved 14 bits
    UInt16 reserved5: 14;
    
    /// Discard support
    bool supportsDiscard: 1;
    
    /// Full user area logical erase support
    bool supportsFULE: 1;
    
    /// Reserved 39 bytes for manufacturer
    UInt8 reserved6[39];
    
    ///
    /// Decode from the given raw data
    ///
    /// @param data The raw SD status register value
    /// @param pssr The parsed SD status register value
    /// @return `true` on success, `false` otherwise.
    ///
    static bool decode(const UInt8* data, SSR& pssr)
    {
        // Note that the driver needs the speed classes only at this moment
        pssr.speedClass = data[8];
        
        pssr.uhsSpeedGrade = (data[14] & 0xF0) >> 4;
        
        pssr.videoSpeedClass = data[15];
        
        pinfo("Speed Class = %d; UHS Speed Grade = %d; Video Speed Class = %d.",
              pssr.speedClass, pssr.uhsSpeedGrade, pssr.videoSpeedClass);
        
        return true;
    }
};

static_assert(sizeof(SSR) == 64, "SSR should be 64 bytes long.");

#endif /* IOSDCard_SSR_hpp */
