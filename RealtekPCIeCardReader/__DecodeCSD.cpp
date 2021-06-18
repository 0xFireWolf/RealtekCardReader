///
/// Decode the given raw card specific data
///
/// @param rcsd The raw card specific data
/// @return `true` on success, `false` otherwise.
/// @note Port: This function replaces `mmc_decode_csd()` defined in `sd.c`.
///
bool IOSDCard::decodeCardSpecificData(CSDVX& rcsd)
{
    /// Data access time (TAAC): Time unit in nanoseconds
    static constexpr UInt32 kTAACTimeUnits[] =
    {
        1, 10, 100, 1000, 10000, 100000, 1000000, 10000000,
    };

    /// Data access time (TAAC): Time value multipliers (scaled to 10x)
    static constexpr UInt32 kTAACTimeValues[] =
    {
        00, 10, 12, 13, 15, 20, 25, 30, // 1.0 - 3.0x
        35, 40, 45, 50, 55, 60, 70, 80, // 3.5 - 8.0x
    };
    
    /// Maximum data transfer rate (DTR): Rate unit (scaled to 1/10x)
    static constexpr UInt32 kDTRUnits[] =
    {
        10000,      // 100 Kbps
        100000,     //   1 Mbps
        1000000,    //  10 Mbps
        10000000,   // 100 Mbps
        0, 0, 0, 0,
    };

    /// Maximum data transfer rate (DTR): Rate multiplier (scaled to 10x)
    static constexpr UInt32 kDTRValues[] =
    {
        00, 10, 12, 13, 15, 20, 25, 30, // 1.0 - 3.0x
        35, 40, 45, 50, 55, 60, 70, 80, // 3.5 - 8.0x
    };
    
    UInt32 version = rcsd.data[0] >> 6;
    
    switch (version)
    {
        // SDSC cards
        case CSD::Version::k1:
        {
            this->isBlockAddressed = false;
            
            this->hasExtendedCapacity = false;
            
            this->csd.taacTimeNanosecs = (kTAACTimeUnits[rcsd.v1.taacTimeUnit] * kTAACTimeValues[rcsd.v1.taacTimeValue] + 9) / 10;
            
            this->csd.taacTimeClocks = rcsd.v1.nasc * 100;
            
            this->csd.maxDataTransferRate = kDTRUnits[rcsd.v1.maxTransferRateUnit] * kDTRValues[rcsd.v1.maxTransferRateValue];
            
            this->csd.cardCommandClasses = rcsd.v1.cardCommandClasses;
            
            this->csd.capacity = (1 + rcsd.v1.deviceSize) << (rcsd.v1.deviceSizeMultiplier + 2);
            
            this->csd.readBlockLength = rcsd.v1.maxReadDataBlockLength;
            
            this->csd.canReadPartialBlock = rcsd.v1.isPartialBlockReadAllowed;
            
            this->csd.canWriteMisalignedBlock = rcsd.v1.writeBlockMisalignment;
            
            this->csd.canReadMisalignedBlock = rcsd.v1.readBlockMisalignment;
            
            this->csd.isDSRImplemented = rcsd.v1.isDSRImplemented;
            
            this->csd.writeSpeedFactor = rcsd.v1.writeSpeedFactor;
            
            this->csd.writeBlockLength = rcsd.v1.maxWriteDataBlockLength;
            
            this->csd.canWritePartialBlock = rcsd.v1.isPartialBlockWriteAllowed;
            
            if (rcsd.v1.isEraseSingleBlockEnabled)
            {
                this->csd.eraseSize = 1;
            }
            else if (this->csd.writeBlockLength >= 9)
            {
                this->csd.eraseSize = rcsd.v1.eraseSectorSize + 1;
                
                this->csd.eraseSize <<= (this->csd.writeBlockLength - 9);
            }
            
            break;
        }
           
        // SDHC / SDXC cards
        case CSD::Version::k2:
        {
            this->isBlockAddressed = true;
            
            this->csd.taacTimeNanosecs = 0;
            
            this->csd.taacTimeClocks = 0;
            
            this->csd.maxDataTransferRate = kDTRUnits[rcsd.v2.maxTransferRateUnit] * kDTRValues[rcsd.v2.maxTransferRateValue];
            
            this->csd.cardCommandClasses = rcsd.v2.cardCommandClasses;
            
            this->csd.deviceSize = rcsd.v2.deviceSize;
            
            this->hasExtendedCapacity = this->csd.deviceSize >= 0xFFFF;
            
            this->csd.capacity = (1 + this->csd.deviceSize) << 10;
            
            this->csd.readBlockLength = 9;
            
            this->csd.canReadPartialBlock = false;
            
            this->csd.canWriteMisalignedBlock = false;
            
            this->csd.canReadMisalignedBlock = false;
            
            this->csd.writeSpeedFactor = 4; // Unused
            
            this->csd.writeBlockLength = 9;
            
            this->csd.canWritePartialBlock = false;
            
            this->csd.eraseSize = 1;
            
            break;
        }
            
        case CSD::Version::k3:
        {
            perr("SDUC card is not supported at this moment.");
            
            return false;
        }
            
        default:
        {
            perr("Unrecognized CSD structure version %d.", version);
            
            return false;
        }
    }
    
    return true;
}