#ifndef USBUS428_H
#define USBUS428_H
#include <linux/usb.h>
#include "../../alsa-kernel/usb/usbaudio.h"
#include "usbus428ctldefs.h" 

#define NRURBS	        2	/* */
#define NRPACKS		1	/* usb-frames/ms per urb */

#ifndef LINUX_2_2
typedef struct urb urb_t;
typedef struct urb* purb_t;
#endif

#define URBS_AsyncSeq 10
#define URB_DataLen_AsyncSeq 32
typedef struct {
	urb_t*	urb[URBS_AsyncSeq];
	char*   buffer;
} snd_us428_AsyncSeq_t;

typedef struct {
	int	submitted;
	int	len;
	urb_t*	urb[0];
} snd_us428_urbSeq_t;


typedef struct {
	snd_usb_audio_t 	chip;
	int			stride;
	purb_t			In04urb;
	void*			In04Buf;
	char			In04Last[24];
	unsigned		In04IntCalls;
	snd_us428_urbSeq_t*	US04;
	int			Seq04;
	int 			Seq04Complete;
	wait_queue_head_t	In04WaitQueue;
	snd_us428_AsyncSeq_t	AS04;
	unsigned int		rate,
				format;
	int			refframes;
	purb_t			play_urb_waiting[2];
	int			pipe0Aframes[NRURBS][NRPACKS];
	snd_hwdep_t*		hwdep;
	int			chip_status;
	struct semaphore	open_mutex;
	us428ctls_sharedmem_t*	us428ctls_sharedmem;
	wait_queue_head_t	us428ctls_wait_queue_head;
} us428dev_t;


#define us428(c) ((us428dev_t*)(c)->private_data)

int snd_us428_audio_create(snd_card_t* card);

#ifndef OLD_USB
void snd_us428_Out04Int(urb_t* urb, struct pt_regs *regs);
void snd_us428_In04Int(urb_t* urb, struct pt_regs *regs);
#else
void snd_us428_Out04Int(urb_t* urb);
void snd_us428_In04Int(urb_t* urb);
#endif

#ifndef CONFIG_SND_DEBUG
#define snd_us428_Out04Int 0
#endif

#define NAME_ALLCAPS "US-428"

#endif
