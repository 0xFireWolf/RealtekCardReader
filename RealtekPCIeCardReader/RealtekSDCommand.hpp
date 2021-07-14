//
//  RealtekSDCommand.hpp
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 5/30/21.
//

#ifndef RealtekSDCommand_hpp
#define RealtekSDCommand_hpp

#include "RealtekCommonRegisters.hpp"
#include "BitOptions.hpp"
#include "Debug.hpp"

///
/// Represents a SD command
///
struct RealtekSDCommand
{
private:
    /// The command index
    UInt32 opcode;
    
    /// The command argument
    UInt32 argument;
    
    /// The amount of time in ms to detect the busy bit until timed out
    UInt32 busyTimeout;
    
    ///
    /// The response buffer
    ///
    /// @note Type R0: Length = 00; The first byte is the value of `SD_STAT1`.
    ///       Type R2: Length = 17; The last byte is the value of `SD_STAT1` instead of the CRC7 checksum and the end bit.
    ///        Others: Length = 06; The last byte is the value of `SD_STAT1` instead of the CRC7 checksum and the end bit.
    ///
    UInt8 response[17] = {};
    
    /// The response type that will be passed to the `SD_CFG2` register
    UInt8 responseType;
    
public:    
    //
    // MARK: - Constructors
    //
    
    RealtekSDCommand(UInt32 opcode, UInt32 argument, UInt32 busyTimeout, UInt32 responseType)
        : opcode(opcode), argument(argument), busyTimeout(busyTimeout), response(), responseType(responseType) {}
    
    //
    // MARK: - Get Command Properties
    //
    
    ///
    /// Get the command opcode
    ///
    /// @return The command opcode.
    ///
    inline UInt32 getOpcode() const
    {
        return this->opcode;
    }
    
    ///
    /// Get the opcode with the start and the transmission bits set properly
    ///
    /// @return The opcode with the start bit cleared and the transmission bit set.
    ///
    inline UInt8 getOpcodeWithStartAndTransmissionBits() const
    {
        return 0x40 | static_cast<UInt8>(this->opcode);
    }
    
    ///
    /// Get the command argument
    ///
    /// @return The command argument.
    ///
    inline UInt32 getArgument() const
    {
        return this->argument;
    }
    
    ///
    /// Get the amount of time in ms to detect the busy bit until timed out
    ///
    /// @param defaultTimeout If the amount of time is not defined (i.e. 0),
    ///                       use the value specified by this parameter instead.
    ///
    inline UInt32 getBusyTimeout(UInt32 defaultTimeout) const
    {
        return this->busyTimeout != 0 ? this->busyTimeout : defaultTimeout;
    }
    
    ///
    /// Get the storage for the command response
    ///
    /// @return A non-null pointer to the response buffer.
    ///
    inline UInt8* getResponseBuffer()
    {
        return this->response;
    }
    
    ///
    /// Reinterpret the command response as the specified type
    ///
    /// @return A non-null pointer to the response of the specified type.
    ///
    template <typename Response>
    const Response* reinterpretResponseAs() const
    {
        return reinterpret_cast<const Response*>(this->response);
    }
    
    ///
    /// Get the response type
    ///
    /// @return The reponse type.
    ///
    inline UInt8 getResponseType() const
    {
        return this->responseType;
    }
    
    ///
    /// Get the response length in bytes
    ///
    /// @return The number of bytes in the response.
    ///
    inline IOByteCount getResponseLength() const
    {
        using namespace RTSX::COM::Chip::SD::CFG2;
        
        BitOptions type = this->responseType;
        
        if (type.contains(kResponseLength17))
        {
            return 17;
        }
        else if (type.contains(kResponseLength6))
        {
            return 6;
        }
        else if (type.contains(kResponseLength0))
        {
            // Note that we reserve 1 byte for the value of `SD_STAT1`
            return 1;
        }
        else
        {
            pfatal("Cannot detected the response length. Type = 0x%x.", type.flatten());
        }
    }
    
