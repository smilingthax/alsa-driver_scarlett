#include "au8810.h"
#include "au88x0.h"
static struct pci_device_id snd_vortex_ids[] = {
    {PCI_VENDOR_ID_AUREAL, PCI_DEVICE_ID_AUREAL_ADVANTAGE,
     PCI_ANY_ID, PCI_ANY_ID, 0, 0, 1,},
    {0,}
};
#include "au88x0.c"
