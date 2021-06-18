//
//  IOSDBusConfig.hpp
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 5/30/21.
//

#ifndef IOSDBusConfig_hpp
#define IOSDBusConfig_hpp

#include <libkern/OSTypes.h>

#define MMC_VDD_165_195      0x00000080    /* VDD voltage 1.65 - 1.95 */
#define MMC_VDD_20_21        0x00000100    /* VDD voltage 2.0 ~ 2.1 */
#define MMC_VDD_21_22        0x00000200    /* VDD voltage 2.1 ~ 2.2 */
#define MMC_VDD_22_23        0x00000400    /* VDD voltage 2.2 ~ 2.3 */
#define MMC_VDD_23_24        0x00000800    /* VDD voltage 2.3 ~ 2.4 */
#define MMC_VDD_24_25        0x00001000    /* VDD voltage 2.4 ~ 2.5 */
#define MMC_VDD_25_26        0x00002000    /* VDD voltage 2.5 ~ 2.6 */
#define MMC_VDD_26_27        0x00004000    /* VDD voltage 2.6 ~ 2.7 */
#define MMC_VDD_27_28        0x00008000    /* VDD voltage 2.7 ~ 2.8 */
#define MMC_VDD_28_29        0x00010000    /* VDD voltage 2.8 ~ 2.9 */
#define MMC_VDD_29_30        0x00020000    /* VDD voltage 2.9 ~ 3.0 */
#define MMC_VDD_30_31        0x00040000    /* VDD voltage 3.0 ~ 3.1 */
#define MMC_VDD_31_32        0x00080000    /* VDD voltage 3.1 ~ 3.2 */
#define MMC_VDD_32_33        0x00100000    /* VDD voltage 3.2 ~ 3.3 */
#define MMC_VDD_33_34        0x00200000    /* VDD voltage 3.3 ~ 3.4 */
#define MMC_VDD_34_35        0x00400000    /* VDD voltage 3.4 ~ 3.5 */
#define MMC_VDD_35_36        0x00800000    /* VDD voltage 3.5 ~ 3.6 */

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
    
    void print() const
    {
        // TODO: IMP THIS
    }
};

#endif /* IOSDBusConfig_hpp */
