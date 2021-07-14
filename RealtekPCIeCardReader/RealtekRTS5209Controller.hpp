//
//  RealtekRTS5209Controller.hpp
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 6/17/21.
//

#ifndef RealtekRTS5209Controller_hpp
#define RealtekRTS5209Controller_hpp

#include "RealtekPCICardReaderController.hpp"

///
/// Represents the RTS5229 card reader controller
///
class RealtekRTS5209Controller: public RealtekPCICardReaderController
{
    //
    // MARK: - Constructors & Destructors
    //
    
    OSDeclareDefaultStructors(RealtekRTS5209Controller);
    
    using super = RealtekPCICardReaderController;
    
    //
    // MARK: - SD Pull Control Tables
    //
    
    /// A sequence of chip registers to enable SD pull control
    static const ChipRegValuePair kSDEnablePullControlTablePairs[];
    static const SimpleRegValuePairs kSDEnablePullControlTable;
    
    /// A sequence of chip registers to disable SD pull control
    static const ChipRegValuePair kSDDisablePullControlTablePairs[];
    static const SimpleRegValuePairs kSDDisablePullControlTable;
    
    //
    // MARK: - LED Management
    //
    
    ///
    /// Turn on the LED
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    /// @note Port: This function replaces `turn_on_led()` defined in `struct pcr_ops`.
    ///             The base controller class implements this function by changing the value of `GPIO_CTL`.
    ///             RTS5209, 5260, 5286, 5287 and 5289 controllers must override this function.
    ///
    IOReturn turnOnLED() override;
    
    ///
    /// Turn off the LED
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    /// @note Port: This function replaces `turn_off_led()` defined in `struct pcr_ops`.
    ///             The base controller class implements this function by changing the value of `GPIO_CTL`.
    ///             RTS5209, 5286, 5287 and 5289 controllers must override this function.
    ///
    IOReturn turnOffLED() override;
    
    ///
    /// Enable LED blinking
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    /// @note Port: This function replaces `enable_auto_blink()` defined in `struct pcr_ops`.
    ///             The base controller class implements this function by changing the value of `OLT_LED_CTL`.
    ///             RTS5209, 5286, 5287 and 5289 controllers must override this function.
    ///
    IOReturn enableLEDBlinking() override;
    
    ///
    /// Disable LED blinking
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    /// @note Port: This function replaces `enable_auto_blink()` defined in `struct pcr_ops`.
    ///             The base controller class implements this function by changing the value of `OLT_LED_CTL`.
    ///             RTS5209, 5286, 5287 and 5289 controllers must override this function.
    ///
    IOReturn disableLEDBlinking() override;
    
    //
    // MARK: - Card Power Management
    //
    
    ///
    /// Power on the SD card
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_pci_card_power_on()` defined in `rtsx_psr.c`.
    ///
    IOReturn powerOnCard() override;
    
    ///
    /// Power off the SD card
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_pci_card_power_off()` defined in `rtsx_psr.c`.
    ///
    IOReturn powerOffCard() override;
    
    ///
    /// Switch to the given output voltage
    ///
    /// @param outputVoltage The new output voltage
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_pci_switch_output_voltage()` defined in `rtsx_psr.c`.
    ///
    IOReturn switchOutputVoltage(OutputVoltage outputVoltage) override;
    
    //
    // MARK: - Power Management
    //
    
    ///
    /// Power down the controller forcedly
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_base_force_power_down()` defined in `rtsx_psr.c` and `*_force_power_down()` defined in each controller file.
    ///
    IOReturn forcePowerDown() override;
    
    //
    // MARK: - Hardware Initialization and Configuration
    //
    
    ///
    /// [Helper] Check whether the vendor settings register 1 contains a valid value
    ///
    /// @param regVal The register value
    /// @return `true` if valid, `false` otherwise.
    ///
    inline bool vsIsRegister1ValueValid(UInt32 regVal)
    {
        return !(regVal & 0x80);
    }
    
    ///
    /// [Helper] Check whether the vendor settings register 2 contains a valid value
    ///
    /// @param regVal The register value
    /// @return `true` if valid, `false` otherwise.
    ///
    inline bool vsIsRegister2ValueValid(UInt32 regVal)
    {
        return (regVal & 0x80);
    }
    
    ///
    /// [Helper] Extract the SD 3.0 card drive selector (1.8V) from the register value
    ///
    /// @param regVal The register value
    /// @return The SD 3.0 card drive selector (1.8V).
    ///
    UInt8 vsGetSD30DriveSelector1d8V(UInt32 regVal) override
    {
        return (regVal >> 3) & 0x07;
    }
    
    ///
    /// [Helper] Extract the SD 3.0 card drive selector (3.3V) from the register value
    ///
    /// @param regVal The register value
    /// @return The SD 3.0 card drive selector (3.3V).
    ///
    UInt8 vsGetSD30DriveSelector3d3V(UInt32 regVal) override
    {
        return regVal & 0x07;
    }
    
    ///
    /// [Helper] Extract the card drive selector from the register value
    ///
    /// @param regVal The register value
    /// @return The card drive selector.
    ///
    UInt8 vsGetCardDriveSelector(UInt32 regVal) override
    {
        return regVal >> 8;
    }
    
    ///
    /// [Helper] Check whether the ASPM L1 is already enabled
    ///
    /// @param regVal The register value
    /// @return `true` if L1 is enabled, `false` otherwise.
    /// @note Port: This function replaces `rtsx_reg_to_aspm()` defined in `rtsx_pcr.h`.
    ///
    bool vsIsASPML1Enabled(UInt32 regVal) override
    {
        return ((regVal >> 5) & 0x02) == 0x02;
    }
    
    ///
    /// Get the IC revision
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `*_get_ic_version()` defined in each controller file.
    ///
    IOReturn getRevision(Revision& revision) override;
    
    ///
    /// Optimize the physical layer
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    ///
    IOReturn optimizePhys() override;
    
    ///
    /// Initialize hardware-specific parameters
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `*_init_params()` defined in each controller file.
    ///
    IOReturn initParameters() override;
    
    ///
    /// Initialize vendor-specific parameters
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `fetch_vendor_settings()` defined in the `pcr->ops`.
    ///
    IOReturn initVendorSpecificParameters() override;
    
    ///
    /// Initialize the hardware (Extra Part)
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `extra_init_hw()` defined in defined in the `pcr->ops`.
    ///
    IOReturn initHardwareExtra() override;
};

#endif /* RealtekRTS5209Controller_hpp */
