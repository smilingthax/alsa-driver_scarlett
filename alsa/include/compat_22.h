/*
 *  Missing Linux 2.2.x features
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)

#include <linux/list.h>
#include <linux/pagemap.h>
#include <linux/ioport.h>

#ifndef CONFIG_HAVE_DMA_ADDR_T
typedef unsigned long dma_addr_t;
#endif

/*
 */

#ifndef CONFIG_HAVE_MUTEX_MACROS
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

#endif /* !CONFIG_HAVE_MUTEX_MACROS */

/**
 * list_del_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
static __inline__ void list_del_init(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
	INIT_LIST_HEAD(entry); 
}

/**
 * list_for_each_safe	-	iterate over a list safe against removal of list entry
 * @pos:	the &struct list_head to use as a loop counter.
 * @n:		another &struct list_head to use as temporary storage
 * @head:	the head for your list.
 */
#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

/* rw_semaphore - replaced with mutex */
#define rw_semaphore semaphore
#define init_rwsem(x) init_MUTEX(x)
#define down_read(x) down(x)
#define down_write(x) down(x)
#define up_read(x) up(x)
#define up_write(x) up(x)

#define virt_to_page(x) (&mem_map[MAP_NR(x)])
#define get_page(p) atomic_inc(&(p)->count)
#define SetPageReserved(p) set_bit(PG_reserved, &(p)->flags)
#define ClearPageReserved(p) clear_bit(PG_reserved, &(p)->flags)
#define vm_private_data vm_pte
#define NOPAGE_OOM 0
#define NOPAGE_SIGBUS (-1)
#define fops_get(x) (x)
#define fops_put(x) do { ; } while (0)

#define local_irq_save(flags) \
	do { __save_flags(flags); __cli(); } while (0)
#define local_irq_restore(flags) \
	do { __restore_flags(flags); } while (0)

/* Some distributions use modified kill_fasync */
#include <linux/fs.h>
#undef kill_fasync
#define kill_fasync(fp, sig, band) snd_wrapper_kill_fasync(fp, sig, band)
void snd_wrapper_kill_fasync(struct fasync_struct **, int, int);

#define tasklet_hi_schedule(t)	queue_task((t), &tq_immediate); \
				mark_bh(IMMEDIATE_BH)

#define tasklet_init(t,f,d)	(t)->next = NULL; \
				(t)->sync = 0; \
				(t)->routine = (void (*)(void *))(f); \
				(t)->data = (void *)(d)

#define tasklet_struct		tq_struct 

#define tasklet_unlock_wait(t)	while (test_bit(0, &(t)->sync)) { }

#define rwlock_init(x) do { *(x) = RW_LOCK_UNLOCKED; } while(0)

#define __init
#define __initdata
#define __exit
#define __exitdata
#define __devinit
#define __devinitdata
#define __devexit
#define __devexitdata

#ifdef MODULE
  #ifndef module_init
  #define module_init(x)      int init_module(void) { return x(); }
  #define module_exit(x)      void cleanup_module(void) { x(); }
  #endif
  #ifndef THIS_MODULE
  #define THIS_MODULE	      (&__this_module)
  #endif
  int try_inc_mod_count(struct module *mod);
#else
  #define module_init(x)
  #define module_exit(x)
  #define THIS_MODULE NULL
  #define try_inc_mod_count(x) do { ; } while (0)
#endif