    ///
    /// Check whether the start and the transmission bits in the response are valid or not
    ///
    /// @return `true` if both bits are valid, `false` otherwise.
    ///
    inline bool verifyStartAndTransmissionBitsInResponse() const
    {
        using namespace RTSX::COM::Chip::SD::CFG2;
        
        // Guard: Check whether the response contains the ST bits
        if (this->responseType == kResponseTypeR0)
        {
            return true;
        }
        else
        {
            return (this->response[0] & 0xC0) == 0;
        }
    }
    
    ///
    /// Check whether the CRC7 checksum is valid
    ///
    /// @return `true` if the checksum is valid, `false` otherwise.
    /// @note Since the last byte in the actual response is the value of the register `SD_STAT1`,
    ///       this function checks the register value instead of calculating the CRC7 checksum manually.
    ///
    inline bool verifyCRC7InResponse() const
    {
        using namespace RTSX::COM::Chip::SD;
        
        BitOptions<UInt8> type = this->responseType;
        
        // Guard: Check whether the driver should ignore the CRC7 checksum
        if (type.contains(CFG2::kNoCheckCRC7))
        {
            return true;
        }
        else
        {
            return (this->response[this->getResponseLength() - 1] & STAT1::kCRC7Error) == 0;
        }
    }
    
    ///
    /// Print the response in bytes
    ///
    inline void printResponse() const
    {
        pinfo("Command Response:");
        
        pbufcol(this->response, sizeof(this->response));
    }
    
    //
    // MARK: - Command Factory
    //
    
    /// Enumerates common SD command opcodes
    enum Opcode: UInt32
    {
        kGoIdleState = 0,
        kAllSendCID = 2,
        kSendRelativeAddress = 3,
        kSwitchFunction = 6,
        kSelectCard = 7,
        kSendIfCond = 8,
        kSendCSD = 9,
        kVoltageSwitch = 11,
        kStopTransmission = 12,
        kSendStatus = 13,
        kReadSingleBlock = 17,
        kReadMultipleBlocks = 18,
        kSendTuningBlock = 19,
        kWriteSingleBlock = 24,
        kWriteMultipleBlocks = 25,
        kAppCommand = 55,
    };
    
    /// Enumerates common SD application command opcodes
    enum AppOpCode: UInt32
    {
        kSetBusWidth = 6,
        kSDStatus = 13,
        kSetEraseCount = 23,
        kSendOpCond = 41,
        kSendSCR = 51,
    };

    static inline RealtekSDCommand CMD0()
    {
        return RealtekSDCommand(kGoIdleState, 0, 0, RTSX::COM::Chip::SD::CFG2::kResponseTypeR0);
    }
    
    static inline RealtekSDCommand CMD2()
    {
        return RealtekSDCommand(kAllSendCID, 0, 0, RTSX::COM::Chip::SD::CFG2::kResponseTypeR2);
    }
    
    static inline RealtekSDCommand CMD3()
    {
        return RealtekSDCommand(kSendRelativeAddress, 0, 0, RTSX::COM::Chip::SD::CFG2::kResponseTypeR6);
    }
    
    static inline RealtekSDCommand CMD6(UInt32 mode, UInt32 group, UInt32 value)
    {
        UInt32 argument = mode << 31 | 0x00FFFFFF;
        
        argument &= ~(0xF << (group * 4));
        
        argument |= value << (group * 4);
        
        return RealtekSDCommand(kSwitchFunction, argument, 0, RTSX::COM::Chip::SD::CFG2::kResponseTypeR1);
    }
    
    static inline RealtekSDCommand CMD7(UInt32 rca)
    {
        return RealtekSDCommand(kSelectCard, rca << 16, 0, rca == 0 ? RTSX::COM::Chip::SD::CFG2::kResponseTypeR0 : RTSX::COM::Chip::SD::CFG2::kResponseTypeR1);
    }
    
