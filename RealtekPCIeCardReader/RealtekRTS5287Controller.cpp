//
//  RealtekRTS5287Controller.cpp
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 6/20/21.
//

#include "RealtekRTS5287Controller.hpp"

//
// MARK: - Meta Class Definitions
//

OSDefineMetaClassAndStructors(RealtekRTS5287Controller, RealtekRTS8411SeriesController);

//
// MARK: - SD Pull Control Tables
//

/// A sequence of registers to transfer to enable SD pull control (QFN48)
const RealtekRTS5287Controller::ChipRegValuePair RealtekRTS5287Controller::kSDEnablePullControlTablePairsQFN48[] =
{
    { RTSX::PCR::Chip::CARD::PULL::rCTL2, 0xAA },
    { RTSX::PCR::Chip::CARD::PULL::rCTL3, 0x69 | 0x90 },
    { RTSX::PCR::Chip::CARD::PULL::rCTL6, 0x08 | 0x11 },
};

const RealtekRTS5287Controller::SimpleRegValuePairs RealtekRTS5287Controller::kSDEnablePullControlTableQFN48 =
{
    RealtekRTS5287Controller::kSDEnablePullControlTablePairsQFN48
};

/// A sequence of registers to transfer to disable SD pull control (QFN48)
const RealtekRTS5287Controller::ChipRegValuePair RealtekRTS5287Controller::kSDDisablePullControlTablePairsQFN48[] =
{
    { RTSX::PCR::Chip::CARD::PULL::rCTL2, 0x55 },
    { RTSX::PCR::Chip::CARD::PULL::rCTL3, 0x65 | 0x90 },
    { RTSX::PCR::Chip::CARD::PULL::rCTL6, 0x04 | 0x11 },
};

const RealtekRTS5287Controller::SimpleRegValuePairs RealtekRTS5287Controller::kSDDisablePullControlTableQFN48 =
{
    RealtekRTS5287Controller::kSDDisablePullControlTablePairsQFN48
};

/// A sequence of registers to transfer to enable SD pull control (QFN64)
const RealtekRTS5287Controller::ChipRegValuePair RealtekRTS5287Controller::kSDEnablePullControlTablePairsQFN64[] =
{
    { RTSX::PCR::Chip::CARD::PULL::rCTL1, 0xAA },
    { RTSX::PCR::Chip::CARD::PULL::rCTL2, 0xAA },
    { RTSX::PCR::Chip::CARD::PULL::rCTL3, 0x09 | 0xD0 },
    { RTSX::PCR::Chip::CARD::PULL::rCTL4, 0x09 | 0x50 },
    { RTSX::PCR::Chip::CARD::PULL::rCTL5, 0x05 | 0x50 },
    { RTSX::PCR::Chip::CARD::PULL::rCTL6, 0x04 | 0x11 },
};

const RealtekRTS5287Controller::SimpleRegValuePairs RealtekRTS5287Controller::kSDEnablePullControlTableQFN64 =
{
    RealtekRTS5287Controller::kSDEnablePullControlTablePairsQFN64
};

/// A sequence of registers to transfer to disable SD pull control (QFN64)
const RealtekRTS5287Controller::ChipRegValuePair RealtekRTS5287Controller::kSDDisablePullControlTablePairsQFN64[] =
{
    { RTSX::PCR::Chip::CARD::PULL::rCTL1, 0x65 },
    { RTSX::PCR::Chip::CARD::PULL::rCTL2, 0x55 },
    { RTSX::PCR::Chip::CARD::PULL::rCTL3, 0x05 | 0xD0 },
    { RTSX::PCR::Chip::CARD::PULL::rCTL4, 0x09 | 0x50 },
    { RTSX::PCR::Chip::CARD::PULL::rCTL5, 0x05 | 0x50 },
    { RTSX::PCR::Chip::CARD::PULL::rCTL6, 0x04 | 0x11 },
};

const RealtekRTS5287Controller::SimpleRegValuePairs RealtekRTS5287Controller::kSDDisablePullControlTableQFN64 =
{
    RealtekRTS5287Controller::kSDDisablePullControlTablePairsQFN64
};

//
// MARK: - Hardware Initialization and Configuration
//

///
/// Check whether the package mode is QFN48
///
/// @param result Set to `true` if the package mode is QFN48 on return
/// @return `kIOReturnSuccess` on success, other values if failed to read the register.
///
IOReturn RealtekRTS5287Controller::isQFN48(bool& result)
{
    UInt8 mode;
    
    IOReturn retVal = this->readChipRegister(RTSX::PCR::Chip::rPKGMODE, mode);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to read the package mode. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    result = (mode & 0x02) == 0x02;
    
    return kIOReturnSuccess;
}

