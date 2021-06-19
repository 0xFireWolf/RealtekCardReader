#  Realtek PCIe Card Reader Driver for macOS
Unleash the full potential of your SDXC UHS-I cards

## Introduction

An unofficial macOS kernel extension for Realtek PCIe-based SD card readers.  
It uses the Linux driver as a reference implementation but is written from scratch and carefully designed for macOS to deliver the best performance.

## Features
- No compatibility layer
- Supports SDSC/HC/XC cards
- Supports the default and the high speed modes
- Supports UHS-I SDR12/25/50/104 and DDR50 modes
- Recognizable as a built-in card reader device
- Device-independent SD host driver stack

## Limitations
- MMC cards are not supported
- SD Express cards are not supported

## Current Status
- **Last Updated:** Jun 18, 2021
- **Reference:** [Linux Kernel 5.11](https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.11.tar.xz)
- **Driver Status:** Pre-1.0 Beta
    - Power management is not implemented.
    - I/O performance and the overall driver stability are of the most concern at this moment.

## Supported Systems
- macOS Big Sur  
- macOS Catalina  

#### Notes:
- Other systems are not tested yet.

## Supported Devices
| Series |   PCI ID   |               Name               | Supported | Since |
|:------:|:----------:|:--------------------------------:|:---------:|:-----:|
|  5209  | 0x10EC5209 |  RTS5209 PCI Express Card Reader |    Yes    | 0.9.1 |
|  5227  | 0x10EC5227 |  RTS5227 PCI Express Card Reader |  Not Yet  |  N/A  |
|  5227  | 0x10EC522A |  RTS522A PCI Express Card Reader |  Not Yet  |  N/A  |
|  5228  | 0x10EC5228 |  RTS5228 PCI Express Card Reader |  Not Yet  |  N/A  |
|  5229  | 0x10EC5229 |  RTS5229 PCI Express Card Reader |    Yes    | 0.9.1 |
|  5249  | 0x10EC5249 |  RTS5249 PCI Express Card Reader |    Yes    | 0.9.0 |
|  5249  | 0x10EC524A |  RTS524A PCI Express Card Reader |    Yes    | 0.9.0 |
|  5249  | 0x10EC525A |  RTS525A PCI Express Card Reader |    Yes    | 0.9.0 |
|  5260  | 0x10EC5260 |  RTS5260 PCI Express Card Reader |  Not Yet  |  N/A  |
|  5261  | 0x10EC5261 |  RTS5261 PCI Express Card Reader |  Not Yet  |  N/A  |
|  8411  | 0x10EC5286 |  RTL8402 PCI Express Card Reader |  Not Yet  |  N/A  |
|  8411  | 0x10EC5287 | RTL8411B PCI Express Card Reader |  Not Yet  |  N/A  |
|  8411  | 0x10EC5289 |  RTL8411 PCI Express Card Reader |  Not Yet  |  N/A  |

#### Notes:
- By design, all listed devices are supported, and devices that have the same series share most of the controller code.
- RTS525A has the highest priority than other chips, because that's the only chip available for me to test the driver.
- If a device's support status is "Not Yet", its controller is not implemented yet.

## Questions, Issues and Documentation

### Users
Please read [FAQs](Docs/FAQ.md) carefully before asking any questions.  
Please use the issue template before submitting any code-level issues.  
Please clearly indicate your chip model, device ID and revision number and attach the kernel log in your issue.  

### Developers
You are welcome to submit pull requests to improve this driver.  
Please read the code documentation to understand how each class/function is related to the Linux driver code and how to add support for other listed devices.  
A detailed port note will be available later.

## Discussion
A discussion thread is available on [InsanelyMac](https://www.insanelymac.com/forum/topic/348130-general-discussion-realtek-pcie-card-reader-driver-for-macos/).

## Support
Writing a driver from scratch is hard and time consuming.  
If you would like to support my work, please consider a donation.  

[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/donate/?business=M6AHXMUVSZQTS&no_recurring=0&item_name=Support+Realtek+PCIe+card+deader+driver+for+macOS&currency_code=USD)


## References
- [SD Physical Layer Simplified Specification](https://www.sdcard.org/downloads/pls/)

## Credits
- [Acidanthera](https://github.com/acidanthera) for [MacKernelSDK](https://github.com/acidanthera/MacKernelSDK)
- [Realtek](https://www.realtek.com/) for the Linux [RTSX PCI](https://github.com/torvalds/linux/tree/master/drivers/misc/cardreader) driver
- [FireWolf](https://github.com/0xFireWolf) for developing the card reader driver for macOS

## License
This project is licensed under BSD-3-Clause.  
Copyright (C) 2021 FireWolf @ FireWolf Pl. All Rights Reserved.
