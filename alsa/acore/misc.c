#define __NO_VERSION__
#include "../alsa-kernel/core/misc.c"
#include <linux/smp_lock.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>


#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)

#include <linux/slab.h>
#include <asm/io.h>

int try_inc_mod_count(struct module *module)
{
	__MOD_INC_USE_COUNT(module);
	return 1;
}

struct resource *snd_compat_request_region(unsigned long start, unsigned long size, const char *name, int is_memory)
{
	struct resource *resource;

#ifdef CONFIG_SND_DEBUG_MEMORY
	/* DON'T use kmalloc here; the allocated resource is released
	 * by kfree without wrapper in each driver
	 */
	resource = snd_wrapper_kmalloc(sizeof(struct resource), GFP_KERNEL);
#else
	resource = kmalloc(sizeof(struct resource), GFP_KERNEL);
#endif
	if (resource == NULL)
		return NULL;
	if (! is_memory) {
		if (check_region(start, size)) {
			kfree_nocheck(resource);
			return NULL;
		}
		snd_wrapper_request_region(start, size, name);
	}
	memset(resource, 0, sizeof(struct resource));
	resource->name = name;
	resource->start = start;
	resource->end = start + size - 1;
	resource->flags = is_memory ? IORESOURCE_MEM : IORESOURCE_IO;
	return resource;
}

int snd_compat_release_resource(struct resource *resource)
{
	snd_runtime_check(resource != NULL, return -EINVAL);
	if (resource->flags & IORESOURCE_MEM)
		return 0;
	release_region(resource->start, (resource->end - resource->start) + 1);
	return 0;
}

#endif


#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0) && defined(CONFIG_APM)

#include <linux/apm_bios.h>

static spinlock_t pm_devs_lock = SPIN_LOCK_UNLOCKED;
static LIST_HEAD(pm_devs);

#ifdef CONFIG_PCI
static struct pm_dev *pci_compat_pm_dev;
static int pci_compat_pm_callback(struct pm_dev *pdev, pm_request_t rqst, void *data)
{
	struct pci_dev *dev;
	switch (rqst) {
	case PM_SUSPEND:
		pci_for_each_dev(dev) {
			struct pci_driver *drv = snd_pci_compat_get_pci_driver(dev);
			if (drv && drv->suspend)
				drv->suspend(dev, 0);
		}
		break;
	case PM_RESUME:
		pci_for_each_dev(dev) {
			struct pci_driver *drv = snd_pci_compat_get_pci_driver(dev);
			if (drv && drv->resume)
				drv->resume(dev);
		}
		break;
	}	
	return 0;
}
#endif

static int snd_apm_callback(apm_event_t ev)
{
	struct list_head *entry;
	pm_request_t rqst;
	void *data;
	int status;
	
	switch (ev) {
	case APM_SYS_SUSPEND:
	case APM_USER_SUSPEND:
	case APM_CRITICAL_SUSPEND:
		rqst = PM_SUSPEND;
		data = (void *)3;
		break;
	case APM_NORMAL_RESUME:
	case APM_CRITICAL_RESUME:
	case APM_STANDBY_RESUME:		/* ??? */
		rqst = PM_RESUME;
		data = (void *)0;
		break;
	default:
		return 0;
	}
	for (entry = pm_devs.next; entry != &pm_devs; entry = entry->next) {
		struct pm_dev *dev = list_entry(entry, struct pm_dev, entry);
		if ((status = pm_send(dev, rqst, data)))
			return status;
	}
	return 0;
}

int __init pm_init(void)
{
	if (apm_register_callback(snd_apm_callback))
		snd_printk(KERN_ERR "apm_register_callback failure!\n");
#ifdef CONFIG_PCI
	pci_compat_pm_dev = pm_register(PM_PCI_DEV, 0, pci_compat_pm_callback);
#endif
	return 0;
}

void __exit pm_done(void)
{
#ifdef CONFIG_PCI
	if (pci_compat_pm_dev)
		pm_unregister(pci_compat_pm_dev);
#endif
	apm_unregister_callback(snd_apm_callback);
}

struct pm_dev *pm_register(pm_dev_t type,
			   unsigned long id,
			   pm_callback callback)
{
	struct pm_dev *dev = kmalloc(sizeof(struct pm_dev), GFP_KERNEL);

	if (dev) {
		unsigned long flags;
		
		memset(dev, 0, sizeof(*dev));
		dev->type = type;
		dev->id = id;
		dev->callback = callback;
		
		spin_lock_irqsave(&pm_devs_lock, flags);
		list_add(&dev->entry, &pm_devs);
		spin_unlock_irqrestore(&pm_devs_lock, flags);
	}
	return dev;
}

void pm_unregister(struct pm_dev *dev)
{
	if (dev) {
		unsigned long flags;
		
		spin_lock_irqsave(&pm_devs_lock, flags);
		list_del(&dev->entry);
		spin_unlock_irqrestore(&pm_devs_lock, flags);

		kfree(dev);
	}
}

int pm_send(struct pm_dev *dev, pm_request_t rqst, void *data)
{
	int status = 0;
	int prev_state, next_state;
	
	switch (rqst) {
	case PM_SUSPEND:
	case PM_RESUME:
		prev_state = dev->state;
		next_state = (int) data;
		if (prev_state != next_state) {
			if (dev->callback)
				status = (*dev->callback)(dev, rqst, data);
			if (!status) {
				dev->state = next_state;
				dev->prev_state = prev_state;
			}
		} else {
			dev->prev_state = prev_state;
		}
		break;
	default:
		if (dev->callback)
			status = (*dev->callback)(dev, rqst, data);
		break;
	}
	return status;
}

#endif /* kernel version < 2.3.0 && CONFIG_APM */

