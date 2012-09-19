#include <linux/config.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#if defined(CONFIG_MODVERSIONS) && !defined(__GENKSYMS__) && !defined(__DEPEND__)
#define MODVERSIONS
#include <linux/modversions.h>
#include "sndversions.h"
#endif
#endif

#include <linux/pci.h>
#include "config.h"
#include "adriver.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0) && defined(CONFIG_PROC_FS)
#include <linux/proc_fs.h>
static inline struct proc_dir_entry *create_proc_read_entry(const char *name,
	mode_t mode, struct proc_dir_entry *base, 
	read_proc_t *read_proc, void * data)
{
	struct proc_dir_entry *res=create_proc_entry(name,mode,base);
	if (res) {
		res->read_proc=read_proc;
		res->data=data;
	}
	return res;
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 0)
static LIST_HEAD(snd_pci_devices);

struct snd_pci_device {
	struct list_head *list;
	struct device dev;
	struct pci_dev *pci;
	u64 dummy_mask;
};

static inline struct pci_dev *to_pci_dev(struct device *dev)
{
	return dev->d.pci;
}

static inline void *dma_alloc_coherent(struct device *dev, size_t size,
				       dma_addr_t *dma_handle, int flag)
{
	return pci_alloc_consistent(to_pci_dev(dev), size, dma_handle);
}

static inline void dma_free_coherent(struct device *dev, size_t size,
				     void *vaddr, dma_addr_t dma_handle)
{
	pci_free_consistent(to_pci_dev(dev), size, vaddr, dma_handle);
}

struct device *snd_kdevice_pci(struct pci_dev *pci)
{
	struct snd_pci_device *pdev;
	struct list_head *pos;

	list_for_each(pos, &snd_pci_devices) {
		pdev = list_entry(pos, struct snd_pci_device, list);
		if (pdev->pci == pci)
			return &pdev->dev;
	}
	pdev = kmalloc(sizeof(struct snd_pci_device), GFP_KERNEL);
	if (pdev == NULL) {
		printk(KERN_ERR "unable to allocate snd_pci_device struct\n");
		return NULL;
	}
	pdev->dev.d.pci = pci;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 0)
	pdev->dev.dma_mask = &pci->dma_mask;
#else
	pdev->dev.dma_mask = &pdev->dummy_mask;
#endif
	return &pdev->dev;
}

static void snd_kdevice_free_all(void)
{
	struct snd_pci_device *pdev;
	struct list_head *pos, *n;

	list_for_each_safe(pos, n, &snd_pci_devices) {
		pdev = list_entry(pos, struct snd_pci_device, list);
		kfree(pdev);
	}
}

static const char *snd_pci_dev_name(struct pci_dev *pci)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 0)
	if (pci) {
		static char tmp[32];
		sprintf(tmp, "%02x:%02x:%02x",
			pci->bus->number,
			pci->bus->primary,
			pci->bus->secondary);
		return tmp;
	}
#else
	if (pci)
		return pci->slot_name;
#endif
	return "???";
}

static const char *snd_kdevice_busid(struct device *dev)
{
	return snd_pci_dev_name(dev->d.pci);
}

#define __GFP_NOWARN	0

#else /* >= 2.5.0 */

static inline struct device *snd_kdevice_pci(struct pci_dev *pci)
{
	return &pci->dev;
}

#define snd_kdevice_free_all()

#endif /* <2.5.0 */
