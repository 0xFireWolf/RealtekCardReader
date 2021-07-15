//
//  IOSDBlockStorageDevice.cpp
//  RealtekCardReader
//
//  Created by FireWolf on 6/2/21.
//

#include "IOSDBlockStorageDevice.hpp"

//
// MARK: - Meta Class Definitions
//

OSDefineMetaClassAndStructors(IOSDBlockStorageDevice, IOBlockStorageDevice);

//
// MARK: - Card Characteristics
//

///
/// Fetch the card characteristics
///
/// @return `true` on success, `false` otherwise.
///
bool IOSDBlockStorageDevice::fetchCardCharacteristics()
{
    // 1. Block Size
    if (this->driver->getCardBlockLength(this->blockSize) != kIOReturnSuccess)
    {
        perr("Failed to fetch the card block size.");
        
        return false;
    }
    
    // 2. Block Number
    if (this->driver->getCardNumBlocks(this->numBlocks) != kIOReturnSuccess)
    {
        perr("Failed to fetch the number of blocks on the card.");
        
        return false;
    }
    
    // 3. Card Name
    if (this->driver->getCardName(this->name, sizeof(this->name)) != kIOReturnSuccess)
    {
        perr("Failed to fetch the card name.");
        
        return false;
    }
    
    // 4. Card Revision
    if (this->driver->getCardRevision(this->revision, sizeof(this->revision)) != kIOReturnSuccess)
    {
        perr("Failed to fetch the card revision.");
        
        return false;
    }
    
    // 5. Card Serial Number
    if (this->driver->getCardSerialNumber(this->serialNumber) != kIOReturnSuccess)
    {
        perr("Failed to fetch the card serial number.");
        
        return false;
    }
    
    snprintf(this->serialString, sizeof(this->serialString), "%08X", this->serialNumber);
    
    pinfo("Card Vendor = %s; Name = %s; Revision = %s; Serial Number = 0x%08X.",
          this->driver->getCardVendor(), this->name, this->revision, this->serialNumber);
    
    pinfo("Card Block Size = %llu Bytes; NumBlocks = %llu; Capacity = %llu Bytes.",
          this->blockSize, this->numBlocks, this->blockSize * this->numBlocks);
    
    return true;
}

///
/// Set the properties for the block storage device
///
void IOSDBlockStorageDevice::setStorageProperties()
{
    OSDictionary* dictionary = OSDictionary::withCapacity(2);
    
    OSString* identifier = OSString::withCString("com.apple.iokit.IOSCSIArchitectureModelFamily");
    
    OSString* iconFile = OSString::withCString("SD.icns");
    
    if (dictionary != nullptr && identifier != nullptr && iconFile != nullptr)
    {
        psoftassert(dictionary->setObject("CFBundleIdentifier", identifier), "Failed to set the bundle identifier.");
        
        psoftassert(dictionary->setObject("IOBundleResourceFile", iconFile), "Failed to set the icon file name.");
        
        psoftassert(this->setProperty(kIOMediaIconKey, dictionary), "Failed to set the storage properties.");
    }
    
    OSSafeReleaseNULL(dictionary);
    
    OSSafeReleaseNULL(identifier);
    
    OSSafeReleaseNULL(iconFile);
}

//
// MARK: - Block Storage Protocol Implementations
//

///
/// Eject the SD card
///
/// @return `kIOReturnSuccess` on success, other values otherwise.
///
IOReturn IOSDBlockStorageDevice::doEjectMedia()
{
    pinfo("The storage subsystem requests to eject the media.");
    
    this->driver->onSDCardRemovedGated();
    
    return kIOReturnSuccess;
}

///
/// Format the media to the given byte capacity
///
/// @param byteCapacity The byte capacity to which the device is to be formatted
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @npte This function always returns `kIOReturnUnsupported` since our driver does not support it.
///
IOReturn IOSDBlockStorageDevice::doFormatMedia(UInt64 byteCapacity)
{
    pinfo("The storage subsystem requests to format the media to the capacity %llu bytes.", byteCapacity);
    
    return kIOReturnUnsupported;
}

