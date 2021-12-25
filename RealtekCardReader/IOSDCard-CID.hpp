//
//  IOSDCard-CID.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 12/24/21.
//

#ifndef IOSDCard_CID_hpp
#define IOSDCard_CID_hpp

#include "Utilities.hpp"

/// Card identification data (16 bytes)
struct PACKED CID
{
    /// The manufacturer ID
    UInt8 manufacturer;
    
    /// The OEM application ID
    UInt16 oem;
    
    /// The product name
    SInt8 name[5];
    
    /// The product revision (hardware)
    UInt8 hwrevision: 4;
    
    /// The product revision (firmware)
    UInt8 fwrevision: 4;
    
    /// The product serial number
    UInt32 serial;
    
    /// Reserved 4 bits
    UInt8 reserved: 4;
    
    /// Manufacturing date (year)
    UInt8 year: 8;
    
    /// Manufacturing date (month)
    UInt8 month: 4;
    
    /// CRC7 checksum and the end bit
    UInt8 end;
    
    /// Get the manufacturing year
    inline UInt32 getYear() const
    {
        return this->year + 2000;
    }
    
    /// Get a non-null vendor string
    inline const char* getVendorString() const
    {
        // CID List: https://www.cameramemoryspeed.com/sd-memory-card-faq/reading-sd-card-cid-serial-psn-internal-numbers/
        switch (this->manufacturer)
        {
            case 0x01:
            {
                return "Panasonic";
            }
                
            case 0x02:
            {
                return "Toshiba";
            }
                
            case 0x03:
            {
                return "SanDisk";
            }
                
            case 0x1b:
            {
                return "Samsung";
            }
                
            case 0x1d:
            {
                return "ADATA";
            }
                
            case 0x27:
            {
                return "Phison";
            }
                
            case 0x28:
            {
                return "Lexar";
            }
                
            case 0x31:
            {
                return "Silicon Power";
            }
                
            case 0x41:
            {
                return "Kingston";
            }
                
            case 0x74:
            {
                return "Transcend";
            }
                
            case 0x76:
            {
                return "Patriot";
            }
                
            case 0x82:
            {
                return "Sony";
            }
                
            default:
            {
                return "Unknown";
            }
        }
    }
    
    ///
    /// Decode from the given raw data
    ///
    /// @param data An array of 4 integers encoded in big endian
    /// @param pcid The parsed card identification data
    /// @return `true` on success, `false` otherwise.
    ///
    static bool decode(const UInt32* data, CID& pcid)
    {
        pcid.manufacturer = UNSTUFF_BITS(data, 120,  8);
        pcid.oem          = UNSTUFF_BITS(data, 104, 16);
        pcid.name[0]      = UNSTUFF_BITS(data,  96,  8);
        pcid.name[1]      = UNSTUFF_BITS(data,  88,  8);
        pcid.name[2]      = UNSTUFF_BITS(data,  80,  8);
        pcid.name[3]      = UNSTUFF_BITS(data,  72,  8);
        pcid.name[4]      = UNSTUFF_BITS(data,  64,  8);
        pcid.hwrevision   = UNSTUFF_BITS(data,  60,  4);
        pcid.fwrevision   = UNSTUFF_BITS(data,  56,  4);
        pcid.serial       = UNSTUFF_BITS(data,  24, 32);
        pcid.year         = UNSTUFF_BITS(data,  12,  8);
        pcid.month        = UNSTUFF_BITS(data,   8,  4);
        
        return true;
    }
    
    /// Equatable IMP
    friend bool operator==(const CID& lhs, const CID& rhs)
    {
        return memcmp(&lhs, &rhs, sizeof(CID)) == 0;
    }
    
    friend bool operator!=(const CID& lhs, const CID& rhs)
    {
        return !(lhs == rhs);
    }
    
    /// Check whether the card identification data is empty
    inline bool isEmpty() const
    {
        auto uint64s = reinterpret_cast<const UInt64*>(this);
        
        return uint64s[0] == 0 && uint64s[1] == 0;
    }
    
    /// Reset the card identification data to zeros
    inline void reset()
    {
        auto uint64s = reinterpret_cast<UInt64*>(this);
        
        uint64s[0] = 0;
        
        uint64s[1] = 0;
    }
};

static_assert(sizeof(CID) == 16, "CID should be 16 bytes long.");

#endif /* IOSDCard_CID_hpp */
