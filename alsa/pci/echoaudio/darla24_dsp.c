// ****************************************************************************
//
//   Copyright Echo Digital Audio Corporation (c) 1998 - 2004
//   All rights reserved
//   www.echoaudio.com
//
//   This file is part of Echo Digital Audio's generic driver library.
//
//   Echo Digital Audio's generic driver library is free software;
//   you can redistribute it and/or modify it under the terms of
//   the GNU General Public License as published by the Free Software Foundation.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program; if not, write to the Free Software
//   Foundation, Inc., 59 Temple Place - Suite 330, Boston,
//   MA  02111-1307, USA.
//
// ****************************************************************************
//
// Translation from C++ and adaptation for use in ALSA-Driver
// were made by Giuliano Pochini <pochini@shiny.it>
//
// ****************************************************************************


static int set_nominal_level(echoaudio_t *chip, u16 index, char consumer);


static int init_hw(echoaudio_t *chip, u16 device_id, u16 subdevice_id)
{
	int err, i;

	DE_INIT(("init_hw() - Darla24\n"));
	snd_assert((subdevice_id & 0xfff0) == DARLA24, return -ENODEV);

	/* This part is common to all the cards */
	if ((err = init_dsp_comm_page(chip))) {
		DE_INIT(("init_hw - could not initialize DSP comm page\n"));
		return err;
	}

	chip->device_id = device_id;
	chip->subdevice_id = subdevice_id;
	chip->bad_board = TRUE;
	chip->dsp_code_to_load = &card_fw[FW_DARLA24_DSP];
	/* Since this card has no ASIC, mark it as loaded so everything works OK */
	chip->asic_loaded = TRUE;
	chip->input_clock_types = ECHO_CLOCK_BIT_INTERNAL | ECHO_CLOCK_BIT_ESYNC;

	/* Load the DSP and the ASIC on the PCI card */
	if ((err = load_firmware(chip)) < 0)
		return err;

	chip->bad_board = FALSE;

	/* Must call this here after DSP is init to init gains and mutes */
	if ((err = init_line_levels(chip)) < 0)
		return err;

	DE_INIT(("init_hw done\n"));
	return err;
}



//===========================================================================
//
// detect_input_clocks returns a bitmask consisting of all the input
// clocks currently connected to the hardware; this changes as the user
// connects and disconnects clock inputs.
//
// You should use this information to determine which clocks the user is
// allowed to select.
//
// Darla24 only supports Esync input clock.
//
//===========================================================================

static u32 detect_input_clocks(const echoaudio_t *chip)
{
	u32 clocks_from_dsp, clock_bits;

	/* Map the DSP clock detect bits to the generic driver clock detect bits */
	clocks_from_dsp = le32_to_cpu(chip->comm_page->status_clocks);

	clock_bits = ECHO_CLOCK_BIT_INTERNAL;

	if (clocks_from_dsp & GLDM_CLOCK_DETECT_BIT_ESYNC)
		clock_bits |= ECHO_CLOCK_BIT_ESYNC;

	return clock_bits;
}



/* The Darla24 has no ASIC. Just do nothing */
static int load_asic(echoaudio_t *chip)
{
	return 0;
}



//===========================================================================
//
// set_sample_rate
//
// Set the audio sample rate for Darla24; this is fairly simple.  You
// just pick the right magic number.
//
//===========================================================================

static int set_sample_rate(echoaudio_t *chip, u32 rate)
{
	u8 clock;

	/* Pick the magic number */
	switch (rate) {
	case 96000:
		clock = GD24_96000;
		break;
	case 88200:
		clock = GD24_88200;
		break;
	case 48000:
		clock = GD24_48000;
		break;
	case 44100:
		clock = GD24_44100;
		break;
	case 32000:
		clock = GD24_32000;
		break;
	case 22050:
		clock = GD24_22050;
		break;
	case 16000:
		clock = GD24_16000;
		break;
	case 11025:
		clock = GD24_11025;
		break;
	case 8000:
		clock = GD24_8000;
		break;
	default:
		DE_ACT(("set_sample_rate: Error, invalid sample rate %d\n", rate));
		return -EINVAL;
	}

	if (wait_handshake(chip))
		return -EIO;

	DE_ACT(("set_sample_rate: %d clock %d\n", rate, clock));
	chip->sample_rate = rate;

	/* Override the sample rate if this card is set to Echo sync. */
	if (chip->input_clock == ECHO_CLOCK_ESYNC)
		clock = GD24_EXT_SYNC;

	chip->comm_page->sample_rate = cpu_to_le32(rate);	/* ignored by the DSP ? */
	chip->comm_page->gd_clock_state = clock;
	clear_handshake(chip);
	return send_vector(chip, DSP_VC_SET_GD_AUDIO_STATE);
}



//===========================================================================
//
// Set input clock
//
// Darla24 supports internal and Esync clock.
//
//===========================================================================

static int set_input_clock(echoaudio_t *chip, u16 clock)
{
	snd_assert(clock == ECHO_CLOCK_INTERNAL || clock == ECHO_CLOCK_ESYNC, return -EINVAL);
	chip->input_clock = clock;
	return set_sample_rate(chip, chip->sample_rate);
}

