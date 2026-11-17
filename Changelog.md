#  Changelog

#### v0.9.8 Beta
- Added support for the new format of the card characteristics dictionary, resolving the error "There was an error loading card reader information" when users click "Card Reader" in System Information.app. Use the new boot argument `-iosdlccd` if you use this driver on earlier macOS versions.
- Fixed a kernel panic "Sleep transition timed out" that occurs when the completion of a card event is lost while the system sleeps or wakes up. The driver now waits for a card event for a bounded amount of time, and each completion is invoked exactly once.
- Fixed a lost wakeup in the synchronous card event wait that delays the system sleep by 10 seconds and stalls the system wake by up to 30 seconds.
- Fixed a kernel panic that occurs when a block request is serviced before the card is attached (e.g. right after the system wakes up or while it shuts down).
- Fixed an issue that pending block requests are dropped without being completed when the card is removed, leaving the caller waiting forever. A request submitted for a removed card is never serviced on the next card.
- Fixed a kernel panic that occurs when a second card insertion event arrives while a card is attached.
- Fixed an issue that the driver fails to initialize a card at a lower speed mode after it has failed at the UHS-I mode. Added the boot argument `iosdpoweroff`.
- Fixed an issue that the driver reads and writes the PCIe Link Control, Device Control 2 and L1 substates registers at wrong offsets.
- The card reader now returns to the idle state (ASPM, LTR and L1 substates) when no request has been processed for 200 ms, so the CPU package can reach the deep C-states (C6 and below) again. Added the boot argument `-rtsxnoidle` to restore the previous behavior.
- Card insertions are now debounced for 200 ms, like the Linux driver. Added the boot argument `rtsxdebounce`.
- The driver now waits 250 ms after the card reader is re-initialized on wake before it checks whether a card is present, as some chips report a wrong card presence right after the re-initialization. Added the boot argument `rtsxwakesettle`.
- Added the boot argument `-rtsxdiag` to publish the power management state of the card reader to the I/O Registry.
- The manufacturing date, the specification version and the product revision of the card are now shown correctly in System Information.app in locales that use a decimal comma (e.g. `2019-01` instead of `2.019,01`, `SD 5.00` and `v8.0` instead of `5,00` and `8,0`).
- Removed build host paths from the binary.

#### v0.9.7 Beta
- ~~Fixed an issue that the card is not mounted after the computer wakes up when sleeps with the card inserted.~~
- Fixed a race condition that may resume the polling thread even though the client requests to pause it.
- Fixed an issue that the driver cannot be loaded on macOS Mojave. (Thanks @reelgirly)
- Cards that have failed to initialize at its maximum speed mode will be initialized at a lower speed mode.

#### v0.9.6 Beta
- Added support for RTS5260.
- Fixed an issue that the driver is not loaded if the USB card reader is not configured and thus does not expose its host interface.
- Addressed an issue that the driver fails to find the bulk endpoints of the USB card reader.
- Resolved an issue that the driver fails to initialize the USB card reader properly.
- Resolved an issue that the driver cannot detect whether the user has inserted/removed the card to/from the USB card reader.
- Added an option to delay the initialization of the card found during the system boots.
- Corrected the address of the USB-based card reader register that controls the SSC depth.
- Fixed an issue that System Information does not list the USB-based card reader properly.
- Resolved an issue that commands are timed out after the driver switches the bus clock of the card reader.
- Resolved an issue that USB-based card readers fail to initialize UHS-I cards properly.
- Pause the polling thread of the USB-based card reader when the host driver is processing a card event.
- Skip setting the bus power mode if the card reader is already operating at the requested mode.
- Resolved an issue that the card clock is incorrectly toggled thus fails to switch to 1.8V after the host device sends the CMD11.
- Revise the design of the host driver stack to make all data structures that are related to host requests device-independent.
- Optimize the host driver stack to avoid unnecessary bounce buffer allocations and data copies in card reader controllers.
- Minimize the amount of time the transfer buffer is ready for DMA to reduce the pressure in the low 4GB of physical memory.
- Addressed an issue that CMD17 and CMD24 fail to transfer any data from/to the card with USB-based card readers.
- Switched the mode from `AutoRead2` to `AutoRead3` when the host device services an inbound DMA transfer request.
- Resolved an issue that bulk pipes halted after the driver issued a DMA transfer request to USB-based card readers. 
- Revise the implementation of processing block requests to fix the regression caused by the new driver stack.
- Fixed an issue that the maximum segment size for DMA transfers is incorrectly defined for PCIe-based card readers.
- Fixed a race condition introduced by the polling thread that corrupts the response sent by USB-based card readers.
- Addressed an issue that UHS-I cards running in SDR50/104 mode are not tuned thus send invalid responses to USB-based card readers.
- Parse the SD status register value so that System Information can show the speed class of the card.
- System Information now shows the information of cards inserted into USB-based card readers.

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
