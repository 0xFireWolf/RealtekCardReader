//
//  IOSDCard.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 6/3/21.
//

#ifndef IOSDCard_hpp
#define IOSDCard_hpp

#include <libkern/OSTypes.h>
#include <IOKit/IOService.h>
#include "Utilities.hpp"
#include "BitOptions.hpp"

#define UNSTUFF_BITS(resp,start,size)                    \
    ({                                \
        const int __size = size;                \
        const UInt32 __mask = (__size < 32 ? 1 << __size : 0) - 1;    \
        const int __off = 3 - ((start) / 32);            \
        const int __shft = (start) & 31;            \
        UInt32 __res;                        \
                                    \
        __res = resp[__off] >> __shft;                \
        if (__size + __shft > 32)                \
            __res |= resp[__off-1] << ((32 - __shft) % 32);    \
        __res & __mask;                        \
    })

/// Forward declaration
class IOSDHostDriver;

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

/// Card specification level
struct PACKED SPEC
{
    UInt8 spec, spec3, spec4, spec5;
};

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

/// Card specific data (Version 1.0) (16 bytes)
struct PACKED CSDV1
{
    // ===============
    // |    Common   |
    // ===============
    
    /// The structure version
    UInt8 version: 2;
    
    /// Reserved 6 bits
    UInt8 reserved0: 6;
    
    /// Data read access time - 1 (Reserved 1 bit)
    UInt8 reserved1: 1;
    
    /// Data read access time - 1 (Time value)
    UInt8 taacTimeValue: 4;
    
    /// Data read access time - 1 (Time unit)
    UInt8 taacTimeUnit: 3;
    
    /// Data read access time - 2 in clock cycles
    UInt8 nasc;
    
    /// Maximum data transfer rate (Reserved 1 bit)
    UInt8 reservedDTR: 1;
    
    /// Maximum data transfer rate (Transfer rate value)
    UInt8 maxTransferRateValue: 4;
    
    /// Maximum data transfer rate (Transfer rate unit)
    UInt8 maxTransferRateUnit: 3;
    
    /// Card command classes
    UInt16 cardCommandClasses: 12;
    
    /// Maximum read data block length
    UInt16 maxReadDataBlockLength: 4;
    
    /// `True` if reading partial blocks is allowed
    bool isPartialBlockReadAllowed: 1;
    
    /// Write block misalignment
    UInt8 writeBlockMisalignment: 1;
    
    /// Read block misalignment
    UInt8 readBlockMisalignment: 1;
    
    /// `True` if DSR is implemented
    bool isDSRImplemented: 1;
    
    // ===============
    // | V1 Specific |
    // ===============
    
    /// Reserved 2 bits
    UInt8 reserved2: 2;
    
    /// Device size
    UInt16 deviceSize: 12;
    
    /// Maximum read current @ the minimum VDD
    UInt8 maxReadCurrentAtVDDMin: 3;
    
    /// Maximum read current @ the maximum VDD
    UInt8 maxReadCurrentAtVDDMax: 3;
    
    /// Maximum write current @ the minimum VDD
    UInt8 maxWriteCurrentAtVDDMin: 3;
    
    /// Maximum write current @ the maximum VDD
    UInt8 maxWriteCurrentAtVDDMax: 3;
    
    /// Device size multiplier
    UInt8 deviceSizeMultiplier: 3;
    
    // ===============
    // |    Common   |
    // ===============
    
    /// `True` if erasing a single block is enabled
    bool isEraseSingleBlockEnabled: 1;
    
    /// Erase sector size
    UInt8 eraseSectorSize: 7;
    
    /// Write protect group size
    UInt8 writeProtectGroupSize: 7;
    
    /// `True` if write protect group is enabled
    bool isWriteProtectGroupEnabled: 1;
    
    /// Reserved 2 bits
    UInt8 reserved3: 2;
    
    /// Write speed factor
    UInt8 writeSpeedFactor: 3;
    
    /// Maximum write data block length
    UInt8 maxWriteDataBlockLength: 4;
    
    /// `True` if writing partial blocks is allowed
    bool isPartialBlockWriteAllowed: 1;
    
    /// Reserved 5 bits
    UInt8 reserved4: 5;
    
    /// File format group
    UInt8 fileFormatGroup: 1;
    
    /// Copy flag
    UInt8 copyFlag: 1;
    
    /// Permanent write protection
    bool permanentWriteProtection: 1;
    
    /// Temporary write protection
    bool temporaryWriteProtection: 1;
    
    /// File format
    UInt8 fileFormat: 2;
    
