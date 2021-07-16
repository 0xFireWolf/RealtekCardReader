//
//  RealtekRTS5260Controller.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 7/15/21.
//

#ifndef RealtekRTS5260Controller_hpp
#define RealtekRTS5260Controller_hpp

#include "RealtekPCICardReaderController.hpp"

///
/// Represents the RTS5260 card reader controller
///
class RealtekRTS5260Controller: public RealtekPCICardReaderController
{
    //
    // MARK: - Constructors & Destructors
    //
    
    OSDeclareDefaultStructors(RealtekRTS5260Controller);
    
    using super = RealtekPCICardReaderController;
    
    //
    // MARK: - Driving Tables for RTS5260
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
    // MARK: - Clear Error
    //
    
    ///
    /// Clear any transfer error on the host side
    ///
    /// @note This function is invoked when a command or data transfer has failed.
    /// @note Port: This function replaces `rtsx_pci_stop_cmd()` defined in `rtsx_psr.c`.
    ///             RTS5228, RTS5260 and RTS5261 controllers must override this function.
    ///
    void clearHostError() override;
    
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
    ///             RTS5209, 5260, 5286, 5287 and 5289 controllers must override this function.
    ///
    IOReturn turnOffLED() override;
    
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
    // MARK: - Overcurrent Protection Support
    //
    
    ///
    /// Initialize and enable overcurrent protection
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_pci_init_ocp()` defined in `rtsx_psr.c`.
    ///             RTS5228, RTS5260 and RTS5261 controllers must override this function.
    ///
    IOReturn initOvercurrentProtection() override;
    
    ///
    /// Enable overcurrent protection
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_pci_enable_ocp()` defined in `rtsx_psr.c`.
    ///             RTS5228, RTS5260 and RTS5261 controllers must override this function.
    ///
    IOReturn enableOvercurrentProtection() override;
    
    ///
    /// Disable overcurrent protection
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_pci_disable_ocp()` defined in `rtsx_psr.c`.
    ///             RTS5228, RTS5260 and RTS5261 controllers must override this function.
    ///
    IOReturn disableOvercurrentProtection() override;
    
    ///
    /// Clear the overcurrent protection status
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_pci_clear_ocpstat()` defined in `rtsx_psr.c`.
    ///             RTS5228, RTS5260 and RTS5261 controllers must override this function.
    ///
    IOReturn clearOvercurrentProtectionStatus() override;
    
    ///
    /// Check whether the controller should power off the card when it receives an OCP interrupt
    ///
    /// @return `true` if the controller should take necessary actions to protect the card, `false` otherwise.
    /// @note Port: This function replaces the code block that examines the OCP status in `rtsx_pci_process_ocp()` defined in `rtsx_psr.c`.
    ///             RTS5260 controller must override this function.
    /// @note This function runs in a gated context and is invoked by the OCP interrupt service routine.
    ///
    bool shouldPowerOffCardOnOvercurrentInterruptGated() override;
    
    //
    // MARK: - Active State Power Management
    //
    
    ///
    /// Set the L1 substates configuration
    ///
    /// @param active Pass `true` to set the active state
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_set_l1off_sub_cfg_d0()` and `set_l1off_cfg_sub_d0()` defined in `rtsx_pcr.c`.
    /// @note The base controller provides a default implementation that simply returns `kIOReturnSuccess`.
    ///
    IOReturn setL1OffSubConfigD0(bool active) override;
    
    //
    // MARK: - Power Management
    //
    
    ///
    /// Check if the controller should enable the clock power management
    ///
    /// @return `true` if the driver should write 0x01 to `PM_CLK_FORCE_CTL`, `false` otherwise.
    /// @note Port: This function replaces the code block that checks the device ID and writes to the register in `rtsx_pci_init_hw()` defined in `rtsx_psr.c`.
    ///             The base controller returns `false` by default.
    ///             RTS524A, RTS525A, RTS5260, RTS5261 and RTS5228 controllers should override this function and return `true`.
    ///
    bool shouldEnableClockPowerManagement() override;
    
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
    /// [Helper] Initialize power saving related parameters
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rts5260_pwr_saving_setting()` defined in the `rts5260.c`.
    ///
    IOReturn initPowerSavingParameters();
    
    ///
    /// [Helper] Initialize the hardware from the PCI config
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rts5260_init_from_cfg()` defined in `rts5260.c`.
    ///
    IOReturn initHardwareFromConfig();
    
    ///
    /// Initialize the hardware (Extra Part)
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `extra_init_hw()` defined in defined in the `pcr->ops`.
    ///
    IOReturn initHardwareExtra() override;
};

#endif /* RealtekRTS5260Controller_hpp */
