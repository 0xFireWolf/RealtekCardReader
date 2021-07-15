//
//  IOSDBlockStorageDevice.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 6/2/21.
//

#ifndef IOSDBlockStorageDevice_hpp
#define IOSDBlockStorageDevice_hpp

#include <IOKit/storage/IOBlockStorageDevice.h>
#include "IOSDHostDriver.hpp"

/// Represents a generic SD block storage device
class IOSDBlockStorageDevice: public IOBlockStorageDevice
{
    //
    // MARK: - Constructors & Destructors
    //
    
    OSDeclareDefaultStructors(IOSDBlockStorageDevice);
    
    using super = IOBlockStorageDevice;
    
    //
    // MARK: - Private Properties
    //
    
    /// The host driver (provider)
    IOSDHostDriver* driver;
    
    /// The block size in bytes
    UInt64 blockSize;
    
    /// The number of blocks available
    UInt64 numBlocks;
    
    /// A buffer to store the card name
    char name[8];
    
    /// A buffer to store the card revision string
    char revision[8];
    
    /// A buffer to store the serial number string
    char serialString[16];
    
    /// The card serial number
    UInt32 serialNumber;
    
    //
    // MARK: - Card Characteristics
    //
    
    ///
    /// Fetch the card characteristics
    ///
    /// @return `true` on success, `false` otherwise.
    ///
    bool fetchCardCharacteristics();
    
    ///
    /// Set the properties for the block storage device
    ///
    void setStorageProperties();
    
public:
    //
    // MARK: - Block Storage Protocol Implementations
    //
    
    ///
    /// Eject the SD card
    ///
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    ///
    IOReturn doEjectMedia() override;
    
    ///
    /// Format the media to the given byte capacity
    ///
    /// @param byteCapacity The byte capacity to which the device is to be formatted
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @npte This function always returns `kIOReturnUnsupported` since our driver does not support it.
    ///
    IOReturn doFormatMedia(UInt64 byteCapacity) override;
    
    ///
    /// Get the allowable formatting byte capacities
    ///
    /// @param capacities A list of capacities
    /// @param capacitiesMaxCount The number of values that can be stored in `capacities`
    /// @return The number of capacity values returned in `capacities`;
    ///         The total number of capacity values available if `capacities` is NULL.
    /// @note This function always returns `kIOReturnUnsupported` since our driver does not support it.
    ///
    UInt32 doGetFormatCapacities(UInt64* capacities, UInt32 capacitiesMaxCount) const override;
    
    ///
    /// Get the vendor of the SD card
    ///
    /// @return The vendor name.
    ///
    char* getVendorString() override;
    
    ///
    /// Get the name of the SD card
    ///
    /// @return The product name.
    ///
    char* getProductString() override;
    
    ///
    /// Get the revision of the SD card
    ///
    /// @return The card revision value.
    ///
    char* getRevisionString() override;
    
    ///
    /// Get additional information of the SD card
    ///
    /// @return The additional information.
    ///
    char* getAdditionalDeviceInfoString() override;
    
    ///
    /// Report the block size of the SD card
    ///
    /// @param blockSize The block size in bytes on return
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note The block size should always be 512 bytes.
    ///
    IOReturn reportBlockSize(UInt64* blockSize) override;

    ///
    /// Report whether the SD card is ejectable
    ///
    /// @param isEjectable Set to `true` if the card can be ejected, `false` otherwise.
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note The card is always ejectable.
    ///
    IOReturn reportEjectability(bool* isEjectable) override;

    ///
    /// Report the index of the highest valid block of the SD card
    ///
    /// @param maxBlock The block index on return
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    ///
    IOReturn reportMaxValidBlock(UInt64* maxBlock) override;
    
    ///
    /// Report the state of the SD card
    ///
    /// @param mediaPresent Set to `true` if the SD card is present, `false` otherwise
    /// @param changedState Set to `true` if the state has changed since the last query
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    ///
    IOReturn reportMediaState(bool* mediaPresent, bool* changedState = 0) override;
    
    ///
    /// Report whether the SD card is removable
    ///
    /// @param isRemovable Set to `true` if the card can be removed, `false` otherwise.
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note The card is always removable.
    ///
    IOReturn reportRemovability(bool* isRemovable) override;
    
    ///
    /// Report whether the SD card is write protected
    ///
    /// @param isWriteProtected Set to `true` if the card is write protected, `false` otherwise
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    ///
    IOReturn reportWriteProtection(bool* isWriteProtected) override;

    ///
    /// Submit an asynchronous I/O request
    ///
    /// @param buffer The data transfer buffer
    /// @param block The starting block number
    /// @param nblks The number of blocks to transfer
    /// @param attributes Attributes of the data transfer
    /// @param completion The completion routine to call once the data transfer completes
    /// @return `kIOReturnSuccess` on success, other values otherwise.
    /// @note The data direction is contained in the given memory descriptor.
    /// @warning The given `completion` is allocated on the caller stack and thus is non-null.
    ///          However, since this function is asynchronous, the host driver must copy the completion to somewhere else,
    ///          otherwise a page fault will occur when the host driver invokes `IOStorage::complete()` on the completion object.
    /// @seealso `IOBlockStorageDriver::breakUpRequestExecute()` and `IOBlockStorageDriver::executeRequest()`.
    /// @ref https://opensource.apple.com/source/IOStorageFamily/IOStorageFamily-260.100.1/IOBlockStorageDriver.cpp.auto.html
    ///
    IOReturn doAsyncReadWrite(IOMemoryDescriptor* buffer, UInt64 block, UInt64 nblks, IOStorageAttributes* attributes, IOStorageCompletion* completion) override;
    
    //
    // MARK: - IOService Implementations
    //
    
    ///
    /// Initialize the block storage device
    ///
    /// @param dictionary The matching dictionary
    /// @return `true` on success, `false` otherwise.
    ///
    bool init(OSDictionary* dictionary = nullptr) override;
    
    ///
    /// Start the block storage device
    ///
    /// @param provider An instance of the host driver
    /// @return `true` on success, `false` otherwise.
    ///
    bool start(IOService* provider) override;
    
    ///
    /// Stop the block storage device
    ///
    /// @param provider An instance of the host driver
    ///
    void stop(IOService* provider) override;
};

#endif /* IOSDBlockStorageDevice_hpp */
