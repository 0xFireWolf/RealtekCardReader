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
- NoACMD23
    - Boot Argument: `-iosdnoacmd23`
    - Value Type: `Boolean`
    - Default Value: `false`
    - Description: Add this boot argument to ask the host driver not to issue the ACMD23 before sending the CMD25 to the card. When the driver processes a multi-block write (CMD25) request, the specification recommends to issue an ACMD23 to pre-erase blocks to be written to improve the write performance. By default, the host driver always sends the ACMD23 before the CMD25. Use this boot argument if you observe any write performance degradation.
- ACMDMaxNumAttempts
    - Boot Argument: `iosdamna`
    - Value Type: `UInt32`
    - Default Value: `2`
    - Minimum Value: `1`
    - Description: Specify the maximum number of attempts to retry an application command (`ACMD*`).
- PowerOffDelay
    - Boot Argument: `iosdpoweroff`
    - Value Type: `UInt32`
    - Default Value: `100`
    - Minimum Value: `1`
    - Description: Specify the amount of time in milliseconds the bus stays powered off before the card is powered on again. A card that has switched to 1.8V signaling only returns to 3.3V once its supply has fully discharged; with a shorter delay the retry at a lower speed mode fails. Previous versions used 1 ms.
- UseLegacyCardCharacteristicsDictionary
    - Boot Argument: `-iosdlccd`
    - Value Type: `Boolean`
    - Default Value: `false`
    - Description: Add this boot argument to use the legacy format of the card characteristics dictionary. Apple has changed the format of the card characteristics dictionary in macOS Sonoma 14.4, so users see an error message like "There was an error loading card reader information" when they click "Card Reader" in System Information.app. Use this boot argument if you use this driver on earlier macOS versions.

### PCIe-based Card Reader Specific
- DelayCardInitAtBoot
    - Boot Argument: `rtsxdcib`
    - Value Type: `UInt32`
    - Default Value: `100`
    - Minimum Value: `0`
    - Description: Specify the amount of time in milliseconds to delay the card initialization if the card is present when the driver starts. Increase the delay if your card cannot be initialized when the system boots.
- CardInsertionDebounce
    - Boot Argument: `rtsxdebounce`
    - Value Type: `UInt32`
    - Default Value: `200`
    - Minimum Value: `0`
    - Description: Specify the amount of time in milliseconds to wait after a card insertion interrupt before the card is set up (the interrupt fires while the card is still sliding in; Linux waits 200 ms as well).
- WakeCardDetectSettle
    - Boot Argument: `rtsxwakesettle`
    - Value Type: `UInt32`
    - Default Value: `250`
    - Minimum Value: `0`
    - Description: Specify the amount of time in milliseconds to wait after the card reader is re-initialized on wake before the driver checks whether a card is present. Some chips (e.g. RTS5227) report the opposite of the slot state right after the re-initialization, so a mounted card would be detached or a card that is not there would be attached. Set it to `0` to disable the delay.
- DisableIdleState
    - Boot Argument: `-rtsxnoidle`
    - Value Type: `Boolean`
    - Default Value: `false`
    - Description: Add this boot argument to keep the card reader in the active state forever, i.e. never re-enable the ASPM when the reader is idle (the behavior of previous versions). Intended for debugging only: the PCIe link then never enters L1 and the CPU package cannot reach the deep C-states (C6 and below).
- PowerDiagnostics
    - Boot Argument: `-rtsxdiag`
    - Value Type: `Boolean`
    - Default Value: `false` (always `true` in debug builds)
    - Description: Add this boot argument to publish the power-management state of the card reader (ASPM, LTR, L1 substates, idle entries) as the `RTSX Power` property of the controller in the I/O Registry, updated on every idle entry. Read it with `ioreg -rc RealtekPCICardReaderController -l | grep "RTSX Power"`.

### USB-based Card Reader Specific
- DeviceStatusPollingInterval
    - Boot Argument: `rtsxdspi`
    - Value Type: `UInt32`
    - Default Value: `500`
    - Minimum Value: `100`
    - Description: Specify the interval in milliseconds of polling for the device status. A background thread checks whether a card is present every `interval` milliseconds and notifies other driver components if a card is inserted or removed. Increasing the interval will increase the latency of processing the card event, while decreasing the value will waste your CPU cycle, so please choose an interval value wisely.
    
- FetchCardStatusViaControlEndpoint
    - Boot Argument: `-rtsxppsta`
    - Value Type: `Boolean`
    - Default Value: `false`
    - Description: Add this boot argument to fetch the card status via the control endpoint instead of a bulk transfer. Some RTS5139 chips can report the card status only via the control endpoint thus are not compatible with the default mechanism.

### PCIe/USB-based Card Reader Specific
- DelayStableSSCClock
    - Boot Argument: `rtsxdssc`
    - Value Type: `UInt32`
    - Default Value: `10`
    - Minimum Value: `1`
    - Description: Specify the amount of time in milliseconds to wait until the SSC clock becomes stable. If the value is too small, commands may time out after the driver switches the card clock. Increase this value if you find that the driver fails to enable the 4-bit bus in the kernel log.