    static inline RealtekSDCommand CMD8(UInt8 vhs, UInt8 checkPattern)
    {
        return { kSendIfCond, static_cast<UInt32>(vhs << 8 | checkPattern), 0, RTSX::COM::Chip::SD::CFG2::kResponseTypeR7 };
    }
    
    static inline RealtekSDCommand CMD9(UInt32 rca)
    {
        return RealtekSDCommand(kSendCSD, rca << 16, 0, RTSX::COM::Chip::SD::CFG2::kResponseTypeR2);
    }
    
    static inline RealtekSDCommand CMD11()
    {
        return RealtekSDCommand(kVoltageSwitch, 0, 0, RTSX::COM::Chip::SD::CFG2::kResponseTypeR1);
    }
    
    static inline RealtekSDCommand CMD12()
    {
        return RealtekSDCommand(kStopTransmission, 0, 0, RTSX::COM::Chip::SD::CFG2::kResponseTypeR1);
    }
    
    static inline RealtekSDCommand CMD12b()
    {
        return RealtekSDCommand(kStopTransmission, 0, 0, RTSX::COM::Chip::SD::CFG2::kResponseTypeR1b);
    }
    
    static inline RealtekSDCommand CMD13(UInt32 rca)
    {
        return RealtekSDCommand(kSendStatus, rca << 16, 0, RTSX::COM::Chip::SD::CFG2::kResponseTypeR1);
    }
    
    static inline RealtekSDCommand CMD17(UInt32 offset)
    {
        return RealtekSDCommand(kReadSingleBlock, offset, 0, RTSX::COM::Chip::SD::CFG2::kResponseTypeR1);
    }
    
    static inline RealtekSDCommand CMD18(UInt32 offset)
    {
        return RealtekSDCommand(kReadMultipleBlocks, offset, 0, RTSX::COM::Chip::SD::CFG2::kResponseTypeR1);
    }
    
    static inline RealtekSDCommand CMD19()
    {
        return RealtekSDCommand(kSendTuningBlock, 0, 0, RTSX::COM::Chip::SD::CFG2::kResponseTypeR1);
    }
    
    static inline RealtekSDCommand CMD24(UInt32 offset)
    {
        return RealtekSDCommand(kWriteSingleBlock, offset, 0, RTSX::COM::Chip::SD::CFG2::kResponseTypeR1);
    }
    
    static inline RealtekSDCommand CMD25(UInt32 offset)
    {
        return RealtekSDCommand(kWriteMultipleBlocks, offset, 0, RTSX::COM::Chip::SD::CFG2::kResponseTypeR1);
    }
    
    static inline RealtekSDCommand CMD55(UInt32 rca)
    {
        return RealtekSDCommand(kAppCommand, rca << 16, 0, RTSX::COM::Chip::SD::CFG2::kResponseTypeR1);
    }
    
    static inline RealtekSDCommand ACMD6(UInt32 busWidth)
    {
        return RealtekSDCommand(kSetBusWidth, busWidth, 0, RTSX::COM::Chip::SD::CFG2::kResponseTypeR1);
    }
    
    static inline RealtekSDCommand ACMD13()
    {
        return RealtekSDCommand(kSDStatus, 0, 0, RTSX::COM::Chip::SD::CFG2::kResponseTypeR1);
    }
    
    static inline RealtekSDCommand ACMD23(UInt32 nblocks)
    {
        return RealtekSDCommand(kSetEraseCount, nblocks, 0, RTSX::COM::Chip::SD::CFG2::kResponseTypeR1);
    }
    
    static inline RealtekSDCommand ACMD41(UInt32 ocr)
    {
        return RealtekSDCommand(kSendOpCond, ocr, 0, RTSX::COM::Chip::SD::CFG2::kResponseTypeR3);
    }
    
    static inline RealtekSDCommand ACMD51()
    {
        return RealtekSDCommand(kSendSCR, 0, 0, RTSX::COM::Chip::SD::CFG2::kResponseTypeR1);
    }
};

#endif /* RealtekSDCommand_hpp */
