#  Frequently Asked Questions

**Last Updated:** Jun 18, 2021

## Device Support

### How do I know if my card reader is supported?

You can find a list of supported card readers at the [front page](../README.md).  
If the support status says `Yes`, your card reader is supported by this driver.

### What if my card reader is not listed in the table?

It means that your card reader is not supported by the original Linux driver module `rtsx_pci`.  
Since this driver is based on that module, your card reader is not and probably will not be supported by this driver.

### My card reader is listed in the table but is not supported yet, when are you going to implement it?

Unfortunately, I don't have an ETA at this moment.  
However, it should be relatively easy to add support for other card readers, because the base controller class contains most of the common code and a rich set of APIs to manipulate the registers.  
Please note that I am not paid to write this driver and I have other work to do, so I cannot dedicate all my time to this project.   
If you would like to support me and this project, please consider a donation.

### What is the roadmap of this project?

Milestones:
- 1: Base controller implementation for all supported Realtek PCIe-based card readers.
- 2: Chip-independent Realtek SD host device implementation.
- 3: Initial SD host driver stack implementation for macOS.
- 4: I/O performance and stability of the driver. (<-- Right Now)
- 5: Power management implementation.
- 6: Support more card readers.

## Installation

### How do I install this driver?

If you are using OpenCore, refer to the guide provided by [Dortania](https://dortania.github.io/getting-started/).  
If you are using Clover, place the driver under the `kexts/<OS Version>` folder.  

### How do I know if the driver is loaded?

Run the following command in your terminal.  

```(sh)
kextstat | grep rtsx
```

If you see `science.firewolf.rtsx` in your terminal, then the driver is loaded.  
As a bonus, you should be able to see your card reader recognized as a native one in **System Information**.

### How do I dump the log produced by the driver?

Make sure that you are using the DEBUG version of the driver and have added the boot argument `msgbuf=10485760`. (Note that there is an extra zero at the end of `1048576`.)

Run the following command in your terminal.

```(sh)
sudo dmesg | grep RTSX > ~/Desktop/rtsx.log
```

You can now find the log on your `Desktop` folder.  
Open your log and verify that your log starts with "Starting the Realtek PCIe card reader controller...".
You should be able to see something similar to the following lines.
```
[    1.132045]: RTSX: virtual bool RealtekPCIeCardReaderController::start(IOService *) DInfo: ===================================================
[    1.132051]: RTSX: virtual bool RealtekPCIeCardReaderController::start(IOService *) DInfo: Starting the Realtek PCIe card reader controller...
[    1.132053]: RTSX: virtual bool RealtekPCIeCardReaderController::start(IOService *) DInfo: ===================================================
[    1.132079]: RTSX: bool RealtekPCIeCardReaderController::mapDeviceMemory() DInfo: Prepare to map the device memory (BAR = 0x10)...
[    1.132085]: RTSX: bool RealtekPCIeCardReaderController::mapDeviceMemory() DInfo: The device memory has been mapped successfully.
[    1.132087]: RTSX: bool RealtekPCIeCardReaderController::setupWorkLoop() DInfo: Creating the workloop and the command gate...
[    1.132105]: RTSX: bool RealtekPCIeCardReaderController::setupWorkLoop() DInfo: The workloop and the command gate have been created.
[    1.132147]: RTSX: int RealtekPCIeCardReaderController::probeMSIIndex() DInfo: Found the MSI index = 1.
```
If the log file is empty or is incomplete, please install the [DebugEnhancer](https://github.com/acidanthera/DebugEnhancer) and try again.


### What is the difference between the DEBUG and the RELEASE version?

The DEBUG version produces a massive amount of log that records accesses to some key registers and thus slows down the boot process and I/O transactions.  
However, you are recommended to use the DEBUG version at first to ensure that the driver is working properly with your hardware.  
You should always attach the log produced by the DEBUG version when you are asking for help on the forum and/or filing an issue on Github.  
Once everything is fine, you may switch to the RELEASE version.

The RELEASE version produces log only when there is an error and thus may not be helpful when you plan to investigate an issue.  
However, you are recommended to use the RELEASE version after you ensure that the driver works fine with your hardware to enjoy the best performance.
