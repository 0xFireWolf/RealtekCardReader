//
//  RealtekUSBRegisters.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 7/10/21.
//

#ifndef RealtekUSBRegisters_hpp
#define RealtekUSBRegisters_hpp

#define RTSX_REG_NAMESPACE_UCR
#include "RealtekCommonRegisters.hpp"

namespace RTSX::UCR::Chip
{
    RTSXDeclareChipRegister(rHWVER, 0xFC01);
    namespace HWVER
    {
        RTSXDeclareChipRegisterValue(kMask, 0x0F);
    }
    
    namespace CFG
    {
        RTSXDeclareChipRegister(rMODE0, 0xFC0E);
        namespace MODE0
        {
            RTSXDeclareChipRegisterValue(kCrystalFree, 0x80);
            RTSXDeclareChipRegisterValue(kClockModeMask, 0x03);
            RTSXDeclareChipRegisterValue(kClockMode12MCrystal, 0x00);
            RTSXDeclareChipRegisterValue(kClockModeNonCrystal, 0x01);
            RTSXDeclareChipRegisterValue(kClockMode24MOscillator, 0x02);
            RTSXDeclareChipRegisterValue(kClockMode48MOscillator, 0x03);
        }
        
        RTSXDeclareChipRegister(rMODE1, 0xFC0F);
        namespace MODE1
        {
            // Bit 1 is set if the chip is RTS5179
            RTSXDeclareChipRegisterValue(kRTS5179, 0x02);
        }
    }
    
    /// Card Detection Deglictch
    namespace CDDEG
    {
        RTSXDeclareChipRegister(rWIDTH, 0xFC20);
        
        RTSXDeclareChipRegister(rCTL, 0xFC21);
        namespace CTL
        {
            RTSXDeclareChipRegisterValue(kEnableSDDeglitch, 0x01);
            RTSXDeclareChipRegisterValue(kEnableMSDeglitch, 0x02);
            RTSXDeclareChipRegisterValue(kEnableXDDeglitch, 0x04);
            
            RTSXDeclareChipRegisterValue(kEnableSDDetection, 0x08);
            RTSXDeclareChipRegisterValue(kEnableMSDetection, 0x10);
            RTSXDeclareChipRegisterValue(kEnableXDDetection, 0x20);
        }
    }
    
    RTSXDeclareChipRegister(rDUMMY0, 0xFC30);
    namespace DUMMY0
    {
        RTSXDeclareChipRegisterValue(kNYETMask, 0x01);
        RTSXDeclareChipRegisterValue(kNYETEnable, 0x01);
    }
    
    RTSXDeclareChipRegister(rSFSM, 0xFC04);
    
    namespace HS
    {
        RTSXDeclareChipRegister(rUSBSTA, 0xFE01);
        RTSXDeclareChipRegister(rVCTRL, 0xFE26);
        RTSXDeclareChipRegister(rVSTAIN, 0xFE27);
        RTSXDeclareChipRegister(rVLOADM, 0xFE28);
        RTSXDeclareChipRegister(rVSTAOUT, 0xFE29);
    }
}

namespace RTSX::UCR::Chip::MC
{
    RTSXDeclareChipRegister(rIRQ, 0xFF00);
    RTSXDeclareChipRegister(rIRQEN, 0xFF01);
    
    namespace FIFO
    {
        RTSXDeclareChipRegister(rCTL, 0xFF02);
        namespace CTL
        {
            RTSXDeclareChipRegisterValue(kFlush, 0x01);
        }
        
        RTSXDeclareChipRegister(rBC0, 0xFF03);
        RTSXDeclareChipRegister(rBC1, 0xFF04);
        RTSXDeclareChipRegister(rSTAT, 0xFF05);
        RTSXDeclareChipRegister(rMODE, 0xFF06);
        RTSXDeclareChipRegister(rRDPTR0, 0xFF07);
        RTSXDeclareChipRegister(rRDPTR1, 0xFF08);
    }
    
    namespace DMA
    {
        RTSXDeclareChipRegister(rCTL, 0xFF10);
        namespace CTL
        {
            RTSXDeclareChipRegisterValue(kDirectionToCard, 0x00);
            RTSXDeclareChipRegisterValue(kDirectionFromCard, 0x02);
            RTSXDeclareChipRegisterValue(kDirectionMask, 0x02);
            
