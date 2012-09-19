#include <linux/config.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#if defined(CONFIG_MODVERSIONS) && !defined(__GENKSYMS__) && !defined(__DEPEND__)
#define MODVERSIONS
#include <linux/modversions.h>
#include "sndversions.h"
#endif
#endif

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

static LIST_HEAD(snd_pci_devices);

struct snd_pci_device {
	struct list_head *list;
	struct device dev;
	struct pci_dev *pci;
};

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
	pdev->dev.dma_mask = &pci->dma_mask;
	return &pdev->dev;
}
EXPORT_SYMBOL(snd_kdevice_pci);

static void snd_kdevice_free_all(void)
{
	struct snd_pci_device *pdev;
	struct list_head *pos, *n;

	list_for_each_safe(pos, n, &snd_pci_devices) {
		pdev = list_entry(pos, struct snd_pci_device, list);
		kfree(pdev);
	}
}

static const char *snd_kdevice_busid(struct device *dev)
{
	if (dev->d.pci)
		return dev->d.pci->slot_name;
	return "???";
}
