//
//  IOSDCard-SCR.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 12/24/21.
//

#ifndef IOSDCard_SCR_hpp
#define IOSDCard_SCR_hpp

#include "BitOptions.hpp"
#include "Utilities.hpp"

/// Card specification level
struct PACKED SPEC
{
    UInt8 spec, spec3, spec4, spec5;
};

/// SD configuration register value (8 bytes)
struct PACKED SCR
{
    /// The structure version
    UInt8 version: 4;
    
    /// SD card specification version
    UInt8 spec: 4;
    
    enum Spec
    {
        /// SD Specification 1.0 - 1.01
        kVersion1x = 0,
        
        /// SD Specification 1.10
        kVersion1d1x = 1,
        
        /// SD Specification 2.00 - 3.0X
        kVersion2x = 2,
    };
    
    /// The data status after erase
    UInt8 dataStatusAfterErase: 1;
    
    /// The security specification version
    UInt8 security: 3;
    
    /// The bus widths supported by the card (Bit 0: 1 bit, Bit 2: 4 bits)
    UInt8 busWidths: 4;
    
    enum BusWidth
    {
        k1Bit = 1 << 0,
        k4Bit = 1 << 2,
    };
    
    /// SD card specification version 3.00+
    UInt8 spec3: 1;
    
    /// Extended security support
    UInt8 extendedSecurity: 4;
    
    /// SD card specification version 4.00+
    UInt8 spec4: 1;
    
    /// SD card specification version 5.00+
    UInt8 spec5: 4;
    
    /// Reserved 2 bits
    UInt8 reserved0: 2;
    
    /// Supported commands (CMD58/59)
    bool supportsCMD5859: 1;
    
    /// Supported commands (CMD48/49)
    bool supportsCMD4849: 1;
    
    /// Supported commands (CMD23)
    bool supportsCMD23: 1;
    
    /// Supported commands (CMD20)
    bool supportsCMD20: 1;
    
    /// Reserved 32 bits for manufacturer usage
    UInt32 reserved1;
    
    ///
    /// Decode from the given raw data
    ///
    /// @param data An array of 2 integers encoded in big endian
    /// @param pscr The parsed SD configuration register value
    /// @return `true` on success, `false` otherwise.
    ///
    static bool decode(const UInt8* data, SCR& pscr)
    {
        pscr.version = (data[0] & 0xF0) >> 4;
        
        if (pscr.version != 0)
        {
            printf("Unrecogized SCR struct version 0x%02x.\n", pscr.version);
            
            return false;
        }
        
        pscr.busWidths = data[1] & 0x0F;
        
        if (!BitOptions<UInt32>(pscr.busWidths).contains(BusWidth::k1Bit) ||
            !BitOptions<UInt32>(pscr.busWidths).contains(BusWidth::k4Bit))
        {
            printf("All cards should set at least bit 0 and bit 2 in the bus widths field (= 0x%x).\n", pscr.busWidths);
            
            return false;
        }
        
        pscr.spec                 = data[0] & 0x0F;
        pscr.dataStatusAfterErase = (data[1] & 0x80) >> 7;
        pscr.security             = (data[1] & 0x70) >> 4;
        pscr.spec3                = (data[2] & 0x80) >> 7;
        pscr.extendedSecurity     = (data[2] & 0x78) >> 3;
        pscr.spec4                = (data[2] & 0x04) >> 2;
        pscr.spec5                = (data[2] & 0x03) << 2;
        pscr.spec5               |= (data[3] & 0xC0) >> 6;
        pscr.supportsCMD5859      = (data[3] & 0x0F) >> 3;
        pscr.supportsCMD4849      = (data[3] & 0x0F) >> 2;
        pscr.supportsCMD23        = (data[3] & 0x0F) >> 1;
        pscr.supportsCMD20        = (data[3] & 0x0F) >> 0;
        
        return true;
    }
};

static_assert(sizeof(SCR) ==  8, "SCR should be  8 bytes long.");

#endif /* IOSDCard_SCR_hpp */
