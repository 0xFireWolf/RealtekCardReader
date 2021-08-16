//
//  IOSDHostDevice.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 6/5/21.
//

#ifndef IOSDHostDevice_hpp
#define IOSDHostDevice_hpp

#include "IOSDHostRequest.hpp"
#include "IOSDBusConfig.hpp"
#include "ClosedRange.hpp"
#include "BitOptions.hpp"
#include "IOSDCard.hpp"

/// Forward declaration (Client of the SD host device)
class IOSDHostDriver;

///
/// Represents an abstract SD host device independent of the underlying card reader controller
///
class IOSDHostDevice: public IOService
{
    //
    // MARK: - Constructors & Destructors
    //
    
    OSDeclareAbstractStructors(IOSDHostDevice);
    
    using super = IOService;
    
    //
    // MARK: - Public Data Structures
    //
    
public:
    /// The maximum current supported by the host device at different signaling voltage levels
    struct MaxCurrents
    {
        /// The maximum current in mA (1.8V)
        UInt32 v18;
        
        /// The maximum current in mA (3.0V)
        UInt32 v30;
        
        /// The maximum current in mA (3.3V)
        UInt32 v33;
    };
    
    /// Limitations imposed by the host device on DMA transactions
    struct DMALimits
    {
        /// The maximum number of segments in the scatter/gather list
        UInt32 maxNumSegments;
        
        /// The maximum number of bytes in one segment
        UInt32 maxSegmentSize;
        
        /// The maximum number of bytes in one DMA transaction
        UInt64 maxRequestSize;
        
        /// Get the maximum number of blocks in one DMA transaction
        inline UInt64 maxRequestNumBlocks() const
        {
            return this->maxRequestSize / 512;
        }
    };
    
    ///
    /// Enumerates all host device capabilities
    ///
    /// @note Not all capabilities are supported by the macOS driver.
    /// @note Not all capabilities are applicable to a SD card reader.
    /// @ref /include/linux/mmc/host.h
    ///
    enum Capability: UInt64
    {
        // ----------
        // | Caps 1 |
        // ----------
        
        /// Can the host do 4 bit transfers
        /// @ref MMC_CAP_4_BIT_DATA
        k4BitData = 1ULL << 0,
        
        /// Can do MMC high-speed timing
        /// @ref MMC_CAP_MMC_HIGHSPEED
        kMMCHighSpeed = 1ULL << 1,
        
        /// Can do SD high-speed timing
        /// @ref MMC_CAP_SD_HIGHSPEED
        kSDHighSpeed = 1ULL << 2,
        
        /// Host supports testing the bus width (MMC CMD14/CMD19)
        /// @ref MMC_CAP_BUS_WIDTH_TEST
        kBusWidthTest = 1ULL << 15,
        
        /// Host supports the UHS SDR12 mode
        /// @ref MMC_CAP_UHS_SDR12
        kUHSSDR12 = 1ULL << 16,
        
        /// Host supports the UHS SDR25 mode
        /// @ref MMC_CAP_UHS_SDR25
        kUHSSDR25 = 1ULL << 17,
        
        /// Host supports the UHS SDR50 mode
        /// @ref MMC_CAP_UHS_SDR50
        kUHSSDR50 = 1ULL << 18,
        
        /// Host supports the UHS SDR104 mode
        /// @ref MMC_CAP_UHS_SDR104
        kUHSSDR104 = 1ULL << 19,
        
        /// Host supports the UHS DDR50 mode
        /// @ref MMC_CAP_UHS_DDR50
        kUHSDDR50 = 1ULL << 20,
        
        // ----------
        // | Caps 2 |
        // ----------
        
        /// Can do a full power cycle
        /// @ref MMC_CAP2_FULL_PWR_CYCLE
        kFullPowerCycle = 1ULL << (32 + 2),
        
        /// Do not power up the bus before the scan
        /// @ref MMC_CAP2_NO_PRESCAN_POWERUP
        kNoPrescanPowerUp = 1ULL << (32 + 14),
        
