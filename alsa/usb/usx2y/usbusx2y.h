#ifndef USBUSX2Y_H
#define USBUSX2Y_H
#include <linux/usb.h>
#include "../../alsa-kernel/usb/usbaudio.h"
#include "usbus428ctldefs.h" 

#define NRURBS	        2	/* */
#define NRPACKS		2	/* usb-frames/ms per urb: 1 and 2 are supported. set to 1 if you want lowest possible latency */

#define URBS_AsyncSeq 10
#define URB_DataLen_AsyncSeq 32
typedef struct {
	struct urb*	urb[URBS_AsyncSeq];
	char*   buffer;
} snd_usX2Y_AsyncSeq_t;

typedef struct {
	int	submitted;
	int	len;
	struct urb*	urb[0];
} snd_usX2Y_urbSeq_t;


typedef struct {
	snd_usb_audio_t 	chip;
	int			stride;
	struct urb*		In04urb;
	void*			In04Buf;
	char			In04Last[24];
	unsigned		In04IntCalls;
	snd_usX2Y_urbSeq_t*	US04;
	wait_queue_head_t	In04WaitQueue;
	snd_usX2Y_AsyncSeq_t	AS04;
	unsigned int		rate,
				format;
	int			refframes;
	struct urb*		play_urb_waiting[2];
	int			pipe0Aframes[NRURBS][NRPACKS];
	snd_hwdep_t*		hwdep;
	int			chip_status;
	struct semaphore	open_mutex;
	us428ctls_sharedmem_t*	us428ctls_sharedmem;
	wait_queue_head_t	us428ctls_wait_queue_head;
} usX2Ydev_t;


#define usX2Y(c) ((usX2Ydev_t*)(c)->private_data)

int usX2Y_audio_create(snd_card_t* card);

#ifndef OLD_USB
void i_usX2Y_Out04Int(struct urb* urb, struct pt_regs *regs);
void i_usX2Y_In04Int(struct urb* urb, struct pt_regs *regs);
#else
void i_usX2Y_Out04Int(struct urb* urb);
void i_usX2Y_In04Int(struct urb* urb);
#endif

#define NAME_ALLCAPS "US-X2Y"

#endif
