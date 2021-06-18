//
//  RealtekRTS5249SeriesController.hpp
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 5/16/21.
//

#ifndef RealtekRTS5249SeriesController_hpp
#define RealtekRTS5249SeriesController_hpp

#include "RealtekPCIeCardReaderController.hpp"

///
/// Represents the abstract RTS5249 series card reader controller
///
class RealtekRTS5249SeriesController: public RealtekPCIeCardReaderController
{
    //
    // MARK: - Constructors & Destructors
    //
    
    OSDeclareAbstractStructors(RealtekRTS5249SeriesController);
    
    using super = RealtekPCIeCardReaderController;
    
    //
    // MARK: - Driving Tables for RTS5249, 524A, 525A
    //
    
    /// SD 3.0 drive table (1.8V)
    static const DrivingTable kSD30DriveTable1d8V;
    
    /// SD 3.0 drive table (3.3V)
    static const DrivingTable kSD30DriveTable3d3V;
    
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
    ///
    IOReturn turnOnLED() override;

    ///
    /// Turn off the LED
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    /// @note Port: This function replaces `turn_off_led()` defined in `struct pcr_ops`.
    ///
    IOReturn turnOffLED() override;

    ///
    /// Enable LED blinking
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    /// @note Port: This function replaces `enable_auto_blink()` defined in `struct pcr_ops`.
    ///
    IOReturn enableLEDBlinking() override;

    ///
    /// Disable LED blinking
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, `kIOReturnError` otherwise.
    /// @note Port: This function replaces `enable_auto_blink()` defined in `struct pcr_ops`.
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
    
    //
    // MARK: - Hardware Initialization and Configuration
    //
    
    ///
    /// Initialize hardware-specific parameters
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `*_init_params()` defined in each controller file.
    ///
    virtual IOReturn initParameters() override;
    
    ///
    /// Initialize vendor-specific parameters
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `fetch_vendor_settings()` defined in the `pcr->ops`.
    ///
    virtual IOReturn initVendorSpecificParameters() override;
    
    ///
    /// [Helper] Initialize the hardware from the PCI config
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rts5249_init_from_cfg()` and `rts5249_init_from_hw()` defined in `rts5249.c`.
    ///
    IOReturn initHardwareFromConfig();
    
    ///
    /// Initialize the hardware (Extra Part)
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `extra_init_hw()` defined in defined in the `pcr->ops`.
    ///
    virtual IOReturn initHardwareExtra() override;
    
    ///
    /// [Helper] Get a sequence of registers needed to initialize the hardware
    ///
    /// @param pairs An array of registers to be populated
    /// @return The number of registers in the array.
    ///
    virtual IOItemCount initHardwareExtraGetChipRegValuePairs(ChipRegValuePair (&pairs)[64]) = 0;
};

#endif /* RealtekRTS5249SeriesController_hpp */
