//
//  IOPCIeDevice.hpp
//  RealtekCardReader
//
//  Created by FireWolf on 7/7/21.
//

#ifndef IOPCIeDevice_hpp
#define IOPCIeDevice_hpp

#include <IOKit/pci/IOPCIDevice.h>
#include "Debug.hpp"

#define PCI_EXP_LNKCTL        16    /* Link Control */
#define  PCI_EXP_LNKCTL_ASPMC    0x0003    /* ASPM Control */
#define  PCI_EXP_LNKCTL_ASPM_L0S 0x0001    /* L0s Enable */
#define  PCI_EXP_LNKCTL_ASPM_L1  0x0002    /* L1 Enable */
#define  PCI_EXP_LNKCTL_RCB    0x0008    /* Read Completion Boundary */
#define  PCI_EXP_LNKCTL_LD    0x0010    /* Link Disable */
#define  PCI_EXP_LNKCTL_RL    0x0020    /* Retrain Link */
#define  PCI_EXP_LNKCTL_CCC    0x0040    /* Common Clock Configuration */
#define  PCI_EXP_LNKCTL_ES    0x0080    /* Extended Synch */
#define  PCI_EXP_LNKCTL_CLKREQ_EN 0x0100 /* Enable clkreq */
#define  PCI_EXP_LNKCTL_HAWD    0x0200    /* Hardware Autonomous Width Disable */
#define  PCI_EXP_LNKCTL_LBMIE    0x0400    /* Link Bandwidth Management Interrupt Enable */
#define  PCI_EXP_LNKCTL_LABIE    0x0800    /* Link Autonomous Bandwidth Interrupt Enable */

// PCIe Capability (From Linux)
#define PCI_EXP_DEVCTL2        40    /* Device Control 2 */
#define  PCI_EXP_DEVCTL2_COMP_TIMEOUT    0x000f    /* Completion Timeout Value */
#define  PCI_EXP_DEVCTL2_COMP_TMOUT_DIS    0x0010    /* Completion Timeout Disable */
#define  PCI_EXP_DEVCTL2_ARI        0x0020    /* Alternative Routing-ID */
#define  PCI_EXP_DEVCTL2_ATOMIC_REQ    0x0040    /* Set Atomic requests */
#define  PCI_EXP_DEVCTL2_ATOMIC_EGRESS_BLOCK 0x0080 /* Block atomic egress */
#define  PCI_EXP_DEVCTL2_IDO_REQ_EN    0x0100    /* Allow IDO for requests */
#define  PCI_EXP_DEVCTL2_IDO_CMP_EN    0x0200    /* Allow IDO for completions */
#define  PCI_EXP_DEVCTL2_LTR_EN        0x0400    /* Enable LTR mechanism */
#define  PCI_EXP_DEVCTL2_OBFF_MSGA_EN    0x2000    /* Enable OBFF Message type A */
#define  PCI_EXP_DEVCTL2_OBFF_MSGB_EN    0x4000    /* Enable OBFF Message type B */
#define  PCI_EXP_DEVCTL2_OBFF_WAKE_EN    0x6000    /* OBFF using WAKE# signaling */

static inline UInt16 IOPCIeDeviceConfigRead16(IOPCIDevice* device, IOByteCount offset)
{
    UInt32 base = device->findPCICapability(kIOPCICapabilityIDPCIExpress);
    
    passert(base != 0, "The given device is not a PCIe device.");
    
    return device->configRead16(base + offset);
}

static inline void IOPCIeDeviceConfigWrite16(IOPCIDevice* device, IOByteCount offset, UInt16 value)
{
    UInt32 base = device->findPCICapability(kIOPCICapabilityIDPCIExpress);
    
    passert(base != 0, "The given device is not a PCIe device.");
    
    device->configWrite16(base + offset, value);
}

static inline void IOPCIeDeviceConfigSet16(IOPCIDevice* device, IOByteCount offset, UInt16 set)
{
    UInt32 base = device->findPCICapability(kIOPCICapabilityIDPCIExpress);
    
    passert(base != 0, "The given device is not a PCIe device.");
    
    UInt16 value = device->configRead16(base + offset) | set;
    
    device->configWrite16(base + offset, value);
}

static inline void IOPCIeDeviceConfigClear16(IOPCIDevice* device, IOByteCount offset, UInt16 clear)
{
    UInt32 base = device->findPCICapability(kIOPCICapabilityIDPCIExpress);
    
    passert(base != 0, "The given device is not a PCIe device.");
    
    UInt16 value = device->configRead16(base + offset) & (~clear);
    
    device->configWrite16(base + offset, value);
}

#endif /* IOPCIeDevice_hpp */
