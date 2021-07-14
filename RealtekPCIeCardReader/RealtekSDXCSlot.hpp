//
//  RealtekSDXCSlot.hpp
//  RealtekPCIeCardReader
//
//  Created by FireWolf on 5/27/21.
//

#ifndef RealtekSDXCSlot_hpp
#define RealtekSDXCSlot_hpp

#include "RealtekCardReaderController.hpp"
#include "AppleSDXCSlot.hpp"
#include "Utilities.hpp"

///
/// Represents a generic Realtek SD (SC/HC/XC) card slot independent of the concrete card reader controller
///
class RealtekSDXCSlot: public AppleSDXCSlot
{
    //
    // MARK: - Constructors & Destructors
    //
    
    OSDeclareAbstractStructors(RealtekSDXCSlot);
    
    using super = AppleSDXCSlot;
    
    //
    // MARK: - Type Definitions
    //
    
    using ChipRegValuePair = RealtekCardReaderController::ChipRegValuePair;
    
    using ChipRegValuePairs = RealtekCardReaderController::ChipRegValuePairs;
    
    using SimpleRegValuePairs = RealtekCardReaderController::SimpleRegValuePairs;
    
    using ContiguousRegValuePairsForReadAccess = RealtekCardReaderController::ContiguousRegValuePairsForReadAccess;
    
    using ContiguousRegValuePairsForWriteAccess = RealtekCardReaderController::ContiguousRegValuePairsForWriteAccess;
    
    using OutputVoltage = RealtekCardReaderController::OutputVoltage;
    
    using SSCDepth = RealtekCardReaderController::SSCDepth;
    
    //
    // MARK: - Constants [Miscellaneous]
    //
    
    /// Tune the Rx phase for three times
    static constexpr UInt32 kRxTuningCount = 3;
    
    //
    // MARK: - Private Properties
    //
    
    /// The card reader (provider)
    RealtekCardReaderController* controller;
    
    /// Card clock in Hz
    UInt32 cardClock;
    
    /// SSC depth value
    SSCDepth sscDepth;
    
    /// `True` if the card is at its initial stage
    bool initialMode;
    
    /// `True` if VPCLOCK is used
    bool vpclock;
    
    /// `True` if the card clock should be doubled
    bool doubleClock;
    
    //
    // MARK: - SD Commander
    //
    
private:    
    ///
    /// [Shared] [Helper] Inform the card reader which SD command to be executed
    ///
    /// @param command The SD command to be executed
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `sd_cmd_set_sd_cmd()` defined in `rtsx_pci_sdmmc.c`.
    /// @warning This function is valid only when there is an active transfer session.
    ///          i.e. The caller should invoke this function in between `Controller::beginCommandTransfer()` and `Controller::endCommandTransfer()`.
    ///
    IOReturn setSDCommandOpcodeAndArgument(const RealtekSDCommand& command);
    
    ///
    /// [Shared] [Helper] Inform the card reader the number of blocks to access
    ///
    /// @param nblocks The number of blocks
    /// @param blockSize The number of bytes in each block
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `sd_cmd_set_data_len()` defined in `rtsx_pci_sdmmc.c`.
    /// @warning This function is valid only when there is an active transfer session.
    ///          i.e. The caller should invoke this function in between `Controller::beginCommandTransfer()` and `Controller::endCommandTransfer()`.
    ///
    IOReturn setSDCommandDataLength(UInt16 nblocks, UInt16 blockSize);
 
public:
    ///
    /// [Case 1] Send a SD command and wait for the response
    ///
    /// @param command The SD command to be sent
    /// @param timeout The amount of time in milliseconds to wait for the response until timed out
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `sd_send_cmd_get_rsp()` defined in `rtsx_pci_sdmmc.c`.
    /// @note This function checks whether the start and the transmission bits and the CRC7 checksum in the response are valid.
    ///       Upon a successful return, the response is guaranteed to be valid, but the caller is responsible for verifying the content.
    /// @note This function is invoked by `RealtekSDXCSlot::CMD*()` and `RealtekSDXCSlot::ACMD*()` that do not involve a data transfer.
    ///
    IOReturn runSDCommand(RealtekSDCommand& command, UInt32 timeout = 100);
    
