#define __NO_VERSION__
/*
 *   US-428 AUDIO

 *   Copyright (c) 2002-2003 by Karsten Wiese
 
 *   based on

 *   (Tentative) USB Audio Driver for ALSA
 *
 *   Main and PCM part
 *
 *   Copyright (c) 2002 by Takashi Iwai <tiwai@suse.de>
 *
 *   Many codes borrowed from audio.c by 
 *	    Alan Cox (alan@lxorguk.ukuu.org.uk)
 *	    Thomas Sailer (sailer@ife.ee.ethz.ch)
 *
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */


#define SND_NEED_USB_WRAPPER
#include <sound/driver.h>
#include <linux/usb.h>

/* wrappers */
#ifdef OLD_USB
#define snd_usb_complete_callback(x) __old_ ## x
static void __old_snd_usX2Y_urb_play_complete(struct urb *urb);
static void __old_snd_usX2Y_urb_capt_complete(struct urb *urb);
#endif

#include <sound/core.h>
#include <sound/info.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include "usx2y.h"
#include "usbusx2y.h"



typedef struct snd_usX2Y_substream snd_usX2Y_substream_t;
typedef struct snd_usX2Y_stream snd_usX2Y_stream_t;


struct snd_usX2Y_substream {
	snd_usX2Y_stream_t *stream;
	snd_pcm_substream_t *pcm_substream;

	unsigned char	endpoint[2];		/* endpoint: 2 capture ports possible */
	int             endpoints;
	int		start_frame;
	int		retired_frame[2];
	unsigned int	datapipe[2];  		/* the data i/o pipe */
	unsigned int	freqn;     		/* nominal sampling rate in USB format, i.e. fs/1000 in Q10.14 */
	unsigned int	freqm;    		/* momentary sampling rate in USB format, i.e. fs/1000 in Q10.14 */
	unsigned int	remainder;
	unsigned int	maxpacksize;		/* max packet size in bytes */

	unsigned int	running: 1,
		    	bussing: 1;		/* running status */

	int		hwptr;			/* free frame position in the buffer (only for playback) */
	int		hwptr_done;		/* processed frame position in the buffer */
	int		transfer_done;		/* processed frames since last period update */

	struct urb*	dataurb[2][NRURBS];	/* data urb table */
	char*		tmpbuf;			/* temporary buffer for playback */

	spinlock_t	lock;
};


struct snd_usX2Y_stream {
	usX2Ydev_t*		usX2Y;
	snd_pcm_t*		pcm;
	snd_usX2Y_substream_t	substream[2];
};

#define chip_t snd_usX2Y_stream_t

static int snd_usX2Y_set_format(snd_usX2Y_substream_t *subs, snd_pcm_runtime_t *runtime);
static int snd_usX2Y_substream_prepare(snd_usX2Y_substream_t *subs, snd_pcm_runtime_t *runtime);
static void snd_usX2Y_urbs_release(snd_usX2Y_substream_t *subs);


/*
 * convert a sampling rate into USB format (fs/1000 in Q10.14)
 * this will overflow at approx 2MSPS
 */
inline static unsigned get_usb_rate(unsigned int rate)
{
	return ((rate << 11) + 62) / 125;
}
/*
 * prepare urb for capture data pipe
 *
 * fill the offset and length of each descriptor.
 *
 * we use a temporary buffer to write the captured data.
 * since the length of written data is determined by host, we cannot
 * write onto the pcm buffer directly...  the data is thus copied
 * later at complete callback to the global buffer.
 */
static int snd_usX2Y_urb_capt_prepare(snd_usX2Y_substream_t *subs,
			       snd_pcm_runtime_t *runtime,
			       struct urb *urb)
{
	unsigned long flags, pack;

	urb->dev = subs->stream->usX2Y->chip.dev; /* we need to set this at each time */
	spin_lock_irqsave(&subs->lock, flags);
	for (pack = 0; pack < NRPACKS; pack++) {
		urb->iso_frame_desc[pack].offset = subs->maxpacksize * pack;
		urb->iso_frame_desc[pack].length = subs->maxpacksize;
	}
	spin_unlock_irqrestore(&subs->lock, flags);
	urb->transfer_buffer_length = subs->maxpacksize * NRPACKS; 
	urb->interval = NRPACKS;

	return 0;
}
/*
 * process after capture complete
 *
 * copy the data from each desctiptor to the pcm buffer, and
 * update the current position.
 */
static void framecpy_4c_16(unsigned char *p_dma_area, unsigned char *cp, int cnt)
{
	do{
		*(int *)p_dma_area = *(int *)cp;
		p_dma_area += 8;
		cp += 4;
	} while (--cnt);
}

static void framecpy_4c_24(unsigned char *p_dma_area, unsigned char *cp, int cnt)
{
	do{
		memcpy(p_dma_area, cp, 6);
		p_dma_area +=  12;
		cp += 6;
	} while (--cnt);
}

static int snd_usX2Y_urb_capt_retire(snd_usX2Y_substream_t *subs,
			      snd_pcm_runtime_t *runtime,
			      struct urb *urb)
{
	unsigned long 	flags;
	unsigned char	*cp;
	int 		i, len, lens = 0, hwptr_done = subs->hwptr_done;
	usX2Ydev_t	*usX2Y = subs->stream->usX2Y;
	void (*fourc_cpy)(unsigned char* p_dma_area, unsigned char* cp, int cnt) = 0;

	if (runtime->channels == 4)
		fourc_cpy = usX2Y->stride == 4 ? framecpy_4c_16 : framecpy_4c_24;