    /// Reserved 2 bits
    UInt8 reserved5: 2;
    
    /// CRC7 checksum and the end bit
    UInt8 end;
};

/// Card specific data (Version 2.0) (16 bytes)
struct PACKED CSDV2
{
    // ===============
    // |    Common   |
    // ===============
    
    /// The structure version
    UInt8 version: 2;
    
    /// Reserved 6 bits
    UInt8 reserved0: 6;
    
    /// Data read access time - 1 (Reserved 1 bit)
    UInt8 reserved1: 1;
    
    /// Data read access time - 1 (Time value)
    UInt8 taacTimeValue: 4;
    
    /// Data read access time - 1 (Time unit)
    UInt8 taacTimeUnit: 3;
    
    /// Data read access time - 2 in clock cycles
    UInt8 nasc;
    
    /// Maximum data transfer rate (Reserved 1 bit)
    UInt8 reservedDTR: 1;
    
    /// Maximum data transfer rate (Transfer rate value)
    UInt8 maxTransferRateValue: 4;
    
    /// Maximum data transfer rate (Transfer rate unit)
    UInt8 maxTransferRateUnit: 3;
    
    /// Card command classes
    UInt16 cardCommandClasses: 12;
    
    /// Maximum read data block length
    UInt16 maxReadDataBlockLength: 4;
    
    /// `True` if reading partial blocks is allowed
    bool isPartialBlockReadAllowed: 1;
    
    /// Write block misalignment
    UInt8 writeBlockMisalignment: 1;
    
    /// Read block misalignment
    UInt8 readBlockMisalignment: 1;
    
    /// `True` if DSR is implemented
    bool isDSRImplemented: 1;
    
    // ===============
    // | V2 Specific |
    // ===============
    
    /// Reserved 6 bits
    UInt8 reserved2: 6;
    
    /// Device size
    UInt32 deviceSize: 22;
    
    /// Reserved 1 bit
    UInt8 reserved21: 1;
    
    // ===============
    // |    Common   |
    // ===============
    
    /// `True` if erasing a single block is enabled
    bool isEraseSingleBlockEnabled: 1;
    
    /// Erase sector size
    UInt8 eraseSectorSize: 7;
    
    /// Write protect group size
    UInt8 writeProtectGroupSize: 7;
    
    /// `True` if write protect group is enabled
    bool isWriteProtectGroupEnabled: 1;
    
    /// Reserved 2 bits
    UInt8 reserved3: 2;
    
    /// Write speed factor
    UInt8 writeSpeedFactor: 3;
    
    /// Maximum write data block length
    UInt8 maxWriteDataBlockLength: 4;
    
    /// `True` if writing partial blocks is allowed
    bool isPartialBlockWriteAllowed: 1;
    
    /// Reserved 5 bits
    UInt8 reserved4: 5;
    
    /// File format group
    UInt8 fileFormatGroup: 1;
    
    /// Copy flag
    UInt8 copyFlag: 1;
    
    /// Permanent write protection
    bool permanentWriteProtection: 1;
    
    /// Temporary write protection
    bool temporaryWriteProtection: 1;
    
    /// File format
    UInt8 fileFormat: 2;
    
    /// Reserved 2 bits
    UInt8 reserved5: 2;
    
    /// CRC7 checksum and the end bit
    UInt8 end;
};

/// Card specific data (Version 3.0) (16 bytes)
struct PACKED CSDV3
{
    // ===============
    // |    Common   |
    // ===============
    
    /// The structure version
    UInt8 version: 2;
    
    /// Reserved 6 bits
    UInt8 reserved0: 6;
    
    /// Data read access time - 1 (Reserved 1 bit)
    UInt8 reserved1: 1;
    
    /// Data read access time - 1 (Time value)
    UInt8 taacTimeValue: 4;
    
    /// Data read access time - 1 (Time unit)
    UInt8 taacTimeUnit: 3;
    
    /// Data read access time - 2 in clock cycles
    UInt8 nasc;
    
    /// Maximum data transfer rate (Reserved 1 bit)
    UInt8 reservedDTR: 1;
    
    /// Maximum data transfer rate (Transfer rate value)
    UInt8 maxTransferRateValue: 4;
    
    /// Maximum data transfer rate (Transfer rate unit)
    UInt8 maxTransferRateUnit: 3;
    
    /// Card command classes
    UInt16 cardCommandClasses: 12;
    
    /// Maximum read data block length
    UInt16 maxReadDataBlockLength: 4;
    
    /// `True` if reading partial blocks is allowed
    bool isPartialBlockReadAllowed: 1;
    