        /// Optimize the function to set the chip select mode
        kOptimizeChipSelect = 1ULL << (32 + 31),
    };
    
    //
    // MARK: - Host Properties
    //
    
protected:
    /// The host driver (client)
    IOSDHostDriver* driver;
    
    /// The host bus configuration
    IOSDBusConfig busConfig;
    
    /// The maximum currents supported by the host at different signaling voltage levels
    MaxCurrents maxCurrents;
    
    /// The range of clock frequencies (in Hz) supported by the host
    ClosedRange<UInt32> clockRange;
    
    /// The initial clock frequency (in Hz)
    UInt32 initialClock;
    
    /// The range of voltage levels supported by the host (as an OCR value)
    UInt32 supportedVoltageRanges;
    
    /// Host device capabilities
    BitOptions<UInt64> capabilities;
    
    /// Host DMA limitations
    DMALimits dmaLimits;
    
    /// A factory that creates host requests
    IOSDHostRequestFactory factory;
    
public:
    //
    // MARK: - SD Request Processors
    //
    
    ///
    /// Preprocess the given SD command request
    ///
    /// @param request A SD command request
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `sdmmc_pre_req()` defined in `rtsx_pci_sdmmc.c`.
    ///
    virtual IOReturn preprocessRequest(IOSDHostRequest& request);
    
    ///
    /// Process the given SD command request
    ///
    /// @param request A SD command request
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `sdmmc_request()` defined in `rtsx_pci_sdmmc.c`.
    ///
    virtual IOReturn processRequest(IOSDHostRequest& request) = 0;
    
    ///
    /// Postprocess the given SD command request
    ///
    /// @param request A SD command request
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `sdmmc_post_req()` defined in `rtsx_pci_sdmmc.c`.
    ///
    virtual IOReturn postprocessRequest(IOSDHostRequest& request);
    
    //
    // MARK: - SD Bus Configurator
    //
    
    ///
    /// Apply the given I/O configuration
    ///
    /// @param config An I/O config
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `sdmmc_set_ios()` defined in `rtsx_pci_sdmmc.c`.
    ///
    virtual IOReturn setBusConfig(const IOSDBusConfig& config) = 0;
    
    ///
    /// Switch the signal voltage
    ///
    /// @param config An I/O config that contains the target signal voltage level
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replace `sdmmc_switch_voltage()` defined in `rtsx_pci_sdmmc.c`.
    ///
    virtual IOReturn switchSignalVoltage(const IOSDBusConfig& config) = 0;
    
    //
    // MARK: - Tuning
    //
    
    ///
    /// Execute the tuning algorithm
    ///
    /// @param config An I/O config that contains the current I/O settings
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `sdmmc_execute_tuning()` defined in `rtsx_pci_sdmmc.c`.
    ///
    virtual IOReturn executeTuning(const IOSDBusConfig& config) = 0;
    
    //
    // MARK: - Card Status
    //
    
    ///
    /// Check whether the card has write protection enabled
    ///
    /// @param result Set `true` if the card is write protected, `false` otherwise.
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `sdmmc_get_ro()` defined in `rtsx_pci_sdmmc.c`.
    ///
    virtual IOReturn isCardWriteProtected(bool& result) = 0;
    
    ///
    /// Check whether the card exists
    ///
    /// @param result Set `true` if the card exists, `false` otherwise.
    /// @return `kIOReturnSuccess` always.
    /// @note Port: This function replaces `sdmmc_get_cd()` defined in `rtsx_pci_sdmmc.c`.
    ///
    virtual IOReturn isCardPresent(bool& result) = 0;
    
    ///
    /// Check whether the command line of the card is busy
    ///
    /// @param result Set `true` if the card CMD line is high, `false` otherwise.
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    ///
    virtual IOReturn isCardCommandLineBusy(bool& result) = 0;
    
    ///
    /// Check whether the data line of the card is busy
    ///
    /// @param result Set `true` if the card DAT lines are high, `false` otherwise.
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    ///
    virtual IOReturn isCardDataLineBusy(bool& result) = 0;
    
