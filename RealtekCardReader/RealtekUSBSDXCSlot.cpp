//
//  RealtekUSBSDXCSlot.cpp
//  RealtekCardReader
//
//  Created by FireWolf on 7/13/21.
//

#include "RealtekUSBSDXCSlot.hpp"
#include "RealtekUSBRegisters.hpp"

//
// MARK: - Meta Class Definitions
//

OSDefineMetaClassAndStructors(RealtekUSBSDXCSlot, RealtekSDXCSlot);

///
/// Postprocess the given SD command request
///
/// @param request A SD command request
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces the code block that flushes the FIFO buffer in `sdmmc_request()` defined in `rtsx_usb_sdmmc.c`.
///
IOReturn RealtekUSBSDXCSlot::postprocessRequest(IOSDHostRequest& request)
{
    using namespace RTSX::UCR::Chip::MC::FIFO;
    
    psoftassert(this->controller->writeChipRegister(rCTL, CTL::kFlush, CTL::kFlush) == kIOReturnSuccess,
                "Failed to flush the FIFO buffer.");
    
    return kIOReturnSuccess;
}

//
// MARK: - Tuning
//

///
/// Execute the tuning algorithm
///
/// @param config An I/O config that contains the current I/O settings
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `sdmmc_execute_tuning()` defined in `rtsx_usb_sdmmc.c`.
///
IOReturn RealtekUSBSDXCSlot::executeTuning(const IOSDBusConfig& config)
{
    // Guard: Check whether the card is still present
    if (!this->controller->isCardPresent())
    {
        perr("The card is not present. Will abort the request.");
        
        return kIOReturnNoMedia;
    }
    
    // Execute tuning for UHS-I SDR50 only
    if (config.busTiming != IOSDBusConfig::BusTiming::kUHSDDR50)
    {
        pinfo("No need to execute tuning for modes other than UHS-I DDR50.");
        
        return kIOReturnSuccess;
    }
    
    // Set the default TX phase
    IOReturn retVal = this->controller->changeTxPhase(0x01);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to set the default TX phase. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    // Tune the RX phase
    return this->tuningRx();
}
