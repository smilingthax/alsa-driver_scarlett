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


static int set_vmixer_gain(echoaudio_t *chip, u16 output, u16 pipe, int gain);
static int update_vmixer_level(echoaudio_t *chip);


static int init_hw(echoaudio_t *chip, u16 device_id, u16 subdevice_id)
{
	int err;

	DE_INIT(("init_hw() - Indigo IO\n"));
	snd_assert((subdevice_id & 0xfff0) == INDIGO_IO, return -ENODEV);

	/* This part is common to all the cards */
	if ((err = init_dsp_comm_page(chip))) {
		DE_INIT(("init_hw - could not initialize DSP comm page\n"));
		return err;
	}

	chip->device_id = device_id;
	chip->subdevice_id = subdevice_id;
	chip->bad_board = TRUE;
	chip->dsp_code_to_load = &card_fw[FW_INDIGO_IO_DSP];
	/* Since this card has no ASIC, mark it as loaded so everything works OK */
	chip->asic_loaded = TRUE;
	chip->input_clock_types = ECHO_CLOCK_BIT_INTERNAL;

	/* Load the DSP and the ASIC on the PCI card */
	if ((err = load_firmware(chip)) < 0)
		return err;

	chip->bad_board = FALSE;

	/* Must call this here after DSP is init to init gains and mutes */
	if ((err = init_line_levels(chip)) < 0)
		return err;

	/* Default routing of the virtual channels: all vchannels are routed
	to the stereo output */
	set_vmixer_gain(chip, 0, 0, 0);
	set_vmixer_gain(chip, 1, 1, 0);
	set_vmixer_gain(chip, 0, 2, 0);
	set_vmixer_gain(chip, 1, 3, 0);
	set_vmixer_gain(chip, 0, 4, 0);
	set_vmixer_gain(chip, 1, 5, 0);
	set_vmixer_gain(chip, 0, 6, 0);
	set_vmixer_gain(chip, 1, 7, 0);
	update_vmixer_level(chip);

	DE_INIT(("init_hw done\n"));
	return err;
}



static u32 detect_input_clocks(const echoaudio_t *chip)
{
	return ECHO_CLOCK_BIT_INTERNAL;
}



/* The Mia has no ASIC. Just do nothing */
static int load_asic(echoaudio_t *chip)
{
	return 0;
}



/****************************************************************************

	Hardware setup and config

 ****************************************************************************/

//===========================================================================
//
// set_sample_rate
//
// Set the audio sample rate for IndigoIO
//
//===========================================================================

static int set_sample_rate(echoaudio_t *chip, u32 rate)
{
	if (wait_handshake(chip))
		return -EIO;

	chip->sample_rate = rate;
	chip->comm_page->sample_rate = cpu_to_le32(rate);
	clear_handshake(chip);
	return send_vector(chip, DSP_VC_UPDATE_CLOCKS);
}



/* This function routes the sound from a virtual channel to a real output */
static int set_vmixer_gain(echoaudio_t *chip, u16 output, u16 pipe, int gain)
{
	int index;

	snd_assert(pipe < NUM_PIPES_OUT && output < NUM_BUSSES_OUT, return -EINVAL);

	if (wait_handshake(chip))
		return -EIO;

	chip->vmixer_gain[output][pipe] = gain;
	index = output * NUM_PIPES_OUT + pipe;
	chip->comm_page->vmixer[index] = gain;

	DE_ACT(("set_vmixer_gain: pipe %d, out %d = %d\n", pipe, output, gain));
	return 0;
}



/* Tell the DSP to read and update virtual mixer levels in comm page. */
static int update_vmixer_level(echoaudio_t *chip)
{
	if (wait_handshake(chip))
		return -EIO;
	clear_handshake(chip);
	return send_vector(chip, DSP_VC_SET_VMIXER_GAIN);
}

