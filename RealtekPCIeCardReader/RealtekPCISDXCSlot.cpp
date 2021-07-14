//
//  RealtekPCISDXCSlot.cpp
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 7/13/21.
//

#include "RealtekPCISDXCSlot.hpp"

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
    // Guard: Check whether the card is still present
    if (!this->controller->isCardPresent())
    {
        perr("The card is not present. Will abort the request.");
        
        return kIOReturnNoMedia;
    }
    
    // Notify the card reader to enter the worker state
    this->controller->enterWorkerState();
    
    IOReturn retVal = kIOReturnSuccess;
    
    switch (config.busTiming)
    {
        case IOSDBusConfig::BusTiming::kUHSSDR104:
        {
            pinfo("Will tune Rx and Tx for the UHS SDR104 mode.");
            
            retVal = this->controller->changeTxPhase(this->controller->getTxClockPhase().sdr104);
            
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
            
            retVal = this->controller->changeTxPhase(this->controller->getTxClockPhase().sdr50);
            
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
            
            retVal = this->controller->changeTxPhase(this->controller->getTxClockPhase().ddr50);
            
            if (retVal != kIOReturnSuccess)
            {
                perr("Failed to tune the Tx phase. Error = 0x%x.", retVal);
                
                break;
            }
            
            retVal = this->controller->changeRxPhase(this->controller->getRxClockPhase().ddr50);
            
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
