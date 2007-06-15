/*
 *  patch_ak5385.c - Driver for C-Media CMI8788 PCI soundcards.
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
 * audio interface patch for AK5385A
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
 * 没有寄存器，不用专门的控制，只需要设置 CMI8788 就可以了
 */

/*
 * Analog playback callbacks
 */
static int ak5385a_playback_pcm_open(void                *hinfo,
                                     cmi_codec           *codec,
                                     struct snd_pcm_substream *substream )
{
    // 待完善 需要设置相关的寄存器
    cmi8788_pcm_stream  *pcm_stream = (cmi8788_pcm_stream  *)hinfo;
        
    return 0;
}

static int ak5385a_playback_pcm_prepare(void                *hinfo,
                                        cmi_codec           *codec,
                                        struct snd_pcm_substream *substream )
{
    // 待完善 需要设置相关的寄存器
    cmi8788_pcm_stream  *pcm_stream = (cmi8788_pcm_stream  *)hinfo;
    
    return 0;
}

static int ak5385a_playback_pcm_cleanup(void                *hinfo,
                                        cmi_codec           *codec,
                                        struct snd_pcm_substream *substream )
{
    // 待完善 需要设置相关的寄存器
    cmi8788_pcm_stream  *pcm_stream = (cmi8788_pcm_stream  *)hinfo;
    
    return 0;
}

/*
 * Analog capture
 */
static int ak5385a_capture_pcm_prepare(void                *hinfo,
                                       cmi_codec           *codec,
                                       struct snd_pcm_substream *substream )
{
    // 待完善 需要设置相关的寄存器
    cmi8788_pcm_stream  *pcm_stream = (cmi8788_pcm_stream  *)hinfo;
    
    return 0;
}

static int ak5385a_capture_pcm_cleanup(void                *hinfo,
                                       cmi_codec           *codec,
                                       struct snd_pcm_substream *substream )
{
    // 待完善 需要设置相关的寄存器
    cmi8788_pcm_stream  *pcm_stream = (cmi8788_pcm_stream  *)hinfo;
    
    return 0;
}

/*
 */
static cmi8788_pcm_stream ak5385a_pcm_analog_playback = {
    .channels = 2,
    .ops = {
        .open    = ak5385a_playback_pcm_open,
        .prepare = ak5385a_playback_pcm_prepare,
        .cleanup = ak5385a_playback_pcm_cleanup
    },
};

static cmi8788_pcm_stream ak5385a_pcm_analog_capture = {
    .channels = 2,
    .ops = {
        .prepare = ak5385a_capture_pcm_prepare,
        .cleanup = ak5385a_capture_pcm_cleanup
    },
};


static int ak5385a_build_pcms(cmi_codec *codec)
{
    cmi8788_pcm_stream  *pcm_substream = codec->pcm_substream;

    pcm_substream[0] = ak5385a_pcm_analog_playback;
    pcm_substream[1] = ak5385a_pcm_analog_capture;

    return 0;
}

/*
 * mixer
 */
static int put_volume(cmi_codec *codec, int l_vol, int r_vol)
{
    u8 data[2]={0,0};
    u8 volume = 0;

    if(!codec)
        return -1;

    cmi8788_controller *controller = codec->controller;
    if(!controller)
        return -1;

    return 0;
}

/*
 * The ak5385a does not support read command.
 */
static int get_volume(cmi_codec *codec, int *l_vol, int *r_vol)
{
    return -1;
}

static cmi8788_mixer_ops  ak5385a_mixer_ops = 
{
//  .get_volume = get_volume,
    .set_volume = put_volume,
};

/*
 * create mixer
 */
static int ak5385a_build_controls(cmi_codec *codec)
{
    //
    if(!codec)
        return -1;

    codec->mixer_ops = ak5385a_mixer_ops;

    return 0;
}

static int ak5385a_init(cmi_codec *codec)
{
    //
    u8 data[2]={0,0};
    if(!codec)
        return -1;

    cmi8788_controller *controller = codec->controller;
    if(!controller)
        return -1;

    codec->addr = CODEC_ADR_AK5385A; // 0
    codec->reg_len_flag = 0;   

    return 0;
}

static void ak5385a_free(cmi_codec *codec)
{
    // 待完善
}

static cmi_codec_ops ak5385a_patch_ops = {
    .build_controls = NULL, // ak5385a_build_controls,
    .build_pcms     = NULL, // ak5385a_build_pcms,
    .init           = NULL, // ak5385a_init,
    .free           = NULL, // ak5385a_free,
};

static int patch_ak5385a(cmi_codec *codec)
{
    codec->patch_ops = ak5385a_patch_ops;

    return 0;
}

/*
 * patch entries
 */
codec_preset snd_preset_ak5385a[] = {
    { .id = 0xFFFFFFFF, .name = "AK5385A", .patch = patch_ak5385a },
    {0,}, /* terminator */
};
