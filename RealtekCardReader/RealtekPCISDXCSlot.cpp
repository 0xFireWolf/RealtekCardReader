//
//  RealtekPCISDXCSlot.cpp
//  RealtekCardReader
//
//  Created by FireWolf on 7/13/21.
//

#include "RealtekPCISDXCSlot.hpp"
#include "RealtekPCICardReaderController.hpp"

//
// MARK: - Meta Class Definitions
//

OSDefineMetaClassAndStructors(RealtekPCISDXCSlot, RealtekSDXCSlot);

///
/// Execute the tuning algorithm
///
/// @param config An I/O config that contains the current I/O settings
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `sdmmc_execute_tuning()` defined in `rtsx_pci_sdmmc.c`.
///
IOReturn RealtekPCISDXCSlot::executeTuning(const IOSDBusConfig& config)
{
    // Guard: Fetch the concrete controller
    auto controller = OSDynamicCast(RealtekPCICardReaderController, this->controller);
    
    passert(controller != nullptr, "The controller should not be null.");
    
    // Guard: Check whether the card is still present
    if (!controller->isCardPresent())
    {
        perr("The card is not present. Will abort the request.");
        
        return kIOReturnNoMedia;
    }
    
    // Notify the card reader to enter the worker state
    controller->enterWorkerState();
    
    IOReturn retVal = kIOReturnSuccess;
    
    switch (config.busTiming)
    {
        case IOSDBusConfig::BusTiming::kUHSSDR104:
        {
            pinfo("Will tune Rx and Tx for the UHS SDR104 mode.");
            
            retVal = controller->changeTxPhase(controller->getTxClockPhase().sdr104);
            
            if (retVal != kIOReturnSuccess)
            {
                perr("Failed to tune the Tx phase. Error = 0x%x.", retVal);
                
                break;
            }
            
            retVal = this->tuningRx();
            
            if (retVal != kIOReturnSuccess)
            {
                perr("Failed to tune the Rx phase. Error = 0x%x.", retVal);
            }
            
            break;
        }
            
        case IOSDBusConfig::BusTiming::kUHSSDR50:
        {
            pinfo("Will tune Rx and Tx for the UHS SDR50 mode.");
            
            retVal = controller->changeTxPhase(controller->getTxClockPhase().sdr50);
            
            if (retVal != kIOReturnSuccess)
            {
                perr("Failed to tune the Tx phase. Error = 0x%x.", retVal);
                
                break;
            }
            
            retVal = this->tuningRx();
            
            if (retVal != kIOReturnSuccess)
            {
                perr("Failed to tune the Rx phase. Error = 0x%x.", retVal);
            }
            
            break;
        }
            
        case IOSDBusConfig::BusTiming::kUHSDDR50:
        {
            pinfo("Will tune Rx and Tx for the UHS DDR50 mode.");
            
            retVal = controller->changeTxPhase(controller->getTxClockPhase().ddr50);
            
            if (retVal != kIOReturnSuccess)
            {
                perr("Failed to tune the Tx phase. Error = 0x%x.", retVal);
                
                break;
            }
            
            retVal = controller->changeRxPhase(controller->getRxClockPhase().ddr50);
            
            if (retVal != kIOReturnSuccess)
            {
                perr("Failed to tune the Rx phase. Error = 0x%x.", retVal);
            }
            
            break;
        }
            
        default:
        {
            break;
        }
    }
    
    return retVal;
}

//
// MARK: - IOService Implementations
//

///
/// Initialize the host device
///
/// @return `true` on success, `false` otherwise.
///
bool RealtekPCISDXCSlot::init(OSDictionary* dictionary)
{
    if (!super::init(dictionary))
    {
        return false;
    }
    
    // Configure the request factory for PCIe-based card readers
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
        OSMemberFunctionCast(IOSDHostRequest::Processor, this, &RealtekSDXCSlot::processSDCommandWithInboundSingleBlockDMATransferRequest),
        
        // Outbound single block request processor
        OSMemberFunctionCast(IOSDHostRequest::Processor, this, &RealtekSDXCSlot::processSDCommandWithOutboundSingleBlockDMATransferRequest),
        
        // Inbound multiple blocks request processor
        OSMemberFunctionCast(IOSDHostRequest::Processor, this, &RealtekSDXCSlot::processSDCommandWithInboundMultiBlocksDMATransferRequest),
        
        // Outbound multiple blocks request processor
        OSMemberFunctionCast(IOSDHostRequest::Processor, this, &RealtekSDXCSlot::processSDCommandWithOutboundMultiBlocksDMATransferRequest),
    };
    
    return true;
}
