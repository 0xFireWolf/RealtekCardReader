#  Changelog

#### v0.9.6 Beta
- Added support for RTS5260.
- Fixed an issue that the driver is not loaded if the USB card reader is not configured and thus does not expose its host interface.
- Addressed an issue that the driver fails to find the bulk endpoints of the USB card reader.
- Resolved an issue that the driver fails to initialize the USB card reader properly.
- Resolved an issue that the driver cannot detect whether the user has inserted/removed the card to/from the USB card reader.
- Added an option to delay the initialization of the card found during the system boots.
- Corrected the address of the USB-based card reader register that controls the SSC depth.
- Fixed an issue that System Information does not list the USB-based card reader properly.

#### v0.9.5 Beta
- Rebranded the project.
- Refactored the controller and the host device code to support USB-based card readers.
- Added support for RTS5129, RTS5139 and RTS5179.
- Reenabled the bonus feature but now requires the companion Lilu plugin [RealtekCardReaderFriend](https://github.com/0xFireWolf/RealtekCardReaderFriend/).

#### v0.9.4 Beta
- Fixed an issue that driving tables are not set for RTS5227 and RTS522A.  
- Disabled the bonus feature temporarily: Your card reader will not be listed in System Information.  
- Implemented the basic power management.
- Support macOS Mojave.

#### v0.9.3 Beta
- Added support for RTS5227 and RTS522A.

#### v0.9.2 Beta
- Fixed an issue that the block size and the total number of blocks are not reported properly for SDSC cards.
- Fixed an issue that a kernel panic occurs when the card is removed from the system without being ejected properly.
- Fixed an issue that the request event source can be reenabled after the card is removed from the system.
- Power off the bus when the card is removed from the system.
- Added support for RTS8402, RTS8411b and RTS8411.

#### v0.9.1 Beta
- Fixed an issue that `Info.plist` is malformed and thus the kext cannot be injected.
- Fixed an issue that the MSI index could not be probed correctly.
- Added support for RTS5209 and RTS5229.

#### v0.9.0 Beta
- Initial version for public beta testing.

