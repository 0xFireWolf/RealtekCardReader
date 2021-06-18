//
//  IOSDHostDevice.hpp
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 6/5/21.
//

#ifndef IOSDHostDevice_hpp
#define IOSDHostDevice_hpp

#include "IOSDBusConfig.hpp"
#include "ClosedRange.hpp"
#include "BitOptions.hpp"
#include "RealtekSDRequest.hpp" // FIXME: SHOULD BE GENERIC

// TODO: Convert to C++

///
/// Enumerates all host device capabilities
///
/// @note Not all capabilities are supported by the macOS driver.
/// @note Not all capabilities are applicable to a SD card dreader.
/// @ref /include/linux/mmc/host.h
///
enum Capability
{
    // ----------
    // | Caps 1 |
    // ----------
    
    /// Can the host do 4 bit transfers
    /// @ref MMC_CAP_4_BIT_DATA
    k4BitData = 1 << 0,
    
    /// Can do MMC high-speed timing
    /// @ref MMC_CAP_MMC_HIGHSPEED
    kMMCHighSpeec = 1 << 1,
    
    
    
};

#define MMC_CAP_MMC_HIGHSPEED    (1 << 1)    /*  */
#define MMC_CAP_SD_HIGHSPEED    (1 << 2)    /* Can do SD high-speed timing */
#define MMC_CAP_SDIO_IRQ    (1 << 3)    /* Can signal pending SDIO IRQs */
#define MMC_CAP_SPI        (1 << 4)    /* Talks only SPI protocols */
#define MMC_CAP_NEEDS_POLL    (1 << 5)    /* Needs polling for card-detection */
#define MMC_CAP_8_BIT_DATA    (1 << 6)    /* Can the host do 8 bit transfers */
#define MMC_CAP_AGGRESSIVE_PM    (1 << 7)    /* Suspend (e)MMC/SD at idle  */
#define MMC_CAP_NONREMOVABLE    (1 << 8)    /* Nonremovable e.g. eMMC */
#define MMC_CAP_WAIT_WHILE_BUSY    (1 << 9)    /* Waits while card is busy */
#define MMC_CAP_3_3V_DDR    (1 << 11)    /* Host supports eMMC DDR 3.3V */
#define MMC_CAP_1_8V_DDR    (1 << 12)    /* Host supports eMMC DDR 1.8V */
#define MMC_CAP_1_2V_DDR    (1 << 13)    /* Host supports eMMC DDR 1.2V */
#define MMC_CAP_DDR        (MMC_CAP_3_3V_DDR | MMC_CAP_1_8V_DDR | \
                 MMC_CAP_1_2V_DDR)
#define MMC_CAP_POWER_OFF_CARD    (1 << 14)    /* Can power off after boot */
#define MMC_CAP_BUS_WIDTH_TEST    (1 << 15)    /* CMD14/CMD19 bus width ok */
#define MMC_CAP_UHS_SDR12    (1 << 16)    /* Host supports UHS SDR12 mode */
#define MMC_CAP_UHS_SDR25    (1 << 17)    /* Host supports UHS SDR25 mode */
#define MMC_CAP_UHS_SDR50    (1 << 18)    /* Host supports UHS SDR50 mode */
#define MMC_CAP_UHS_SDR104    (1 << 19)    /* Host supports UHS SDR104 mode */
#define MMC_CAP_UHS_DDR50    (1 << 20)    /* Host supports UHS DDR50 mode */
#define MMC_CAP_UHS        (MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 | \
                 MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_SDR104 | \
                 MMC_CAP_UHS_DDR50)
#define MMC_CAP_SYNC_RUNTIME_PM    (1 << 21)    /* Synced runtime PM suspends. */
#define MMC_CAP_NEED_RSP_BUSY    (1 << 22)    /* Commands with R1B can't use R1. */
#define MMC_CAP_DRIVER_TYPE_A    (1 << 23)    /* Host supports Driver Type A */
#define MMC_CAP_DRIVER_TYPE_C    (1 << 24)    /* Host supports Driver Type C */
#define MMC_CAP_DRIVER_TYPE_D    (1 << 25)    /* Host supports Driver Type D */
#define MMC_CAP_DONE_COMPLETE    (1 << 27)    /* RW reqs can be completed within mmc_request_done() */
#define MMC_CAP_CD_WAKE        (1 << 28)    /* Enable card detect wake */
#define MMC_CAP_CMD_DURING_TFR    (1 << 29)    /* Commands during data transfer */
#define MMC_CAP_CMD23        (1 << 30)    /* CMD23 supported. */
#define MMC_CAP_HW_RESET    (1 << 31)    /* Reset the eMMC card via RST_n */

    //u32            caps2;        /* More host capabilities */