	for (i = 0; i < NRPACKS; i++) {
		cp = (unsigned char*)urb->transfer_buffer + urb->iso_frame_desc[i].offset;
		if (urb->iso_frame_desc[i].status) { /* active? hmm, skip this */
			snd_printd("activ frame status %i\n", urb->iso_frame_desc[i].status);
			return urb->iso_frame_desc[i].status;
		}
		len = urb->iso_frame_desc[i].actual_length / usX2Y->stride;
		{
			unsigned long x = subs->freqm;
			x *= (1 << 11) - 1;
			x += (len << 14) + subs->remainder;
			subs->remainder = x % (1 << 11);
			x /= (1 << 11);
			subs->freqm = x ;
		}
		if (! len) {
			snd_printk("0 == len ERROR!\n");
			continue;
		}
		/* update the current pointer */
		if (urb->pipe == subs->datapipe[0]) {
			int j = subs->start_frame == urb->start_frame ? 1 : 0;//FIXME : j should propably never become 1
			usX2Y->pipe0Aframes[j][i] = len;
			if (j) {
				usX2Y->play_urb_waiting[1] = usX2Y->play_urb_waiting[0];
				usX2Y->play_urb_waiting[0] = NULL;
				snd_printd("%i\n", i);
			}
			subs->retired_frame[0] = urb->start_frame;
		} else {
			subs->retired_frame[1] = urb->start_frame;
			if (runtime->channels != 4)
				break;
		}

		if (! subs->running)
			continue;
		
		/* copy a data chunk */
		if ((hwptr_done + len) > runtime->buffer_size) {
			int cnt = runtime->buffer_size - hwptr_done;
			if (fourc_cpy) {
				unsigned char *p_dma_area = runtime->dma_area + (urb->pipe == subs->datapipe[0] ? 0 : usX2Y->stride);
				fourc_cpy(p_dma_area + hwptr_done * usX2Y->stride * 2, cp, cnt);
				fourc_cpy(p_dma_area, cp + cnt * usX2Y->stride, len - cnt);
			} else {
				int blen = cnt * usX2Y->stride;
				memcpy(runtime->dma_area + hwptr_done * usX2Y->stride, cp, blen);
				memcpy(runtime->dma_area, cp + blen, len * usX2Y->stride - blen);
			}
		} else {
			if (fourc_cpy) {
				unsigned char *p_dma_area = runtime->dma_area + (urb->pipe == subs->datapipe[0] ? 0 : usX2Y->stride);
				fourc_cpy(p_dma_area + hwptr_done * usX2Y->stride * 2, cp, len);
			} else {
				memcpy(runtime->dma_area + hwptr_done * usX2Y->stride, cp, len * usX2Y->stride);
			}
		}
		lens += len;
		if ((hwptr_done += len) >= runtime->buffer_size)
			hwptr_done -= runtime->buffer_size;
	}
	if ((runtime->channels == 4 && subs->retired_frame[0] != subs->retired_frame[1]) || ! subs->running)
		return 0;

	spin_lock_irqsave(&subs->lock, flags);
	subs->hwptr_done = hwptr_done;
	subs->transfer_done += lens;
	/* update the pointer, call callback if necessary */
	if (subs->transfer_done >= runtime->period_size) {
		subs->transfer_done -= runtime->period_size;
		spin_unlock_irqrestore(&subs->lock, flags);
		snd_pcm_period_elapsed(subs->pcm_substream);
	} else
		spin_unlock_irqrestore(&subs->lock, flags);

	return 0;
}
/*
 * prepare urb for playback data pipe
 *
 * we copy the data directly from the pcm buffer.
 * the current position to be copied is held in hwptr field.
 * since a urb can handle only a single linear buffer, if the total
 * transferred area overflows the buffer boundary, we cannot send
 * it directly from the buffer.  thus the data is once copied to
 * a temporary buffer and urb points to that.
 */
static int snd_usX2Y_urb_play_prepare(snd_usX2Y_substream_t *subs,
				snd_pcm_runtime_t *runtime,
				struct urb *urb)
{
	int count, counts, pack;
	unsigned long flags;
	usX2Ydev_t* usX2Y = subs->stream->usX2Y;

	urb->dev = usX2Y->chip.dev; /* we need to set this at each time */
	spin_lock_irqsave(&subs->lock, flags);
	subs->freqm = subs->stream->substream[SNDRV_PCM_STREAM_CAPTURE].freqm;
	count = 0;
	for (pack = 0; pack < NRPACKS; pack++) {
		/* calculate the size of a packet */
		count += (counts = usX2Y->pipe0Aframes[0][pack]);

		if (counts < 43 || counts > 50) {
			snd_printk("should not be here with counts=%i\n", counts);
			spin_unlock_irqrestore(&subs->lock, flags);
			return -1;
		}

		usX2Y->pipe0Aframes[0][pack] = usX2Y->pipe0Aframes[1][pack];
		usX2Y->pipe0Aframes[1][pack] = 0;
		/* set up descriptor */
		urb->iso_frame_desc[pack].offset = pack ? urb->iso_frame_desc[pack - 1].length : 0;
		urb->iso_frame_desc[pack].length = counts * usX2Y->stride;
	}

	if (subs->hwptr + count > runtime->buffer_size) {
		/* err, the transferred area goes over buffer boundary.
		 * copy the data to the temp buffer.
		 */
		int len;
		len = runtime->buffer_size - subs->hwptr;
		urb->transfer_buffer = subs->tmpbuf;
		memcpy(subs->tmpbuf, runtime->dma_area + subs->hwptr * usX2Y->stride, len * usX2Y->stride);
		memcpy(subs->tmpbuf + len * usX2Y->stride, runtime->dma_area, (count - len) * usX2Y->stride);
		subs->hwptr += count;
		subs->hwptr -= runtime->buffer_size;
	} else {
		/* set the buffer pointer */
		urb->transfer_buffer = runtime->dma_area + subs->hwptr * usX2Y->stride;
		if ((subs->hwptr += count) >= runtime->buffer_size)
			subs->hwptr -= runtime->buffer_size;			
	}
	spin_unlock_irqrestore(&subs->lock, flags);
	urb->transfer_buffer_length = count * usX2Y->stride;

	return 0;
}

/*
 * process after playback data complete
 *
 * update the current position and call callback if a period is processed.
 */
inline static int snd_usX2Y_urb_play_retire(snd_usX2Y_substream_t *subs,
			       snd_pcm_runtime_t *runtime,
			       struct urb *urb)
{
	unsigned long	flags;
	int		len = (urb->iso_frame_desc[0].actual_length
#if NRPACKS > 1
			       + urb->iso_frame_desc[1].actual_length
#endif
		               ) / subs->stream->usX2Y->stride;

	spin_lock_irqsave(&subs->lock, flags);
	
