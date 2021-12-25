//
//  IOSDCard-SwitchCaps.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 12/24/21.
//

#ifndef IOSDCard_SwitchCaps_hpp
#define IOSDCard_SwitchCaps_hpp

#include <libkern/OSTypes.h>

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

#endif /* IOSDCard_SwitchCaps_hpp */
