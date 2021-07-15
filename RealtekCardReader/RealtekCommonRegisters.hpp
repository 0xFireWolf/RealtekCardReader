//
//  RealtekCommonRegisters.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 7/11/21.
//

#ifndef RealtekCommonRegisters_hpp
#define RealtekCommonRegisters_hpp

#include <libkern/OSTypes.h>

// A trick to share common registers across different namespaces
// PCIe-based controllers use the namespace `RTSX::PCR::Chip` to access the following common registers,
// while USB-based controllers use the namespace `RTSX::UCR::Chip` instead.
// Controller-independent classes should use the namespace `RTSX::COM::Chip`.
#if defined(RTSX_REG_NAMESPACE_UCR)
    #define NAMESPACE UCR
#elif defined(RTSX_REG_NAMESPACE_PCR)
    #define NAMESPACE PCR
#else
    #define NAMESPACE COM
#endif

//
// MARK: - 0. Macros
//

#define RTSXDeclareChipRegister(name, address) \
    static constexpr UInt16 name = address;

#define RTSXDeclareChipRegisterValue(name, value) \
    static constexpr UInt8 name = value;

//
// MARK: - 1. Shared Chip Registers & Values
//

/// Chip registers that control the ping pong buffer
namespace RTSX::NAMESPACE::Chip::PPBUF
{
    RTSXDeclareChipRegister(rBASE1, 0xF800);
    RTSXDeclareChipRegister(rBASE2, 0xFA00);
}

/// Chip registers that controls the power of SSC and OCP
namespace RTSX::NAMESPACE::Chip
{
    RTSXDeclareChipRegister(rFPDCTL, 0xFC00);
    
    namespace FPDCTL
    {
        /// Bit 0 is set to power down SSC
        RTSXDeclareChipRegisterValue(kSSCPowerMask, 0x01);
        RTSXDeclareChipRegisterValue(kSSCPowerUpValue, 0x00);
        RTSXDeclareChipRegisterValue(kSSCPowerDownValue, 0x01);
        
        /// Bit 1 is set to power down overcurrent protection
        RTSXDeclareChipRegisterValue(kOCPPowerMask, 0x02);
        RTSXDeclareChipRegisterValue(kOCPPowerUpValue, 0x00);
        RTSXDeclareChipRegisterValue(kOCPPowerDownValue, 0x02);
        
        /// Bit 0 and 1 are set to power down SSC and OCP
        RTSXDeclareChipRegisterValue(kAllPowerMask, 0x03);
        RTSXDeclareChipRegisterValue(kAllPowerUpValue, 0x00);
        RTSXDeclareChipRegisterValue(kAllPowerDownValue, 0x03);
    }
}

namespace RTSX::NAMESPACE::Chip::SD
{
    RTSXDeclareChipRegister(rVPCLK0CTL, 0xFC2A);
    RTSXDeclareChipRegister(rVPCLK1CTL, 0xFC2B);
    RTSXDeclareChipRegister(rVPTXCTL, rVPCLK0CTL);
    RTSXDeclareChipRegister(rVPRXCTL, rVPCLK1CTL);
    namespace VPCTL
    {
        RTSXDeclareChipRegisterValue(kPhaseSelectMask, 0x1F);
        RTSXDeclareChipRegisterValue(kPhaseSelectMask32, 0x1F);
        RTSXDeclareChipRegisterValue(kPhaseSelectMask16, 0x0F);
        RTSXDeclareChipRegisterValue(kPhaseChange, 0x80);
        RTSXDeclareChipRegisterValue(kPhaseNotReset, 0x40);
    }
    
    /// Clock divider, bus mode and width
    RTSXDeclareChipRegister(rCFG1, 0xFDA0);
    namespace CFG1
    {
        RTSXDeclareChipRegisterValue(kClockDivider0, 0x00);
        RTSXDeclareChipRegisterValue(kClockDivider128, 0x80);
        RTSXDeclareChipRegisterValue(kClockDivider256, 0xC0);
        RTSXDeclareChipRegisterValue(kClockDividerMask, 0xC0);
        
