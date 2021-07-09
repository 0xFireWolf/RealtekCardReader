//
//  IOSDBusConfig.hpp
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 5/30/21.
//

#ifndef IOSDBusConfig_hpp
#define IOSDBusConfig_hpp

#include <libkern/OSTypes.h>
#include "Debug.hpp"

///
/// Reflects the struct `mmc_ios` and contains I/O related bus settings
///
/// @reference `mmc_ios_show()` defined in `drivers/mmc/core/debugfs.c`.
///
struct IOSDBusConfig
{
    /// Clock rate in Hz
    UInt32 clock;
    
    /// The bit index of the selected voltage range
    UInt32 vdd;
    
    /// The amount of time in milliseconds to wait until the power is stable
    UInt32 powerDelay;
    
    /// Command output mode
    /// Ingored by the Realtek host driver
    enum class BusMode: UInt8
    {
        kOpenDrain = 1,
        kPushPull = 2,
    } busMode;
    
    /// SPI chip select mode
    /// Ignored by the Realtek host driver:
    /// SPI is always high, preventing the chip from entering into the SPI mode
    enum class ChipSelect: UInt8
    {
        kDoNotCare = 0,
        kHigh = 1,
        kLow = 2,
    } chipSelect;
    
    /// Power supply mode
    enum class PowerMode: UInt8
    {
        kPowerOff = 0,
        kPowerUp = 1,
        kPowerOn = 2,
        kPowerUndefined = 3,
    } powerMode;
    
    /// Data bus width
    enum class BusWidth: UInt8
    {
        k1Bit = 0,
        k4Bit = 2,
        k8Bit = 3,
    } busWidth;
    
    /// Bus Speed Mode (Timing)
    enum class BusTiming: UInt8
    {
        kLegacy = 0,        // SD Default Speed ( 12.5MB/s,  25MHz, 3.3V)
        kMMCHighSpeed = 1,
        kSDHighSpeed = 2,   // SD High Speed    ( 25.0MB/s,  50MHz, 3.3V)
        kUHSSDR12 = 3,      // SD UHS-I SDR12   ( 12.5MB/s,  25MHz, 1.8V)
        kUHSSDR25 = 4,      // SD UHS-I SDR25   ( 25.0MB/s,  50MHz, 1.8V)
        kUHSSDR50 = 5,      // SD UHS-I SDR50   ( 50.0MB/s, 100MHz, 1.8V)
        kUHSSDR104 = 6,     // SD UHS-I SDR104  (104.0MB/s, 208MHz, 1.8V)
        kUHSDDR50 = 7,      // SD UHS-I DDR50   ( 50.0MB/s,  50MHz, 1.8V)
        kMMCDDR52 = 8,
        kMMCHS200 = 9,
        kMMCHS400 = 10,
        kSDExpress = 11,
        kSDExpress1d2V = 12
    } busTiming;
    
    /// Signalling voltage
    enum class SignalVoltage: UInt8
    {
        k3d3V = 0,          // SDSC/HC Card
        k1d8V = 1,          // SDHC/XC Card
        k1d2V = 2,          // SD Express Card
    } signalVoltage;
    
    /// Driver Type
    /// Not used by the Realtek host driver
    enum class DriverType: UInt8
    {
        kTypeB = 0,
        kTypeA = 1,
        kTypeC = 2,
        kTypeD = 3,
    } driverType;
    
    /// Reserved field
    UInt8 reserved;
    
    /// Enumerates all possible VDD voltage levels
    enum VDD: UInt32
    {
        /// 1.65V ~ 1.95V
        k165_195 = 0x00000080,

        /// 2.0V ~ 2.1V
        k20_21 = 0x00000100,

        /// 2.1V ~ 2.2V
        k21_22 = 0x00000200,

        /// 2.2V ~ 2.3V
        k22_23 = 0x00000400,

        /// 2.3V ~ 2.4V
        k23_24 = 0x00000800,

        /// 2.4V ~ 2.5V
        k24_25 = 0x00001000,

        /// 2.5V ~ 2.6V
        k25_26 = 0x00002000,

        /// 2.6V ~ 2.7V
        k26_27 = 0x00004000,

        /// 2.7V ~ 2.8V
        k27_28 = 0x00008000,

        /// 2.8V ~ 2.9V
        k28_29 = 0x00010000,

        /// 2.9V ~ 3.0V
        k29_30 = 0x00020000,

        /// 3.0V ~ 3.1V
        k30_31 = 0x00040000,

        /// 3.1V ~ 3.2V
        k31_32 = 0x00080000,

        /// 3.2V ~ 3.3V
        k32_33 = 0x00100000,

        /// 3.3V ~ 3.4V
        k33_34 = 0x00200000,

        /// 3.4V ~ 3.5V
        k34_35 = 0x00400000,

        /// 3.5V ~ 3.6V
        k35_36 = 0x00800000,
    };
    
    /// Print the bus config
    inline void print() const
    {
        pinfo("Bus Config:");
        pinfo("|- Clock = %u Hz.", this->clock);
        pinfo("|- VDD Index = %u.", this->vdd);
        pinfo("|- Power Delay = %u ms.", this->powerDelay);
        pinfo("|- Bus Mode = %hhu.", this->busMode);
        pinfo("|- Chip Select = %hhu.", this->chipSelect);
        pinfo("|- Power Mode = %hhu.", this->powerMode);
        pinfo("|- Bus Width = %u.", 1 << static_cast<UInt32>(this->busWidth));
        pinfo("|- Bus Timing = %hhu.", this->busTiming);
        pinfo("|- Signal Voltage = %hhu.", this->signalVoltage);
        pinfo("|- Driver Type = %hhu.", this->driverType);
    }
};

#endif /* IOSDBusConfig_hpp */
