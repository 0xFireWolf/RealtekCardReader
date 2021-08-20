# Boot Arguments

### Overview
This page documents all boot arguments that can be used to customize the driver behavior.  
However, they are intended to debug the driver only, and under normal circumstances, you do not need them at all.  
Please carefully read the description before you change any default settings.

### How To
If a boot argument has a `Boolean` type, you simply add the boot argument string to your bootloader configuration or the `boot-args` entry in NVRAM.  
If a boot argument has a numeric type, for example `UInt32`, you should add the boot argument string along with the value in the form of `<Name>=<Value>` instead.
For example, if the name is `rtsxabcd` and the value is `10`, `<Name>=<Value>` becomes `rtsxabcd=10`.

### Card Specific
- InitAt3v3
    - Boot Argument: `-iosd3v3`
    - Value Type: `Boolean`
    - Default Value: `false`
    - Description: Add this boot argument to initialize all cards at 3.3V, so cards can work at the default or the high speed mode only.
- InitAtDefaultSpeed
    - Boot Argument: `-iosddsm`
    - Value Type: `Boolean`
    - Default Value: `false`
    - Description: Add this boot argument to initialize all cards at the default speed mode. The maximum data transfer rate is limited to 12 MB/s.
- InitAtHighSpeed
    - Boot Argument: `-iosdhsm`
    - Value Type: `Boolean`
    - Default Value: `false`
    - Description: Add this boot argument to initialize all cards at the high speed mode. The maximum data transfer rate is limited to 25 MB/s.
- SeparateAccessBlocksRequest
    - Boot Argument: `-iosdsabr`
    - Value Type: `Boolean`
    - Default Value: `false`
    - Description: Add this boot argument to separate each CMD18/25 request into multiple CMD17/24 ones, so the host driver will not access multiple blocks on the card in one shot.
- ACMDMaxNumAttempts
    - Boot Argument: `iosdamna`
    - Value Type: `UInt32`
    - Default Value: `2`
    - Minimum Value: `1`
    - Description: Specify the maximum number of attempts to retry an application command (`ACMD*`).

### PCIe-based Card Reader Specific
- DelayCardInitAtBoot
    - Boot Argument: `rtsxdcib`
    - Value Type: `UInt32`
    - Default Value: `100`
    - Minimum Value: `0`
    - Description: Specify the amount of time in milliseconds to delay the card initialization if the card is present when the driver starts. Increase the delay if your card cannot be initialized when the system boots.

### USB-based Card Reader Specific
- DeviceStatusPollingInterval
    - Boot Argument: `rtsxdspi`
    - Value Type: `UInt32`
    - Default Value: `500`
    - Minimum Value: `100`
    - Description: Specify the interval in milliseconds of polling for the device status. A background thread checks whether a card is present every `interval` milliseconds and notifies other driver components if a card is inserted or removed. Increasing the interval will increase the latency of processing the card event, while decreasing the value will waste your CPU cycle, so please choose an interval value wisely.

### PCIe/USB-based Card Reader Specific
- DelayStableSSCClock
    - Boot Argument: `rtsxdssc`
    - Value Type: `UInt32`
    - Default Value: `10`
    - Minimum Value: `1`
    - Description: Specify the amount of time in milliseconds to wait until the SSC clock becomes stable. If the value is too small, commands may timeout after the driver switches the card clock. Increase this value if you find that the driver fails to enable the 4-bit bus in the kernel log.