    /// Write block misalignment
    UInt8 writeBlockMisalignment: 1;
    
    /// Read block misalignment
    UInt8 readBlockMisalignment: 1;
    
    /// `True` if DSR is implemented
    bool isDSRImplemented: 1;
    
    // ===============
    // | V3 Specific |
    // ===============
    
    /// Device size
    UInt32 deviceSize: 28;
    
    /// Reserved 1 bit
    UInt8 reserved2: 1;
    
    // ===============
    // |    Common   |
    // ===============
    
    /// `True` if erasing a single block is enabled
    bool isEraseSingleBlockEnabled: 1;
    
    /// Erase sector size
    UInt8 eraseSectorSize: 7;
    
    /// Write protect group size
    UInt8 writeProtectGroupSize: 7;
    
    /// `True` if write protect group is enabled
    bool isWriteProtectGroupEnabled: 1;
    
    /// Reserved 2 bits
    UInt8 reserved3: 2;
    
    /// Write speed factor
    UInt8 writeSpeedFactor: 3;
    
    /// Maximum write data block length
    UInt8 maxWriteDataBlockLength: 4;
    
    /// `True` if writing partial blocks is allowed
    bool isPartialBlockWriteAllowed: 1;
    
    /// Reserved 5 bits
    UInt8 reserved4: 5;
    
    /// File format group
    UInt8 fileFormatGroup: 1;
    
    /// Copy flag
    UInt8 copyFlag: 1;
    
    /// Permanent write protection
    bool permanentWriteProtection: 1;
    
    /// Temporary write protection
    bool temporaryWriteProtection: 1;
    
    /// File format
    UInt8 fileFormat: 2;
    
    /// Reserved 2 bits
    UInt8 reserved5: 2;
    
    /// CRC7 checksum and the end bit
    UInt8 end;
};

/// Card specific data (Raw Data)
union CSDVX
{
    UInt8 data[16];
    CSDV1 v1;
    CSDV2 v2;
    CSDV3 v3;
};

/// Card specific data (Parsed Data)
struct CSD
{
    /// CSD struct version
    enum Version
    {
        k1 = 0,
        k2 = 1,
        k3 = 2,
    };
    
    /// Data read access time in nanoseconds
    UInt32 taacTimeNanosecs;
    
    /// Data read access time in number of clocks
    UInt32 taacTimeClocks;
    
    /// Card command classes
    UInt32 cardCommandClasses;
    
    /// Enumerate all SD command classes
    enum CommandClass
    {
        /// Class 0: Basic protocol functions
        /// CMD0, 1, 2, 3, 4, 7, 9, 10, 12, 13, 15
        kBasic = 1 << 0,
        
        /// Class 1: Stream read commands
        /// CMD11
        kStreamRead = 1 << 1,
        
        /// Class 2: Block read commands
        /// CMD16, 17, 18
        kBlockRead = 1 << 2,
        
        /// Class 3: Stream write commands
        /// CMD20
        kStreamWrite = 1 << 3,
        
        /// Class 4: Block write commands
        /// CMD16, 24, 25, 26, 27
        kBlockWrite = 1 << 4,
        
        /// Class 5: Ability to erase blocks
        /// CMD32, 33, 34, 35, 36, 37, 38, 39
        kErase = 1 << 5,
        
        /// Class 6: Ability to write protect blocks
        /// CMD28, 29, 30
        kWriteProtect = 1 << 6,
        
        /// Class 7: Ability to lock down the card
        /// CMD16, 42
        kLockCard = 1 << 7,
        
        /// Class 8: Application specific commands
        /// CMD55, 56, 57, ACMD*
        kAppSpecific = 1 << 8,
        
        /// Class 9: SD I/O mode
        /// CMD5, 39, 40, 52, 53
        kIOMode = 1 << 9,
        
        /// Class 10: High Speed Switch
        /// CMD6, 11, 34, 35, 36, 37, 50
        kSwitch = 1 << 10,
    };
    
    /// Maximum data transfer rate (in bps or Hz)
    UInt32 maxDataTransferRate;
    
    /// Card capacity (in number of read blocks)
    UInt32 capacity;
    
    /// Read block length is 2^ReadBlockLength
    UInt32 readBlockLength;
    
    /// Write block length
    UInt32 writeBlockLength;
    
    /// Write speed factor
    UInt32 writeSpeedFactor;
    
    /// Erase size in number of sectors
    UInt32 eraseSize;
    
    /// Device size
    UInt32 deviceSize;
    
    /// True if reading a partial block is allowed
    bool canReadPartialBlock;
    