///
/// Get the allowable formatting byte capacities
///
/// @param capacities A list of capacities
/// @param capacitiesMaxCount The number of values that can be stored in `capacities`
/// @return The number of capacity values returned in `capacities`;
///         The total number of capacity values available if `capacities` is NULL.
/// @note This function always returns `kIOReturnUnsupported` since our driver does not support it.
///
UInt32 IOSDBlockStorageDevice::doGetFormatCapacities(UInt64* capacities, UInt32 capacitiesMaxCount) const
{
    pinfo("The storage subsystem requests to fetch all supported capacities.");
    
    return kIOReturnUnsupported;
}

///
/// Get the vendor of the SD card
///
/// @return The vendor name.
///
char* IOSDBlockStorageDevice::getVendorString()
{
    pinfo("The storage subsystem requests the vendor of the media.");
    
    return const_cast<char*>(this->driver->getCardVendor());
}

///
/// Get the name of the SD card
///
/// @return The product name.
///
char* IOSDBlockStorageDevice::getProductString()
{
    pinfo("The storage subsystem requests the name of the media.");
    
    return this->name;
}

///
/// Get the revision of the SD card
///
/// @return The card revision value.
///
char* IOSDBlockStorageDevice::getRevisionString()
{
    pinfo("The storage subsystem requests the revision of the media.");
    
    return this->revision;
}

///
/// Get additional information of the SD card
///
/// @return The additional information.
///
char* IOSDBlockStorageDevice::getAdditionalDeviceInfoString()
{
    pinfo("The storage subsystem requests the addition information of the media.");
    
    return this->serialString;
}

///
/// Report the block size of the SD card
///
/// @param blockSize The block size in bytes on return
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note The block size should always be 512 bytes.
///
IOReturn IOSDBlockStorageDevice::reportBlockSize(UInt64* blockSize)
{
    pinfo("The storage subsystem requests the block size of the media.");
    
    return this->driver->getCardBlockLength(*blockSize);
}

///
/// Report whether the SD card is ejectable
///
/// @param isEjectable Set to `true` if the card can be ejected, `false` otherwise.
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note The card is always ejectable.
///
IOReturn IOSDBlockStorageDevice::reportEjectability(bool* isEjectable)
{
    pinfo("The storage subsystem asks whether the media is ejectable.");
    
    *isEjectable = true;
    
    return kIOReturnSuccess;
}

///
/// Report the index of the highest valid block of the SD card
///
/// @param maxBlock The block index on return
/// @return `kIOReturnSuccess` on success, other values otherwise.
///
IOReturn IOSDBlockStorageDevice::reportMaxValidBlock(UInt64* maxBlock)
{
    pinfo("The storage subsystem requests the index of the maximum block on the media.");
    
    return this->driver->getCardMaxBlockIndex(*maxBlock);
}

///
/// Report the state of the SD card
///
/// @param mediaPresent Set to `true` if the SD card is present, `false` otherwise
/// @param changedState Set to `true` if the state has changed since the last query
/// @return `kIOReturnSuccess` on success, other values otherwise.
///
IOReturn IOSDBlockStorageDevice::reportMediaState(bool* mediaPresent, bool* changedState)
{
    pinfo("The storage subsystem requests the state of the media.");
    
    if (changedState != nullptr)
    {
        *changedState = false;
    }
    
    return this->driver->isCardPresent(*mediaPresent);
}

///
/// Report whether the SD card is removable
///
/// @param isRemovable Set to `true` if the card can be removed, `false` otherwise.
/// @return `kIOReturnSuccess` on success, other values otherwise.
/// @note The card is always removable.
///
IOReturn IOSDBlockStorageDevice::reportRemovability(bool* isRemovable)
{
    pinfo("The storage subsystem asks whether the media is removable.");
    
    *isRemovable = true;
    
    return kIOReturnSuccess;
}