	subs->transfer_done += len;
	subs->hwptr_done +=  len;
	if (subs->hwptr_done >= runtime->buffer_size)
		subs->hwptr_done -= runtime->buffer_size;
	if (subs->transfer_done >= runtime->period_size) {
		subs->transfer_done -= runtime->period_size;
		spin_unlock_irqrestore(&subs->lock, flags);
		snd_pcm_period_elapsed(subs->pcm_substream);
	} else
		spin_unlock_irqrestore(&subs->lock, flags);
		
	//snd_printd("play %06u %i %i\n", *((unsigned*)&runtime->status->hw_ptr), subs->transfer_done, runtime->period_size);
	return 0;
}

inline static int snd_usX2Y_urb_submit(snd_usX2Y_substream_t *subs, struct urb *urb)
{
	int err;
	urb->start_frame = (urb->start_frame + NRURBS*NRPACKS) & (1024 - 1);
	urb->hcpriv = 0;
	if ((err = usb_submit_urb(urb, GFP_ATOMIC)) < 0) {
		return err;
	}
	subs->start_frame = urb->start_frame;
	return 0;
}
/*
 * complete callback from data urb
 */
static void _snd_usX2Y_urb_play_complete(struct urb* urb)
{
	snd_usX2Y_substream_t *subs = (snd_usX2Y_substream_t*)urb->context;
	snd_pcm_substream_t *substream = subs->pcm_substream;
	int err;

	if (urb->status) {
		snd_printk("play urb->status = %i\n", urb->status);
		urb->status = 0;
		return;
	}
	if (subs->running && snd_usX2Y_urb_play_retire(subs, substream->runtime, urb))
		return;
	if (! subs->running) /* can be stopped during retire callback */
		return;
	if (	(err = snd_usX2Y_urb_play_prepare(subs, substream->runtime, urb)) < 0
	    ||	(err = snd_usX2Y_urb_submit(subs, urb)) < 0
		) {
		snd_printd(KERN_ERR "cannot submit urb (err = %d)\n", err);
		snd_pcm_stop(substream, SNDRV_PCM_STATE_XRUN);
		return;
	}
}

static void snd_usX2Y_urb_play_complete(struct urb* urb, struct pt_regs *regs)
{
	snd_usX2Y_substream_t *subs = (snd_usX2Y_substream_t*)urb->context;
	if (! subs->stream->usX2Y->pipe0Aframes[0][0]) {
		// wait for no of frames info from capture pipe
		snd_printd("playurb has to wait?!\n");
		subs->stream->usX2Y->play_urb_waiting[0] = urb;
		return;
	}
	_snd_usX2Y_urb_play_complete(urb);
}
/*
 * complete callback from data urb
 */
static void snd_usX2Y_urb_capt_complete(struct urb* urb, struct pt_regs *regs)
{
	snd_usX2Y_substream_t *captsubs = (snd_usX2Y_substream_t*)urb->context;
	snd_pcm_substream_t *pcm_captsubs = captsubs->pcm_substream;
	snd_pcm_runtime_t* runtime = NULL;
	int err;

	if (urb->status) {
		snd_printk("snd_usX2Y_urb_capt_complete(): urb->status = %i\n", urb->status);
		urb->status = 0;
		return;
	}
	if (pcm_captsubs && snd_pcm_running(pcm_captsubs))
		runtime = pcm_captsubs->runtime;
	if (NULL == runtime) {
		snd_usX2Y_substream_t *playsubs = captsubs->stream->substream + SNDRV_PCM_STREAM_PLAYBACK;
		snd_pcm_substream_t *pcm_playsubs = playsubs->pcm_substream;
		if (pcm_playsubs && snd_pcm_running(pcm_playsubs))
			runtime = pcm_playsubs->runtime;
	}
	if (NULL == runtime) {
		snd_printd("NULL == runtime\n");
		return;
	}
	if (captsubs->bussing && snd_usX2Y_urb_capt_retire(captsubs, runtime, urb))
		return;
 	if (! captsubs->bussing) /* can be stopped during retire callback */
		return;
	if ((err = snd_usX2Y_urb_capt_prepare(captsubs, runtime, urb)) < 0 ||
	    (err = snd_usX2Y_urb_submit(captsubs, urb)) < 0) {
		snd_printd(KERN_ERR "cannot submit urb (err = %d)\n", err);
		if (pcm_captsubs)
			snd_pcm_stop(pcm_captsubs, SNDRV_PCM_STATE_XRUN);
		return;
	}
	if (urb->pipe == captsubs->datapipe[0] && 
	    captsubs->stream->usX2Y->play_urb_waiting[0]) {
		_snd_usX2Y_urb_play_complete(captsubs->stream->usX2Y->play_urb_waiting[0]);
		captsubs->stream->usX2Y->play_urb_waiting[0] = captsubs->stream->usX2Y->play_urb_waiting[1];
		captsubs->stream->usX2Y->play_urb_waiting[1] = NULL;
	}
}
/*
 * unlink active urbs.
 * return the number of active urbs.
 */
static int snd_usX2Y_urbs_deactivate(snd_usX2Y_substream_t *subs)
{
	int i, alive, ep;

	if (subs == (subs->stream->substream + SNDRV_PCM_STREAM_PLAYBACK)) {
		snd_usX2Y_substream_t *capsubs = subs->stream->substream + SNDRV_PCM_STREAM_CAPTURE;
		subs->running = subs->bussing = 0;
		if (capsubs->bussing  &&  ! capsubs->running)
			capsubs->bussing = 0;
	} else
		if (0x08 == subs->endpoint[0]) {
			snd_usX2Y_substream_t *playsubs = subs->stream->substream + SNDRV_PCM_STREAM_PLAYBACK;
			if (playsubs->running) {
				subs->running = 0;
				return 0;
			}
		}

	subs->running = subs->bussing = 0;
	alive = 0;

	for (ep = 0; ep < subs->endpoints; ep++)
		for (i = 0; i < NRURBS; i++) {
			if (subs->dataurb[ep][i] &&
			    subs->dataurb[ep][i]->status == -EINPROGRESS) {
				alive++;
			}
		}

	return alive;
}