    ///
    /// [Case 1] Send a SD command and wait for the response
    ///
    /// @param request The SD command request to process
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `sd_send_cmd_get_rsp()` defined in `rtsx_pci_sdmmc.c`.
    /// @note This function is invoked by `RealtekSDXCSlot::CMD*()` and `RealtekSDXCSlot::ACMD*()` that do not involve a data transfer.
    ///
    IOReturn runSDCommand(RealtekSDSimpleCommandRequest& request);
    
    ///
    /// [Case 2] [Helper] Send a SD command and read the data
    ///
    /// @param command The SD command to be sent
    /// @param buffer A buffer to store the response data and is nullable if the given command is one of the tuning command
    /// @param length The number of bytes to read (up to 512 bytes)
    /// @param timeout The amount of time in milliseonds to wait for the response data until timed out
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `sd_read_data()` defined in `rtsx_pci_sdmmc.c`.
    /// @note This function is invoked by `RealtekSDXCSlot::CMD*()` and `RealtekSDXCSlot::ACMD*()` that involve a data transfer.
    ///
    IOReturn runSDCommandAndReadData(const RealtekSDCommand& command, UInt8* buffer, IOByteCount length, UInt32 timeout);
    
    ///
    /// [Case 2] [Helper] Send a SD command along with the data
    ///
    /// @param command The SD command to be sent
    /// @param buffer A non-null buffer that contains the data for the given command
    /// @param length The number of bytes to write (up to 512 bytes)
    /// @param timeout The amount of time in milliseonds to wait for completion until timed out
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `sd_write_data()` defined in `rtsx_pci_sdmmc.c`.
    /// @note This function is invoked by `RealtekSDXCSlot::CMD*()` and `RealtekSDXCSlot::ACMD*()` that involve a data transfer.
    ///
    IOReturn runSDCommandAndWriteData(RealtekSDCommand& command, const UInt8* buffer, IOByteCount length, UInt32 timeout);
    
    ///
    /// [Case 2] Send a SD command and read the data
    ///
    /// @param request A data transfer request to service
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces the read portion of `sd_normal_rw()` defined in `rtsx_pci_sdmmc.c`.
    /// @note This function is invoked by `RealtekSDXCSlot::CMD*()` and `RealtekSDXCSlot::ACMD*()` that involve a data transfer.
    ///
    IOReturn runSDCommandWithInboundDataTransfer(RealtekSDCommandWithInboundDataTransferRequest& request);
    
    ///
    /// [Case 2] Send a SD command along with the data
    ///
    /// @param request A data transfer request to service
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces the write portion of `sd_normal_rw()` defined in `rtsx_pci_sdmmc.c`.
    /// @note This function is invoked by `RealtekSDXCSlot::CMD*()` and `RealtekSDXCSlot::ACMD*()` that involve a data transfer.
    ///
    IOReturn runSDCommandWithOutboundDataTransfer(RealtekSDCommandWithOutboundDataTransferRequest& request);
        
    ///
    /// [Case 3] Send a SD command along with an inbound DMA transfer
    ///
    /// @param request A block-oriented data transfer request to service
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `sd_read_long_data()` defined in `rtsx_pci_sdmmc.c`.
    /// @note This function is invoked by `RealtekSDXCSlot::CMD*()` and `RealtekSDXCSlot::ACMD*()` that involve a DMA transfer.
    ///
    IOReturn runSDCommandWithInboundDMATransfer(RealtekSDCommandWithBlockDataTransferRequest& request);
    
    ///
    /// [Case 3] Send a SD command along with an outbound DMA transfer
    ///
    /// @param request A block-oriented data transfer request to service
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `sd_write_long_data()` defined in `rtsx_pci_sdmmc.c`.
    /// @note This function is invoked by `RealtekSDXCSlot::CMD*()` and `RealtekSDXCSlot::ACMD*()` that involve a DMA transfer.
    ///
    IOReturn runSDCommandWithOutboundDMATransfer(RealtekSDCommandWithBlockDataTransferRequest& request);
        
    ///
    /// Process the given SD command request
    ///
    /// @param request A SD command request
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `sdmmc_request()` defined in `rtsx_pci_sdmmc.c`.
    ///
    IOReturn processRequest(RealtekSDRequest& request) override;
    
    //
    // MARK: - SD Bus Configurator
    //
    
private:
    ///
    /// Check whether the host device is running in the ultra high speed mode
    ///
    /// @return `true` if the current bus timing mode is one of SDR12/25/50/104 and DDR50
    ///
    bool isRunningInUltraHighSpeedMode();
    
