/*
 *  Missing Linux 2.2.x features
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,2,18)

#define init_MUTEX(x) *(x) = MUTEX
#define DECLARE_MUTEX(x) struct semaphore x = MUTEX
typedef struct wait_queue wait_queue_t;
typedef struct wait_queue * wait_queue_head_t;
#define init_waitqueue_head(x) *(x) = NULL
#define init_waitqueue_entry(q,p) ((q)->task = (p))
#define set_current_state(xstate) do { current->state = xstate; } while (0)

/*
 * Insert a new entry before the specified head..
 */
static __inline__ void list_add_tail(struct list_head *new, struct list_head *head)
{
	__list_add(new, head->prev, head);
}

#endif /* <2.2.18 */

#define virt_to_page(x) (&mem_map[MAP_NR(x)])

#define local_irq_save(flags) \
	do { __save_flags(flags); __cli(); } while (0)
#define local_irq_restore(flags) \
	do { __restore_flags(flags); } while (0)

#define tasklet_hi_schedule(t)	queue_task((t), &tq_immediate); \
				mark_bh(IMMEDIATE_BH)

#define tasklet_init(t,f,d)	(t)->next = NULL; \
				(t)->sync = 0; \
				(t)->routine = (void (*)(void *))(f); \
				(t)->data = (void *)(d)

#define tasklet_struct		tq_struct 

#define tasklet_unlock_wait(t)	while (test_bit(0, &(t)->sync)) { }

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


/**
 * list_for_each        -       iterate over a list
 * @pos:        the &struct list_head to use as a loop counter.
 * @head:       the head for your list.
 */
#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

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

/*
 * Power management requests
 */
enum
{
	PM_SUSPEND, /* enter D1-D3 */
	PM_RESUME,  /* enter D0 */

	/* enable wake-on */
	PM_SET_WAKEUP,

	/* bus resource management */
	PM_GET_RESOURCES,
	PM_SET_RESOURCES,

	/* base station management */
	PM_EJECT,
	PM_LOCK,
};

typedef int pm_request_t;

/*
 * Device types
 */
enum
{
	PM_UNKNOWN_DEV = 0, /* generic */
	PM_SYS_DEV,	    /* system device (fan, KB controller, ...) */
	PM_PCI_DEV,	    /* PCI device */
	PM_USB_DEV,	    /* USB device */
	PM_SCSI_DEV,	    /* SCSI device */
	PM_ISA_DEV,	    /* ISA device */
};

typedef int pm_dev_t;

/*
 * System device hardware ID (PnP) values
 */
enum
{
	PM_SYS_UNKNOWN = 0x00000000, /* generic */
	PM_SYS_KBC =	 0x41d00303, /* keyboard controller */
	PM_SYS_COM =	 0x41d00500, /* serial port */
	PM_SYS_IRDA =	 0x41d00510, /* IRDA controller */
	PM_SYS_FDC =	 0x41d00700, /* floppy controller */
	PM_SYS_VGA =	 0x41d00900, /* VGA controller */
	PM_SYS_PCMCIA =	 0x41d00e00, /* PCMCIA controller */
};

/*
 * Device identifier
 */
#define PM_PCI_ID(dev) ((dev)->bus->number << 16 | (dev)->devfn)

/*
 * Request handler callback
 */
struct pm_dev;

typedef int (*pm_callback)(struct pm_dev *dev, pm_request_t rqst, void *data);

/*
 * Dynamic device information
 */
struct pm_dev
{
	pm_dev_t	 type;
	unsigned long	 id;
	pm_callback	 callback;
	void		*data;

	unsigned long	 flags;
	int		 state;
	int		 prev_state;

	struct list_head entry;
};

#ifdef CONFIG_APM

int pm_init(void);
void pm_done(void);

#define CONFIG_PM

#define PM_IS_ACTIVE() 1

/*
 * Register a device with power management
 */
struct pm_dev *pm_register(pm_dev_t type,
			   unsigned long id,
			   pm_callback callback);

/*
 * Unregister a device with power management
 */
void pm_unregister(struct pm_dev *dev);

/*
 * Send a request to a single device
 */
int pm_send(struct pm_dev *dev, pm_request_t rqst, void *data);

#else /* CONFIG_APM */

#define PM_IS_ACTIVE() 0

extern inline struct pm_dev *pm_register(pm_dev_t type,
					 unsigned long id,
					 pm_callback callback)
{
	return 0;
}

extern inline void pm_unregister(struct pm_dev *dev) {}

extern inline int pm_send(struct pm_dev *dev, pm_request_t rqst, void *data)
{
	return 0;
}

#endif /* CONFIG_APM */

#endif /* <2.3.0 */
