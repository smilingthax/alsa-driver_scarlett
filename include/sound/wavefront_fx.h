#ifndef __WAVEFRONT_FX_H__
#define __WAVEFRONT_FX_H__

extern int  snd_wavefront_fx_detect (snd_wavefront_t *);
extern void snd_wavefront_fx_ioctl  (snd_synth_t *sdev, 
				     unsigned int cmd, 
				     unsigned long arg);

#endif  __WAVEFRONT_FX_H__
