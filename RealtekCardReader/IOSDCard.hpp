//
//  IOSDCard.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 6/3/21.
//

#ifndef IOSDCard_hpp
#define IOSDCard_hpp

#include <IOKit/IOService.h>
#include "IOSDCard-CID.hpp"
#include "IOSDCard-CSD.hpp"
#include "IOSDCard-OCR.hpp"
#include "IOSDCard-SCR.hpp"
#include "IOSDCard-SSR.hpp"
#include "IOSDCard-SwitchCaps.hpp"
#include "BitOptions.hpp"

/// Forward declaration
class IOSDHostDriver;

/// IORegistry Keys
static const char* kIOSDCardCharacteristics = "Card Characteristics";
static const char* kIOSDCardPresent = "Card Present";
static const char* kIOSDCardSpeedMode = "Card Speed Mode";
static const char* kIOSDCardInitFailures = "Card Init Failures";

/// Represents a generic SD(SC/HC/XC) card
class IOSDCard: public IOService
{
    //
    // MARK: - Constructors & Destructors
    //
    
    OSDeclareDefaultStructors(IOSDCard);
    
    using super = IOService;
    
    //
    // MARK: - Constants and Definitions
    //
    
private:
    /// The specification table
    static const Pair<SPEC, const char*> kSpecTable[13];
    
public:
    ///
    /// Enumerates all possible speed modes
    ///
    /// @note The host driver provides a fallback mechanism so that cards do not work
    ///       at a given speed mode will be initialized at the next lower speed mode.
    ///
    enum SpeedMode: UInt32
    {
        // Supported modes
        kUltraHighSpeed = 2,
        kHighSpeed      = 1,
        kDefaultSpeed   = 0,
        
        // Speed ranges
        kMaxSpeed = kUltraHighSpeed,
        kMinSpeed = kDefaultSpeed,
    };
    
    ///
    /// Get the next lower speed mode
    ///
    /// @param speedMode The current speed mode
    /// @return The next lower speed mode.
    /// @warning The caller should ensure that the given speed mode is not the lowest one.
    ///
    static inline SpeedMode nextLowerSpeedMode(SpeedMode speedMode)
    {
        return static_cast<SpeedMode>(speedMode - 1);
    }
    
    //
    // MARK: - Card Properties
    //
    
private:
    /// The host driver
    IOSDHostDriver* driver;
    
    /// The card identification data
    CID cid;
    
    /// The card specification data
    CSD csd;
    
    /// SD configuration data
    SCR scr;
    
    /// SD status data
    SSR ssr;
    
    /// Switch capabilities
    SwitchCaps switchCaps;
    
    /// The card relative address
    UInt32 rca;
    
    //
    // MARK: - Query Card Properties
    //
    
public:
    /// Get the card identification data
    inline const CID& getCID() const
    {
        return this->cid;
    }
    
    /// Get the card specification data
    inline const CSD& getCSD() const
    {
        return this->csd;
    }
    
    /// Get the SD configuration data
    inline const SCR& getSCR() const
    {
        return this->scr;
    }
    
    /// Get the SD status data
    inline const SSR& getSSR() const
    {
        return this->ssr;
    }
    
    /// Get the card relative address
    inline UInt32 getRCA() const
    {
        return this->rca;
    }
    
    /// Get the card type
    inline const char* getCardType() const
    {
        if (!this->csd.isBlockAddressed)
        {
            return "SDSC";
        }
        
        if (!this->csd.hasExtendedCapacity)
        {
            return "SDHC";
        }
        else
        {
            return "SDXC";
        }
    }
    
    /// Get the specification version string
    inline const char* getSpecificationVersion() const
    {
        SPEC spec = this->scr.getCardSpecLevel();
        
        for (const auto& entry : kSpecTable)
        {
            if (spec == entry.first)
            {
                return entry.second;
            }
        }
        
        return "Unknown";
    }
    
    ///
    /// Get the card product name
    ///
    /// @param name A non-null buffer that can hold at least 8 bytes.
    /// @param length The buffer length
    /// @return `true` on success, `false` if the buffer is NULL or too small.
    ///
    inline bool getCardName(char* name, IOByteCount length) const
    {
        if (name == nullptr || length < 8)
        {
            return false;
        }
        
        bzero(name, length);
        
        memcpy(name, this->cid.name, sizeof(this->cid.name));
        
        return true;
    }
    