///
/// Initialize hardware-specific parameters
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `*_init_params()` defined in each controller file.
/// @note Port: This function replaces `rtl8411_init_common_params()` defined in `rts8411.c`.
///             Concrete controllers must override and extend this function to set the pull control tables.
///
IOReturn RealtekRTS5287Controller::initParameters()
{
    IOReturn retVal = super::initParameters();
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to fetch the common device-specific parameters. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    bool result = false;
    
    retVal = this->isQFN48(result);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to check whether the package mode is QFN48. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    if (result)
    {
        this->parameters.sdEnablePullControlTable = &RealtekRTS5287Controller::kSDEnablePullControlTableQFN48;
        
        this->parameters.sdDisablePullControlTable = &RealtekRTS5287Controller::kSDDisablePullControlTableQFN48;
    }
    else
    {
        this->parameters.sdEnablePullControlTable = &RealtekRTS5287Controller::kSDEnablePullControlTableQFN64;
        
        this->parameters.sdDisablePullControlTable = &RealtekRTS5287Controller::kSDDisablePullControlTableQFN64;
    }
    
    return kIOReturnSuccess;
}

///
/// Initialize vendor-specific parameters
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `fetch_vendor_settings()` defined in the `pcr->ops`.
/// @note Port: This function replaces `rtl8411_fetch_vendor_settings()` defined in `rts8411.c`.
///             RTS5287 controller must override this function.
///
IOReturn RealtekRTS5287Controller::initVendorSpecificParameters()
{
    pinfo("Initializing the vendor-specific parameters...");
    
    // PCR Config 1
    UInt32 regVal = this->device->configRead32(RTSX::PCR::kSREG1);
    
    if (!this->vsIsRegisterValueValid(regVal))
    {
        perr("Vendor settings are invalid.");
        
        return kIOReturnError;
    }
    
    this->parameters.pm.isASPML1Enabled = this->vsIsASPML1Enabled(regVal);
    
    this->parameters.sd30DriveSelector1d8V = this->vsMapDriveSelector(this->vsGetSD30DriveSelector1d8V(regVal));
    
    this->parameters.sd30DriveSelector3d3V = this->vsMapDriveSelector(this->vsGetSD30DriveSelector3d3V(regVal));
    
    pinfo("PCR CFG1: RegVal = 0x%08x.", regVal);
    
    pinfo("PCR CFG1: ASPM L1 Enabled: %s.", YESNO(this->parameters.pm.isASPML1Enabled));
    
    pinfo("PCR CFG1: SD30 Drive Selector (1.8V) = %d.", this->parameters.sd30DriveSelector1d8V);
    
    pinfo("PCR CFG1: SD30 Drive Selector (3.3V) = %d.", this->parameters.sd30DriveSelector3d3V);
    
    pinfo("Vendor-specific parameters have been initialized.");
    
    return kIOReturnSuccess;
}

///
/// Initialize the hardware (Extra Part)
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `extra_init_hw()` defined in defined in the `pcr->ops`.
/// @note Port: This function replaces `rtl8411_extra_init_hw()` defined in `rts8411.c`.
///             RTS5287 controller must override this function.
///
IOReturn RealtekRTS5287Controller::initHardwareExtra()
{
    using namespace RTSX::PCR::Chip;
    
    const ChipRegValuePair pairs[] =
    {
        // QFN48 only
        { CARD::PULL::rCTL3, 0xFF, 0xF5 },
        
        // Set the initial driving selector
        { CARD::SD30::DRVSEL::rCFG, 0xFF, this->parameters.sd30DriveSelector3d3V },
        
        // Enable the card
        { CARD::rPADCTL, CARD::PADCTL::kDisableMask | CARD::PADCTL::kAutoDisable, CARD::PADCTL::kEnableAll },
        
        // ???
        { rFCTL, 0x06, 0x00 }
    };
    
    bool result = false;
    
    IOReturn retVal = this->isQFN48(result);
    
    if (retVal != kIOReturnSuccess)
    {
        perr("Failed to check whether the package mode is QFN48. Error = 0x%x.", retVal);
        
        return retVal;
    }
    
    if (result)
    {
        return this->transferWriteRegisterCommands(SimpleRegValuePairs(pairs));
    }
    else
    {
        return this->transferWriteRegisterCommands(SimpleRegValuePairs(pairs + 1, arrsize(pairs) - 1));
    }
}
