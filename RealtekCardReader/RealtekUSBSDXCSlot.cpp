//
//  RealtekUSBSDXCSlot.cpp
//  RealtekCardReader
//
//  Created by FireWolf on 7/13/21.
//

#include "RealtekUSBSDXCSlot.hpp"
#include "RealtekUSBRegisters.hpp"
#include "RealtekUSBCardReaderController.hpp"

//
// MARK: - Meta Class Definitions
//

OSDefineMetaClassAndStructors(RealtekUSBSDXCSlot, RealtekSDXCSlot);

///
/// Preprocess the given SD command request
///
/// @param request A SD command request
/// @return `kIOReturnSuccess` on success, other values otherwise.
///
IOReturn RealtekUSBSDXCSlot::preprocessRequest(IOSDHostRequest& request)
{
    auto controller = OSDynamicCast(RealtekUSBCardReaderController, this->controller);
    
    passert(controller != nullptr, "The controller is not valid.");
    
    controller->pausePollingThread();
    
    return kIOReturnSuccess;
}

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
    
    auto controller = OSDynamicCast(RealtekUSBCardReaderController, this->controller);
    
    passert(controller != nullptr, "The controller is not valid.");
    
    controller->resumePollingThread();
    
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
    
    // Execute tuning for UHS-I SDR50/104 only
    if (config.busTiming == IOSDBusConfig::BusTiming::kUHSDDR50)
    {
        pinfo("No need to execute tuning for cards running in the UHS-I DDR50 mode.");
        
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

//
// MARK: - IOService Implementations
//

///
/// Initialize the host device
///
/// @return `true` on success, `false` otherwise.
///
bool RealtekUSBSDXCSlot::init(OSDictionary* dictionary)
{
    if (!super::init(dictionary))
    {
        return false;
    }
    
    // Configure the request factory for USB-based card readers
    this->factory =
    {
        // Opaque target pointer
        this,
        
        // Simple command request processor
        OSMemberFunctionCast(IOSDHostRequest::Processor, this, &RealtekSDXCSlot::processSDCommandRequest),
        
        // Inbound data transfer request processor
        OSMemberFunctionCast(IOSDHostRequest::Processor, this, &RealtekSDXCSlot::processSDCommandWithInboundDataTransferRequest),
        
        // Outbound data transfer request processor
        OSMemberFunctionCast(IOSDHostRequest::Processor, this, &RealtekSDXCSlot::processSDCommandWithOutboundDataTransferRequest),
        
        // Inbound single block request processor
        OSMemberFunctionCast(IOSDHostRequest::Processor, this, &RealtekSDXCSlot::processSDCommandWithInboundDataTransferRequest),
        
        // Outbound single block request processor
        OSMemberFunctionCast(IOSDHostRequest::Processor, this, &RealtekSDXCSlot::processSDCommandWithOutboundDataTransferRequest),
        
        // Inbound multiple blocks request processor
        OSMemberFunctionCast(IOSDHostRequest::Processor, this, &RealtekSDXCSlot::processSDCommandWithInboundMultiBlocksDMATransferRequest),
        
        // Outbound multiple blocks request processor
        OSMemberFunctionCast(IOSDHostRequest::Processor, this, &RealtekSDXCSlot::processSDCommandWithOutboundMultiBlocksDMATransferRequest),
    };
    
    return true;
}