/* workqueue-alike; 2.5.45 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 45)

static int work_caller(void *data)
{
	struct work_struct *works = data;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 0)
	lock_kernel();
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 2, 18)
	daemonize();
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 0) && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 8)
	reparent_to_init();
#endif
	strcpy(current->comm, "snd"); /* FIXME: different names? */

	works->func(works->data);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 0)
	unlock_kernel();
#endif

	return 0;
}

int snd_compat_schedule_work(struct work_struct *works)
{
	return kernel_thread(work_caller, works, 0) >= 0;
}

struct workqueue_struct {
	spinlock_t lock;
	const char *name;
	struct list_head worklist;
	int task_pid;
	task_t *task;
	wait_queue_head_t more_work;
	wait_queue_head_t work_done;
	struct completion thread_exited;
};

static void run_workqueue(struct workqueue_struct *wq)
{
	unsigned long flags;

	spin_lock_irqsave(&wq->lock, flags);
	while (!list_empty(&wq->worklist)) {
		struct work_struct *work = list_entry(wq->worklist.next,
						      struct work_struct, entry);
		void (*f) (void *) = work->func;
		void *data = work->data;

		list_del_init(wq->worklist.next);
		spin_unlock_irqrestore(&wq->lock, flags);
		clear_bit(0, &work->pending);
		f(data);
		spin_lock_irqsave(&wq->lock, flags);
		wake_up(&wq->work_done);
	}
	spin_unlock_irqrestore(&wq->lock, flags);
}

void snd_compat_flush_workqueue(struct workqueue_struct *wq)
{
	if (wq->task == current) {
		run_workqueue(wq);
	} else {
		wait_queue_t wait;

		init_waitqueue_entry(&wait, current);
		set_current_state(TASK_UNINTERRUPTIBLE);
		spin_lock_irq(&wq->lock);
		add_wait_queue(&wq->work_done, &wait);
		while (!list_empty(&wq->worklist)) {
			spin_unlock_irq(&wq->lock);
			schedule();
			spin_lock_irq(&wq->lock);
		}
		set_current_state(TASK_RUNNING);
		remove_wait_queue(&wq->work_done, &wait);
		spin_unlock_irq(&wq->lock);
	}
}

void snd_compat_destroy_workqueue(struct workqueue_struct *wq)
{
	snd_compat_flush_workqueue(wq);
	kill_proc(wq->task_pid, SIGKILL, 1);
	if (wq->task_pid >= 0)
		wait_for_completion(&wq->thread_exited);
	kfree(wq);
}

static int xworker_thread(void *data)
{
	struct workqueue_struct *wq = data;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 0)
	lock_kernel();
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 2, 18)
	daemonize();
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 0) && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 8)
	reparent_to_init();
#endif
	strcpy(current->comm, wq->name);

	do {
		run_workqueue(wq);
		wait_event_interruptible(wq->more_work, !list_empty(&wq->worklist));
	} while (!signal_pending(current));

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 0)
	unlock_kernel();
#endif
	complete_and_exit(&wq->thread_exited, 0);
}

struct workqueue_struct *snd_compat_create_workqueue(const char *name)
{
	struct workqueue_struct *wq;
	
	BUG_ON(strlen(name) > 10);
	
	wq = kmalloc(sizeof(*wq), GFP_KERNEL);
	if (!wq)
		return NULL;
	memset(wq, 0, sizeof(*wq));
	
	spin_lock_init(&wq->lock);
	INIT_LIST_HEAD(&wq->worklist);
	init_waitqueue_head(&wq->more_work);
	init_waitqueue_head(&wq->work_done);
	init_completion(&wq->thread_exited);
	wq->name = name;
	wq->task_pid = kernel_thread(xworker_thread, wq, 0);
	if (wq->task_pid < 0) {
		printk(KERN_ERR "snd: failed to start thread %s\n", name);
		snd_compat_destroy_workqueue(wq);
		wq = NULL;
	}
	wq->task = find_task_by_pid(wq->task_pid);
	return wq;
}

static void __x_queue_work(struct workqueue_struct *wq, struct work_struct *work)
{
	unsigned long flags;

	spin_lock_irqsave(&wq->lock, flags);
	work->wq_data = wq;
	list_add_tail(&work->entry, &wq->worklist);
	wake_up(&wq->more_work);
	spin_unlock_irqrestore(&wq->lock, flags);
}

int snd_compat_queue_work(struct workqueue_struct *wq, struct work_struct *work)
{
	if (!test_and_set_bit(0, &work->pending)) {
		__x_queue_work(wq, work);
		return 1;
	}
	return 0;
}

static void delayed_work_timer_fn(unsigned long __data)
{
	struct work_struct *work = (struct work_struct *)__data;
	struct workqueue_struct *wq = work->wq_data;
	
	__x_queue_work(wq, work);
}

int snd_compat_queue_delayed_work(struct workqueue_struct *wq, struct work_struct *work, unsigned long delay)
{
	struct timer_list *timer = &work->timer;

	if (!test_and_set_bit(0, &work->pending)) {
		work->wq_data = wq;
		timer->expires = jiffies + delay;
		timer->data = (unsigned long)work;
		timer->function = delayed_work_timer_fn;
		add_timer(timer);
		return 1;
	}
	return 0;
}

#endif

#ifndef CONFIG_HAVE_KCALLOC
#ifndef CONFIG_SND_DEBUG_MEMORY
/* Don't put this to wrappers.c.  We need to call the kmalloc wrapper here. */
void *snd_compat_kcalloc(size_t n, size_t size, int flags)
{
	void *ret = NULL;

	if (n != 0 && size > INT_MAX / n)
		return ret;

	ret = kmalloc(n * size, flags);
	if (ret)
		memset(ret, 0, n * size);
	return ret;
}
#endif
#endif