        RTSXDeclareChipRegisterValue(kBusWidth1Bit, 0x00);
        RTSXDeclareChipRegisterValue(kBusWidth4Bit, 0x01);
        RTSXDeclareChipRegisterValue(kBusWidth8Bit, 0x02);
        RTSXDeclareChipRegisterValue(kBusWidthMask, 0x03);
        
        RTSXDeclareChipRegisterValue(kModeSD20, 0x00);
        RTSXDeclareChipRegisterValue(kModeSDDDR, 0x04);
        RTSXDeclareChipRegisterValue(kModeSD30, 0x08);
        RTSXDeclareChipRegisterValue(kModeMask, 0x0C);
        
        RTSXDeclareChipRegisterValue(kAsyncFIFONotRST, 0x10);
    }
    
    /// SD command control and response
    RTSXDeclareChipRegister(rCFG2, 0xFDA1);
    namespace CFG2
    {
        RTSXDeclareChipRegisterValue(kCalcCRC7, 0x00);
        RTSXDeclareChipRegisterValue(kNoCalcCRC7, 0x80);
        
        RTSXDeclareChipRegisterValue(kCheckCRC16, 0x00);
        RTSXDeclareChipRegisterValue(kNoCheckCRC16, 0x40);
        RTSXDeclareChipRegisterValue(kNoCheckWaitCRCTo, 0x20);
        
        RTSXDeclareChipRegisterValue(kWaitBusyEnd, 0x08);
        RTSXDeclareChipRegisterValue(kNoWaitBusyEnd, 0x00);
        
        RTSXDeclareChipRegisterValue(kCheckCRC7, 0x00);
        RTSXDeclareChipRegisterValue(kNoCheckCRC7, 0x04);
        
        RTSXDeclareChipRegisterValue(kResponseLength0, 0x00);   // No response
        RTSXDeclareChipRegisterValue(kResponseLength6, 0x01);   // 48-bit response
        RTSXDeclareChipRegisterValue(kResponseLength17, 0x02);  // 136-bit response
        
        RTSXDeclareChipRegisterValue(kResponseTypeR0, 0x04);
        RTSXDeclareChipRegisterValue(kResponseTypeR1, 0x01);
        RTSXDeclareChipRegisterValue(kResponseTypeR1b, 0x09);
        RTSXDeclareChipRegisterValue(kResponseTypeR2, 0x02);
        RTSXDeclareChipRegisterValue(kResponseTypeR3, 0x05);
        RTSXDeclareChipRegisterValue(kResponseTypeR4, 0x05);
        RTSXDeclareChipRegisterValue(kResponseTypeR5, 0x01);
        RTSXDeclareChipRegisterValue(kResponseTypeR6, 0x01);
        RTSXDeclareChipRegisterValue(kResponseTypeR7, 0x01);
    }
    
    /// Used by the RTSX PCI driver only
    RTSXDeclareChipRegister(rCFG3, 0xFDA2);
    namespace CFG3
    {
        RTSXDeclareChipRegisterValue(kEnableSD30ClockEnd, 0x10);
        RTSXDeclareChipRegisterValue(kEnableResponse80ClockTimeout, 0x01);
    }

    RTSXDeclareChipRegister(rSTAT1, 0xFDA3);
    namespace STAT1
    {
        RTSXDeclareChipRegisterValue(kCRC7Error, 0x80);
        RTSXDeclareChipRegisterValue(kCRC16Error, 0x40);
        RTSXDeclareChipRegisterValue(kCRCWriteError, 0x20);
        RTSXDeclareChipRegisterValue(kCRCWriteErrorMask, 0x1C);
        RTSXDeclareChipRegisterValue(kCRCGetTimeout, 0x02);
        RTSXDeclareChipRegisterValue(kTuningCompareError, 0x01);
    }
    
    /// Used by the RTSX USB driver only
    RTSXDeclareChipRegister(rSTAT2, 0xFDA4);
    namespace STAT2
    {
        RTSXDeclareChipRegisterValue(kResponse80ClockTimeout, 0x01);
    }
    
