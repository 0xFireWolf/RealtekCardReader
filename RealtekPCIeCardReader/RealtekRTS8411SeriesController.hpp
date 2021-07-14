//
//  RealtekRTS8411SeriesController.hpp
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 6/20/21.
//

#ifndef RealtekRTS8411SeriesController_hpp
#define RealtekRTS8411SeriesController_hpp

#include "RealtekPCICardReaderController.hpp"

///
/// Represents the abstract RTS8411 series card reader controller
///
class RealtekRTS8411SeriesController: public RealtekPCICardReaderController
{
    //
    // MARK: - Constructors & Destructors
    //
    
    OSDeclareAbstractStructors(RealtekRTS8411SeriesController);
    
    using super = RealtekPCICardReaderController;
    
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
    /// [Helper, Common] Switch to the given output voltage
    ///
    /// @param outputVoltage The new output voltage
    /// @param shift Shift `kBppRegTuned18` by the given value
    /// @param asic The ASIC value for the 1.8V output voltage
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtl8411_do_switch_output_voltage()` defined in `rts8411.c`.
    ///
    IOReturn switchOutputVoltage(OutputVoltage outputVoltage, UInt8 shift, UInt8 asic);
    
    ///
    /// Switch to the given output voltage
    ///
    /// @param outputVoltage The new output voltage
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_pci_switch_output_voltage()` defined in `rtsx_psr.c`.
    /// @note Port: This function replaces `rtl8411_switch_output_voltage()` definde in `rts8411.c`.
    ///             RTS5286 controller must override this function and provide the correct shift and ASIC values.
    ///
    IOReturn switchOutputVoltage(OutputVoltage outputVoltage) override;
    
    //
    // MARK: - Card Detection and Write Protection
    //
    
    ///
    /// Check whether a card is present
    ///
    /// @return `true` if a card exists, `false` otherwise.
    /// @note Port: This function replaces `rtsx_pci_card_exist()` and `cd_deglitch()` defined in `rtsx_psr.c`.
    /// @warning: This function supports SD cards only.
    ///
    bool isCardPresent() override;
    
    //
    // MARK: - Card Clock Configurations
    //
    
    ///
    /// Convert the given SSC clock to the divider N
    ///
    /// @param clock The SSC clock in MHz
    /// @return The divider N.
    /// @note Port: This function replaces `conv_clk_and_div_n()` defined in `rtsx_psr.c`.
    ///             RTL8411 series controllers must override this function.
    ///
    UInt32 sscClock2DividerN(UInt32 clock) override;
    
    ///
    /// Convert the given divider N back to the SSC clock
    ///
    /// @param n The divider N
    /// @return The SSC clock in MHz.
    /// @note Port: This function replaces `conv_clk_and_div_n()` defined in `rtsx_psr.c`.
    ///             RTL8411 series controllers must override this function.
    ///
    UInt32 dividerN2SSCClock(UInt32 n) override;
    
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
    /// [Helper] Extract the SD 3.0 card drive selector (3.3V) from the register value
    ///
    /// @param regVal The register value
    /// @return The SD 3.0 card drive selector (3.3V).
    /// @note RTS5287 controller must override this function.
    ///
    UInt8 vsGetSD30DriveSelector3d3V(UInt32 regVal) override
    {
        return (regVal >> 5) & 0x07;
    }
    
    ///
    /// Get the IC revision
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `*_get_ic_version()` defined in each controller file.
    ///
    IOReturn getRevision(Revision& revision) override;
    
    ///
    /// Initialize hardware-specific parameters
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `*_init_params()` defined in each controller file.
    /// @note Port: This function replaces `rtl8411_init_common_params()` defined in `rts8411.c`.
    ///             Concrete controllers must override and extend this function to set the pull control tables.
    ///
    IOReturn initParameters() override;
    
    ///
    /// Initialize vendor-specific parameters
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `fetch_vendor_settings()` defined in the `pcr->ops`.
    /// @note Port: This function replaces `rtl8411_fetch_vendor_settings()` defined in `rts8411.c`.
    ///             RTS5287 controller must override this function.
    ///
    IOReturn initVendorSpecificParameters() override;
    
    ///
    /// Initialize the hardware (Extra Part)
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `extra_init_hw()` defined in defined in the `pcr->ops`.
    /// @note Port: This function replaces `rtl8411_extra_init_hw()` defined in `rts8411.c`.
    ///             RTS5287 controller must override this function.
    ///
    IOReturn initHardwareExtra() override;
};

#endif /* RealtekRTS8411SeriesController_hpp */
