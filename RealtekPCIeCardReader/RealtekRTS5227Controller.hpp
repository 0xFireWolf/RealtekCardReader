//
//  RealtekRTS5227Controller.hpp
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 6/20/21.
//

#ifndef RealtekRTS5227Controller_hpp
#define RealtekRTS5227Controller_hpp

#include "RealtekRTS5227SeriesController.hpp"

///
/// Represents the RTS5227 card reader controller
///
class RealtekRTS5227Controller: public RealtekRTS5227SeriesController
{
    //
    // MARK: - Constructors & Destructors
    //
    
    OSDeclareDefaultStructors(RealtekRTS5227Controller);
    
    using super = RealtekRTS5227SeriesController;
    
    //
    // MARK: - Card Power Management
    //
    
    ///
    /// Switch to the given output voltage
    ///
    /// @param outputVoltage The new output voltage
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `rtsx_pci_switch_output_voltage()` defined in `rtsx_psr.c`.
    ///
    IOReturn switchOutputVoltage(OutputVoltage outputVoltage) override;
    
    //
    // MARK: - Card Clock Configurations
    //
    
    ///
    /// Adjust the card clock if DMA transfer errors occurred
    ///
    /// @param cardClock The current card clock in Hz
    /// @return The adjusted card clock.
    /// @note Port: This function replaces the code block that reduces the card clock in `rtsx_pci_switch_clock()` defined in `rtsx_psr.c`.
    ///             By default, this function does not adjust the card clock and return the given clock.
    ///             RTS5227 controller must override this function.
    ///
    UInt32 getAdjustedCardClockOnDMAError(UInt32 cardClock) override;
    
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
    /// Optimize the physical layer
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    ///
    IOReturn optimizePhys() override;
};

#endif /* RealtekRTS5227Controller_hpp */