static int snd_usX2Y_urb_start(snd_usX2Y_substream_t *subs)
{
	int i, err, ep;

	for (ep = 0; ep < subs->endpoints; ep++) {
		subs->retired_frame[ep] = -1;
		for (i = 0; i < NRURBS; i++) {
			if (0 == ep)
				subs->dataurb[0][i]->transfer_flags = URB_ISO_ASAP;
			else{
				subs->dataurb[ep][i]->transfer_flags = 0;
				subs->dataurb[ep][i]->start_frame = subs->dataurb[0][i]->start_frame;
			}
			if ((err = usb_submit_urb(subs->dataurb[ep][i], GFP_ATOMIC)) < 0) {
				snd_printk (KERN_ERR "cannot submit datapipe for urb %d %d, err = %d\n", ep, i, err);
				return -EPIPE;
			}
			if (0 == ep) {
				subs->dataurb[0][i]->transfer_flags = 0;
				subs->start_frame = subs->dataurb[0][i]->start_frame;
			}
		}
	}
	subs->bussing = 1;
	return 0;
}



/*
 * set up and start data/sync urbs
 */
static int snd_usX2Y_urb_capt_start(snd_usX2Y_substream_t *subs, snd_pcm_runtime_t *runtime)
{
	int i;

	if (! subs->bussing) {
		int ep;

		for (ep = 0; ep < subs->endpoints; ep++)
			for (i = 0; i < NRURBS; i++) {
				snd_assert(subs->dataurb[ep][i], return -EINVAL);
				if (snd_usX2Y_urb_capt_prepare(subs, runtime, subs->dataurb[ep][i]) < 0) {
					snd_printk(KERN_ERR "cannot prepare datapipe for urb %d\n", i);
					goto __error;
				}
			}
		subs->remainder = 0;
	}

	subs->running = 1;

	if (! subs->bussing) {
		if (snd_usX2Y_urb_start(subs))
			goto __error;
	}
	return 0;

 __error:
	// snd_pcm_stop(subs->pcm_substream, SNDRV_PCM_STATE_XRUN);
	snd_usX2Y_urbs_deactivate(subs);
	return -EPIPE;
}
/*
 * set up and start data/sync urbs
 */
static int snd_usX2Y_urb_play_start(snd_usX2Y_substream_t *subs, snd_pcm_runtime_t *runtime)
{
	int i;
	usX2Ydev_t* usX2Y = subs->stream->usX2Y;

	usX2Y->play_urb_waiting[0] = usX2Y->play_urb_waiting[1] = NULL;

	for (i = 0; i < NRURBS; i++) {// FIXME: start capture first, wait for refframes, then start playback.
		snd_assert(subs->dataurb[i], return -EINVAL);
		usX2Y->pipe0Aframes[0][0] = usX2Y->refframes + 1;
#if NRPACKS > 1
		usX2Y->pipe0Aframes[0][1] = usX2Y->refframes;
#endif
		if (snd_usX2Y_urb_play_prepare(subs, runtime, subs->dataurb[0][i]) < 0) {
			snd_printk(KERN_ERR "cannot prepare datapipe for urb %d\n", i);
			goto __error;
		}
	}
	memset(usX2Y->pipe0Aframes, 0, sizeof(usX2Y->pipe0Aframes));
	subs->remainder = 0;

	{
		snd_usX2Y_substream_t *capsubs = subs->stream->substream + SNDRV_PCM_STREAM_CAPTURE;
		if (! capsubs->bussing) {
			int ep;
			snd_printd("starting capture pipes for playpipe\n");
			snd_usX2Y_set_format(capsubs, runtime);
			snd_usX2Y_substream_prepare(capsubs, runtime);
			for (ep = 0; ep < capsubs->endpoints; ep++)
				for (i = 0; i < NRURBS; i++) {
					snd_assert(capsubs->dataurb[ep][i], return -EINVAL);
					if (snd_usX2Y_urb_capt_prepare(capsubs, runtime, capsubs->dataurb[ep][i]) < 0) {
						snd_printk(KERN_ERR "cannot prepare datapipe for urb %d\n", i);
						goto __error;
					}
				}
			capsubs->remainder = 0;
			if (snd_usX2Y_urb_start(capsubs))
				goto __error;
		}
	}

	subs->running = 1;

	if (snd_usX2Y_urb_start(subs))
		goto __error;

	return 0;

 __error:
	// snd_pcm_stop(subs->pcm_substream, SNDRV_PCM_STATE_XRUN);
	snd_usX2Y_urbs_deactivate(subs);
	return -EPIPE;
}
/* 
 *  wait until all urbs are processed.
 */
static int snd_usX2Y_urbs_wait_clear(snd_usX2Y_substream_t *subs)
{
	int timeout = HZ;
	int i, alive, ep;

	do {
		alive = 0;
		for (ep = 0; ep < subs->endpoints; ep++)
			for (i = 0; i < NRURBS; i++) {
				if (subs->dataurb[ep][i] && subs->dataurb[ep][i]->status)
					alive++;
			}
		if (! alive)
			break;
		set_current_state(TASK_UNINTERRUPTIBLE);
		snd_printd("snd_usX2Y_urbs_wait_clear waiting\n");
		schedule_timeout(1);
	} while (--timeout > 0);
	if (alive)
		snd_printk(KERN_ERR "timeout: still %d active urbs..\n", alive);
	return 0;
}
/*
 * return the current pcm pointer.  just return the hwptr_done value.
 */
static snd_pcm_uframes_t snd_usb_pcm_pointer(snd_pcm_substream_t *substream)
{
	snd_usX2Y_substream_t *subs = (snd_usX2Y_substream_t *)substream->runtime->private_data;
	return subs->hwptr_done;
}
/*
 * start/stop substream
 */
static int snd_usX2Y_pcm_capt_trigger(snd_pcm_substream_t *substream, int cmd)
{
	snd_usX2Y_substream_t *subs = (snd_usX2Y_substream_t *)substream->runtime->private_data;
	int err;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		snd_printd("snd_usX2Y_pcm_capt_trigger(START)\n");
		err = snd_usX2Y_urb_capt_start(subs, substream->runtime);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		snd_printd("snd_usX2Y_pcm_capt_trigger(STOP)\n");
		err = snd_usX2Y_urbs_deactivate(subs);
		break;
	default:
		err = -EINVAL;
		break;
	}
	return err < 0 ? err : 0;
}
/*
 * start/stop substream, can be called in interrupt!
 */
