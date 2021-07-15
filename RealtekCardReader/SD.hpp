//
//  SD.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 5/27/21.
//

#ifndef SD_hpp
#define SD_hpp

#include <libkern/OSTypes.h>
#include "Utilities.hpp"

// TODO: TO C++
/*
  MMC status in R1, for native mode (SPI bits are different)
  Type
    e : error bit
    s : status bit
    r : detected and set for the actual command response
    x : detected and set during command execution. the host must poll
            the card by sending status command in order to read these bits.
  Clear condition
    a : according to the card state
    b : always related to the previous command. Reception of
            a valid command will clear it (with a delay of one command)
    c : clear by read
 */

// TODO: Convert to C++
#define R1_OUT_OF_RANGE        (1 << 31)    /* er, c */
#define R1_ADDRESS_ERROR    (1 << 30)    /* erx, c */
#define R1_BLOCK_LEN_ERROR    (1 << 29)    /* er, c */
#define R1_ERASE_SEQ_ERROR      (1 << 28)    /* er, c */
#define R1_ERASE_PARAM        (1 << 27)    /* ex, c */
#define R1_WP_VIOLATION        (1 << 26)    /* erx, c */
#define R1_CARD_IS_LOCKED    (1 << 25)    /* sx, a */
#define R1_LOCK_UNLOCK_FAILED    (1 << 24)    /* erx, c */
#define R1_COM_CRC_ERROR    (1 << 23)    /* er, b */
#define R1_ILLEGAL_COMMAND    (1 << 22)    /* er, b */
#define R1_CARD_ECC_FAILED    (1 << 21)    /* ex, c */
#define R1_CC_ERROR        (1 << 20)    /* erx, c */
#define R1_ERROR        (1 << 19)    /* erx, c */
#define R1_UNDERRUN        (1 << 18)    /* ex, c */
#define R1_OVERRUN        (1 << 17)    /* ex, c */
#define R1_CID_CSD_OVERWRITE    (1 << 16)    /* erx, c, CID/CSD overwrite */
#define R1_WP_ERASE_SKIP    (1 << 15)    /* sx, c */
#define R1_CARD_ECC_DISABLED    (1 << 14)    /* sx, a */
#define R1_ERASE_RESET        (1 << 13)    /* sr, c */
#define R1_STATUS(x)            (x & 0xFFF9A000)
#define R1_CURRENT_STATE(x)    ((x & 0x00001E00) >> 9)    /* sx, b (4 bits) */
#define R1_READY_FOR_DATA    (1 << 8)    /* sx, a */
#define R1_SWITCH_ERROR        (1 << 7)    /* sx, c */
#define R1_EXCEPTION_EVENT    (1 << 6)    /* sr, a */
#define R1_APP_CMD        (1 << 5)    /* sr, c */

/// Represents the 48-bit R1 response
struct PACKED SDResponse1 
{
private:
    /// The start bit (must be 0)
    UInt8 start: 1;
    
    /// The transmission bit (must be 0)
    UInt8 transmission: 1;
    
    /// The command index
    UInt8 command: 6;
    
    /// The card status (little endian)
    /// @warning The card status in the response buffer is encoded in big endian.
    ///          The caller should use `getStatus()` to fetch the correct value.
    UInt32 status;
    
    /// The CRC7 value
    UInt8 crc7: 7;
    
    /// The end bit (must be 1)
    UInt8 end: 1;

public:
    /// Get the card status
    inline UInt32 getStatus() const
    {
        return OSSwapInt32(this->status);
    }
};

/// Represents the 48-bit R1b response
struct PACKED SDResponse1b
{
private:
    /// The start bit (must be 0)
    UInt8 start: 1;
    
    /// The transmission bit (must be 0)
    UInt8 transmission: 1;
    
    /// The command index
    UInt8 command: 6;
    
    /// The card status (little endian)
    /// @warning The card status in the response buffer is encoded in big endian.
    ///          The caller should use `getStatus()` to fetch the correct value.
    UInt32 status;
    
