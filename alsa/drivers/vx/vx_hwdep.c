#define __NO_VERSION__

/*
 * Driver for Digigram VX soundcards
 *
 * hwdep device manager
 *
 * Copyright (c) 2002 by Takashi Iwai <tiwai@suse.de>
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

#include <sound/driver.h>
#include <sound/core.h>
#include <sound/hwdep.h>
#include "vx_core.h"

static int vx_hwdep_open(snd_hwdep_t *hw, struct file *file)
{
	vx_core_t *vx = snd_magic_cast(vx_core_t, hw->private_data, return -ENXIO);

	down(&vx->hwdep_mutex);
	if (vx->hwdep_used) {
		up(&vx->hwdep_mutex);
		return -EAGAIN;
	}
	vx->hwdep_used++;
	up(&vx->hwdep_mutex);

	return 0;
}

static int vx_hwdep_release(snd_hwdep_t *hw, struct file *file)
{
	vx_core_t *vx = snd_magic_cast(vx_core_t, hw->private_data, return -ENXIO);

	down(&vx->hwdep_mutex);
	vx->hwdep_used--;
	up(&vx->hwdep_mutex);

	return 0;
}

static int vx_hwdep_ioctl(snd_hwdep_t *hw, struct file *file, unsigned int cmd, unsigned long arg)
{
	vx_core_t *vx = snd_magic_cast(vx_core_t, hw->private_data, return -ENXIO);
	int err;

	switch (cmd) {
	case SND_VX_HWDEP_IOCTL_VERSION: {
		struct snd_vx_version info;

		memset(&info, 0, sizeof(info));
		info.type = vx->type;
		info.status = vx->chip_status;
		strncpy(info.name, vx->card->shortname, sizeof(info.name) - 1);
		info.name[sizeof(info.name)-1] = 0;
		info.version = VX_DRIVER_VERSION;
		info.num_codecs = vx->hw->num_codecs;
		info.num_ins = vx->hw->num_ins;
		info.num_outs = vx->hw->num_outs;

		if (copy_to_user((void *)arg, &info, sizeof(info)))
			return -EFAULT;
		return 0;
	}

	case SND_VX_HWDEP_IOCTL_LOAD_XILINX: {
		struct snd_vx_loader loader;

		if (vx->chip_status & VX_STAT_XILINX_LOADED)
			return -EBUSY;
		if (copy_from_user(&loader, (void *)arg, sizeof(loader)))
			return -EFAULT;

		snd_assert(vx->ops->load_xilinx &&
			   vx->ops->test_xilinx, return -ENXIO);

		if (*loader.boot.name)
			snd_printdd("loading xilinx boot: %s\n", loader.boot.name);
		if (*loader.binary.name)
			snd_printdd("loading xilinx image: %s\n", loader.binary.name);
		if ((err = vx->ops->load_xilinx(vx, &loader)) < 0)
			return err;
		if ((err = vx->ops->test_xilinx(vx)) < 0)
			return err;
		vx->chip_status |= VX_STAT_XILINX_LOADED;
		return 0;
	}

	case SND_VX_HWDEP_IOCTL_LOAD_DSP: {
		struct snd_vx_loader loader;

		if (! (vx->chip_status & VX_STAT_XILINX_LOADED))
			return -EIO;
		if (vx->chip_status & VX_STAT_DSP_LOADED)
			return -EIO;
		if (copy_from_user(&loader, (void *)arg, sizeof(loader)))
			return -EFAULT;

		if (*loader.boot.name)
			snd_printdd("loading DSP boot: %s\n", loader.boot.name);
		if (*loader.binary.name)
			snd_printdd("loading DSP image: %s\n", loader.binary.name);
		if ((err = snd_vx_dsp_init(vx, &loader)) < 0)
			return err;

		vx->chip_status |= VX_STAT_DSP_LOADED;
		return 0;
	}

	case SND_VX_HWDEP_IOCTL_INIT_DEVICE:
		if (vx->chip_status & VX_STAT_DEVICE_INIT)
			return -EBUSY;

		if ((err = snd_vx_pcm_new(vx)) < 0)
			return err;

		if ((err = snd_vx_mixer_new(vx)) < 0)
			return err;

		if (vx->ops->add_controls)
			if ((err = vx->ops->add_controls(vx)) < 0)
				return err;

		if ((err = snd_card_register(vx->card)) < 0)
			return err;

		vx->chip_status |= VX_STAT_DEVICE_INIT | VX_STAT_CHIP_INIT;
		return 0;

	case SND_VX_HWDEP_IOCTL_RESUME:

#define VX_STAT_OK	(VX_STAT_XILINX_LOADED|VX_STAT_DSP_LOADED|VX_STAT_DEVICE_INIT)
		if ((vx->chip_status & VX_STAT_OK) != VX_STAT_OK)
			return -EIO;
		if (vx->chip_status & VX_STAT_CHIP_INIT)
			return -EBUSY;

		/* restore the clock and source */
		vx_change_clock_source(vx, vx->clock_source);
		vx_sync_audio_source(vx);

		/* restore the mixer setting */
		/* vx_resume_mixer(vx); */

		vx->chip_status &= ~VX_STAT_RESUMING;
		vx->chip_status |= VX_STAT_CHIP_INIT;
		return 0;

	}

	return -EINVAL;
}


/* exported */
int snd_vx_hwdep_new(vx_core_t *chip)
{
	int err;
	snd_hwdep_t *hw;

	if ((err = snd_hwdep_new(chip->card, "VX Loader", 0, &hw)) < 0)
		return err;

	init_MUTEX(&chip->hwdep_mutex);
	hw->iface = SNDRV_HWDEP_IFACE_VX_LOADER;
	hw->private_data = chip;
	hw->ops.open = vx_hwdep_open;
	hw->ops.ioctl = vx_hwdep_ioctl;
	hw->ops.release = vx_hwdep_release;

	return 0;
}
