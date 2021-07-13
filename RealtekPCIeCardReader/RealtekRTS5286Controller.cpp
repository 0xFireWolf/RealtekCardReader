//
//  RealtekRTS5286Controller.cpp
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 6/20/21.
//

#include "RealtekRTS5286Controller.hpp"

//
// MARK: - Meta Class Definitions
//

OSDefineMetaClassAndStructors(RealtekRTS5286Controller, RealtekRTS8411SeriesController);

//
// MARK: - SD Pull Control Tables
//

/// A sequence of registers to transfer to enable SD pull control
const RealtekRTS5286Controller::ChipRegValuePair RealtekRTS5286Controller::kSDEnablePullControlTablePairs[] =
{
    { RTSX::Chip::CARD::PULL::rCTL1, 0xAA },
    { RTSX::Chip::CARD::PULL::rCTL2, 0xAA },
    { RTSX::Chip::CARD::PULL::rCTL3, 0xA9 },
    { RTSX::Chip::CARD::PULL::rCTL4, 0x09 },
    { RTSX::Chip::CARD::PULL::rCTL5, 0x09 },
    { RTSX::Chip::CARD::PULL::rCTL6, 0x04 },
};

const RealtekRTS5286Controller::SimpleRegValuePairs RealtekRTS5286Controller::kSDEnablePullControlTable =
{
    RealtekRTS5286Controller::kSDEnablePullControlTablePairs
};

/// A sequence of registers to transfer to disable SD pull control
const RealtekRTS5286Controller::ChipRegValuePair RealtekRTS5286Controller::kSDDisablePullControlTablePairs[] =
{
    { RTSX::Chip::CARD::PULL::rCTL1, 0x65 },
    { RTSX::Chip::CARD::PULL::rCTL2, 0x55 },
    { RTSX::Chip::CARD::PULL::rCTL3, 0x95 },
    { RTSX::Chip::CARD::PULL::rCTL4, 0x09 },
    { RTSX::Chip::CARD::PULL::rCTL5, 0x05 },
    { RTSX::Chip::CARD::PULL::rCTL6, 0x04 },
};

const RealtekRTS5286Controller::SimpleRegValuePairs RealtekRTS5286Controller::kSDDisablePullControlTable =
{
    RealtekRTS5286Controller::kSDDisablePullControlTablePairs
};

//
// MARK: - Card Power Management
//

///
/// Switch to the given output voltage
///
/// @param outputVoltage The new output voltage
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note Port: This function replaces `rtsx_pci_switch_output_voltage()` defined in `rtsx_psr.c`.
/// @note Port: This function replaces `rtl8411_switch_output_voltage()` definde in `rts8411.c`.
///             RTS5286 controller must override this function and provide the correct shift and ASIC values.
///
IOReturn RealtekRTS5286Controller::switchOutputVoltage(OutputVoltage outputVoltage)
{
    using namespace RTSX::Chip;
    
    return super::switchOutputVoltage(outputVoltage, LDO::CTL::kBppTuned18Shift_8402, LDO::CTL::kBppAsic2V0);
}

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
IOReturn RealtekRTS5286Controller::initParameters()
{
    IOReturn retVal = super::initParameters();
    
    if (retVal == kIOReturnSuccess)
    {
        this->parameters.sdEnablePullControlTable = &RealtekRTS5286Controller::kSDEnablePullControlTable;
        
        this->parameters.sdDisablePullControlTable = &RealtekRTS5286Controller::kSDDisablePullControlTable;
    }
    
    return retVal;
}