    ///
    /// Set the bus width
    ///
    /// @param width The target bus width
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `sd_set_bus_width()` defined in `rtsx_pci_sdmmc.c`.
    /// @note This function acts as a service routine of `RealtekSDXCSlot::setIOConfig()`.
    ///
    IOReturn setBusWidth(IOSDBusConfig::BusWidth width);
    
    ///
    /// [Helper] Power on the card slot
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `sd_power_on()` defined in `rtsx_pci_sdmmc.c`.
    /// @note This function acts as a service routine of `RealtekSDXCSlot::setIOConfig()`.
    ///
    IOReturn powerOn();
    
    ///
    /// [Helper] Power off the card slot
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `sd_power_off()` defined in `rtsx_pci_sdmmc.c`.
    /// @note This function acts as a service routine of `RealtekSDXCSlot::setIOConfig()`.
    ///
    IOReturn powerOff();
    
    ///
    /// Set the power mode
    ///
    /// @param mode The target power mode
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `sd_set_power_mode()` defined in `rtsx_pci_sdmmc.c`.
    /// @note This function acts as a service routine of `RealtekSDXCSlot::setIOConfig()`.
    ///
    IOReturn setPowerMode(IOSDBusConfig::PowerMode mode);
    
    ///
    /// Set the bus speed mode
    ///
    /// @param timing The target bus speed mode
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `sd_set_timing()` defined in `rtsx_pci_sdmmc.c`.
    /// @note This function acts as a service routine of `RealtekSDXCSlot::setIOConfig()`.
    ///
    IOReturn setBusTiming(IOSDBusConfig::BusTiming timing);
    
    ///
    /// Apply the given I/O configuration
    ///
    /// @param config An I/O config
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `sdmmc_set_ios()` defined in `rtsx_pci_sdmmc.c`.
    ///
    IOReturn setBusConfig(const IOSDBusConfig& config) override;

    ///
    /// [Helper] [Phase 1] Wait until the signal voltage becomes stable
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `sd_wait_voltage_stable_1()` defined in `rtsx_pci_sdmmc.c`.
    /// @note This function is called after the host driver sends a CMD11 to switch to 1.8V.
    ///
    IOReturn waitVoltageStable1();
    
    ///
    /// [Helper] [Phase 2] Wait until the signal voltage becomes stable
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `sd_wait_voltage_stable_2()` defined in `rtsx_pci_sdmmc.c`.
    /// @note This function is called after the controller switches the signal voltage.
    ///
    IOReturn waitVoltageStable2();
    
    ///
    /// [Helper] Switch the signal voltage to 1.8V
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note This helper function is called by `RealtekSDXCSlot::switchSignalVoltage()` to simplify error handling.
    ///
    IOReturn switchSignalVoltage1d8V();
    
    ///
    /// [Helper] Switch the signal voltage to 3.3V
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note This helper function is called by `RealtekSDXCSlot::switchSignalVoltage()` to simplify error handling.
    ///
    IOReturn switchSignalVoltage3d3V();

public:
    ///
    /// Switch the signal voltage
    ///
    /// @param config An I/O config that contains the target signal voltage level
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replace `sdmmc_switch_voltage()` defined in `rtsx_pci_sdmmc.c`.
    ///
    IOReturn switchSignalVoltage(const IOSDBusConfig& config) override;
    
    //
    // MARK: - Tuning
    //
    
private:
    ///
    /// Get the phase length for the given bit index
    ///
    /// @param phaseMap The phase map
    /// @param sindex The index of the start bit
    /// @return The phase length.
    /// @note Port: This function replaces `sd_get_phase_len()` and its helper `test_phase_bit()` defined in `rtsx_pci_sdmmc.c`.
    ///
    UInt32 getPhaseLength(UInt32 phaseMap, UInt32 sindex);
    
    ///
    /// Search for the final phase in the given map
    ///
    /// @param phaseMap The phase map
    /// @return The final sample point value.
    /// @note Port: This function replaces `sd_search_final_phase()` defined in `rtsx_pci_sdmmc.c`.
    ///
    UInt8 searchFinalPhase(UInt32 phaseMap);
    
