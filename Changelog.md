#  Changelog

#### v0.9.2
- Fixed an issue that the block size and the total number of blocks are not reported properly for SDSC cards.
- Fixed an issue that a kernel panic occurs when the card is removed from the system without being ejected properly.
- Fixed an issue that the request event source can be reenabled after the card is removed from the system.
- Power off the bus when the card is removed from the system.

#### v0.9.1
- Fixed an issue that `Info.plist` is malformed and thus the kext cannot be injected.
- Fixed an issue that the MSI index could not be probed correctly.
- Added support for RTS5209 and RTS5229.

#### v0.9.0
- Initial version for public beta testing.

