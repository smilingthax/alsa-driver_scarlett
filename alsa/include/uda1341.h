/*
 *  linux/include/linux/l3/uda1341.h
 *
 * Philips UDA1341 mixer device driver for ALSA
 *
 * Copyright (c) 2002 Tomas Kasparek <tomas.kasparek@seznam.cz>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License.
 *
 * History:
 *
 * 2002-03-13 Tomas Kasparek Initial release - based on uda1341.h from OSS
 *
 */

#define UDA1341_ALSA_NAME "snd-uda1341"

struct uda1341_cfg {
	unsigned int fs:16;
	unsigned int format:3;
};

#define FMT_I2S		0
#define FMT_LSB16	1
#define FMT_LSB18	2
#define FMT_LSB20	3
#define FMT_MSB		4
#define FMT_LSB16MSB	5
#define FMT_LSB18MSB	6
#define FMT_LSB20MSB	7

#define L3_UDA1341_CONFIGURE	0x13410001

struct l3_gain {
	unsigned int	left:8;
	unsigned int	right:8;
	unsigned int	unused:8;
	unsigned int	channel:8;
};

#define L3_SET_VOLUME		0x13410002
#define L3_SET_TREBLE		0x13410003
#define L3_SET_BASS		0x13410004
#define L3_SET_GAIN		0x13410005

struct l3_agc {
	unsigned int	level:8;
	unsigned int	enable:1;
	unsigned int	attack:7;
	unsigned int	decay:8;
	unsigned int	channel:8;
};

#define L3_INPUT_AGC		0x13410006

#include <sound/core.h>
int __init snd_chip_uda1341_mixer_new(snd_card_t *card, struct l3_client **clnt);
void __init snd_chip_uda1341_mixer_del(snd_card_t *card);

#ifdef CONFIG_SND_DEBUG_MEMORY
#define h3600_t_magic				0xa15a3a00
#define l3_client_t_magic			0xa15a3b00
#endif

#ifdef DEBUG_MODE
#define DEBUG(format, args...)      do{printk(format, ##args);}while(0)
#else
#define DEBUG(format, args...)      /* nothing */
#endif

#ifdef DEBUG_FUNCTION_NAMES
#define DEBUG_NAME(format, args...)     do{printk(format, ##args);}while(0)
#else
#define DEBUG_NAME(format, args...)     /* nothing */
#endif
