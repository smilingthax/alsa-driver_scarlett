/*
 * Driver for Digigram pcxhr compatible soundcards
 *
 * include file for mixer
 *
 * Copyright (c) 2004 by Digigram <alsa@digigram.com>
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

#ifndef __SOUND_PCXHR_MIXER_H
#define __SOUND_PCXHR_MIXER_H

/* exported */
int pcxhr_update_playback_stream_level(pcxhr_t* chip, int idx, int put_switch_only);
int pcxhr_update_audio_pipe_level(pcxhr_t* chip, int capture, int channel);
int pcxhr_create_mixer(pcxhr_mgr_t* mgr);

#endif /* __SOUND_PCXHR_MIXER_H */