    /// True if reading a block that spreads over more than one physical block is allowed
    bool canReadMisalignedBlock;
    
    /// True if writing a partial is allowed
    bool canWritePartialBlock;
    
    /// True if writing a block that spreads over more than one physical block is allowed
    bool canWriteMisalignedBlock;
    
    /// True if the driver stage register is implemented
    bool isDSRImplemented;
    
    /// `True` if the card is block-addressed (SDHC/SDXC)
    /// @note This field is derived from the CSD struct version.
    bool isBlockAddressed;
    
    /// `True` if the card has extended capacity (SDXC)
    /// @note This field is derived from the device size.
    bool hasExtendedCapacity;
    
    /// Data access time (TAAC): Time unit in nanoseconds
    static constexpr UInt32 kTAACTimeUnits[] =
    {
        1, 10, 100, 1000, 10000, 100000, 1000000, 10000000,
    };

    /// Data access time (TAAC): Time value multipliers (scaled to 10x)
    static constexpr UInt32 kTAACTimeValues[] =
    {
        00, 10, 12, 13, 15, 20, 25, 30, // 1.0 - 3.0x
        35, 40, 45, 50, 55, 60, 70, 80, // 3.5 - 8.0x
    };
    
    /// Maximum data transfer rate (DTR): Rate unit (scaled to 1/10x)
    static constexpr UInt32 kDTRUnits[] =
    {
        10000,      // 100 Kbps
        100000,     //   1 Mbps
        1000000,    //  10 Mbps
        10000000,   // 100 Mbps
        0, 0, 0, 0,
    };

    /// Maximum data transfer rate (DTR): Rate multiplier (scaled to 10x)
    static constexpr UInt32 kDTRValues[] =
    {
        00, 10, 12, 13, 15, 20, 25, 30, // 1.0 - 3.0x
        35, 40, 45, 50, 55, 60, 70, 80, // 3.5 - 8.0x
    };
    
    ///
    /// Decode from the given raw data
    ///
    /// @param data An array of 4 integers encoded in big endian
    /// @param pcsd The parsed card specific data on return
    /// @return `true` on success, `false` otherwise.
    ///
    static bool decode(const UInt32* data, CSD& pcsd)
    {
        UInt32 version = UNSTUFF_BITS(data, 126, 2);
        
        UInt32 unit, value;
        
        switch (version)
        {
            // SDSC cards
            case CSD::Version::k1:
            {
                pcsd.isBlockAddressed = false;
                
                pcsd.hasExtendedCapacity = false;
                
                value = UNSTUFF_BITS(data, 115, 4);
                unit  = UNSTUFF_BITS(data, 112, 3);
                pcsd.taacTimeNanosecs = (kTAACTimeUnits[unit] * kTAACTimeValues[value] + 9) / 10;
                
                pcsd.taacTimeClocks = UNSTUFF_BITS(data, 104, 8) * 100;
                
                value = UNSTUFF_BITS(data, 99, 4);
                unit  = UNSTUFF_BITS(data, 96, 3);
                pcsd.maxDataTransferRate = kDTRUnits[unit] * kDTRValues[value];
                
                pcsd.cardCommandClasses = UNSTUFF_BITS(data, 84, 12);
                
                pcsd.capacity = (1 + UNSTUFF_BITS(data, 62, 12)) << (UNSTUFF_BITS(data, 47, 3) + 2);
                
                pcsd.readBlockLength = UNSTUFF_BITS(data, 80, 4);
                
                pcsd.canReadPartialBlock = UNSTUFF_BITS(data, 79, 1);
                
                pcsd.canWriteMisalignedBlock = UNSTUFF_BITS(data, 78, 1);
                
                pcsd.canReadMisalignedBlock = UNSTUFF_BITS(data, 77, 1);
                
                pcsd.isDSRImplemented = UNSTUFF_BITS(data, 76, 1);
                
                pcsd.writeSpeedFactor = UNSTUFF_BITS(data, 26, 3);
                
                pcsd.writeBlockLength = UNSTUFF_BITS(data, 22, 4);
                
                pcsd.canWritePartialBlock = UNSTUFF_BITS(data, 21, 1);
                
                if (UNSTUFF_BITS(data, 46, 1))
                {
                    pcsd.eraseSize = 1;
                }
                else if (pcsd.writeBlockLength >= 9)
                {
                    pcsd.eraseSize = UNSTUFF_BITS(data, 39, 7) + 1;
                    
                    pcsd.eraseSize <<= (pcsd.writeBlockLength - 9);
                }
                
                break;
            }
               
            // SDHC / SDXC cards
            case CSD::Version::k2:
            {
                pcsd.isBlockAddressed = true;
                
                pcsd.taacTimeNanosecs = 0;
                
                pcsd.taacTimeClocks = 0;
                
                value = UNSTUFF_BITS(data, 99, 4);
                unit  = UNSTUFF_BITS(data, 96, 3);
                pcsd.maxDataTransferRate = kDTRUnits[unit] * kDTRValues[value];
                
                pcsd.cardCommandClasses = UNSTUFF_BITS(data, 84, 12);
                
                pcsd.deviceSize = UNSTUFF_BITS(data, 48, 22);
                
                pcsd.hasExtendedCapacity = pcsd.deviceSize >= 0xFFFF;
                
                pcsd.capacity = (1 + pcsd.deviceSize) << 10;
                
                pcsd.readBlockLength = 9;
                
                pcsd.canReadPartialBlock = false;
                
                pcsd.canWriteMisalignedBlock = false;
                
                pcsd.canReadMisalignedBlock = false;
                
                pcsd.writeSpeedFactor = 4; // Unused
                
                pcsd.writeBlockLength = 9;
                
                pcsd.canWritePartialBlock = false;
                
                pcsd.eraseSize = 1;
                
                break;
            }
                
            case CSD::Version::k3:
            {
                perr("SDUC card is not supported at this moment.");
                
                return false;
            }
                
            default:
            {
                perr("Unrecognized CSD structure version %d.", version);
                
                return false;
            }
        }
        
        return true;
    }
};

