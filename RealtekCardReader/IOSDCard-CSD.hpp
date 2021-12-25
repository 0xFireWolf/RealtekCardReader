//
//  IOSDCard-CSD.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 12/24/21.
//

#ifndef IOSDCard_CSD_hpp
#define IOSDCard_CSD_hpp

#include "Utilities.hpp"

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

static_assert(sizeof(CSDVX) == 16, "CSD X.0 should be 16 bytes long.");
static_assert(sizeof(CSDV1) == 16, "CSD 1.0 should be 16 bytes long.");
static_assert(sizeof(CSDV2) == 16, "CSD 2.0 should be 16 bytes long.");
static_assert(sizeof(CSDV3) == 16, "CSD 3.0 should be 16 bytes long.");

#endif /* IOSDCard_CSD_hpp */
