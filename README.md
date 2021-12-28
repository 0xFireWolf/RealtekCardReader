#  Realtek Card Reader Driver for macOS
Unleash the full potential of your SDXC UHS-I cards

## Introduction

An unofficial macOS kernel extension for Realtek PCIe/USB-based SD card readers.  
It uses the Linux driver as a reference implementation but is written from scratch and carefully designed for macOS to deliver the best performance.

## Features
- No compatibility layer
- Supports SDSC/HC/XC cards
- Supports the default and the high speed modes
- Supports UHS-I SDR12/25/50/104 and DDR50 modes
- Recognizable as a built-in card reader device*
- Device-independent SD host driver stack

#### Notes:
- [RealtekCardReaderFriend](https://github.com/0xFireWolf/RealtekCardReaderFriend/) is required to enable this feature as of v0.9.5.

## Limitations
- MMC cards are not supported
- SD Express cards are not supported

## Current Status
- **Last Updated:** Dec 27, 2021
- **Reference:** [Linux Kernel 5.11](https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.11.tar.xz)
- **Driver Status:** Pre-1.0 Beta (v0.9.6)
    - USB-based RTS5129, 5139 and 5179 card readers are now supported and should be working properly.
    - I/O performance and the overall driver stability are of the most concern at this moment.
- **Known Issues:** [Link](Docs/KnownIssues.md)

## Supported Systems
- macOS Monterey
- macOS Big Sur  
- macOS Catalina  
- macOS Mojave

#### Notes:
- Other systems are not tested yet.

## Supported Devices
| Series | Device ID  |               Name               | Supported | Since | Status |
|:------:|:----------:|:--------------------------------:|:---------:|:-----:|:------:|
|  51X9  | 0x0BDA0129 |  RTS5129 USB 2.0/3.0 Card Reader |    Yes    | 0.9.5 | [![Issues](https://img.shields.io/github/issues-raw/0xFireWolf/RealtekCardReader/RTS5129?color=informational&label=Issues)](https://github.com/0xFireWolf/RealtekCardReader/issues?q=is%3Aissue+is%3Aopen+label%3ARTS5129) |
|  51X9  | 0x0BDA0139 |  RTS5139 USB 2.0/3.0 Card Reader |    Yes    | 0.9.5 | [![Issues](https://img.shields.io/github/issues-raw/0xFireWolf/RealtekCardReader/RTS5139?color=informational&label=Issues)](https://github.com/0xFireWolf/RealtekCardReader/issues?q=is%3Aissue+is%3Aopen+label%3ARTS5139) |
|  51X9  | 0x0BDA0140 |  RTS5179 USB 2.0/3.0 Card Reader |    Yes    | 0.9.5 | [![Issues](https://img.shields.io/github/issues-raw/0xFireWolf/RealtekCardReader/RTS5179?color=informational&label=Issues)](https://github.com/0xFireWolf/RealtekCardReader/issues?q=is%3Aissue+is%3Aopen+label%3ARTS5179) |
|  5209  | 0x10EC5209 |  RTS5209 PCI Express Card Reader |    Yes    | 0.9.1 | [![Issues](https://img.shields.io/github/issues-raw/0xFireWolf/RealtekCardReader/RTS5209?color=informational&label=Issues)](https://github.com/0xFireWolf/RealtekCardReader/issues?q=is%3Aissue+is%3Aopen+label%3ARTS5209) |
|  5227  | 0x10EC5227 |  RTS5227 PCI Express Card Reader |    Yes    | 0.9.3 | [![Issues](https://img.shields.io/github/issues-raw/0xFireWolf/RealtekCardReader/RTS5227?color=informational&label=Issues)](https://github.com/0xFireWolf/RealtekCardReader/issues?q=is%3Aissue+is%3Aopen+label%3ARTS5227) |
|  5227  | 0x10EC522A |  RTS522A PCI Express Card Reader |    Yes    | 0.9.3 | [![Issues](https://img.shields.io/github/issues-raw/0xFireWolf/RealtekCardReader/RTS522A?color=informational&label=Issues)](https://github.com/0xFireWolf/RealtekCardReader/issues?q=is%3Aissue+is%3Aopen+label%3ARTS522A) |
|  5228  | 0x10EC5228 |  RTS5228 PCI Express Card Reader |  Not Yet  |  N/A  | [![Issues](https://img.shields.io/github/issues-raw/0xFireWolf/RealtekCardReader/RTS5228?color=informational&label=Issues)](https://github.com/0xFireWolf/RealtekCardReader/issues?q=is%3Aissue+is%3Aopen+label%3ARTS5228) |
|  5229  | 0x10EC5229 |  RTS5229 PCI Express Card Reader |    Yes    | 0.9.1 | [![Issues](https://img.shields.io/github/issues-raw/0xFireWolf/RealtekCardReader/RTS5229?color=informational&label=Issues)](https://github.com/0xFireWolf/RealtekCardReader/issues?q=is%3Aissue+is%3Aopen+label%3ARTS5229) |
|  5249  | 0x10EC5249 |  RTS5249 PCI Express Card Reader |    Yes    | 0.9.0 | [![Issues](https://img.shields.io/github/issues-raw/0xFireWolf/RealtekCardReader/RTS5249?color=informational&label=Issues)](https://github.com/0xFireWolf/RealtekCardReader/issues?q=is%3Aissue+is%3Aopen+label%3ARTS5249) |
|  5249  | 0x10EC524A |  RTS524A PCI Express Card Reader |    Yes    | 0.9.0 | [![Issues](https://img.shields.io/github/issues-raw/0xFireWolf/RealtekCardReader/RTS524A?color=informational&label=Issues)](https://github.com/0xFireWolf/RealtekCardReader/issues?q=is%3Aissue+is%3Aopen+label%3ARTS524A) |
|  5249  | 0x10EC525A |  RTS525A PCI Express Card Reader |    Yes    | 0.9.0 | [![Issues](https://img.shields.io/github/issues-raw/0xFireWolf/RealtekCardReader/RTS525A?color=informational&label=Issues)](https://github.com/0xFireWolf/RealtekCardReader/issues?q=is%3Aissue+is%3Aopen+label%3ARTS525A) |
|  5260  | 0x10EC5260 |  RTS5260 PCI Express Card Reader |    Yes    | 0.9.6 | [![Issues](https://img.shields.io/github/issues-raw/0xFireWolf/RealtekCardReader/RTS5260?color=informational&label=Issues)](https://github.com/0xFireWolf/RealtekCardReader/issues?q=is%3Aissue+is%3Aopen+label%3ARTS5260) |
|  5261  | 0x10EC5261 |  RTS5261 PCI Express Card Reader |  Not Yet  |  N/A  | [![Issues](https://img.shields.io/github/issues-raw/0xFireWolf/RealtekCardReader/RTS5261?color=informational&label=Issues)](https://github.com/0xFireWolf/RealtekCardReader/issues?q=is%3Aissue+is%3Aopen+label%3ARTS5261) |
|  8411  | 0x10EC5286 |  RTL8402 PCI Express Card Reader |    Yes    | 0.9.2 | [![Issues](https://img.shields.io/github/issues-raw/0xFireWolf/RealtekCardReader/RTL8402?color=informational&label=Issues)](https://github.com/0xFireWolf/RealtekCardReader/issues?q=is%3Aissue+is%3Aopen+label%3ARTL8402) |
|  8411  | 0x10EC5287 | RTL8411B PCI Express Card Reader |    Yes    | 0.9.2 | [![Issues](https://img.shields.io/github/issues-raw/0xFireWolf/RealtekCardReader/RTL8411B?color=informational&label=Issues)](https://github.com/0xFireWolf/RealtekCardReader/issues?q=is%3Aissue+is%3Aopen+label%3ARTL8411B) |
|  8411  | 0x10EC5289 |  RTL8411 PCI Express Card Reader |    Yes    | 0.9.2 | [![Issues](https://img.shields.io/github/issues-raw/0xFireWolf/RealtekCardReader/RTL8411?color=informational&label=Issues)](https://github.com/0xFireWolf/RealtekCardReader/issues?q=is%3Aissue+is%3Aopen+label%3ARTL8411) |

#### Notes:
- By design, all listed devices are supported, and devices that have the same series share most of the controller code.
- RTS525A has the highest priority than other chips, because that's the only chip available for me to test the driver.
- If a device's support status is "Not Yet", its controller is not implemented yet.
- Other Realtek USB card readers (e.g., RTS5138) conform to the USB Attached SCSI (UAS) protocol and therefore may be supported by the native driver.

## Downloads
- [Stable Releases](https://github.com/0xFireWolf/RealtekCardReader/releases) (Recommended)
- [Nightly Builds](https://github.com/0xFireWolf/RealtekCardReader/actions) (Artifacts under each workflow run)

## Boot Arguments
Please refer to this dedicated [page](Docs/BootArgs.md) to see all boot arguments.

## Questions, Issues and Documentation

### Users
Please read [FAQs](Docs/FAQ.md) and [Known Issues](Docs/KnownIssues.md) carefully before asking any questions.  
Please use the issue template before submitting any code-level issues.  
Please clearly indicate your chip model, device ID and revision number and attach the kernel log in your issue.  
Please make sure that your kernel log is complete. Issues without any details will be closed and left unanswered.

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
- [Realtek](https://www.realtek.com/) for the Linux [RTSX PCI/USB](https://github.com/torvalds/linux/tree/master/drivers/misc/cardreader) driver
- [FireWolf](https://github.com/0xFireWolf) for developing the card reader driver for macOS

## Acknowledgements
I would like to acknowledge the following beta testers without whom support for USB-based card readers won't be available.
- [m0d16l14n1](https://github.com/m0d16l14n1) for testing unstable, experimental builds to capture and fix a large number of issues.
- [Sherlocks](https://github.com/Sher1ocks) for testing early builds to debug the timeout issues.
- [whatnameisit](https://github.com/whatnameisit) for testing early builds to debug the hardware initialization issues.
- [gnpaone](https://github.com/gnpaone) for providing multiple comprehensive bug reports.

## License
This project is licensed under BSD-3-Clause.  
Copyright (C) 2021 FireWolf @ FireWolf Pl. All Rights Reserved.