/// High speed switch capabilities
struct SwitchCaps
{
    /// The maximum clock frequency for the current bus speed mode
    struct MaxClockFrequencies
    {
        /// The maximum clock frequency for the high speed mode
        UInt32 highSpeedMode;
        
        /// The maximum clock frequency for the ultra high speed mode
        UInt32 ultraHighSpeedMode;
        
    } maxClockFrequencies;
    
    /// Maximum clock frequency for each bus speed mode
    enum MaxClockFrequency: UInt32
    {
        kClockDefaultSpeed = MHz2Hz(25),
        kClockHighSpeed    = MHz2Hz(50),
        kClockUHSSDR12     = MHz2Hz(25),
        kClockUHSSDR25     = MHz2Hz(50),
        kClockUHSDDR50     = MHz2Hz(50),
        kClockUHSSDR50     = MHz2Hz(100),
        kClockUHSSDR104    = MHz2Hz(208),
    };
    
    /// Supported bus modes (SD 3.0+)
    UInt32 sd3BusMode;
    
    /// Enumerates all possible bus speeds
    /// The value is passed to the CMD6
    enum BusSpeed: UInt32
    {
        kSpeedUHSSDR12 = 0,
        kSpeedUHSSDR25 = 1,
        kSpeedUHSSDR50 = 2,
        kSpeedUHSSDR104 = 3,
        kSpeedUHSDDR50 = 4,
        kSpeedHighSpeed = 1,
    };
    
    /// Enumerates all possible bus modes
    /// Use `BitOptions(busMode).contains()` to check whether a mode is supported
    enum BusMode: UInt32
    {
        kModeUHSSDR12   = 1 << BusSpeed::kSpeedUHSSDR12,
        kModeUHSSDR25   = 1 << BusSpeed::kSpeedUHSSDR25,
        kModeUHSSDR50   = 1 << BusSpeed::kSpeedUHSSDR50,
        kModeUHSSDR104  = 1 << BusSpeed::kSpeedUHSSDR104,
        kModeUHSDDR50   = 1 << BusSpeed::kSpeedUHSDDR50,
        kModeHighSpeed  = 1 << BusSpeed::kSpeedHighSpeed,
    };
    
    /// Driver strength (SD 3.0+)
    UInt32 sd3DriverType;
    
    /// Enumerates all possible driver types
    enum DriverType: UInt32
    {
        kTypeB = 0x01,
        kTypeA = 0x02,
        kTypeC = 0x04,
        kTypeD = 0x08,
    };
    
    /// The current limit at the host signal voltage level
    UInt32 sd3MaxCurrent;
    
    /// Enumerates all possible current limits
    /// The value is passed to the CMD6
    enum SetCurrentLimit: UInt32
    {
        kCurrentLimit200mA = 0,
        kCurrentLimit400mA = 1,
        kCurrentLimit600mA = 2,
        kCurrentLimit800mA = 3,
    };
    