///
/// Report whether the SD card is write protected
///
/// @param isWriteProtected Set to `true` if the card is write protected, `false` otherwise
/// @return `kIOReturnSuccess` on success, other values otherwise.
///
IOReturn IOSDBlockStorageDevice::reportWriteProtection(bool* isWriteProtected)
{
    pinfo("The storage subsystem asks whether the media is write protected.");
    
    return this->driver->isCardWriteProtected(*isWriteProtected);
}

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
IOReturn IOSDBlockStorageDevice::doAsyncReadWrite(IOMemoryDescriptor* buffer, UInt64 block, UInt64 nblks, IOStorageAttributes* attributes, IOStorageCompletion* completion)
{
    // Request to access the SD card
    pinfo("The storage subsystem requests to do asynchronous I/O transfer.");
    
    // Guard: Reject the request if the block device has been terminated
    if (this->isInactive())
    {
        perr("The block storage device has been terminated.");
        
        return kIOReturnNotAttached;
    }
    
    // Guard: Check the buffer direction
    IODirection direction = buffer->getDirection();
    
    if (isNotOneOf(direction, kIODirectionIn, kIODirectionOut))
    {
        perr("The buffer direction %d is invalid.", direction);
        
        return kIOReturnBadArgument;
    }
    
    // Guard: Ensure that the request does not access an out-of-bounds block
    if (block + nblks > this->numBlocks)
    {
        perr("The incoming request attempts to access an out-of-bounds block.");
        
        return kIOReturnBadArgument;
    }
    
    // Examine and submit the request
    if (direction == kIODirectionIn)
    {
        // This is a read request
        if (nblks == 1)
        {
            // Request to read a single block
            pinfo("The storage subsystem requests to read a single block from the block at %llu.", block);
            
            return this->driver->submitReadBlockRequest(buffer, block, nblks, attributes, completion);
        }
        else
        {
            // Request to read multiple blocks
            pinfo("The storage subsystem requests to read %llu blocks from the block at %llu.", nblks, block);
            
            return this->driver->submitReadBlocksRequest(buffer, block, nblks, attributes, completion);
        }
    }
    else
    {
        // This is a write request
        if (nblks == 1)
        {
            // Request to write a single block
            pinfo("The storage subsystem requests to write a single block to the block at %llu.", block);
            
            return this->driver->submitWriteBlockRequest(buffer, block, nblks, attributes, completion);
        }
        else
        {
            // Request to write multiple blocks
            pinfo("The storage subsystem requests to write %llu blocks to the block at %llu.", nblks, block);
            
            return this->driver->submitWriteBlocksRequest(buffer, block, nblks, attributes, completion);
        }
    }
}

//
// MARK: - IOService Implementations
//

///
/// Initialize the block storage device
///
/// @param dictionary The matching dictionary
/// @return `true` on success, `false` otherwise.
///
bool IOSDBlockStorageDevice::init(OSDictionary* dictionary)
{
    if (!super::init(dictionary))
    {
        return false;
    }
    
    this->setStorageProperties();
    
    return true;
}

///
/// Start the block storage device
///
/// @param provider An instance of the host driver
/// @return `true` on success, `false` otherwise.
///
bool IOSDBlockStorageDevice::start(IOService* provider)
{
    pinfo("=======================================");
    pinfo("Starting the SD block storage device...");
    pinfo("=======================================");
    
    if (!super::start(provider))
    {
        perr("Failed to start the super class.");
        
        return false;
    }
    
    // Fetch the host driver
    this->driver = OSDynamicCast(IOSDHostDriver, provider);
    
    if (this->driver == nullptr)
    {
        perr("The provider is not a valid host driver instance.");
        
        return false;
    }
    
    this->driver->retain();
    
    // Fetch the card characteristics
    if (!this->fetchCardCharacteristics())
    {
        perr("Failed to fetch the card characteristics.");
        
        goto error;
    }
    
    // Publish the service to start the storage subsystem
    this->registerService();
    
    pinfo("====================================");
    pinfo("The SD block storage device started.");
    pinfo("====================================");
    
    return true;
    
error:
    OSSafeReleaseNULL(this->driver);
    
    pinfo("============================================");
    perr("Failed to start the SD block storage device.");
    pinfo("============================================");
    
    return false;
}

///
/// Stop the block storage device
///
/// @param provider An instance of the host driver
///
void IOSDBlockStorageDevice::stop(IOService* provider)
{
    OSSafeReleaseNULL(this->driver);
    
    super::stop(provider);
}