static int snd_usX2Y_pcm_play_trigger(snd_pcm_substream_t *substream, int cmd)
{
	snd_usX2Y_substream_t *subs = (snd_usX2Y_substream_t *)substream->runtime->private_data;
	int err;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		snd_printd("snd_usX2Y_pcm_play_trigger(START)\n");
		err = snd_usX2Y_urb_play_start(subs, substream->runtime);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		snd_printd("snd_usX2Y_pcm_play_trigger(STOP)\n");
		err = snd_usX2Y_urbs_deactivate(subs);
		break;
	default:
		err = -EINVAL;
		break;
	}
	return err < 0 ? err : 0;
}
/*
 * release a urb data
 */
static void release_urb_ctx(struct urb** urb, int free_tb)
{
	if (*urb) {
		if (free_tb)
			kfree((*urb)->transfer_buffer);
		usb_free_urb(*urb);
		*urb = 0;
	}
}
/*
 * release a substream
 */
static void snd_usX2Y_urbs_release(snd_usX2Y_substream_t *subs)
{
	int i, ep;
	snd_printd("snd_usX2Y_urbs_release()\n");
	/* stop urbs (to be sure) */
	if (snd_usX2Y_urbs_deactivate(subs) > 0)
		snd_printd("waitclear\n");
	snd_usX2Y_urbs_wait_clear(subs);
	
	for (ep = 0; ep < subs->endpoints; ep++)
		for (i = 0; i < NRURBS; i++)
			release_urb_ctx(subs->dataurb[ep] + i, subs == (subs->stream->substream + SNDRV_PCM_STREAM_CAPTURE));

	if (subs->tmpbuf) {
		kfree(subs->tmpbuf);
		subs->tmpbuf = 0;
	}
}
/*
 * initialize a substream for plaback/capture
 */
static int snd_usX2Y_substream_prepare(snd_usX2Y_substream_t *subs, snd_pcm_runtime_t *runtime)
{
	int i, ep;
	int is_playback = subs == (subs->stream->substream + SNDRV_PCM_STREAM_PLAYBACK);

	/* calculate the frequency in 10.14 format */
	subs->freqn = subs->freqm = get_usb_rate(runtime->rate);

	/* reset the pointer */
	subs->hwptr = 0;
	subs->hwptr_done = 0;
	subs->transfer_done = 0;

	if (subs->bussing)
		return 0;

	/* allocate a temporary buffer for playback */
	if (is_playback && ! subs->tmpbuf) {
		subs->tmpbuf = kmalloc(subs->maxpacksize * NRPACKS, GFP_KERNEL);
		if (! subs->tmpbuf) {
			snd_printk(KERN_ERR "cannot malloc tmpbuf\n");
			return -ENOMEM;
		}
	}

	/* allocate and initialize data urbs */
	for (ep = 0; ep < subs->endpoints; ep++)
		for (i = 0; i < NRURBS; i++) {
			struct urb** purb = subs->dataurb[ep] + i;
			if (*purb)
				continue;
			*purb = usb_alloc_urb(NRPACKS, GFP_KERNEL);
			if (! *purb) {
				snd_usX2Y_urbs_release(subs);
				return -ENOMEM;
			}
			if (!is_playback && !(*purb)->transfer_buffer) {
				/* allocate a capture buffer per urb */
				(*purb)->transfer_buffer = kmalloc(subs->maxpacksize*NRPACKS, GFP_KERNEL);
				if (!(*purb)->transfer_buffer) {
					snd_usX2Y_urbs_release(subs);
					return -ENOMEM;
				}
			}
			(*purb)->dev = subs->stream->usX2Y->chip.dev;
			(*purb)->pipe = subs->datapipe[ep];
			(*purb)->transfer_flags = URB_ISO_ASAP | URB_ASYNC_UNLINK;
			(*purb)->number_of_packets = NRPACKS;
			(*purb)->context = subs;
			(*purb)->complete = is_playback ?
				snd_usb_complete_callback(snd_usX2Y_urb_play_complete) :
				snd_usb_complete_callback(snd_usX2Y_urb_capt_complete);
		}

	return 0;
}

/*
 * find a matching format and set up the interface
 */
static int snd_usX2Y_set_format(snd_usX2Y_substream_t *subs, snd_pcm_runtime_t *runtime)
{
	struct usb_device *dev = subs->stream->usX2Y->chip.dev;
	snd_printd("about to set format: format = %s, rate = %d, channels = %d\n",
			   snd_pcm_format_name(runtime->format), runtime->rate, runtime->channels);
	/* create data pipes */
	if (subs == (subs->stream->substream + SNDRV_PCM_STREAM_PLAYBACK)) {
		subs->datapipe[0] = usb_sndisocpipe(dev, subs->endpoint[0]);
		subs->maxpacksize = dev->epmaxpacketout[subs->endpoint[0]];
	} else {
		subs->datapipe[0] = usb_rcvisocpipe(dev, subs->endpoint[0]);
		subs->datapipe[1] = usb_rcvisocpipe(dev, subs->endpoint[1]);
		subs->maxpacksize = dev->epmaxpacketin[subs->endpoint[0]];
	}
	return 0;
}

#ifndef OLD_USB
static void snd_usX2Y_04Int(struct urb* urb, struct pt_regs *regs)
#else
static void snd_usX2Y_04Int(struct urb* urb)
#endif
{
	usX2Ydev_t*	usX2Y = urb->context;
	
	if (urb->status) {
		snd_printk("snd_usX2Y_04Int() urb->status=%i\n", urb->status);
		return;
	}
	if (0 == --usX2Y->US04->len)
		wake_up_interruptible(&usX2Y->In04WaitQueue);
}
/*
 * allocate a buffer, setup samplerate
 *
 * so far we use a physically linear buffer although packetize transfer
 * doesn't need a continuous area.
 * if sg buffer is supported on the later version of alsa, we'll follow
 * that.
 */