    ///
    /// Get the card revision value
    ///
    /// @param revision A non-null buffer that can hold at least 8 bytes.
    /// @param length The buffer length
    /// @return `true` on success, `false` if the buffer is NULL or too small.
    ///
    inline bool getCardRevision(char* revision, IOByteCount length) const
    {
        if (revision == nullptr || length < 8)
        {
            return false;
        }
        
        bzero(revision, length);
        
        snprintf(revision, length, "%d.%d", this->cid.hwrevision, this->cid.fwrevision);
        
        return true;
    }
    
    ///
    /// Get the card manufacture date
    ///
    /// @param date A non-null buffer that can hold at least 8 bytes.
    /// @param length The buffer length
    /// @return `true` on success, `false` if the buffer is NULL or too small.
    ///
    inline bool getCardProductionDate(char* date, IOByteCount length) const
    {
        if (date == nullptr || length < 8)
        {
            return false;
        }
        
        bzero(date, length);
        
        snprintf(date, length, "%04d.%02d", this->cid.getYear(), this->cid.month);
        
        return true;
    }
    
    ///
    /// Get the card characteristics
    ///
    /// @return A dictionary that contains card characteristics which can be recognized by the System Profiler.
    /// @note The caller is responsible for releasing the returned dictionary.
    ///
    OSDictionaryPtr getCardCharacteristics() const;
    
    //
    // MARK: - Card Event Utilities
    //
    
    ///
    /// Type of a completion routine that is called once an asynchronous card event is processed by the host driver
    ///
    /// @param target An opaque client-supplied pointer (or the instance pointer for a C++ callback)
    /// @param parameter An opaque client-supplied parameter pointer
    /// @param status `kIOReturnSuccess` if the card event has been processed without errors, other values otherwise.
    /// @param characteristics A non-null dictionary that contains characteristics of the card inserted and initialized successfully,
    ///                        `nullptr` if the card inserted by users cannot be initialized or has been removed from the card slot.
    ///
    using CompletionAction = void (*)(void* target, void* parameter, IOReturn status, OSDictionary* characteristics);
    
    ///
    /// Describe the completion routine to be called when the host driver has processed an asynchronous card event
    ///
    struct Completion
    {
    private:
        /// An opaque client-supplied pointer (or the instance pointer for a C++ callback)
        void* target;
        
        /// A non-null completion routine
        CompletionAction action;
        
        /// An opaque client-supplied parameter pointer
        void* parameter;
        
    public:
        ///
        /// Default constructor
        ///
        Completion() = default;
        
        ///
        /// Create a completion routine
        ///
        Completion(void* target, CompletionAction action, void* parameter = nullptr) :
            target(target), action(action), parameter(parameter) {}
        
        ///
        /// Invoke the completion routine
        ///
        /// @param status Pass `kIOReturnSuccess` if the card event has been processed without errors, other values otherwise.
        /// @param characteristics A non-null dictionary that contains characteristics of the card inserted and initialized successfully,
        ///                        `nullptr` if the card inserted by users cannot be initialized or has been removed from the card slot.
        /// @warning This function prints a warning message if the action routine is null.
        ///
        inline void invoke(IOReturn status, OSDictionary* characteristics)
        {
            if (this->action != nullptr)
            {
                (*this->action)(this->target, this->parameter, status, characteristics);
            }
            else
            {
                pwarning("The action routine is null.");
            }
        }
        
        ///
        /// Reset the completion routine
        ///
        inline void reset()
        {
            this->target = nullptr;
            
            this->action = nullptr;
            
            this->parameter = nullptr;
        }
        
        ///
        /// Create a completion descriptor with the given member function
        ///
        /// @param self The instance pointer for the given C++ callback function
        /// @param function A C++ callback function
        /// @param parameter An optional opaque parameter pointer
        /// @return The completion routine.
        ///
        template <typename Function>
        static Completion withMemberFunction(OSObject* self, Function function, void* parameter = nullptr)
        {
            return { self, OSMemberFunctionCast(CompletionAction, self, function), parameter };
        }
    };
    
    ///
    /// Invoke the given completion routine
    ///
    /// @param completion A nullable completion routine
    /// @param status Pass `kIOReturnSuccess` if the card event has been processed without errors, other values otherwise.
    /// @param characteristics A non-null dictionary that contains characteristics of the card inserted and initialized successfully,
    ///                        `nullptr` if the card inserted by users cannot be initialized or has been removed from the card slot.
    /// @note This function is a noop if the given completion is null.
    ///
    static inline void complete(Completion* completion, IOReturn status, OSDictionary* characteristics = nullptr)
    {
        if (completion != nullptr)
        {
            completion->invoke(status, characteristics);
        }
    }
    