    RTSXDeclareChipRegister(rBUSSTAT, 0xFDA5);
    namespace BUSSTAT
    {
        RTSXDeclareChipRegisterValue(kClockToggleEnable, 0x80);
        RTSXDeclareChipRegisterValue(kClockForceStop, 0x40);
        
        // Data line status
        RTSXDeclareChipRegisterValue(kData3Status, 0x10);
        RTSXDeclareChipRegisterValue(kData2Status, 0x08);
        RTSXDeclareChipRegisterValue(kData1Status, 0x04);
        RTSXDeclareChipRegisterValue(kData0Status, 0x02);
        
        // Command line status
        RTSXDeclareChipRegisterValue(kCommandStatus, 0x01);
        
        RTSXDeclareChipRegisterValue(kAllLinesStatus, kCommandStatus |
                                                      kData0Status   |
                                                      kData1Status   |
                                                      kData2Status   |
                                                      kData3Status);
    }
    
    RTSXDeclareChipRegister(rPADCTL, 0xFDA6);
    namespace PADCTL
    {
        RTSXDeclareChipRegisterValue(kUse1d8V, 0x80);
        RTSXDeclareChipRegisterValue(kUse3d3V, 0x7F);
        RTSXDeclareChipRegisterValue(kUseTypeADriving, 0x00);
        RTSXDeclareChipRegisterValue(kUseTypeBDriving, 0x01);
        RTSXDeclareChipRegisterValue(kUseTypeCDriving, 0x02);
        RTSXDeclareChipRegisterValue(kUseTypeDDriving, 0x03);
    }
    
    // Sample point control
    RTSXDeclareChipRegister(rSPCTL, 0xFDA7);
    namespace SPCTL
    {
        RTSXDeclareChipRegisterValue(kDDRFixRxData, 0x00);
        RTSXDeclareChipRegisterValue(kDDRVarRxData, 0x80);
        RTSXDeclareChipRegisterValue(kDDRFixRxDataEdge, 0x00);
        RTSXDeclareChipRegisterValue(kDDRFixRxData14Delay, 0x40);
        
        RTSXDeclareChipRegisterValue(kDDRFixRxCommand, 0x00);
        RTSXDeclareChipRegisterValue(kDDRVarRxCommand, 0x20);
        RTSXDeclareChipRegisterValue(kDDRFixRxCommandPosEdge, 0x00);
        RTSXDeclareChipRegisterValue(kDDRFixRxCommand14Delay, 0x10);
        
        RTSXDeclareChipRegisterValue(kSD20RxPosEdge, 0x00);
        RTSXDeclareChipRegisterValue(kSD20Rx14Delay, 0x08);
        RTSXDeclareChipRegisterValue(kSD20RxSelMask, 0x08);
    }
    
    // Push point control
    RTSXDeclareChipRegister(rPPCTL, 0xFDA8);
    namespace PPCTL
    {
        RTSXDeclareChipRegisterValue(kDDRFixTxCommandData, 0x00);
        RTSXDeclareChipRegisterValue(kDDRVarTxCommandData, 0x80);
        
        RTSXDeclareChipRegisterValue(kDDRFixTxData14TSU, 0x00);
        RTSXDeclareChipRegisterValue(kDDRFixTxData12TSU, 0x40);
        
        RTSXDeclareChipRegisterValue(kDDRFixTxCommandNegEdge, 0x00);
        RTSXDeclareChipRegisterValue(kDDRFixTxCommand14Ahead, 0x20);
        
        RTSXDeclareChipRegisterValue(kSD20TxNegEdge, 0x00);
        RTSXDeclareChipRegisterValue(kSD20Tx14Ahead, 0x10);
        RTSXDeclareChipRegisterValue(kSD20TxSelMask, 0x10);
        
        RTSXDeclareChipRegisterValue(kSSDRVarSDClockPolSwap, 0x01);
    }
    
