#include <stdio.h>

#include <pci/pci.h>
#include "unistd.h"

//#define MANUAL_READ

static char *link_speed(int speed)
{
	switch (speed)
	{
	case 1:
		return "2.5GT/s";
	case 2:
		return "5GT/s";
	case 3:
		return "8GT/s";
	case 4:
		return "16GT/s";
	case 5:
		return "32GT/s";
	case 6:
		return "64GT/s";
	default:
		return "unknown";
	}
}

static char *link_compare(int type, int sta, int cap)
{
	if (sta > cap)
		return " (overdriven)";
	if (sta == cap)
		return "";
	if ((type == PCI_EXP_TYPE_ROOT_PORT) || (type == PCI_EXP_TYPE_DOWNSTREAM) ||
		(type == PCI_EXP_TYPE_PCIE_BRIDGE))
		return "";
	return " (downgraded)";
}

static u8 get_pciexp_cap_addr(struct pci_dev *dev)
{
	u8 id;
	u8 capstrptr = pci_read_byte(dev, PCI_CAPABILITY_LIST); // 0x34
	while (capstrptr != 0x00) {
		id = pci_read_byte(dev, capstrptr);
		if (id == PCI_CAP_ID_EXP)
			return capstrptr;
		else if (id == PCI_CAP_ID_NULL)
			return 0;
		else
			capstrptr = pci_read_byte(dev, capstrptr + PCI_CAP_LIST_NEXT);
	}
	return 0;
}

int main(void)
{
	struct pci_access *pacc;
	struct pci_dev *dev;
	unsigned int c, b;
	char namebuf[1024], *name;
	u32 aspm, cap_speed, sta_speed;
	int pcap_type;
	struct pci_cap *pcap;

	u8 manual_addr;

	pacc = pci_alloc();		/* Get the pci_access structure */
	/* Set all options you want -- here we stick with the defaults */
	pci_init(pacc);		/* Initialize the PCI library */
	pci_scan_bus(pacc);		/* We want to get the list of devices */
	for (dev=pacc->devices; dev; dev=dev->next)	/* Iterate over all devices */
	{
		pci_fill_info(dev, PCI_FILL_IDENT | PCI_FILL_BASES | PCI_FILL_CLASS );	// Fill in header info we need 
		c = pci_read_byte(dev, PCI_INTERRUPT_PIN);					// Read config register directly 
		printf("%04x:%02x:%02x.%d vendor=%04x device=%04x class=%04x irq=%d (pin %d) base0=%lx",
			dev->domain, dev->bus, dev->dev, dev->func, dev->vendor_id, dev->device_id,
			dev->device_class, dev->irq, c, (long) dev->base_addr[0]);
			// Look up and print the full name of the device 
		name = pci_lookup_name(pacc, namebuf, sizeof(namebuf), PCI_LOOKUP_DEVICE, dev->vendor_id, dev->device_id);
		printf(" (%s)\n", name);

		manual_addr = get_pciexp_cap_addr(dev);
#ifdef MANUAL_READ		
		if (manual_addr != 0) {
			//printf("addr_cap = 0x%x\taddr_sta = 0x%x\n",manual_addr + PCI_EXP_LNKCAP, manual_addr + PCI_EXP_LNKSTA);
			cap_speed = pci_read_long(dev, manual_addr + PCI_EXP_LNKCAP) & PCI_EXP_LNKCAP_SPEED;
			sta_speed = pci_read_word(dev, manual_addr + PCI_EXP_LNKSTA) & PCI_EXP_LNKSTA_SPEED;
			pcap_type = pci_read_byte(dev, manual_addr + PCI_EXP_FLAGS_TYPE);
			printf("Manual LnkSta:\t Gen%d %s%s\n", sta_speed, link_speed(sta_speed), link_compare(pcap_type, sta_speed, cap_speed));
			printf("Manual LnkCap:\t Gen%d %s\n", cap_speed, link_speed(cap_speed));
		}
#else
		pcap = pci_find_cap(dev, PCI_CAP_ID_EXP, PCI_CAP_NORMAL);
		if (pcap != NULL) {
			cap_speed = pci_read_long(dev, pcap->addr + PCI_EXP_LNKCAP) & PCI_EXP_LNKCAP_SPEED;
			sta_speed = pci_read_word(dev, pcap->addr + PCI_EXP_LNKSTA) & PCI_EXP_LNKSTA_SPEED;
			pcap_type = pci_read_byte(dev, pcap->addr + PCI_EXP_FLAGS_TYPE);
			//printf("type = 0x%x\n", (pcap_type&PCI_EXP_FLAGS_TYPE) >>4);
			printf("LnkSta:\t Gen%d %s%s\n", sta_speed, link_speed(sta_speed), link_compare(pcap_type, sta_speed, cap_speed));
			printf("LnkCap:\t Gen%d %s\n", cap_speed, link_speed(cap_speed));
		}
#endif
		printf("  |00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
		printf("__________________________________________________\n");
		for (int i = 0; i < 256; i++) {
			if (i == 0)
				printf ("0 |");
			b = pci_read_byte(dev, PCI_VENDOR_ID + i);
			printf("%02x ", b);
			if ((i+1)%16==0 && i<255)
				printf("\n%01x |", (i+1)/16);
		}
		printf("\n\n");

	}
	pci_cleanup(pacc);		/* Close everything */
	return 0;
}
