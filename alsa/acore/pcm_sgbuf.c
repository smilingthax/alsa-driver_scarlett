#define __NO_VERSION__
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 0)
#include "../alsa-kernel/core/pcm_sgbuf.c"
#else

#include <sound/driver.h>
#include <linux/slab.h>
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

void *snd_pcm_sgbuf_alloc_pages(struct snd_sg_buf *sgbuf, size_t size)
{
	unsigned int i, pages;
	void *vmaddr = NULL;

	pages = snd_pcm_sgbuf_pages(size);
	sgbuf->tblsize = sgbuf_align_table(pages);
	sgbuf->table = snd_kcalloc(sizeof(*sgbuf->table) * sgbuf->tblsize, GFP_KERNEL);
	if (! sgbuf->table)
		goto _failed;
	sgbuf->page_table = snd_kcalloc(sizeof(*sgbuf->page_table) * sgbuf->tblsize, GFP_KERNEL);
	if (! sgbuf->page_table)
		goto _failed;

	sgbuf->size = size;
	vmaddr = vmalloc_nocheck(pages << PAGE_SHIFT);
	if (! vmaddr)
		goto _failed;

	sgbuf->pages = pages;
	for (i = 0; i < pages; i++) {
		void *ptr = get_vmalloc_addr(vmaddr + (i << PAGE_SHIFT));
		sgbuf->table[i].buf = ptr;
		sgbuf->table[i].addr = virt_to_bus(ptr);
		sgbuf->page_table[i] = virt_to_page(ptr);
	}

	return vmaddr;

 _failed:
	snd_pcm_sgbuf_free_pages(sgbuf, vmaddr); /* free the table */
	return NULL;
}

int snd_pcm_sgbuf_free_pages(struct snd_sg_buf *sgbuf, void *vmaddr)
{
	if (vmaddr)
		vfree_nocheck(vmaddr);

	if (sgbuf->table)
		kfree(sgbuf->table);
	sgbuf->table = NULL;
	if (sgbuf->page_table)
		kfree(sgbuf->page_table);
	sgbuf->page_table = NULL;
	sgbuf->tblsize = 0;
	sgbuf->pages = 0;
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
#endif /* < 2.4.0 */