    RTSXDeclareChipRegister(rCMD0, 0xFDA9);
    RTSXDeclareChipRegister(rCMD1, 0xFDAA);
    RTSXDeclareChipRegister(rCMD2, 0xFDAB);
    RTSXDeclareChipRegister(rCMD3, 0xFDAC);
    RTSXDeclareChipRegister(rCMD4, 0xFDAD);
    RTSXDeclareChipRegister(rCMD5, 0xFDAE);
    
    RTSXDeclareChipRegister(rBYTECNTL, 0xFDAF);
    RTSXDeclareChipRegister(rBYTECNTH, 0xFDB0);
    RTSXDeclareChipRegister(rBLOCKCNTL, 0xFDB1);
    RTSXDeclareChipRegister(rBLOCKCNTH, 0xFDB2);
    
    RTSXDeclareChipRegister(rTRANSFER, 0xFDB3);
    namespace TRANSFER
    {
        /// Transfer Control
        RTSXDeclareChipRegisterValue(kTransferStart, 0x80);
        RTSXDeclareChipRegisterValue(kTransferEnd, 0x40);
        RTSXDeclareChipRegisterValue(kTransferIdle, 0x20);
        RTSXDeclareChipRegisterValue(kTransferError, 0x10);
        
        /// Write one or two bytes from SD_CMD2 and SD_CMD3 to the card
        RTSXDeclareChipRegisterValue(kTMNormalWrite, 0x00);
        
        /// Write `nblocks * blockSize` bytes from the ring buffer to the card
        RTSXDeclareChipRegisterValue(kTMAutoWrite3, 0x01);
        
        /// Write `nblocks * blockSize` bytes from the ring buffer to the card and send a CMD12 when done.
        /// The response to the CMD12 is written to the `SD_CMD{0-4}` registers.
        RTSXDeclareChipRegisterValue(kTMAutoWrite4, 0x02);
        
        /// Read `nblocks * blockSize` bytes from the card to the ring buffer
        RTSXDeclareChipRegisterValue(kTMAutoRead3, 0x05);
        
        /// Read `nblocks * blockSize` bytes from the card to the ring buffer and send a CMD12 when done.
        /// The response to the CMD12 is written to the `SD_CMD{0-4}` registers.
        RTSXDeclareChipRegisterValue(kTMAutoRead4, 0x06);
        
        /// Send an SD command as specified in the `SD_CMD{0-4}` registers to the card and
        /// put the 48-bit response into those registers as well.
        /// However, the 136-bit response is put into the ping-pong buffer 2 instead.
        RTSXDeclareChipRegisterValue(kTMCmdResp, 0x08);
        
        /// Send a write command, get the response from the card,
        /// write the data from the ring buffer to the card and send a CMD12 when done.
        /// The response to the CMD12 is written to the `SD_CMD{0-4}` registers.
        RTSXDeclareChipRegisterValue(kTMAutoWrite1, 0x09);
        
        /// Same as `kTMAutoWrite1` except that no CMD12 is sent.
        RTSXDeclareChipRegisterValue(kTMAutoWrite2, 0x0A);
        
        /// Send a read command and read up to 512 bytes (`nblocks * blockSize`)
        /// from the card to the ring buffer or the ping-pong buffer 2.
        RTSXDeclareChipRegisterValue(kTMNormalRead, 0x0C);
        
        /// Same as `kTMAutoWrite1` but perform a read operation
        RTSXDeclareChipRegisterValue(kTMAutoRead1, 0x0D);
        
        /// Same as `kTMAutoWrite2` but perform a read operation
        RTSXDeclareChipRegisterValue(kTMAutoRead2, 0x0E);
        
        /// Send a CMD19, receive a response and the tuning pattern from the card and report the result
        RTSXDeclareChipRegisterValue(kTMAutoTuning, 0x0F);
    }
    
    RTSXDeclareChipRegister(rCMDSTATE, 0xFDB5);
    namespace CMDSTATE
    {
        RTSXDeclareChipRegisterValue(kIdle, 0x80);
    }
    
    RTSXDeclareChipRegister(rDATSTATE, 0xFDB6);
    namespace DATSTATE
    {
        RTSXDeclareChipRegisterValue(kIdle, 0x80);
    }
}

#endif /* RealtekCommonRegisters_hpp */