            RTSXDeclareChipRegisterValue(kEnable, 0x01);
            
            RTSXDeclareChipRegisterValue(kPackSize128, 0 << 2);
            RTSXDeclareChipRegisterValue(kPackSize256, 1 << 2);
            RTSXDeclareChipRegisterValue(kPackSize512, 2 << 2);
            RTSXDeclareChipRegisterValue(kPackSize1024, 3 << 2);
            RTSXDeclareChipRegisterValue(kPackSizeMask, 0x0C);
            
            RTSXDeclareChipRegisterValue(kTCEQ0, 0x80);
        }
        
        RTSXDeclareChipRegister(rTC0, 0xFF11);
        RTSXDeclareChipRegister(rTC1, 0xFF12);
        RTSXDeclareChipRegister(rTC2, 0xFF13);
        RTSXDeclareChipRegister(rTC3, 0xFF14);
        
        RTSXDeclareChipRegister(rRST, 0xFF15);
        namespace RST
        {
            RTSXDeclareChipRegisterValue(kReset, 0x01);
        }
    }
}

/// Chip registers that control the SSC clock
namespace RTSX::UCR::Chip::CLK
{
    RTSXDeclareChipRegister(rDIV, 0xFC03);
    namespace DIV
    {
        RTSXDeclareChipRegisterValue(kChangeClock, 0x80);
        RTSXDeclareChipRegisterValue(k1, 0x00);
        RTSXDeclareChipRegisterValue(k2, 0x01);
        RTSXDeclareChipRegisterValue(k4, 0x02);
        RTSXDeclareChipRegisterValue(k8, 0x03);
    }
}

/// Chip registers that control the SSC
namespace RTSX::UCR::Chip::SSC
{
    RTSXDeclareChipRegister(rDIVN0, 0xFC07);
    RTSXDeclareChipRegister(rDIVN1, 0xFC08);
    
    RTSXDeclareChipRegister(rCTL1, 0xFC09);
    namespace CTL1
    {
        // Same as the ones for PCIe-based controllers
        RTSXDeclareChipRegisterValue(kRSTB, 0x80);
        RTSXDeclareChipRegisterValue(kEnable8x, 0x40);
        RTSXDeclareChipRegisterValue(kFixFraction, 0x20);
        
        RTSXDeclareChipRegisterValue(kSelector1M, 0x00);
        RTSXDeclareChipRegisterValue(kSelector2M, 0x08);
        RTSXDeclareChipRegisterValue(kSelector4M, 0x10);
        RTSXDeclareChipRegisterValue(kSelector8M, 0x18);
    }
    
    RTSXDeclareChipRegister(rCTL2, 0xFC10);
    namespace CTL2
    {
        // Different from the ones for PCIe-based controllers
        RTSXDeclareChipRegisterValue(kDepthMask, 0x03);
        RTSXDeclareChipRegisterValue(kDepthDisable, 0x00);
        RTSXDeclareChipRegisterValue(kDepth2M, 0x01);
        RTSXDeclareChipRegisterValue(kDepth1M, 0x02);
        RTSXDeclareChipRegisterValue(kDepth512K, 0x03);
    }
}

/// Chip registers that manages the card
namespace RTSX::UCR::Chip::CARD
{
    RTSXDeclareChipRegister(rCLKSRC, 0xFC2E); // Same as the PCIe one
    namespace CLKSRC
    {
        RTSXDeclareChipRegisterValue(kCRCFixClock, 0x00 << 0);
        RTSXDeclareChipRegisterValue(kCRCVarClock0, 0x01 << 0);
        RTSXDeclareChipRegisterValue(kCRCVarClock1, 0x02 << 0);
        
        RTSXDeclareChipRegisterValue(kSD30FixClock, 0x00 << 2);
        RTSXDeclareChipRegisterValue(kSD30VarClock0, 0x01 << 2);
        RTSXDeclareChipRegisterValue(kSD30VarClock1, 0x02 << 2);
        
