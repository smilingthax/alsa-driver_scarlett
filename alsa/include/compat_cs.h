#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 0) && (defined(CONFIG_PCMCIA) || defined(CONFIG_PCMCIA_MODULE))

#include <pcmcia/cs_types.h>
#include <pcmcia/cs.h>
#include <pcmcia/cistpl.h>
#include <pcmcia/ds.h>

struct cs_device_driver {
	const char *name;
};

struct pcmcia_driver {
	int                     use_count;
	dev_link_t              *(*attach)(void);
	void                    (*detach)(dev_link_t *);
	struct module           *owner;
	struct cs_device_driver	drv;
};

/* driver registration */
int snd_compat_pcmcia_register_driver(struct pcmcia_driver *driver);
#define pcmcia_register_driver(driver) snd_compat_pcmcia_register_driver(driver)
void snd_compat_pcmcia_unregister_driver(struct pcmcia_driver *driver);
#define pcmcia_unregister_driver(driver) snd_compat_pcmcia_unregister_driver(driver)

#endif /* 2.5.0+ */