#define MMC_CAP2_BOOTPART_NOACC    (1 << 0)    /* Boot partition no access */
#define MMC_CAP2_FULL_PWR_CYCLE    (1 << 2)    /* Can do full power cycle */
#define MMC_CAP2_FULL_PWR_CYCLE_IN_SUSPEND (1 << 3) /* Can do full power cycle in suspend */
#define MMC_CAP2_HS200_1_8V_SDR    (1 << 5)        /* can support */
#define MMC_CAP2_HS200_1_2V_SDR    (1 << 6)        /* can support */
#define MMC_CAP2_HS200        (MMC_CAP2_HS200_1_8V_SDR | \
                 MMC_CAP2_HS200_1_2V_SDR)
#define MMC_CAP2_SD_EXP        (1 << 7)    /* SD express via PCIe */
#define MMC_CAP2_SD_EXP_1_2V    (1 << 8)    /* SD express 1.2V */
#define MMC_CAP2_CD_ACTIVE_HIGH    (1 << 10)    /* Card-detect signal active high */
#define MMC_CAP2_RO_ACTIVE_HIGH    (1 << 11)    /* Write-protect signal active high */
#define MMC_CAP2_NO_PRESCAN_POWERUP (1 << 14)    /* Don't power up before scan */
#define MMC_CAP2_HS400_1_8V    (1 << 15)    /* Can support HS400 1.8V */
#define MMC_CAP2_HS400_1_2V    (1 << 16)    /* Can support HS400 1.2V */
#define MMC_CAP2_HS400        (MMC_CAP2_HS400_1_8V | \
                 MMC_CAP2_HS400_1_2V)
#define MMC_CAP2_HSX00_1_8V    (MMC_CAP2_HS200_1_8V_SDR | MMC_CAP2_HS400_1_8V)
#define MMC_CAP2_HSX00_1_2V    (MMC_CAP2_HS200_1_2V_SDR | MMC_CAP2_HS400_1_2V)
#define MMC_CAP2_SDIO_IRQ_NOTHREAD (1 << 17)
#define MMC_CAP2_NO_WRITE_PROTECT (1 << 18)    /* No physical write protect pin, assume that card is always read-write */
#define MMC_CAP2_NO_SDIO    (1 << 19)    /* Do not send SDIO commands during initialization */
#define MMC_CAP2_HS400_ES    (1 << 20)    /* Host supports enhanced strobe */
#define MMC_CAP2_NO_SD        (1 << 21)    /* Do not send SD commands during initialization */
#define MMC_CAP2_NO_MMC        (1 << 22)    /* Do not send (e)MMC commands during initialization */
#define MMC_CAP2_CQE        (1 << 23)    /* Has eMMC command queue engine */
#define MMC_CAP2_CQE_DCMD    (1 << 24)    /* CQE can issue a direct command */
#define MMC_CAP2_AVOID_3_3V    (1 << 25)    /* Host must negotiate down from 3.3V */
#define MMC_CAP2_MERGE_CAPABLE    (1 << 26)    /* Host can merge a segment over the segment size */

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
    BitOptions<UInt32> caps1;
    
    /// Host device capabilities
    BitOptions<UInt32> caps2;
    
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
    virtual IOReturn preprocessRequest(RealtekSDRequest& request) = 0;
    
    ///
    /// Process the given SD command request
    ///
    /// @param request A SD command request
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `sdmmc_request()` defined in `rtsx_pci_sdmmc.c`.
    ///
    virtual IOReturn processRequest(RealtekSDRequest& request) = 0;
    
    ///
    /// Postprocess the given SD command request
    ///
    /// @param request A SD command request
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `sdmmc_post_req()` defined in `rtsx_pci_sdmmc.c`.
    ///
    virtual IOReturn postprocessRequest(RealtekSDRequest& request) = 0;
    
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
    
    inline BitOptions<UInt32> getCaps1()
    {
        return this->caps1;
    }
    
    inline BitOptions<UInt32> getCaps2()
    {
        return this->caps2;
    }
    
    //
    // MARK: - Card Events Callbacks
    //
    
    ///
    /// [UPCALL] Notify the host device when a SD card is inserted
    ///
    /// @note This callback function runs in a gated context provided by the underlying card reader controller.
    ///       The host device should implement this function without any blocking operations.
    ///       A default implementation that notifies the host driver is provided.
    ///
    virtual void onSDCardInsertedGated();
    
    ///
    /// [UPCALL] Notify the host device when a SD card is removed
    ///
    /// @note This callback function runs in a gated context provided by the underlying card reader controller.
    ///       The host device should implement this function without any blocking operations.
    ///       A default implementation that notifies the host driver is provided.
    ///
    virtual void onSDCardRemovedGated();
    
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
};

#endif /* IOSDHostDevice_hpp */
