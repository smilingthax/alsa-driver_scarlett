#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)

#define init_MUTEX(x) *(x) = MUTEX
#define DECLARE_MUTEX(x) struct semaphore x = MUTEX
typedef struct wait_queue wait_queue_t;
typedef struct wait_queue * wait_queue_head_t;
#define init_waitqueue_head(x) *(x) = NULL
#define init_waitqueue_entry(q,p) ((q)->task = (p))
#define set_current_state(xstate) do { current->state = xstate; } while (0)

#define local_irq_save(flags) \
	do { __save_flags(flags); __cli(); } while (0)
#define local_irq_restore(flags) \
	do { __restore_flags(flags); } while (0)

#define __init
#define __exit
#define __exitdata
#define __devinit
#define __devinitdata
#define __devexit
#define __devexitdata

#ifdef MODULE
  #define module_init(x)      int init_module(void) { return x(); }
  #define module_exit(x)      void cleanup_module(void) { x(); }
#else
  #define module_init(x)
  #define module_exit(x)
#endif

#define MODULE_DEVICE_TABLE(foo,bar)

/*
 * Insert a new entry before the specified head..
 */
static __inline__ void list_add_tail(struct list_head *new, struct list_head *head)
{
	__list_add(new, head->prev, head);
}

#define IORESOURCE_IO           0x00000100      /* Resource type */
#define IORESOURCE_MEM          0x00000200

#ifdef CONFIG_PCI

/* New-style probing supporting hot-pluggable devices */

#define PCI_PM_CTRL		4	/* PM control and status register */
#define PCI_PM_CTRL_STATE_MASK	0x0003	/* Current power state (D0 to D3) */

#define PCI_ANY_ID (~0)

#define PCI_GET_DRIVER_DATA snd_pci_compat_get_driver_data
#define PCI_SET_DRIVER_DATA snd_pci_compat_set_driver_data

#define pci_enable_device snd_pci_compat_enable_device
#define pci_register_driver snd_pci_compat_register_driver
#define pci_unregister_driver snd_pci_compat_unregister_driver

#define pci_dev_g(n) list_entry(n, struct pci_dev, global_list)
#define pci_dev_b(n) list_entry(n, struct pci_dev, bus_list)

#define pci_for_each_dev(dev) \
	for(dev = pci_devices; dev; dev = dev->next)

#define pci_resource_start(dev,bar) \
	(((dev)->base_address[(bar)] & PCI_BASE_ADDRESS_SPACE) ? \
	 ((dev)->base_address[(bar)] & PCI_BASE_ADDRESS_IO_MASK) : \
	 ((dev)->base_address[(bar)] & PCI_BASE_ADDRESS_MEM_MASK))
#define pci_resource_end(dev,bar) \
	(pci_resource_start(dev,bar) + snd_pci_compat_get_size((dev),(bar)))
#define pci_resource_flags(dev,bar) (snd_pci_compat_get_flags((dev),(bar)))

struct pci_device_id {
	unsigned int vendor, device;		/* Vendor and device ID or PCI_ANY_ID */
	unsigned int subvendor, subdevice;	/* Subsystem ID's or PCI_ANY_ID */
	unsigned int class, class_mask;		/* (class,subclass,prog-if) triplet */
	unsigned long driver_data;		/* Data private to the driver */
};

struct pci_driver {
	struct list_head node;
	struct pci_dev *dev;
	char *name;
	const struct pci_device_id *id_table;	/* NULL if wants all devices */
	int (*probe)(struct pci_dev *dev, const struct pci_device_id *id); /* New device inserted */
	void (*remove)(struct pci_dev *dev);	/* Device removed (NULL if not a hot-plug capable driver) */
	void (*suspend)(struct pci_dev *dev);	/* Device suspended */
	void (*resume)(struct pci_dev *dev);	/* Device woken up */
};

/*
 *
 */
const struct pci_device_id * snd_pci_compat_match_device(const struct pci_device_id *ids, struct pci_dev *dev);
int snd_pci_compat_register_driver(struct pci_driver *drv);
void snd_pci_compat_unregister_driver(struct pci_driver *drv);
unsigned long snd_pci_compat_get_size (struct pci_dev *dev, int n_base);
int snd_pci_compat_get_flags (struct pci_dev *dev, int n_base);
int snd_pci_compat_set_power_state(struct pci_dev *dev, int new_state);
int snd_pci_compat_enable_device(struct pci_dev *dev);
int snd_pci_compat_find_capability(struct pci_dev *dev, int cap);
void * snd_pci_compat_get_driver_data (struct pci_dev *dev);
void snd_pci_compat_set_driver_data (struct pci_dev *dev, void *driver_data);

static inline int pci_module_init(struct pci_driver *drv)
{
	int res = snd_pci_compat_register_driver(drv);
	if (res < 0)
		return res;
	if (res == 0)
		return -ENODEV;
	return 0;
}

#endif /* CONFIG_PCI */

#endif /* <2.3.0 */