    /// The CRC7 value
    UInt8 crc7: 7;
    
    /// The end bit (must be 1)
    UInt8 end: 1;
    
public:
    /// Get the card status
    inline UInt32 getStatus() const
    {
        return OSSwapInt32(this->status);
    }
};

/// Represents the 136-bit R2 response
struct PACKED SDResponse2
{
    /// The start bit (must be 0)
    UInt8 start: 1;
    
    /// The transmission bit (must be 0)
    UInt8 transmission: 1;
    
    /// The reserved 6 bits (all set to 1)
    UInt8 reserved: 6;
    
    /// The register value including the CRC7 value and the end bit
    UInt8 value[16];
};

/// Represents the 48-bit R3 response
struct PACKED SDResponse3
{
private:
    /// The start bit (must be 0)
    UInt8 start: 1;
    
    /// The transmission bit (must be 0)
    UInt8 transmission: 1;
    
    /// The reserved 6 bits (all set to 1)
    UInt8 reserved: 6;
    
    /// The register value (little endian)
    /// @warning The OCR value in the response buffer is encoded in big endian.
    ///          The caller should use `getValue()` to fetch the correct value.
    UInt32 value;
    
    /// The reserved 7 bits and the end bit (all set to 1)
    UInt8 end;
    
public:
    /// Get the register value
    inline UInt32 getValue() const
    {
        return OSSwapInt32(this->value);
    }
};

/// Represents the 48-bit R6 response
struct PACKED SDResponse6
{
private:
    /// The start bit (must be 0)
    UInt8 start: 1;
    
    /// The transmission bit (must be 0)
    UInt8 transmission: 1;
    
    /// The command index
    UInt8 command: 6;
    
    /// The RCA value (little endian)
    UInt16 rca;
    
    /// The card status (Bits 23, 22, 19, 12:0)
    UInt16 status;
    
    /// The CRC7 value
    UInt8 crc7: 7;
    
    /// The end bit (must be 1)
    UInt8 end: 1;
    
public:
    /// Get the card relative address
    inline UInt16 getRCA() const
    {
        return OSSwapInt16(this->rca);
    }
};

/// Represents the 48-bit R7 response
struct PACKED SDResponse7
{
    /// The start bit (must be 0)
    UInt8 start: 1;
    
    /// The transmission bit (must be 0)
    UInt8 transmission: 1;
    
    /// The command index
    UInt8 command: 6;
    
    /// The reserved 18 bits (all set to 0)
    UInt32 reserved: 18;
    
    /// Indicate whether the card supports VDD3 (PCIe 1.2V Support)
    bool supports1d2V: 1;
    
    /// Indicate whether the card responds PCIe acceptance
    bool acceptsPCIe: 1;
    
    ///
    /// Indicate the range of voltages that the card supports
    ///
    /// @note Possible values: 0b0000: Not defined
    ///                        0b0001: 2.7V - 3.6V
    ///                        0b0010: Reserved for Low Voltage Range
    ///                        0b0100: Reserved
    ///                        0b1000: Reserved
    ///                        Others: Not defined
    ///
    UInt8 supportedVoltages: 4;
    
    /// Echo-back of the check pattern
    UInt8 checkPattern;
    
    /// The CRC7 value
    UInt8 crc7: 7;
    
    /// The end bit (must be 1)
    UInt8 end: 1;
};

static_assert(sizeof(SDResponse1) == 06, "ABI Error: R1 must be 48-bit long.");
static_assert(sizeof(SDResponse2) == 17, "ABI Error: R2 must be 136-bit long.");
static_assert(sizeof(SDResponse3) == 06, "ABI Error: R3 must be 48-bit long.");
static_assert(sizeof(SDResponse6) == 06, "ABI Error: R6 must be 48-bit long.");
static_assert(sizeof(SDResponse7) == 06, "ABI Error: R7 must be 48-bit long.");

#endif /* SD_hpp */
