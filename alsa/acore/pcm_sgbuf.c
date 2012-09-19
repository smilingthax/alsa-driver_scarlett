#define __NO_VERSION__
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0)
#include "../alsa-kernel/core/pcm_sgbuf.c"
#else

/*
 * we don't have vmap/vunmap, so use vmalloc_32 and vmalloc_dma instead
 */

#include <sound/driver.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_sgbuf.h>
#include <asm/io.h>

/* table entries are align to 32 */
#define SGBUF_TBL_ALIGN		32
#define sgbuf_align_table(tbl)	((((tbl) + SGBUF_TBL_ALIGN - 1) / SGBUF_TBL_ALIGN) * SGBUF_TBL_ALIGN)

struct snd_sg_buf *snd_pcm_sgbuf_new(struct pci_dev *pci)
{
	struct snd_sg_buf *sgbuf;

	sgbuf = snd_magic_kcalloc(snd_pcm_sgbuf_t, 0, GFP_KERNEL);
	if (! sgbuf)
		return NULL;
	sgbuf->pci = pci;
	sgbuf->pages = 0;
	sgbuf->tblsize = 0;

	return sgbuf;
}

void snd_pcm_sgbuf_delete(struct snd_sg_buf *sgbuf)
{
	snd_pcm_sgbuf_free_pages(sgbuf, NULL);
	snd_magic_kfree(sgbuf);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 0)
/* get the virtual address of the given vmalloc'ed pointer */
static void *get_vmalloc_addr(void *pageptr)
{
	pgd_t *pgd;
	pmd_t *pmd;
	pte_t *pte;
	unsigned long lpage;

	lpage = VMALLOC_VMADDR(pageptr);
	pgd = pgd_offset_k(lpage);
	pmd = pmd_offset(pgd, lpage);
	pte = pte_offset(pmd, lpage);
	return (void *)pte_page(*pte);
}    
#endif

/* set up the page table from the given vmalloc'ed buffer pointer.
 * return a negative error if the page is out of the pci address mask.
 */
static int store_page_tables(struct snd_sg_buf *sgbuf, void *vmaddr, unsigned int pages)
{
	unsigned int i;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 0)
	unsigned long rmask;
	if (sgbuf->pci)
		rmask = ~((unsigned long)sgbuf->pci->dma_mask);
	else
		rmask = ~0xffffffUL;
#endif

	for (i = 0; i < pages; i++) {
		struct page *page;
		void *ptr;
		dma_addr_t addr;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 0)
		page = vmalloc_to_page(vmaddr + (i << PAGE_SHIFT));
		ptr = page_address(page);
		addr = virt_to_bus(ptr);
		if (addr & rmask)
			return -EINVAL;
#else
		ptr = get_vmalloc_addr(vmaddr + (i << PAGE_SHIFT));
		addr = virt_to_bus(ptr);
		page = virt_to_page(ptr);
#endif
		sgbuf->table[i].buf = ptr;
		sgbuf->table[i].addr = addr;
		sgbuf->page_table[i] = page;
		SetPageReserved(page);
		sgbuf->pages++;
	}
	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 2, 18)
#define vmalloc_32(x) vmalloc_nocheck(x) /* don't use wrapper */
#endif

/* remove all vmalloced pages */
static void release_vm_buffer(struct snd_sg_buf *sgbuf, void *vmaddr)
{
	int i;

	for (i = 0; i < sgbuf->pages; i++)
		if (sgbuf->page_table[i]) {
			ClearPageReserved(sgbuf->page_table[i]);
			sgbuf->page_table[i] = NULL;
		}
	sgbuf->pages = 0;
	vfree_nocheck(vmaddr); /* don't use wrapper */
}

void *snd_pcm_sgbuf_alloc_pages(struct snd_sg_buf *sgbuf, size_t size)
{
	unsigned int pages;
	void *vmaddr = NULL;

	pages = snd_pcm_sgbuf_pages(size);
	sgbuf->pages = 0;
	sgbuf->tblsize = sgbuf_align_table(pages);
	sgbuf->table = snd_kcalloc(sizeof(*sgbuf->table) * sgbuf->tblsize, GFP_KERNEL);
	if (! sgbuf->table)
		goto _failed;
	sgbuf->page_table = snd_kcalloc(sizeof(*sgbuf->page_table) * sgbuf->tblsize, GFP_KERNEL);
	if (! sgbuf->page_table)
		goto _failed;

	sgbuf->size = size;
	vmaddr = vmalloc_32(pages << PAGE_SHIFT);
	if (! vmaddr)
		goto _failed;

	if (store_page_tables(sgbuf, vmaddr, pages) < 0) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 0)
		/* reallocate with DMA flag */
		release_vm_buffer(sgbuf, vmaddr);
		vmaddr = vmalloc_dma(pages << PAGE_SHIFT);
		if (! vmaddr)
			goto _failed;
		store_page_tables(sgbuf, vmaddr, pages);
#else
		goto _failed;
#endif
	}

	return vmaddr;

 _failed:
	snd_pcm_sgbuf_free_pages(sgbuf, vmaddr); /* free the table */
	return NULL;
}

int snd_pcm_sgbuf_free_pages(struct snd_sg_buf *sgbuf, void *vmaddr)
{
	if (vmaddr)
		release_vm_buffer(sgbuf, vmaddr);
	if (sgbuf->table)
		kfree(sgbuf->table);
	sgbuf->table = NULL;
	if (sgbuf->page_table)
		kfree(sgbuf->page_table);
	sgbuf->page_table = NULL;
	sgbuf->tblsize = 0;
	sgbuf->size = 0;
	
	return 0;
}

struct page *snd_pcm_sgbuf_ops_page(snd_pcm_substream_t *substream, unsigned long offset)
{
	struct snd_sg_buf *sgbuf = snd_magic_cast(snd_pcm_sgbuf_t, substream->dma_private, return NULL);

	unsigned int idx = offset >> PAGE_SHIFT;
	if (idx >= sgbuf->pages)
		return NULL;
	return sgbuf->page_table[idx];
}


int snd_pcm_lib_preallocate_sg_pages(struct pci_dev *pci,
				     snd_pcm_substream_t *substream)
{
	if ((substream->dma_private = snd_pcm_sgbuf_new(pci)) == NULL)
		return -ENOMEM;
	substream->dma_type = SNDRV_PCM_DMA_TYPE_PCI_SG;
	substream->dma_area = 0;
	substream->dma_addr = 0;
	substream->dma_bytes = 0;
	substream->buffer_bytes_max = UINT_MAX;
	substream->dma_max = 0;
	return 0;
}

int snd_pcm_lib_preallocate_sg_pages_for_all(struct pci_dev *pci,
					     snd_pcm_t *pcm)
{
	snd_pcm_substream_t *substream;
	int stream, err;

	for (stream = 0; stream < 2; stream++)
		for (substream = pcm->streams[stream].substream; substream; substream = substream->next)
			if ((err = snd_pcm_lib_preallocate_sg_pages(pci, substream)) < 0)
				return err;
	return 0;
}

EXPORT_SYMBOL(snd_pcm_lib_preallocate_sg_pages);
EXPORT_SYMBOL(snd_pcm_lib_preallocate_sg_pages_for_all);
EXPORT_SYMBOL(snd_pcm_sgbuf_ops_page);
#endif /* < 2.5.0 */