    /// Enumerates all possible maximum currents
    /// Use `BitOptions(maxCurrent).contains()` to check whether a current is supported
    enum MaxCurrent
    {
        kMaxCurrent200mA = 1 << SetCurrentLimit::kCurrentLimit200mA,
        kMaxCurrent400mA = 1 << SetCurrentLimit::kCurrentLimit400mA,
        kMaxCurrent600mA = 1 << SetCurrentLimit::kCurrentLimit600mA,
        kMaxCurrent800mA = 1 << SetCurrentLimit::kCurrentLimit800mA,
    };
};

static_assert(sizeof(CID) == 16, "CID should be 16 bytes long.");
static_assert(sizeof(SCR) ==  8, "SCR should be  8 bytes long.");
static_assert(sizeof(SSR) == 64, "SSR should be 64 bytes long.");
static_assert(sizeof(CSDVX) == 16, "CSD X.0 should be 16 bytes long.");
static_assert(sizeof(CSDV1) == 16, "CSD 1.0 should be 16 bytes long.");
static_assert(sizeof(CSDV2) == 16, "CSD 2.0 should be 16 bytes long.");
static_assert(sizeof(CSDV3) == 16, "CSD 3.0 should be 16 bytes long.");

/// Represents a generic SD(SC/HC/XC) card
class IOSDCard: public IOService
{
    //
    // MARK: - Constructors & Destructors
    //
    
    OSDeclareDefaultStructors(IOSDCard);
    
    using super = IOService;
    
    /// The host driver
    IOSDHostDriver* driver;
    
    /// The card identification data
    CID cid;
    
    /// The card specification data
    CSD csd;
    
    /// SD configuration data
    SCR scr;
    
    /// SD status data
    SSR ssr;
    
    /// Switch capabilities
    SwitchCaps switchCaps;
    
    /// The card relative address
    UInt32 rca;
    
    ///
    /// Initialize the card with the given OCR register value
    ///
    /// @param driver The host driver
    /// @param ocr The current operating condition register value
    /// @return `true` on success, `false` otherwise.
    /// @note Port: This function replaces `mmc_sd_init_card()` defined in `sd.c`.
    ///
    bool init(IOSDHostDriver* driver, UInt32 ocr);
    
    ///
    /// [Helper] Initialize the card with Default Speed Mode enabled
    ///
    /// @return `true` on success, `false` otherwise.
    /// @note Port: This function replaces the default speed portion of `mmc_sd_init_card()` defined in `sd.c`.
    ///
    bool initDefaultSpeedMode();
    
    ///
    /// [Helper] Initialize the card with High Speed Mode enabled
    ///
    /// @return `true` on success, `false` otherwise.
    /// @note Port: This function replaces the high speed portion of `mmc_sd_init_card()` defined in `sd.c`.
    ///
    bool initHighSpeedMode();
    
    ///
    /// [Helper] Initialize the card with UHS-I Mode enabled
    ///
    /// @return `true` on success, `false` otherwise.
    /// @note Port: This function replaces `mmc_sd_init_uhs_card()` defined in `sd.c`.
    ///
    bool initUltraHighSpeedMode();
    
    ///
    /// [Helper] Read the card switch capabilities
    ///
    /// @return `true` on success, `false` otherwise.
    /// @note Port: This function replaces `mmc_read_switch()` defined in `sd.c`.
    ///
    bool readSwitchCapabilities();
    
    ///
    /// [Helper] Enable the 4-bit wide bus for data transfer
    ///
    /// @return `true` on success, `false` otherwise.
    ///
    bool enable4BitWideBus();
    
    ///
    /// [Helper] Enable the signal voltage for the UHS-I mode
    ///
    /// @param ocr The current operating condition register value
    /// @return `true` on success, `false` otherwise.
    /// @note Port: This function replaces `mmc_set_uhs_voltage()` defined in `core.c`.
    ///
    bool enableUltraHighSpeedSignalVoltage(UInt32 ocr);
    
    ///
    /// [Helper] Select the bus speed for the UHS-I mode
    ///
    /// @return The bus speed supported by both the card and the host.
    /// @note Port: This function replaces `sd_update_bus_speed_mode()` defined in `sd.c`.
    ///
    SwitchCaps::BusSpeed selectUltraHighSpeedBusSpeed();
    
    
    
    
    
    
    bool setUHSDriverType(SwitchCaps::BusSpeed busSpeed);
    
    
    
    
    ///
    /// [Helper] Set the current limit for the UHS-I card
    ///
    /// @param busSpeed The bus speed
    /// @return `true` on success, `false` otherwise.
    /// @note Port: This function replaces `sd_set_current_limit()` defined in `sd.c`.
    /// @note This function is a part of the UHS-I card initialization routine `initUltraHighSpeedMode()`.
    ///
    bool setUHSCurrentLimit(SwitchCaps::BusSpeed busSpeed);
    
