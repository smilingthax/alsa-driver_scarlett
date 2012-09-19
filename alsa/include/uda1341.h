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
 * 2002-03-30 Tomas Kasparek Proc filesystem support, complete mixer and DSP
 *                           features support
 */

/* $Id: uda1341.h,v 1.4 2002/04/04 07:27:09 perex Exp $ */

#define UDA1341_ALSA_NAME "snd-uda1341"

enum uda1341_onoff {
        OFF=0,
        ON,
};

const char *uda1341_onoff_names[] = {
        "Off",
        "On",
};

enum uda1341_format {
        I2S=0,
        LSB16,
        LSB18,
        LSB20,
        MSB,
        LSB16MSB,
        LSB18MSB,
        LSB20MSB,        
};

const char *uda1341_format_names[] = {
        "I2S-bus",
        "LSB 16bits",
        "LSB 18bits",
        "LSB 20bits",
        "MSB",
        "in LSB 16bits/out MSB",
        "in LSB 18bits/out MSB",
        "in LSB 20bits/out MSB",        
};

enum uda1341_fs {
        F512=0,
        F384,
        F256,
        Funused,
};

const char *uda1341_fs_names[] = {
        "512*fs",
        "384*fs",
        "256*fs",
        "Unused - bad value!",
};

enum uda1341_peak {
        BEFORE=0,
        AFTER,
};

const char *uda1341_peak_names[] = {
        "before",
        "after",
};

enum uda1341_filter {
        FLAT=0,
        MIN,
        MIN2,
        MAX,
};

const char *uda1341_filter_names[] = {
        "flat",
        "min",
        "min",
        "max",
};

enum uda1341_mixer {
        DOUBLE,
        LINE,
        MIC,
        MIXER,
};

const char *uda1341_mixer_names[] = {
        "double differential",
        "input channel 1 (line in)",
        "input channel 2 (microphone)",
        "digital mixer",
};

enum uda1341_deemp {
        NONE,
        D32,
        D44,
        D48,
};

const char *uda1341_deemp_names[] = {
        "none",
        "32 kHz",
        "44.1 kHz",
        "48 kHz",        
};

const unsigned short uda1341_AGC_atime[] = {11, 16, 11, 16, 21, 11, 16, 21};
const unsigned short uda1341_AGC_dtime[] = {100, 100, 200, 200, 200, 400, 400, 400};

enum uda1341_config {
        CMD_RESET = 0,
	CMD_FS,
	CMD_FORMAT,
        CMD_OGAIN,
        CMD_IGAIN,
        CMD_DAC,
        CMD_ADC,
        CMD_VOLUME,
        CMD_BASS,
        CMD_TREBBLE,
        CMD_PEAK,
        CMD_DEEMP,
        CMD_MUTE,        
        CMD_FILTER,
        CMD_CH1,
        CMD_CH2,
        CMD_MIC,       
        CMD_MIXER,
        CMD_AGC,
        CMD_IG,
        CMD_AGC_TIME,
        CMD_AGC_LEVEL,
        CMD_LAST,
};

#include <sound/core.h>
int __init snd_chip_uda1341_mixer_new(snd_card_t *card, struct l3_client **clnt);
void __init snd_chip_uda1341_mixer_del(snd_card_t *card);

#ifdef CONFIG_SND_DEBUG_MEMORY
#define h3600_t_magic				0xa15a3a00
#define uda1341_t_magic				0xa15a3b00
#define l3_client_t_magic			0xa15a3c00
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
