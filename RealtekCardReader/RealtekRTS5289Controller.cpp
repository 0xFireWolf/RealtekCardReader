//
//  RealtekRTS5289Controller.cpp
//  RealtekCardReader
//
//  Created by FireWolf on 6/20/21.
//

#include "RealtekRTS5289Controller.hpp"

//
// MARK: - Meta Class Definitions
//

OSDefineMetaClassAndStructors(RealtekRTS5289Controller, RealtekRTS8411SeriesController);

//
// MARK: - SD Pull Control Tables
//

/// A sequence of registers to transfer to enable SD pull control
const RealtekRTS5289Controller::ChipRegValuePair RealtekRTS5289Controller::kSDEnablePullControlTablePairs[] =
{
    { RTSX::PCR::Chip::CARD::PULL::rCTL1, 0xAA },
    { RTSX::PCR::Chip::CARD::PULL::rCTL2, 0xAA },
    { RTSX::PCR::Chip::CARD::PULL::rCTL3, 0xA9 },
    { RTSX::PCR::Chip::CARD::PULL::rCTL4, 0x09 },
    { RTSX::PCR::Chip::CARD::PULL::rCTL5, 0x09 },
    { RTSX::PCR::Chip::CARD::PULL::rCTL6, 0x04 },
};

const RealtekRTS5289Controller::SimpleRegValuePairs RealtekRTS5289Controller::kSDEnablePullControlTable =
{
    RealtekRTS5289Controller::kSDEnablePullControlTablePairs
};

/// A sequence of registers to transfer to disable SD pull control
const RealtekRTS5289Controller::ChipRegValuePair RealtekRTS5289Controller::kSDDisablePullControlTablePairs[] =
{
    { RTSX::PCR::Chip::CARD::PULL::rCTL1, 0x65 },
    { RTSX::PCR::Chip::CARD::PULL::rCTL2, 0x55 },
    { RTSX::PCR::Chip::CARD::PULL::rCTL3, 0x95 },
    { RTSX::PCR::Chip::CARD::PULL::rCTL4, 0x09 },
    { RTSX::PCR::Chip::CARD::PULL::rCTL5, 0x05 },
    { RTSX::PCR::Chip::CARD::PULL::rCTL6, 0x04 },
};

const RealtekRTS5289Controller::SimpleRegValuePairs RealtekRTS5289Controller::kSDDisablePullControlTable =
{
    RealtekRTS5289Controller::kSDDisablePullControlTablePairs
};

//
// MARK: - Hardware Initialization and Configuration
//

///
/// Initialize hardware-specific parameters
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `*_init_params()` defined in each controller file.
/// @note Port: This function replaces `rtl8411_init_common_params()` defined in `rts8411.c`.
///             Concrete controllers must override and extend this function to set the pull control tables.
///
IOReturn RealtekRTS5289Controller::initParameters()
{
    IOReturn retVal = super::initParameters();
    
    if (retVal == kIOReturnSuccess)
    {
        this->parameters.sdEnablePullControlTable = &RealtekRTS5289Controller::kSDEnablePullControlTable;
        
        this->parameters.sdDisablePullControlTable = &RealtekRTS5289Controller::kSDDisablePullControlTable;
    }
    
    return retVal;
}