static struct s_c2
{
	char c1, c2;
}
	SetRate44100[] =
{
	{ 0x14, 0x08},	// this line sets 44100, well actually a little less
	{ 0x18, 0x40},	// only tascam / frontier design knows the further lines .......
	{ 0x18, 0x42},
	{ 0x18, 0x45},
	{ 0x18, 0x46},
	{ 0x18, 0x48},
	{ 0x18, 0x4A},
	{ 0x18, 0x4C},
	{ 0x18, 0x4E},
	{ 0x18, 0x50},
	{ 0x18, 0x52},
	{ 0x18, 0x54},
	{ 0x18, 0x56},
	{ 0x18, 0x58},
	{ 0x18, 0x5A},
	{ 0x18, 0x5C},
	{ 0x18, 0x5E},
	{ 0x18, 0x60},
	{ 0x18, 0x62},
	{ 0x18, 0x64},
	{ 0x18, 0x66},
	{ 0x18, 0x68},
	{ 0x18, 0x6A},
	{ 0x18, 0x6C},
	{ 0x18, 0x6E},
	{ 0x18, 0x70},
	{ 0x18, 0x72},
	{ 0x18, 0x74},
	{ 0x18, 0x76},
	{ 0x18, 0x78},
	{ 0x18, 0x7A},
	{ 0x18, 0x7C},
	{ 0x18, 0x7E}
};
static struct s_c2 SetRate48000[] =
{
	{ 0x14, 0x09},	// this line sets 48000, well actually a little less
	{ 0x18, 0x40},	// only tascam / frontier design knows the further lines .......
	{ 0x18, 0x42},
	{ 0x18, 0x45},
	{ 0x18, 0x46},
	{ 0x18, 0x48},
	{ 0x18, 0x4A},
	{ 0x18, 0x4C},
	{ 0x18, 0x4E},
	{ 0x18, 0x50},
	{ 0x18, 0x52},
	{ 0x18, 0x54},
	{ 0x18, 0x56},
	{ 0x18, 0x58},
	{ 0x18, 0x5A},
	{ 0x18, 0x5C},
	{ 0x18, 0x5E},
	{ 0x18, 0x60},
	{ 0x18, 0x62},
	{ 0x18, 0x64},
	{ 0x18, 0x66},
	{ 0x18, 0x68},
	{ 0x18, 0x6A},
	{ 0x18, 0x6C},
	{ 0x18, 0x6E},
	{ 0x18, 0x70},
	{ 0x18, 0x73},
	{ 0x18, 0x74},
	{ 0x18, 0x76},
	{ 0x18, 0x78},
	{ 0x18, 0x7A},
	{ 0x18, 0x7C},
	{ 0x18, 0x7E}
};
#define NOOF_SETRATE_URBS (sizeof(SetRate48000)/sizeof(SetRate48000[0]))

static int usX2Y_rate_set(snd_usX2Y_stream_t *usX2Y_stream, int rate)
{
	int			err = 0, i;
	snd_usX2Y_urbSeq_t	*us = NULL;
	int			*usbdata = NULL;
	DECLARE_WAITQUEUE(wait, current);
	struct s_c2		*ra = rate == 48000 ? SetRate48000 : SetRate44100;

	if (usX2Y_stream->usX2Y->rate != rate) {
		do {
			us = kmalloc(sizeof(*us) + sizeof(struct urb*) * NOOF_SETRATE_URBS, GFP_KERNEL);
			if (NULL == us) {
				err = -ENOMEM;
				break;
			}
			memset(us, 0, sizeof(*us) + sizeof(struct urb*) * NOOF_SETRATE_URBS); 
			usbdata = kmalloc(sizeof(int)*NOOF_SETRATE_URBS, GFP_KERNEL);
			if (NULL == usbdata) {
				err = -ENOMEM;
				break;
			}
			for (i = 0; i < NOOF_SETRATE_URBS; ++i) {
				if (NULL == (us->urb[i] = usb_alloc_urb(0, GFP_KERNEL))) {
					err = -ENOMEM;
					break;
				}
				((char*)(usbdata + i))[0] = ra[i].c1;
				((char*)(usbdata + i))[1] = ra[i].c2;
				usb_fill_bulk_urb(us->urb[i], usX2Y_stream->usX2Y->chip.dev, usb_sndbulkpipe(usX2Y_stream->usX2Y->chip.dev, 4),
						  usbdata + i, 2, snd_usX2Y_04Int, usX2Y_stream->usX2Y);
#ifdef OLD_USB
				us->urb[i]->transfer_flags = USB_QUEUE_BULK;
#endif
			}
			if (err)
				break;

			add_wait_queue(&usX2Y_stream->usX2Y->In04WaitQueue, &wait);
			set_current_state(TASK_INTERRUPTIBLE);
			us->submitted =		0;
			us->len =		NOOF_SETRATE_URBS;
			usX2Y_stream->usX2Y->US04 =	us;
		
			do {
				signed long	timeout = schedule_timeout(HZ/2);
                	
				if (signal_pending(current)) {
					err = -ERESTARTSYS;
					break;
				}
				if (0 == timeout) {
					err = -ENODEV;
					break;
				}
				usX2Y_stream->usX2Y->rate = rate;
				usX2Y_stream->usX2Y->refframes = rate == 48000 ? 47 : 44;
			} while (0);
		
			set_current_state(TASK_RUNNING);
			remove_wait_queue(&usX2Y_stream->usX2Y->In04WaitQueue, &wait);
		} while (0);

		if (us) {
			us->submitted =	2*NOOF_SETRATE_URBS;
			for (i = 0; i < NOOF_SETRATE_URBS; ++i) {
				usb_unlink_urb(us->urb[i]);
				usb_free_urb(us->urb[i]);
			}
			usX2Y_stream->usX2Y->US04 =NULL;
			kfree(usbdata);
			kfree(us);
		}
	}

	return err;
}

