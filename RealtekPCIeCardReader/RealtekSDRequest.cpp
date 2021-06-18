//
//  RealtekSDRequest.cpp
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 5/30/21.
//

#include "RealtekSDRequest.hpp"
#include "RealtekSDXCSlot.hpp"

///
/// Service the request
///
/// @param slot A non-null Realtek card slot instance.
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note This function exploits a simple visitor pattern to select the service routine based on the request type.
///
IOReturn RealtekSDSimpleCommandRequest::service(RealtekSDXCSlot* slot)
{
    return slot->runSDCommand(*this);
}

///
/// Service the request
///
/// @param slot A non-null Realtek card slot instance.
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note This function exploits a simple visitor pattern to select the service routine based on the request type.
///
IOReturn RealtekSDCommandWithInboundDataTransferRequest::service(RealtekSDXCSlot* slot)
{
    return slot->runSDCommandWithInboundDataTransfer(*this);
}

///
/// Service the request
///
/// @param slot A non-null Realtek card slot instance.
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note This function exploits a simple visitor pattern to select the service routine based on the request type.
///
IOReturn RealtekSDCommandWithOutboundDataTransferRequest::service(RealtekSDXCSlot* slot)
{
    return slot->runSDCommandWithOutboundDataTransfer(*this);
}

///
/// Service the request
///
/// @param slot A non-null Realtek card slot instance.
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note This function exploits a simple visitor pattern to select the service routine based on the request type.
///
IOReturn RealtekSDSingleBlockReadRequest::service(RealtekSDXCSlot* slot)
{
    return slot->runSDCommandWithInboundDMATransfer(*this);
}

///
/// Service the request
///
/// @param slot A non-null Realtek card slot instance.
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note This function exploits a simple visitor pattern to select the service routine based on the request type.
///
IOReturn RealtekSDSingleBlockWriteRequest::service(RealtekSDXCSlot* slot)
{
    return slot->runSDCommandWithOutboundDMATransfer(*this);
}

///
/// Service the request
///
/// @param slot A non-null Realtek card slot instance.
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note This function exploits a simple visitor pattern to select the service routine based on the request type.
///
IOReturn RealtekSDMultiBlockReadRequest::service(RealtekSDXCSlot* slot)
{
    IOReturn retVal = RealtekSDSingleBlockReadRequest::service(slot);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to service the request that reads multiple blocks. Error = 0x%x.", retVal);
    }
    
    psoftassert(slot->runSDCommand(this->stopCommand) == kIOReturnSuccess, "Failed to send the STOP command.");
    
    return retVal;
}

///
/// Service the request
///
/// @param slot A non-null Realtek card slot instance.
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note This function exploits a simple visitor pattern to select the service routine based on the request type.
///
IOReturn RealtekSDMultiBlockWriteRequest::service(RealtekSDXCSlot* slot)
{
    IOReturn retVal = RealtekSDSingleBlockWriteRequest::service(slot);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to service the request that writes multiple blocks. Error = 0x%x.", retVal);
    }
    
    psoftassert(slot->runSDCommand(this->stopCommand) == kIOReturnSuccess, "Failed to send the STOP command.");
    
    return retVal;
}
