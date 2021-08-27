# Known Issues

### Overview
This page documents all known issues found during developments and reported by users.  

### PCIe Card Readers

#### Command Timeouts on certain RTS5227
- **Category:** Device Support
- **Affected Machines:** Lenovo Thinkpad T440/450
- **Description:** Cards are not recognized and the kernel log reports a bunch of timeout errors.
- **Workaround:** N/A as kernel logs provided by users are incomplete. 

### USB Card Readers

### All Card Readers

#### Card Ejected After Waked Up
- **Category:** Power Management
- **Description:** If users put the computer into sleep while the card remains inserted, the card will be ejected by the storage subsystem when the computer wakes up.
- **Workaround:** Users must remove the card from the card slot and insert it back after the computer wakes up.