    ///
    /// Wait until the data lines are idle
    ///
    /// @return `kIOReturnSuccess` on success, `kIOReturnTimeout` if timed out, other values otherwise.
    /// @note Port: This function replaces `sd_wait_data_idle()` defined in `rtsx_pci_sdmmc.c`.
    ///
    DEPRECATE("Need further investigation. Will be replaced by Controller::waitForIdleDataLine().")
    IOReturn waitForIdleDataLine();
    
    ///
    /// Use the given sample point to tune Rx commands
    ///
    /// @param samplePoint The sample point
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `sd_tuning_rx_cmd()` defined in `rtsx_pci_sdmmc.c` but removes tuning support for MMC cards.
    ///       i.e. This function assumes that the command opcode is always 19.
    ///
    IOReturn tuningRxCommand(UInt8 samplePoint);
    
    ///
    /// Tune the Rx phase
    ///
    /// @return The phase map.
    /// @note Port: This function replaces `sd_tuning_phase()` defined in `rtsx_pci_sdmmc.c`.
    ///
    UInt32 tuningRxPhase();
    
protected:
    ///
    /// Tune the Rx transfer
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `sd_tuning_rx()` defined in `rtsx_pci_sdmmc.c`.
    ///
    IOReturn tuningRx();

public:
    ///
    /// Execute the tuning algorithm
    ///
    /// @param config An I/O config that contains the current I/O settings
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `sdmmc_execute_tuning()` defined in `rtsx_pci/usb_sdmmc.c`.
    ///
    IOReturn executeTuning(const IOSDBusConfig& config) override = 0;
    
    //
    // MARK: - Card Detection and Write Protection
    //
        
    ///
    /// Check whether the card has write protection enabled
    ///
    /// @param result Set `true` if the card is write protected, `false` otherwise.
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `sdmmc_get_ro()` defined in `rtsx_pci_sdmmc.c`.
    ///
    IOReturn isCardWriteProtected(bool& result) override;
    
    ///
    /// Check whether the card exists
    ///
    /// @param result Set `true` if the card exists, `false` otherwise.
    /// @return `kIOReturnSuccess` always.
    /// @note Port: This function replaces `sdmmc_get_cd()` defined in `rtsx_pci_sdmmc.c`.
    ///
    IOReturn isCardPresent(bool& result) override;
    
    ///
    /// Check whether the command line of the card is busy
    ///
    /// @param result Set `true` if the card CMD line is high, `false` otherwise.
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    ///
    DEPRECATE("Need further investigation. Will be replaced by Controller::isCardCommandLineBusy().")
    IOReturn isCardCommandLineBusy(bool& result) override;
    
    ///
    /// Check whether the data line of the card is busy
    ///
    /// @param result Set `true` if the card DAT lines are high, `false` otherwise.
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    ///
    DEPRECATE("Need further investigation. Will be replaced by Controller::isCardDataLineBusy().")
    IOReturn isCardDataLineBusy(bool& result) override;
    
    //
    // MARK: - Manage Initial Modes
    //
    
private:
    ///
    /// Set the clock divider to 128 if `initial mode` is true
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `sd_enable_initial_mode()` defined in `rtsx_pci_sdmmc.c`.
    ///
    IOReturn enableInitialModeIfNecessary();
    
    ///
    /// Set the clock divider to 0 if `initial mode` is true
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note Port: This function replaces `sd_disable_initial_mode()` defined in `rtsx_pci_sdmmc.c`.
    ///
    IOReturn disableInitialModeIfNecessary();
    
    //
    // MARK: - Fetch Host Capabilities
    //
    
    ///
    /// Fetch the host capabilities
    ///
    void fetchHostCapabilities();
    
    //
    // MARK: - IOService Implementations
    //
    
public:
    ///
    /// Initialize the host device
    ///
    /// @return `true` on success, `false` otherwise.
    ///
    bool init(OSDictionary* dictionary = nullptr) override;
    
    ///
    /// Start the host device
    ///
    /// @param provider An instance of Realtek PCIe card reader controller
    /// @return `true` on success, `false` otherwise.
    ///
    bool start(IOService* provider) override;
    
    ///
    /// Stop the host device
    ///
    /// @param provider An instance of Realtek PCIe card reader controller
    ///
    void stop(IOService* provider) override;
};

#endif /* RealtekSDXCSlot_hpp */