        RTSXDeclareChipRegisterValue(kSampleFixClock, 0x00 << 4);
        RTSXDeclareChipRegisterValue(kSampleVarClock0, 0x01 << 4);
        RTSXDeclareChipRegisterValue(kSampleVarClock1, 0x02 << 4);
    }
    
    RTSXDeclareChipRegister(rSHAREMODE, 0xFD51);
    namespace SHAREMODE
    {
        RTSXDeclareChipRegisterValue(kQFN24, 0x00);
        RTSXDeclareChipRegisterValue(kLQFP48, 0x04);
        RTSXDeclareChipRegisterValue(kPkgSel, 0x04);
        
        RTSXDeclareChipRegisterValue(kXD, 0x00);
        RTSXDeclareChipRegisterValue(kSD, 0x01);
        RTSXDeclareChipRegisterValue(kMS, 0x02);
        RTSXDeclareChipRegisterValue(kMask, 0x03);
    }
    
    RTSXDeclareChipRegister(rDRVSEL, 0xFD52);
    namespace DRVSEL
    {
        RTSXDeclareChipRegisterValue(kMask, 0x03);
    }
    
    RTSXDeclareChipRegister(rSTOP, 0xFD53);
    namespace STOP
    {
        RTSXDeclareChipRegisterValue(kStopSPI, 0x01);
        RTSXDeclareChipRegisterValue(kStopXD, 0x02);
        RTSXDeclareChipRegisterValue(kStopSD, 0x04);
        RTSXDeclareChipRegisterValue(kStopMS, 0x08);
        
        RTSXDeclareChipRegisterValue(kClearSPIError, 0x10);
        RTSXDeclareChipRegisterValue(kClearXDError, 0x20);
        RTSXDeclareChipRegisterValue(kClearSDError, 0x40);
        RTSXDeclareChipRegisterValue(kClearMSError, 0x80);
    }
    
    /// Address of the register that enables the card output
    RTSXDeclareChipRegister(rOUTPUT, 0xFD54);
    
    /// Special values and flags for the OUTPUT register
    namespace OUTPUT
    {        
        /// Bit 2 is set to enable SD output
        RTSXDeclareChipRegisterValue(kSDMask, 1 << 2);
        RTSXDeclareChipRegisterValue(kEnableSDValue, 1 << 2);
        RTSXDeclareChipRegisterValue(kDisableSDValue, 0);
        
        /// Bit 3 is set to enable MS output
        RTSXDeclareChipRegisterValue(kMSMask, 1 << 3);
        RTSXDeclareChipRegisterValue(kEnableMSValue, 1 << 3);
        RTSXDeclareChipRegisterValue(kDisableMSValue, 0);
    }
    
    RTSXDeclareChipRegister(rBLINK, 0xFD55);
    
    RTSXDeclareChipRegister(rGPIO, 0xFD56);
    namespace GPIO
    {
        RTSXDeclareChipRegisterValue(kLEDMask, 0x03);
        RTSXDeclareChipRegisterValue(kLEDOn, 0x02);
        RTSXDeclareChipRegisterValue(kLEDOff, 0x03);
    }
    
    namespace SD30::DRVSEL
    {
        RTSXDeclareChipRegister(rCLK, 0xFD57);
        namespace CLK
        {
            RTSXDeclareChipRegisterValue(kDriverTypeA, 0x05);
            RTSXDeclareChipRegisterValue(kDriverTypeB, 0x03);
            RTSXDeclareChipRegisterValue(kDriverTypeC, 0x02);
            RTSXDeclareChipRegisterValue(kDriverTypeD, 0x01);
            RTSXDeclareChipRegisterValue(kDriverMask, 0x07);
        }
    }
    
    RTSXDeclareChipRegister(rDMA1CTL, 0xFD5C);
    namespace DMA1CTL
    {
        RTSXDeclareChipRegisterValue(kEntendDMA1AsyncSignal, 0x02);
    }
    
    RTSXDeclareChipRegister(rDATASRC, 0xFD5D);
    namespace DATASRC
    {
        RTSXDeclareChipRegisterValue(kPingPongBuffer, 0x01);
        RTSXDeclareChipRegisterValue(kRingBuffer, 0x00);
        RTSXDeclareChipRegisterValue(kMask, 0x01);
    }
    
