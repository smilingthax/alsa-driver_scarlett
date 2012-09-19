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

static int load_asic(echoaudio_t *chip);
static int dsp_set_digital_mode(echoaudio_t *chip, u8 mode);
static int set_nominal_level(echoaudio_t *chip, u16 index, char consumer);
static int set_digital_mode(echoaudio_t *chip, u8 mode);
static int check_asic_status(echoaudio_t *chip);
static int set_sample_rate(echoaudio_t *chip, u32 rate);
static int set_input_clock(echoaudio_t *chip, u16 clock);
static int set_professional_spdif(echoaudio_t *chip, char prof);
static int set_phantom_power(echoaudio_t *chip, char on);
static int write_control_reg(echoaudio_t *chip, u32 ctl, u32 frq, char force);


static int init_hw(echoaudio_t *chip, u16 device_id, u16 subdevice_id)
{
	int err, i;

	DE_INIT(("init_hw() - Gina3G\n"));
	snd_assert((subdevice_id & 0xfff0) == ECHO3G, return -ENODEV);

	/* This part is common to all the cards */
	if ((err = init_dsp_comm_page(chip))) {
		DE_INIT(("init_hw - could not initialize DSP comm page\n"));
		return err;
	}

	chip->comm_page->e3g_frq_register = __constant_cpu_to_le32((E3G_MAGIC_NUMBER / 48000) - 2);
	chip->device_id = device_id;
	chip->subdevice_id = subdevice_id;
	chip->bad_board = TRUE;
	chip->has_midi = TRUE;
	chip->dsp_code_to_load = &card_fw[FW_GINA3G_DSP];
	chip->input_clock_types =	ECHO_CLOCK_BIT_INTERNAL |
					ECHO_CLOCK_BIT_SPDIF |
					ECHO_CLOCK_BIT_ADAT;
	chip->digital_modes =	ECHOCAPS_HAS_DIGITAL_MODE_SPDIF_RCA |
				ECHOCAPS_HAS_DIGITAL_MODE_SPDIF_OPTICAL |
				ECHOCAPS_HAS_DIGITAL_MODE_ADAT;
	chip->digital_mode = DIGITAL_MODE_SPDIF_RCA;
	chip->professional_spdif = FALSE;
	chip->non_audio_spdif = FALSE;

	/* Load the DSP and the ASIC on the PCI card */
	if ((err = load_firmware(chip)) < 0)
		return err;

	chip->bad_board = FALSE;

	/* Must call this here after DSP is init to init gains and mutes */
	err = init_line_levels(chip);

	/* Set professional nominal levels (FALSE is +4dBu) */
	for (i = 0; i < NUM_ANALOG_BUSSES_OUT; i++)
		err = set_nominal_level(chip, i, FALSE);

	for (i = 0; i < NUM_ANALOG_BUSSES_IN; i++)
		err = set_nominal_level(chip, BX_ANALOG_IN + i, FALSE);

	/* Set the digital mode to S/PDIF RCA */
	set_digital_mode(chip, DIGITAL_MODE_SPDIF_RCA);
	set_phantom_power(chip, 0);

	/* Set the S/PDIF output format to "professional" */
	set_professional_spdif(chip, TRUE);

	DE_INIT(("init_hw done\n"));
	return err;
}



static int set_phantom_power(echoaudio_t *chip, char on)
{
	u32 control_reg = le32_to_cpu(chip->comm_page->control_register);

	if (on)
		control_reg |= E3G_PHANTOM_POWER;
	else
		control_reg &= ~E3G_PHANTOM_POWER;

	chip->phantom_power = on;
	return write_control_reg(chip, control_reg, le32_to_cpu(chip->comm_page->e3g_frq_register), 0);
}