    //
    // MARK: - Host Capabilities
    //
    
    ///
    /// Get the current host bus configuration
    ///
    /// @return A reference to the host bus configuration.
    ///
    inline IOSDBusConfig& getHostBusConfig()
    {
        return this->busConfig;
    }
    
    ///
    /// Get the maximum currents supported by the host at different signaling voltage levels
    ///
    /// @return The host maximum currents.
    ///
    inline MaxCurrents getHostMaxCurrents()
    {
        return this->maxCurrents;
    }
    
    ///
    /// Get the range of clock frequencies supported by the host
    ///
    /// @return The host clock range.
    ///
    inline ClosedRange<UInt32> getHostClockRange()
    {
        return this->clockRange;
    }
    
    ///
    /// Get the initial clock frequency
    ///
    /// @return The host initial clock frequency.
    ///
    inline UInt32 getHostInitialClock()
    {
        return this->initialClock;
    }
    
    ///
    /// Set the initial clock frequency
    ///
    /// @param clock The host initial clock frequency
    ///
    inline void setHostInitialClock(UInt32 clock)
    {
        this->initialClock = clock;
    }
    
    ///
    /// Get the host supported ranges of voltage levels
    ///
    /// @return The voltage range as an OCR value.
    ///
    inline UInt32 getHostSupportedVoltageRanges()
    {
        return this->supportedVoltageRanges;
    }
    
    ///
    /// Get the host capabilities
    ///
    /// @return The host capabilities.
    ///
    inline BitOptions<UInt64> getCapabilities()
    {
        return this->capabilities;
    }
    
    ///
    /// Get the host DMA limitations
    ///
    /// @return The host DMA limitations.
    ///
    inline DMALimits getDMALimits()
    {
        return this->dmaLimits;
    }
    
    ///
    /// Get the factory that creates host requests
    ///
    /// @return The host request factory.
    ///
    inline const IOSDHostRequestFactory& getRequestFactory()
    {
        return this->factory;
    }
    
    //
    // MARK: - Card Events Callbacks
    //
    
    ///
    /// [UPCALL] Notify the host device when a SD card is inserted
    ///
    /// @param completion A nullable completion routine to be invoked when the card is attached
    /// @note This callback function runs in a gated context provided by the underlying card reader controller.
    ///       The host device should implement this function without any blocking operations.
    ///       A default implementation that notifies the host driver is provided.
    ///
    virtual void onSDCardInsertedGated(IOSDCard::Completion* completion = nullptr);
    
    ///
    /// [UPCALL] Notify the host device when a SD card is removed
    ///
    /// @param completion A nullable completion routine to be invoked when the card is detached
    /// @note This callback function runs in a gated context provided by the underlying card reader controller.
    ///       The host device should implement this function without any blocking operations.
    ///       A default implementation that notifies the host driver is provided.
    ///
    virtual void onSDCardRemovedGated(IOSDCard::Completion* completion = nullptr);
    
    //
    // MARK: - Power Management
    //
    
    ///
    /// Adjust the power state in response to system-wide power events
    ///
    /// @param powerStateOrdinal The number in the power state array of the state the driver is being instructed to switch to
    /// @param whatDevice A pointer to the power management object which registered to manage power for this device
    /// @return `kIOPMAckImplied` always.
    ///
    IOReturn setPowerState(unsigned long powerStateOrdinal, IOService* whatDevice) override;
    
    //
    // MARK: - IOService Implementation
    //
    
    ///
    /// Initialize the host device
    ///
    /// @param dictionary A nullable matching dictionary
    /// @return `true` on success, `false` otherwise.
    ///
    bool init(OSDictionary* dictionary = nullptr) override;
    
    ///
    /// Start the host device
    ///
    /// @param provider An instance of card reader controller
    /// @return `true` on success, `false` otherwise.
    ///
    bool start(IOService* provider) override;
    
    ///
    /// Stop the host device
    ///
    /// @param provider An instance of card reader controller
    ///
    void stop(IOService* provider) override;
};

#endif /* IOSDHostDevice_hpp */