#define MODULE_GENERIC_TABLE(gtype,name)        \
static const unsigned long __module_##gtype##_size \
  __attribute__ ((unused)) = sizeof(struct gtype##_id); \
static const struct gtype##_id * __module_##gtype##_table \
  __attribute__ ((unused)) = name
#define MODULE_DEVICE_TABLE(type,name)          \
  MODULE_GENERIC_TABLE(type##_device,name)

/**
 * list_for_each        -       iterate over a list
 * @pos:        the &struct list_head to use as a loop counter.
 * @head:       the head for your list.
 */
#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

#ifndef IORESOURCE_IO
struct resource {
	const char *name;
	unsigned long start, end;
	unsigned long flags;
	struct resource *parent, *sibling, *child;
};

#define IORESOURCE_IO           0x00000100      /* Resource type */
#define IORESOURCE_MEM		0x00000200
#endif

void snd_wrapper_request_region(unsigned long from, unsigned long extent, const char *name);
#undef request_region
#define request_region snd_compat_request_region
#define release_resource snd_compat_release_resource
#define request_mem_region(from, size, name) (&snd_compat_mem_region)

extern struct resource snd_compat_mem_region;

struct resource *snd_compat_request_region(unsigned long start, unsigned long size, const char *name);
int snd_compat_release_resource(struct resource *resource);

#ifdef CONFIG_PCI

/* New-style probing supporting hot-pluggable devices */

#define PCI_PM_CTRL		4	/* PM control and status register */
#define PCI_PM_CTRL_STATE_MASK	0x0003	/* Current power state (D0 to D3) */

#define PCI_ANY_ID (~0)

#define pci_get_drvdata snd_pci_compat_get_driver_data
#define pci_set_drvdata snd_pci_compat_set_driver_data

#define pci_set_dma_mask snd_pci_compat_set_dma_mask

#undef pci_enable_device
#define pci_enable_device snd_pci_compat_enable_device
#define pci_register_driver snd_pci_compat_register_driver
#define pci_unregister_driver snd_pci_compat_unregister_driver
#define pci_set_power_state snd_pci_compat_set_power_state

#define pci_alloc_consistent snd_pci_compat_alloc_consistent
#define pci_free_consistent snd_pci_compat_free_consistent
#define pci_dma_supported snd_pci_compat_dma_supported

#define pci_dev_g(n) list_entry(n, struct pci_dev, global_list)
#define pci_dev_b(n) list_entry(n, struct pci_dev, bus_list)

#define pci_for_each_dev(dev) \
	for(dev = pci_devices; dev; dev = dev->next)

#undef pci_resource_start
#define pci_resource_start(dev,bar) \
	(((dev)->base_address[(bar)] & PCI_BASE_ADDRESS_SPACE) ? \
	 ((dev)->base_address[(bar)] & PCI_BASE_ADDRESS_IO_MASK) : \
	 ((dev)->base_address[(bar)] & PCI_BASE_ADDRESS_MEM_MASK))
#undef pci_resource_end
#define pci_resource_end(dev,bar) \
	(pci_resource_start(dev,bar) + snd_pci_compat_get_size((dev),(bar)))
#undef pci_resource_len
#define pci_resource_len(dev,bar) \
	((pci_resource_start((dev),(bar)) == 0 &&	\
	  pci_resource_end((dev),(bar)) ==		\
	  pci_resource_start((dev),(bar))) ? 0 :	\
							\
	(pci_resource_end((dev),(bar)) -		\
	 pci_resource_start((dev),(bar)) + 1))
#undef pci_resource_flags
#define pci_resource_flags(dev,bar) (snd_pci_compat_get_flags((dev),(bar)))

struct pci_device_id {
	unsigned int vendor, device;		/* Vendor and device ID or PCI_ANY_ID */
	unsigned int subvendor, subdevice;	/* Subsystem ID's or PCI_ANY_ID */
	unsigned int class, class_mask;		/* (class,subclass,prog-if) triplet */
	unsigned long driver_data;		/* Data private to the driver */
};

#ifndef PCI_OLD_SUSPEND
#define PCI_OLD_SUSPEND
#endif

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
void *snd_pci_compat_alloc_consistent(struct pci_dev *, long, dma_addr_t *);
void snd_pci_compat_free_consistent(struct pci_dev *, long, void *, dma_addr_t);
int snd_pci_compat_dma_supported(struct pci_dev *, dma_addr_t mask);
unsigned long snd_pci_compat_get_dma_mask(struct pci_dev *);
void snd_pci_compat_set_dma_mask(struct pci_dev *, unsigned long mask);
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

#define __LINUX_PM_H	/* <linux/pm.h> in 2.2.18 is a bit stripped */

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