    RTSXDeclareChipRegister(rSEL, 0xFD5E);
    namespace SEL
    {
        RTSXDeclareChipRegisterValue(kSD, 0x2);
        RTSXDeclareChipRegisterValue(kMS, 0x3);
        RTSXDeclareChipRegisterValue(kMask, 0x7);
    }
    
    /// Card pull control registers
    namespace PULL
    {
        RTSXDeclareChipRegister(rCTL1, 0xFD60);
        RTSXDeclareChipRegister(rCTL2, 0xFD61);
        RTSXDeclareChipRegister(rCTL3, 0xFD62);
        RTSXDeclareChipRegister(rCTL4, 0xFD63);
        RTSXDeclareChipRegister(rCTL5, 0xFD64);
        RTSXDeclareChipRegister(rCTL6, 0xFD65);
    }
    
    RTSXDeclareChipRegister(rEXIST, 0xFD6F);
    
    RTSXDeclareChipRegister(rINTPEND, 0xFD71);
    namespace INTPEND
    {
        RTSXDeclareChipRegister(kSD, 0x04);
        RTSXDeclareChipRegister(kMS, 0x08);
        RTSXDeclareChipRegister(kXD, 0x10);
        RTSXDeclareChipRegister(kAll, kSD | kMS | kXD);
    }
    
    /// Register to enable the card clock
    RTSXDeclareChipRegister(rCLK, 0xFD79);
    namespace CLK
    {
        RTSXDeclareChipRegisterValue(kDisable, 0x00);
        RTSXDeclareChipRegisterValue(kEnableSD, 0x04);
        RTSXDeclareChipRegisterValue(kEnableMS, 0x08);
    }
    
    RTSXDeclareChipRegister(rPWRCTRL, 0xFD7A);
    namespace PWRCTRL
    {        
        RTSXDeclareChipRegisterValue(kSDPowerOn, 0x00);
        RTSXDeclareChipRegisterValue(kSDPartialPowerOn, 0x02);
        RTSXDeclareChipRegisterValue(kSDPowerOff, 0x03);
        RTSXDeclareChipRegisterValue(kSDPowerMask, 0x03);
        
        RTSXDeclareChipRegisterValue(kLDO3318PowerOn, 0x00);
        RTSXDeclareChipRegisterValue(kLDO3318PowerSuspend, 0x08);
        RTSXDeclareChipRegisterValue(kLDO3318PowerOff, 0x0C);
        RTSXDeclareChipRegisterValue(kLDO3318PowerMask, 0x0C);
        
        RTSXDeclareChipRegisterValue(kDV3318AutoPowerOff, 0x10);
        RTSXDeclareChipRegisterValue(kForceLDOPowerB, 0x60);
    }
}

namespace RTSX::UCR::Chip::LDO
{
    RTSXDeclareChipRegister(rPWRCFG, 0xFD7B);
    namespace PWRCFG
    {
        RTSXDeclareChipRegisterValue(kMask, 0x1C);
        RTSXDeclareChipRegisterValue(k1V7, 0x00 << 2);
        RTSXDeclareChipRegisterValue(k1V8, 0x01 << 2);
        RTSXDeclareChipRegisterValue(k1V9, 0x02 << 2);
        RTSXDeclareChipRegisterValue(k2V0, 0x03 << 2);
        RTSXDeclareChipRegisterValue(k2V7, 0x04 << 2);
        RTSXDeclareChipRegisterValue(k2V8, 0x05 << 2);
        RTSXDeclareChipRegisterValue(k2V9, 0x06 << 2);
        RTSXDeclareChipRegisterValue(k3V3, 0x07 << 2);
    }
}

namespace RTSX::UCR::Chip::OCP
{
    RTSXDeclareChipRegister(rCTL, 0xFD80);
    RTSXDeclareChipRegister(rPARA1, 0xFD81);
    RTSXDeclareChipRegister(rPARA2, 0xFD82);
    RTSXDeclareChipRegister(rSTAT, 0xFD83);
}

#endif /* RealtekUSBRegisters_hpp */
