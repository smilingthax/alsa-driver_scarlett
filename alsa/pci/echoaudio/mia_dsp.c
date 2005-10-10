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


static int set_input_clock(echoaudio_t *chip, u16 clock);
static int set_professional_spdif(echoaudio_t *chip, char prof);
static int set_nominal_level(echoaudio_t *chip, u16 index, char consumer);
static int update_flags(echoaudio_t *chip);
static int set_vmixer_gain(echoaudio_t *chip, u16 output, u16 pipe, int gain);
static int update_vmixer_level(echoaudio_t *chip);


static int init_hw(echoaudio_t *chip, u16 device_id, u16 subdevice_id)
{
	int err;

	DE_INIT(("init_hw() - Mia\n"));
	snd_assert((subdevice_id & 0xfff0) == MIA, return -ENODEV);

	/* This part is common to all the cards */
	if ((err = init_dsp_comm_page(chip))) {
		DE_INIT(("init_hw - could not initialize DSP comm page\n"));
		return err;
	}

	chip->device_id = device_id;
	chip->subdevice_id = subdevice_id;
	chip->bad_board = TRUE;
	chip->dsp_code_to_load = &card_fw[FW_MIA_DSP];
	/* Since this card has no ASIC, mark it as loaded so everything works OK */
	chip->asic_loaded = TRUE;
	if ((subdevice_id & 0x0000f) == MIA_MIDI_REV)
		chip->has_midi = TRUE;
	chip->input_clock_types = ECHO_CLOCK_BIT_INTERNAL | ECHO_CLOCK_BIT_SPDIF;

	/* Load the DSP and the ASIC on the PCI card */
	if ((err = load_firmware(chip)) < 0)
		return err;

	chip->bad_board = FALSE;

	/* Must call this here after DSP is init to init gains and mutes */
	if ((err = init_line_levels(chip)))
		return err;

	/* Default routing of the virtual channels: vchannels 0-3 go to analog
	outputs and vchannels 4-7 go to S/PDIF outputs */
	set_vmixer_gain(chip, 0, 0, 0);
	set_vmixer_gain(chip, 1, 1, 0);
	set_vmixer_gain(chip, 0, 2, 0);
	set_vmixer_gain(chip, 1, 3, 0);
	set_vmixer_gain(chip, 2, 4, 0);
	set_vmixer_gain(chip, 3, 5, 0);
	set_vmixer_gain(chip, 2, 6, 0);
	set_vmixer_gain(chip, 3, 7, 0);
	err = update_vmixer_level(chip);

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
// Mia supports S/PDIF input clock.
//
//===========================================================================

static u32 detect_input_clocks(const echoaudio_t *chip)
{
	u32 clocks_from_dsp, clock_bits;

	/* Map the DSP clock detect bits to the generic driver clock detect bits */
	clocks_from_dsp = le32_to_cpu(chip->comm_page->status_clocks);

	clock_bits = ECHO_CLOCK_BIT_INTERNAL;

	if (clocks_from_dsp & GLDM_CLOCK_DETECT_BIT_SPDIF)
		clock_bits |= ECHO_CLOCK_BIT_SPDIF;

	return clock_bits;
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
// Set the audio sample rate for Mia
//
//===========================================================================

static int set_sample_rate(echoaudio_t *chip, u32 rate)
{
	u32 control_reg;

	switch (rate) {
	case 96000:
		control_reg = MIA_96000;
		break;
	case 88200:
		control_reg = MIA_88200;
		break;
	case 48000:
		control_reg = MIA_48000;
		break;
	case 44100:
		control_reg = MIA_44100;
		break;
	case 32000:
		control_reg = MIA_32000;
		break;
	default:
		DE_ACT(("set_sample_rate: %d invalid!\n", rate));
		return -EINVAL;
	}

	/* Override the clock setting if this Mia is set to S/PDIF clock */
	if (chip->input_clock == ECHO_CLOCK_SPDIF)
		control_reg |= MIA_SPDIF;

	/* Set the control register if it has changed */
	if (control_reg != le32_to_cpu(chip->comm_page->control_register)) {
		if (wait_handshake(chip))
			return -EIO;

		/* Set the values in the comm page */
		chip->comm_page->sample_rate = cpu_to_le32(rate);	/* ignored by the DSP */
		chip->comm_page->control_register = cpu_to_le32(control_reg);
		chip->sample_rate = rate;
		/* Poke the DSP */
		clear_handshake(chip);
		return send_vector(chip, DSP_VC_UPDATE_CLOCKS);
	}
	return 0;
}



//===========================================================================
//
//      Set the input clock
//
//===========================================================================

static int set_input_clock(echoaudio_t *chip, u16 clock)
{
	DE_ACT(("set_input_clock(%d)\n", clock));
	snd_assert(clock == ECHO_CLOCK_INTERNAL || clock == ECHO_CLOCK_SPDIF, return -EINVAL);

	chip->input_clock = clock;
	return set_sample_rate(chip, chip->sample_rate);
}



/* This function routes the sound from a virtual channel to a real output */
static int set_vmixer_gain(echoaudio_t *chip, u16 output, u16 pipe, int gain)
{
	int index;

	snd_assert(pipe < num_pipes_out(chip) && output < num_busses_out(chip), return -EINVAL);

	if (wait_handshake(chip))
		return -EIO;

	chip->vmixer_gain[output][pipe] = gain;
	index = output * num_pipes_out(chip) + pipe;
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



/* Tell the DSP to reread the flags from the comm page */
static int update_flags(echoaudio_t *chip)
{
	if (wait_handshake(chip))
		return -EIO;
	clear_handshake(chip);
	return send_vector(chip, DSP_VC_UPDATE_FLAGS);
}



static int set_professional_spdif(echoaudio_t *chip, char prof)
{
	DE_ACT(("set_professional_spdif %d\n", prof));
	if (prof)
		chip->comm_page->flags |= __constant_cpu_to_le32(DSP_FLAG_PROFESSIONAL_SPDIF);
	else
		chip->comm_page->flags &= ~__constant_cpu_to_le32(DSP_FLAG_PROFESSIONAL_SPDIF);
	chip->professional_spdif = prof;
	return update_flags(chip);
}