    ///
    /// [Helper] Set the bus speed for the UHS-I card
    ///
    /// @param busSpeed The bus speed
    /// @return `true` on success, `false` otherwise.
    /// @note Port: This function replaces `sd_set_bus_speed_mode()` defined in `sd.c`.
    /// @note This function is a part of the UHS-I card initialization routine `initUltraHighSpeedMode()`.
    ///
    bool setUHSBusSpeedMode(SwitchCaps::BusSpeed busSpeed);
    
public:
    /// Get the card identification data
    inline const CID& getCID()
    {
        return this->cid;
    }
    
    /// Get the card specification data
    inline const CSD& getCSD()
    {
        return this->csd;
    }
    
    /// Get the SD configuration data
    inline const SCR& getSCR()
    {
        return this->scr;
    }
    
    /// Get the SD status data
    inline const SSR& getSSR()
    {
        return this->ssr;
    }
    
    /// Get the card relative address
    inline UInt32 getRCA()
    {
        return this->rca;
    }
    
    /// Get the card type
    inline const char* getCardType()
    {
        if (!this->csd.isBlockAddressed)
        {
            return "SDSC";
        }
        
        if (!this->csd.hasExtendedCapacity)
        {
            return "SDHC";
        }
        else
        {
            return "SDXC";
        }
    }
    
    /// Check whether the card matches the given specification version
    inline bool matchesSpecificationLevel(const SPEC& spec)
    {
        return this->scr.spec  == spec.spec  &&
               this->scr.spec3 == spec.spec3 &&
               this->scr.spec4 == spec.spec4 &&
               this->scr.spec5 == spec.spec5;
    }
    
    /// Get the specification version string
    inline const char* getSpecificationVersion()
    {
        // SPEC Fields
        static constexpr SPEC kSpecTable[] =
        {
            { 0, 0, 0, 0 }, // Version 1.0 and 1.01
            { 1, 0, 0, 0 }, // Version 1.10
            { 2, 0, 0, 0 }, // Version 2.00
            { 2, 1, 0, 0 }, // Version 3.0X
            { 2, 1, 1, 0 }, // Version 4.XX
            { 2, 1, 0, 1 }, // Version 5.XX
            { 2, 1, 1, 1 }, // Version 5.XX
            { 2, 1, 0, 2 }, // Version 6.XX
            { 2, 1, 1, 2 }, // Version 6.XX
            { 2, 1, 0, 3 }, // Version 7.XX
            { 2, 1, 1, 3 }, // Version 7.XX
            { 2, 1, 0, 4 }, // Version 8.XX
            { 2, 1, 1, 4 }, // Version 8.XX
        };
        
        // SPEC Version Strings
        static constexpr const char* kSpecString[] =
        {
            "1.00",
            "1.10",
            "2.00",
            "3.00",
            "4.00",
            "5.00",
            "5.00",
            "6.00",
            "6.00",
            "7.00",
            "7.00",
            "8.00",
            "8.00",
        };
        
        static_assert(arrsize(kSpecTable) == arrsize(kSpecString), "Inconsistent SPEC tables.");
        
        for (auto index = 0; index < arrsize(kSpecTable); index += 1)
        {
            if (this->matchesSpecificationLevel(kSpecTable[index]))
            {
                return kSpecString[index];
            }
        }
        
        return "Unknown";
    }
    
    ///
    /// Get the card product name
    ///
    /// @param name A non-null buffer that can hold at least 8 bytes.
    /// @param length The buffer length
    /// @return `true` on success, `false` if the buffer is NULL or too small.
    ///
    inline bool getCardName(char* name, IOByteCount length)
    {
        if (name == nullptr || length < 8)
        {
            return false;
        }
        
        bzero(name, length);
        
        memcpy(name, this->cid.name, sizeof(this->cid.name));
        
        return true;
    }
    
    ///
    /// Get the card revision value
    ///
    /// @param revision A non-null buffer that can hold at least 8 bytes.
    /// @param length The buffer length
    /// @return `true` on success, `false` if the buffer is NULL or too small.
    ///
    inline bool getCardRevision(char* revision, IOByteCount length)
    {
        if (revision == nullptr || length < 8)
        {
            return false;
        }
        
        bzero(revision, length);
        
        snprintf(revision, length, "%d.%d", this->cid.hwrevision, this->cid.fwrevision);
        
        return true;
    }
    