    ///
    /// Type of the card event options
    ///
    using EventOptions = BitOptions<UInt32>;
    
    ///
    /// Enumerate all card event options that can be passed to the handler at the host driver layer
    ///
    enum EventOption: UInt32
    {
        /// Indicate that the card event handler is invoked by the interrupt service routine
        kInterruptContext = 0,
        
        /// Indicate that the card event handler is invoked by the power management routine
        kPowerManagementContext = 1,
    };
    
    //
    // MARK: - Card Initialization Process
    //
    
private:    
    ///
    /// [Helper] Initialize the card with Default Speed Mode enabled
    ///
    /// @return `true` on success, `false` otherwise.
    /// @note Port: This function replaces the default speed portion of `mmc_sd_init_card()` defined in `sd.c`.
    ///
    bool initDefaultSpeedMode();
    
    ///
    /// [Helper] Initialize the card with High Speed Mode enabled
    ///
    /// @return `true` on success, `false` otherwise.
    /// @note Port: This function replaces the high speed portion of `mmc_sd_init_card()` defined in `sd.c`.
    ///
    bool initHighSpeedMode();
    
    ///
    /// [Helper] Initialize the card with UHS-I Mode enabled
    ///
    /// @return `true` on success, `false` otherwise.
    /// @note Port: This function replaces `mmc_sd_init_uhs_card()` defined in `sd.c`.
    ///
    bool initUltraHighSpeedMode();
    
    ///
    /// [Helper] Read the card switch capabilities
    ///
    /// @return `true` on success, `false` otherwise.
    /// @note Port: This function replaces `mmc_read_switch()` defined in `sd.c`.
    ///
    bool readSwitchCapabilities();
    
    ///
    /// [Helper] Enable the 4-bit wide bus for data transfer
    ///
    /// @return `true` on success, `false` otherwise.
    ///
    bool enable4BitWideBus();
    
    ///
    /// [Helper] Enable the signal voltage for the UHS-I mode
    ///
    /// @param ocr The current operating condition register value
    /// @return `true` on success, `false` otherwise.
    /// @note Port: This function replaces `mmc_set_uhs_voltage()` defined in `core.c`.
    ///
    bool enableUltraHighSpeedSignalVoltage(UInt32 ocr);
    
    ///
    /// [Helper] Select the bus speed for the UHS-I mode
    ///
    /// @return The bus speed supported by both the card and the host.
    /// @note Port: This function replaces `sd_update_bus_speed_mode()` defined in `sd.c`.
    ///
    SwitchCaps::BusSpeed selectUltraHighSpeedBusSpeed();
    
    ///
    /// [Helper] Set the driver strength for the UHS-I card
    ///
    /// @param busSpeed The bus speed
    /// @return `true` on success, `false` otherwise.
    /// @note Port: This function replaces `sd_select_driver_type()` defined in `sd.c`.
    /// @note This function is a part of the UHS-I card initialization routine `initUltraHighSpeedMode()`.
    ///
    bool setUHSDriverType(SwitchCaps::BusSpeed busSpeed);
    
    ///
    /// [Helper] Set the current limit for the UHS-I card
    ///
    /// @param busSpeed The bus speed
    /// @return `true` on success, `false` otherwise.
    /// @note Port: This function replaces `sd_set_current_limit()` defined in `sd.c`.
    /// @note This function is a part of the UHS-I card initialization routine `initUltraHighSpeedMode()`.
    ///
    bool setUHSCurrentLimit(SwitchCaps::BusSpeed busSpeed);
    
    ///
    /// [Helper] Set the bus speed for the UHS-I card
    ///
    /// @param busSpeed The bus speed
    /// @return `true` on success, `false` otherwise.
    /// @note Port: This function replaces `sd_set_bus_speed_mode()` defined in `sd.c`.
    /// @note This function is a part of the UHS-I card initialization routine `initUltraHighSpeedMode()`.
    ///
    bool setUHSBusSpeedMode(SwitchCaps::BusSpeed busSpeed);
    
