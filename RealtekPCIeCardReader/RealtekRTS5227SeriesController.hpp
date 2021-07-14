//
//  RealtekRTS5227SeriesController.hpp
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 6/20/21.
//

#ifndef RealtekRTS5227SeriesController_hpp
#define RealtekRTS5227SeriesController_hpp

#include "RealtekPCICardReaderController.hpp"

///
/// Represents the abstract RTS5227 series card reader controller
///
class RealtekRTS5227SeriesController: public RealtekPCICardReaderController
{
    //
    // MARK: - Constructors & Destructors
    //
    
    OSDeclareAbstractStructors(RealtekRTS5227SeriesController);
    
    using super = RealtekPCICardReaderController;
        
    //
    // MARK: - Driving Tables for RTS5227 and RTS522A
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
    IOReturn initParameters() override;
    
    ///
    /// Initialize vendor-specific parameters
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `fetch_vendor_settings()` defined in the `pcr->ops`.
    ///
    IOReturn initVendorSpecificParameters() override;
    
    ///
    /// [Helper] Initialize the hardware from the PCI config
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rts5227_init_from_cfg()` defined in `rts5249.c`.
    ///
    IOReturn initHardwareFromConfig();
    
    ///
    /// Initialize the hardware (Extra Part)
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `extra_init_hw()` defined in defined in the `pcr->ops`.
    ///
    IOReturn initHardwareExtra() override;
    
    ///
    /// [Helper] Get a sequence of registers needed to initialize the hardware
    ///
    /// @param pairs An array of registers to be populated
    /// @return The number of registers in the array.
    ///
    virtual IOItemCount initHardwareExtraGetChipRegValuePairs(ChipRegValuePair (&pairs)[64]);
};

#endif /* RealtekRTS5227SeriesController_hpp */