static int snd_usX2Y_hw_params(	snd_pcm_substream_t*	substream,
				snd_pcm_hw_params_t*	hw_params)
{
	int			err = 0;
	unsigned int		rate = params_rate(hw_params),
				format = params_format(hw_params);
 	snd_usX2Y_stream_t	*usX2Y_stream = snd_pcm_substream_chip(substream);
	if (usX2Y_stream->usX2Y->format != format) {
		int alternate;
		if (format == SNDRV_PCM_FORMAT_S24_3LE) {
			alternate = 2;
			usX2Y_stream->usX2Y->stride = 6;
		} else {
			alternate = 1;
			usX2Y_stream->usX2Y->stride = 4;
		}
		if ((err = usb_set_interface(usX2Y_stream->usX2Y->chip.dev, 0, alternate))) {
			snd_printk("usb_set_interface error \n");
			return err;
		}
		usX2Y_stream->usX2Y->format = format;
		usX2Y_stream->usX2Y->rate = 0;
	}
	if (usX2Y_stream->usX2Y->rate != rate)
		err = usX2Y_rate_set(usX2Y_stream, rate);
	if (0 == err) {
		if (0 > (err = snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(hw_params))))
			snd_printd("snd_pcm_lib_malloc_pages(%x, %i) returned %i\n", substream, params_buffer_bytes(hw_params), err);
	}
	return err;
}
/*
 * free the buffer
 */
static int snd_usb_hw_free(snd_pcm_substream_t *substream)
{
	return snd_pcm_lib_free_pages(substream);
}
/*
 * prepare callback
 *
 * set format and initialize urbs
 */
static int snd_usX2Y_pcm_prepare(snd_pcm_substream_t *substream)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_usX2Y_substream_t *subs = (snd_usX2Y_substream_t *)runtime->private_data;
	int err;
	snd_printd("snd_usX2Y_pcm_prepare()\n");

	if (! subs->bussing)	
		snd_usX2Y_urbs_release(subs);
	if ((err = snd_usX2Y_set_format(subs, runtime)) < 0)
		return err;

	return snd_usX2Y_substream_prepare(subs, runtime);
}

static snd_pcm_hardware_t snd_usX2Y_2c =
{
	.info =			(SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_INTERLEAVED |
				 SNDRV_PCM_INFO_BLOCK_TRANSFER |
				 SNDRV_PCM_INFO_MMAP_VALID),
	.formats =                 SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_3LE,
	.rates =                   SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000,
	.rate_min =                44100,
	.rate_max =                48000,
	.channels_min =            2,
	.channels_max =            2,
	.buffer_bytes_max =	(2*128*1024),
	.period_bytes_min =	64,
	.period_bytes_max =	(128*1024),
	.periods_min =		2,
	.periods_max =		1024,
	.fifo_size =              0
};

static snd_pcm_hardware_t snd_usX2Y_4c =
{
	.info =			(SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_INTERLEAVED |
				 SNDRV_PCM_INFO_BLOCK_TRANSFER |
				 SNDRV_PCM_INFO_MMAP_VALID),
	.formats =                 SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_3LE,
	.rates =                   SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000,
	.rate_min =                44100,
	.rate_max =                48000,
	.channels_min =            2,
	.channels_max =            4,
	.buffer_bytes_max =	(2*2*128*1024),
	.period_bytes_min =	2*64,
	.period_bytes_max =	(2*128*1024),
	.periods_min =		2,
	.periods_max =		1024,
};


static int snd_usb_pcm_open(snd_pcm_substream_t *substream, int direction,
			    snd_pcm_hardware_t *hw)
{
	snd_usX2Y_stream_t	*as = snd_pcm_substream_chip(substream);
	snd_pcm_runtime_t	*runtime = substream->runtime;
	snd_usX2Y_substream_t	*subs = &as->substream[direction];

	runtime->hw = *hw;
	runtime->private_data = subs;
	subs->pcm_substream = substream;
	snd_pcm_hw_constraint_minmax(runtime, SNDRV_PCM_HW_PARAM_PERIOD_TIME, 1000, 200000);
	return 0;
}


static int snd_usb_playback_open(snd_pcm_substream_t *substream)
{
	snd_printd("snd_usb_playback_open()\n");
	return snd_usb_pcm_open(substream, SNDRV_PCM_STREAM_PLAYBACK, &snd_usX2Y_2c);
}

static int snd_usb_playback_close(snd_pcm_substream_t *substream)
{
	snd_usX2Y_stream_t	*as = snd_pcm_substream_chip(substream);
	snd_usX2Y_substream_t	*captsubs = as->substream + SNDRV_PCM_STREAM_CAPTURE,
				*playsubs = as->substream + SNDRV_PCM_STREAM_PLAYBACK;
	down(&as->usX2Y->open_mutex);
	snd_usX2Y_urbs_release(playsubs);
	playsubs->pcm_substream = NULL;
	if (captsubs->pcm_substream == NULL) {
		snd_usX2Y_urbs_release(captsubs);
		snd_assert(captsubs->pcm_substream == NULL);
	}
	up(&as->usX2Y->open_mutex);
	return 0;
}

static int snd_usb_capture_open(snd_pcm_substream_t *substream)
{
	snd_usX2Y_stream_t *as = snd_pcm_substream_chip(substream);
	snd_pcm_hardware_t *hardware = as->usX2Y->chip.dev->descriptor.idProduct == USB_ID_US428 ? &snd_usX2Y_4c : &snd_usX2Y_2c;
	return snd_usb_pcm_open(substream, SNDRV_PCM_STREAM_CAPTURE, hardware);
}

static int snd_usb_capture_close(snd_pcm_substream_t *substream)
{
	int err = 0;
	snd_usX2Y_stream_t *as = snd_pcm_substream_chip(substream);
	snd_usX2Y_substream_t* subs = as->substream + SNDRV_PCM_STREAM_CAPTURE;

	down(&as->usX2Y->open_mutex);

	if (! subs->bussing)
		snd_usX2Y_urbs_release(subs);
	subs->pcm_substream = NULL;

	up(&as->usX2Y->open_mutex);
	return err;
}


static snd_pcm_ops_t snd_usb_playback_ops = 
{
	.open =		snd_usb_playback_open,
	.close =	snd_usb_playback_close,
	.ioctl =	snd_pcm_lib_ioctl,
	.hw_params =	snd_usX2Y_hw_params,
	.hw_free =	snd_usb_hw_free,
	.prepare =	snd_usX2Y_pcm_prepare,
	.trigger =	snd_usX2Y_pcm_play_trigger,
	.pointer =	snd_usb_pcm_pointer,
};

