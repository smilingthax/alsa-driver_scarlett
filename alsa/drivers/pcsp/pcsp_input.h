/*
 * PC-Speaker driver for Linux
 *
 * Volume conversion tables, sine-based amplification.
 * Copyright (C) 2001-2004  Stas Sergeev
 */

#ifndef __PCSP_INPUT_H__
#define __PCSP_INPUT_H__

int pcspkr_input_init(struct snd_pcsp *chip);
int pcspkr_input_remove(struct snd_pcsp *chip);

#endif
