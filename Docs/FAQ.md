#  Frequently Asked Questions

**Last Updated:** Aug 25, 2021

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
- 4: I/O performance and stability of the driver.
- 5: Power management implementation.
- 6: Support more card readers. (<-- Right Now)

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

#### macOS Big Sur and later
Make sure that you are using the DEBUG version of the driver and have added the boot argument `msgbuf=10485760`. (Note that there is an extra zero at the end of `1048576`.)

Run the following command in your terminal.

```(sh)
sudo dmesg | grep RTSX > ~/Desktop/rtsx.log
```

You can now find the log on your `Desktop` folder.  
Open your log and verify that your log starts with "Starting the Realtek PCIe card reader controller...".
You should be able to see something similar to the following lines.
```
[    1.132045]: RTSX: virtual bool RealtekPCICardReaderController::start(IOService *) DInfo: ===================================================
[    1.132051]: RTSX: virtual bool RealtekPCICardReaderController::start(IOService *) DInfo: Starting the Realtek PCIe card reader controller...
[    1.132053]: RTSX: virtual bool RealtekPCICardReaderController::start(IOService *) DInfo: ===================================================
[    1.132079]: RTSX: bool RealtekPCICardReaderController::mapDeviceMemory() DInfo: Prepare to map the device memory (BAR = 0x10)...
[    1.132085]: RTSX: bool RealtekPCICardReaderController::mapDeviceMemory() DInfo: The device memory has been mapped successfully.
[    1.132087]: RTSX: bool RealtekPCICardReaderController::setupWorkLoop() DInfo: Creating the workloop and the command gate...
[    1.132105]: RTSX: bool RealtekPCICardReaderController::setupWorkLoop() DInfo: The workloop and the command gate have been created.
[    1.132147]: RTSX: int RealtekPCICardReaderController::probeMSIIndex() DInfo: Found the MSI index = 1.
```
If the log file is empty or is incomplete, please install the [DebugEnhancer](https://github.com/acidanthera/DebugEnhancer) and try again.  
Note that please avoid using the DEBUG version of other kexts when you test this driver.  
The kernel buffer has a limited capacity thus cannot hold the logs produced by these kexts together.  

#### macOS Catalina and earlier

Make sure that you are using the DEBUG version of the driver.  
Run the following command in your terminal instead.  

```(sh)
log show --last boot | grep RTSX > ~/Desktop/rtsx.log
```

### What is the difference between the DEBUG and the RELEASE version?

The DEBUG version produces a massive amount of log that records accesses to some key registers and thus slows down the boot process and I/O transactions.  
However, you are recommended to use the DEBUG version at first to ensure that the driver is working properly with your hardware.  
You should always attach the log produced by the DEBUG version when you are asking for help on the forum and/or filing an issue on Github.  
Once everything is fine, you may switch to the RELEASE version.

The RELEASE version produces log only when there is an error and thus may not be helpful when you plan to investigate an issue.  
However, you are recommended to use the RELEASE version after you ensure that the driver works fine with your hardware to enjoy the best performance.

## Performance

### General Notes

- Please use a SDXC UHS-I card to get a more precise estimate of the I/O speed under macOS.  
- Please install the RELEASE version to avoid the performance degradation due to logging.
- Most cards have a high read speed but a rather low write speed. Refer to the product manual of your cards.
- Some high-end cards may have a speed that exceeds the limit of your hardware. You may test the speed under other systems to know the limits.

### How do I test the read speed under macOS?

Supposed that your card is mounted at `/Volumes/SDXC/` and there is a file named `Test.bin` on your card.
Open the terminal and type the following command to test the read speed.

```(sh)
dd if=/Volumes/SDXC/Test.bin of=/dev/null bs=128k
```

Once the test finishes, you should be able to see the result similar to below.

```(sh)
bash$ dd if=/Volumes/SDXC/1GB.bin of=/dev/null bs=128k
8000+0 records in
8000+0 records out
1048576000 bytes transferred in 12.131642 secs (86433147 bytes/sec)
```

In this case, the read speed is 86433147 bytes/s (82.42 MB/s).
If you want to test the speed again with the **same** file, type `sudo purge` to clear the filesystem cache before issuing the next `dd` command.

### How do I test the write speed under macOS?

Supposed that your card is mounted at `/Volumes/SDXC/` and there is a file named `Test.bin` on your desktop.
Open the terminal and type the following command to test the write speed.

```(sh)
dd if=/Users/<YourUserName>/Desktop/Test.bin of=/Volumes/SDXC/Test.bin bs=128k
```

Once the test finishes, you should be able to see the result similar to below.

```(sh)
bash$ dd if=/Users/FireWolf/Desktop/Archive.7z of=/Volumes/SDXC/Archive.7z bs=128k
7938+1 records in
7938+1 records out
1040485250 bytes transferred in 12.380992 secs (84038925 bytes/sec)
```

In this case, the write speed is 84038925 bytes/s (80.15 MB/s).