static snd_pcm_ops_t snd_usb_capture_ops = 
{
	.open =		snd_usb_capture_open,
	.close =	snd_usb_capture_close,
	.ioctl =	snd_pcm_lib_ioctl,
	.hw_params =	snd_usX2Y_hw_params,
	.hw_free =	snd_usb_hw_free,
	.prepare =	snd_usX2Y_pcm_prepare,
	.trigger =	snd_usX2Y_pcm_capt_trigger,
	.pointer =	snd_usb_pcm_pointer,
};



/*
 * intialize the substream instance.
 */
static snd_usX2Y_substream_t *snd_usX2Y_substream_init(snd_usX2Y_stream_t *stream, enum sndrv_pcm_stream  dir)
{
	struct usb_device *dev;
	snd_usX2Y_substream_t *subs = stream->substream + dir;

	dev = stream->usX2Y->chip.dev;
	if (SNDRV_PCM_STREAM_PLAYBACK == dir) {
		subs->endpoint[0] = 0x0A;
		subs->endpoint[1] = 0;
		subs->endpoints = 1;
	} else {
		subs->endpoint[0] = 0x08;
		subs->endpoint[1] = 0x0A;
		subs->endpoints = dev->descriptor.idProduct == USB_ID_US428 ? 2 : 1;
	}
	subs->stream = stream;
	spin_lock_init(&subs->lock);
	return subs;
}


/*
 * free a substream
 */
static void snd_usX2Y_substream_free(snd_usX2Y_substream_t *subs)
{
}



/*
 * free a usb stream instance
 */
static void snd_usX2Y_audio_stream_free(snd_usX2Y_stream_t *stream)
{
	snd_usX2Y_substream_free(&stream->substream[0]);
	snd_usX2Y_substream_free(&stream->substream[1]);
	snd_magic_kfree(stream);
}

static void snd_usX2Y_audio_pcm_free(snd_pcm_t *pcm)
{
	snd_usX2Y_stream_t *stream = pcm->private_data;
	if (stream) {
		stream->pcm = NULL;
		snd_pcm_lib_preallocate_free_for_all(pcm);
		snd_usX2Y_audio_stream_free(stream);
	}
}

static int snd_usX2Y_audio_stream_new(snd_card_t* card)
{
	snd_usX2Y_stream_t *usX2Y_stream;
	snd_usX2Y_substream_t *usX2Y_capt_substream;
	snd_pcm_t *pcm;
	int err;

	usX2Y_stream = snd_magic_kmalloc(snd_usX2Y_stream_t, 0, GFP_KERNEL);
	if (usX2Y_stream == NULL) {
		snd_printk(KERN_ERR "cannot malloc\n");
		return -ENOMEM;
	}
	memset(usX2Y_stream, 0, sizeof(*usX2Y_stream));
	usX2Y_stream->usX2Y = usX2Y(card);

	snd_usX2Y_substream_init(usX2Y_stream, SNDRV_PCM_STREAM_PLAYBACK);
	usX2Y_capt_substream = snd_usX2Y_substream_init(usX2Y_stream, SNDRV_PCM_STREAM_CAPTURE);

	err = snd_pcm_new(card, NAME_ALLCAPS" Audio", usX2Y(card)->chip.pcm_devs,
			  1,
			  1,
			  &pcm);
	if (err < 0) {
		snd_usX2Y_audio_stream_free(usX2Y_stream);
		return err;
	}

	usX2Y_stream->pcm = pcm;
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &snd_usb_playback_ops);
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE, &snd_usb_capture_ops);

	pcm->private_data = usX2Y_stream;
	pcm->private_free = snd_usX2Y_audio_pcm_free;
	pcm->info_flags = SNDRV_PCM_INFO_NONATOMIC_OPS;

	sprintf(pcm->name, NAME_ALLCAPS" Audio #%d", usX2Y(card)->chip.pcm_devs);

	if (0 > (err = snd_pcm_lib_preallocate_pages(pcm->streams[SNDRV_PCM_STREAM_PLAYBACK].substream, 64*1024, 128*1024, GFP_KERNEL)) ||
	    0 > (err = snd_pcm_lib_preallocate_pages(pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream,
							usX2Y_capt_substream->endpoints * 64*1024,
							usX2Y_capt_substream->endpoints * 128*1024, GFP_KERNEL)) ||
	    (usX2Y(card)->chip.dev->descriptor.idProduct == USB_ID_US428 &&
	     0 > (err = usX2Y_rate_set(usX2Y_stream, 44100)))) {	// Lets us428 recognize output-volume settings, disturbs us122.
		snd_usX2Y_audio_stream_free(usX2Y_stream);
		return err;
	}
	usX2Y(card)->chip.pcm_devs++;

	return 0;
}

/*
 * free the chip instance
 *
 * here we have to do not much, since pcm and controls are already freed
 *
 */
static int snd_usX2Y_audio_dev_free(snd_device_t *device)
{
	return 0;
}


/*
 * create a chip instance and set its names.
 */
int snd_usX2Y_audio_create(snd_card_t* card)
{
	int err = 0;
	static snd_device_ops_t ops = {
		.dev_free = snd_usX2Y_audio_dev_free,
	};
	
	INIT_LIST_HEAD(&usX2Y(card)->chip.pcm_list);

	if ((err = snd_device_new(card, SNDRV_DEV_LOWLEVEL, usX2Y(card), &ops)) < 0) {
//		snd_usX2Y_audio_free(usX2Y(card));
		return err;
	}

	err = snd_usX2Y_audio_stream_new(card);

	return err;
}


/*
 * compatible layers
 */
#ifdef OLD_USB
static void __old_snd_usX2Y_urb_play_complete(struct urb *urb)
{
	snd_usX2Y_urb_play_complete(urb, NULL);
}

static void __old_snd_usX2Y_urb_capt_complete(struct urb *urb)
{
	snd_usX2Y_urb_capt_complete(urb, NULL);
}
#endif