    ///
    /// Get the card manufacture date
    ///
    /// @param date A non-null buffer that can hold at least 8 bytes.
    /// @param length The buffer length
    /// @return `true` on success, `false` if the buffer is NULL or too small.
    ///
    inline bool getCardProductionDate(char* date, IOByteCount length)
    {
        if (date == nullptr || length < 8)
        {
            return false;
        }
        
        bzero(date, length);
        
        snprintf(date, length, "%04d.%02d", this->cid.getYear(), this->cid.month);
        
        return true;
    }
    
    ///
    /// Get the card characteristics
    ///
    /// @return A dictionary that contains card characteristics which can be recognized by the System Profiler.
    /// @note The caller is responsible for releasing the returned dictionary.
    ///
    OSDictionaryPtr getCardCharacteristics();
    
    ///
    /// Create the card and initialize it with the given OCR value
    ///
    /// @param driver The non-null host driver
    /// @param ocr The OCR value that contains the voltage level supported by the host and the card
    /// @return A non-null card instance on success, `nullptr` otherwise.
    ///
    static IOSDCard* createWithOCR(IOSDHostDriver* driver, UInt32 ocr);
    
    ///
    /// Type of a completion routine that is called once an asynchronous card event is processed by the host driver
    ///
    /// @param target An opaque client-supplied pointer (or the instance pointer for a C++ callback)
    /// @param parameter An opaque client-supplied parameter pointer
    /// @param status `kIOReturnSuccess` if the card event has been processed without errors, other values otherwise.
    /// @param characteristics A non-null dictionary that contains characteristics of the card inserted and initialized successfully,
    ///                        `nullptr` if the card inserted by users cannot be initialized or has been removed from the card slot.
    ///
    using CompletionAction = void (*)(void* target, void* parameter, IOReturn status, OSDictionary* characteristics);
    
    ///
    /// Describe the completion routine to be called when the host driver has processed an asynchronous card event
    ///
    struct Completion
    {
    private:
        /// An opaque client-supplied pointer (or the instance pointer for a C++ callback)
        void* target;
        
        /// A non-null completion routine
        CompletionAction action;
        
        /// An opaque client-supplied parameter pointer
        void* parameter;
        
    public:
        ///
        /// Default constructor
        ///
        Completion() = default;
        
        ///
        /// Create a completion routine
        ///
        Completion(void* target, CompletionAction action, void* parameter = nullptr) :
            target(target), action(action), parameter(parameter) {}
        
        ///
        /// Invoke the completion routine
        ///
        /// @param status Pass `kIOReturnSuccess` if the card event has been processed without errors, other values otherwise.
        /// @param characteristics A non-null dictionary that contains characteristics of the card inserted and initialized successfully,
        ///                        `nullptr` if the card inserted by users cannot be initialized or has been removed from the card slot.
        /// @warning This function prints a warning message if the action routine is null.
        ///
        inline void invoke(IOReturn status, OSDictionary* characteristics)
        {
            if (this->action != nullptr)
            {
                (*this->action)(this->target, this->parameter, status, characteristics);
            }
            else
            {
                pwarning("The action routine is null.");
            }
        }
        
        ///
        /// Reset the completion routine
        ///
        inline void reset()
        {
            this->target = nullptr;
            
            this->action = nullptr;
            
            this->parameter = nullptr;
        }
        
        ///
        /// Create a completion descriptor with the given member function
        ///
        /// @param self The instance pointer for the given C++ callback function
        /// @param function A C++ callback function
        /// @param parameter An optional opaque parameter pointer
        /// @return The completion routine.
        ///
        template <typename Function>
        static Completion withMemberFunction(OSObject* self, Function function, void* parameter = nullptr)
        {
            return { self, OSMemberFunctionCast(CompletionAction, self, function), parameter };
        }
    };
    
    ///
    /// Invoke the given completion routine
    ///
    /// @param completion A nullable completion routine
    /// @param status Pass `kIOReturnSuccess` if the card event has been processed without errors, other values otherwise.
    /// @param characteristics A non-null dictionary that contains characteristics of the card inserted and initialized successfully,
    ///                        `nullptr` if the card inserted by users cannot be initialized or has been removed from the card slot.
    /// @note This function is a noop if the given completion is null.
    ///
    static inline void complete(Completion* completion, IOReturn status, OSDictionary* characteristics = nullptr)
    {
        if (completion != nullptr)
        {
            completion->invoke(status, characteristics);
        }
    }
};

#endif /* IOSDCard_hpp */
