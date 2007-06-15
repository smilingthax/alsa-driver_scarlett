/*
 *  patch_wm8785.c - Driver for C-Media CMI8788 PCI soundcards.
 *
 *      Copyright (C) 2005  C-media support
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
 * 
 *  Revision history
 *
 *    Weifeng Sui <weifengsui@163.com>
 */

/*
 * audio interface patch for WM8785
 */

#include <sound/driver.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <sound/core.h>

#include "cmi8788.h"
#include "codec.h"
#include "cmi_controller.h"

/*
 *
 */
#define WM8785_R0        0x00
#define WM8785_R1        0x01
#define WM8785_R2        0x02
#define WM8785_R7        0x07


/*
 * Analog playback callbacks
 */
static int wm8785_playback_pcm_open(void                *hinfo,
                                    cmi_codec           *codec,
                                    struct snd_pcm_substream *substream )
{
    cmi8788_pcm_stream  *pcm_stream = (cmi8788_pcm_stream  *)hinfo;
        
    return 0;
}

static int wm8785_playback_pcm_prepare(void                *hinfo,
                                       cmi_codec           *codec,
                                       struct snd_pcm_substream *substream )
{
    cmi8788_pcm_stream  *pcm_stream = (cmi8788_pcm_stream  *)hinfo;
    
    return 0;
}

static int wm8785_playback_pcm_cleanup(void                *hinfo,
                                       cmi_codec           *codec,
                                       struct snd_pcm_substream *substream )
{
    cmi8788_pcm_stream  *pcm_stream = (cmi8788_pcm_stream  *)hinfo;
    
    return 0;
}

/*
 * Analog capture
 */
static int wm8785_capture_pcm_prepare(void                *hinfo,
                                      cmi_codec           *codec,
                                      struct snd_pcm_substream *substream )
{
    cmi8788_pcm_stream  *pcm_stream = (cmi8788_pcm_stream  *)hinfo;
    
    return 0;
}

static int wm8785_capture_pcm_cleanup(void                *hinfo,
                                      cmi_codec           *codec,
                                      struct snd_pcm_substream *substream )
{
    cmi8788_pcm_stream  *pcm_stream = (cmi8788_pcm_stream  *)hinfo;
    
    return 0;
}

/*
 */
static cmi8788_pcm_stream wm8785_pcm_analog_playback = {
    .channels = 2,
    .ops = {
        .open    = wm8785_playback_pcm_open,
        .prepare = wm8785_playback_pcm_prepare,
        .cleanup = wm8785_playback_pcm_cleanup
    },
};

static cmi8788_pcm_stream wm8785_pcm_analog_capture = {
    .channels = 2,


    .ops = {
        .prepare = wm8785_capture_pcm_prepare,
        .cleanup = wm8785_capture_pcm_cleanup
    },
};


static int wm8785_build_pcms(cmi_codec *codec)
{
    cmi8788_pcm_stream  *pcm_substream = codec->pcm_substream;

    pcm_substream[0] = wm8785_pcm_analog_playback;
    pcm_substream[1] = wm8785_pcm_analog_capture;

    return 0;
}

/*
 * mixer
 */
static int put_volume(cmi_codec *codec, int l_vol, int r_vol)
{
    u8 data[2]={0,0};
    u8 l_volume = 0, r_volume = 0;

    if(!codec)
        return -1;

    cmi8788_controller *controller = codec->controller;
    if(!controller)
        return -1;

    return 0;
}

/*
 * The ak4396 does not support read command.
 */
static int get_volume(cmi_codec *codec, int *l_vol, int *r_vol)
{
    return -1;
}

static cmi8788_mixer_ops  wm8785_mixer_ops = 
{
  .get_volume = NULL,//get_volume,
  .set_volume = NULL,//put_volume,
};

/*
 * create mixer
 */
static int wm8785_build_controls(cmi_codec *codec)
{
    //
    if(!codec)
        return -1;

    codec->mixer_ops = wm8785_mixer_ops;

    return 0;
}

//
// use SPI
//
static int wm8785_init(cmi_codec *codec)
{
    //
//  u16 data= 0;
    u8 data[3]={0,0,0};

    cmi_printk(("  >> wm8785_init\n"));

    if(!codec)
        return -1;

    cmi8788_controller *controller = codec->controller;
    if(!controller)
        return -1;

    // 3 bytes
    //    7Bit     + 1Bit +    7Bit       +             1Bit           +            8Bit
    // device addr + R/W  + register addr + first bit of register data + register remaining 8 bit data
    codec->reg_len_flag = 0;   

    // R7 reset
    data[0] = 0x01;
    data[1] = (WM8785_R7 < 1) | 0x00; // Data Bit-8: 0
    data[2] = (0x1A < 1) & 0xFE; // WM8785 device addr 0011010, ; Bit-0 0: write
    data[0] = 0x01;
    data[1] = 0x0E;
    controller->ops.spi_cmd(codec, data);

    data[0] = 0x03;
    data[1] = (WM8785_R0 < 1) | 0x00; // Data Bit-8: 0
    data[2] = (0x1A < 1) & 0xFE; // WM8785 device addr 0011010, ; Bit-0 0: write
    data[0] = 0x20;
    data[1] = 0x00;
    controller->ops.spi_cmd(codec, data);

    data[0] = 0x0A;
    data[1] = (WM8785_R1 < 1) | 0x00; // Data Bit-8: 0
    data[2] = (0x1A < 1) & 0xFE; // WM8785 device addr 0011010, ; Bit-0 0: write
    data[0] = 0x00;
    data[1] = 0x02;
    controller->ops.spi_cmd(codec, data);

    
    data[0] = 0x03;
    data[1] = (WM8785_R2 < 1) | 0x00; // Data Bit-8: 0
    data[2] = (0x1A < 1) & 0xFE; // WM8785 device addr 0011010, ; Bit-0 0: write
    data[0] = 0x04;
    data[1] = 0x03;   

    cmi_printk(("  << wm8785_init\n"));

    return 0;
}

static void wm8785_free(cmi_codec *codec)
{
}

static cmi_codec_ops wm8785_patch_ops = {
    .build_controls = NULL, // wm8785_build_controls,
    .build_pcms     = wm8785_build_pcms,
    .init           = wm8785_init,
    .free           = wm8785_free,
};

static int patch_wm8785(cmi_codec *codec)
{
    codec->patch_ops = wm8785_patch_ops;

    return 0;
}

/*
 * patch entries
 */
codec_preset snd_preset_wm8785[] = {
    { .id = 0xFFFFFFFF, .name = "WM8785", .patch = patch_wm8785 },
    {0,}, /* terminator */
};