    ///
    /// [Helper] Initialize the card at the default speed mode
    ///
    /// @param speedMode Set to `IOSDCard::SpeedMode::kDefaultSpeed` on return
    /// @return `kIOReturnSuccess` on success,
    ///         `kIOReturnNotResponding` if the host driver should invoke this function again to initialize the card at a lower speed mode,
    ///         `kIOReturnAborted` if the host driver should abort the initialization of the attached card.
    /// @note This function is a wrapper of `IOSDCard::initDefaultSpeedMode()` to support the new fallback mechanism as of v0.9.7.
    /// @note The return value of this function is inherited from the caller `IOSDCard::initializeCard(ocr:speedMode:)`.
    /// @seealso `IOSDCard::initializeCard(ocr:speedMode:)` and `IOSDCard::initDefaultSpeedMode()`.
    ///
    IOReturn initializeCardAtDefaultSpeedMode(SpeedMode& speedMode);
    
    ///
    /// [Helper] Initialize the card at the high speed mode
    ///
    /// @param speedMode Set to `IOSDCard::SpeedMode::kHighSpeed` on return
    /// @return `kIOReturnSuccess` on success,
    ///         `kIOReturnNotResponding` if the host driver should invoke this function again to initialize the card at a lower speed mode,
    ///         `kIOReturnAborted` if the host driver should abort the initialization of the attached card.
    /// @note This function is a wrapper of `IOSDCard::initHighSpeedMode()` to support the new fallback mechanism as of v0.9.7.
    /// @note The return value of this function is inherited from the caller `IOSDCard::initializeCard(ocr:speedMode:)`.
    /// @seealso `IOSDCard::initializeCard(ocr:speedMode:)` and `IOSDCard::initHighSpeedMode()`.
    ///
    IOReturn initializeCardAtHighSpeedMode(SpeedMode& speedMode);
    
    ///
    /// [Helper] Initialize the card at the ultra high speed mode
    ///
    /// @param speedMode Set to `IOSDCard::SpeedMode::kUltraHighSpeed` on return
    /// @return `kIOReturnSuccess` on success,
    ///         `kIOReturnNotResponding` if the host driver should invoke this function again to initialize the card at a lower speed mode,
    ///         `kIOReturnAborted` if the host driver should abort the initialization of the attached card.
    /// @note This function is a wrapper of `IOSDCard::initUltraHighSpeedMode()` to support the new fallback mechanism as of v0.9.7.
    /// @note The return value of this function is inherited from the caller `IOSDCard::initializeCard(ocr:speedMode:)`.
    /// @seealso `IOSDCard::initializeCard(ocr:speedMode:)` and `IOSDCard::initUltraHighSpeedMode()`.
    ///
    IOReturn initializeCardAtUltraHighSpeedMode(SpeedMode& speedMode);
    
public:
    ///
    /// Initialize the card with the given OCR register value
    ///
    /// @param ocr The current operating condition register value
    /// @param speedMode The suggested speed mode (see below)
    /// @return `kIOReturnSuccess` on success,
    ///         `kIOReturnNotResponding` if the host driver should invoke this function again to initialize the card at a lower speed mode,
    ///         `kIOReturnAborted` if the host driver should abort the initialization of the attached card.
    /// @note The given speed mode is treated as a hint and may be overridden by user configurations or the capability of the card.
    ///       The caller should always invoke this function with the maximum speed mode (i.e. `IOSDCard::SpeedMode::kMaxSpeed`) at the beginning.
    ///       If this function returns `kIOReturnNotResponding`, `speedMode` is set to the speed mode at which the card has failed to initialize.
    ///       The caller may repeatedly invoke this function with a lower speed mode until one of the following scenarios occurs.
    ///       (1) The function returns `kIOReturnSuccess`, indicating that the card has been initialized successfully.
    ///       (2) The function returns `kIOReturnAborted`, indicating that the card failed to initialize thus the caller should abort the initialization process.
    ///       (3) The function returns `kIOReturnNotResponding` with `speedMode` set to the minimum speed mode (i.e. `IOSDCard::SpeedMode::kMinSpeed`),
    ///           indicating that the caller has tried all possible speed modes but the card still failed to initialize thus should abort the initialization process.
    ///       The caller may use the return value of this function to implement a graceful fallback mechanism.
    /// @note Port: This function replaces `mmc_sd_init_card()` defined in `sd.c`.
    ///
    IOReturn initializeCard(UInt32 ocr, SpeedMode& speedMode);
    
    //
    // MARK: - IOService Implementations
    //
    
public:
    ///
    /// Start the SD card
    ///
    /// @param provider An instance of the host driver
    /// @return `true` on success, `false` otherwise.
    ///
    bool start(IOService* provider) override;
    
    ///
    /// Stop the SD card
    ///
    /// @param provider An instance of the host driver
    ///
    void stop(IOService* provider) override;
};

#endif /* IOSDCard_hpp */
