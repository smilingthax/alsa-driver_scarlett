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

#if PAGE_SIZE < 4096
#error PAGE_SIZE is < 4k
#endif

static int restore_dsp_rettings(echoaudio_t *chip);


//===========================================================================
//
// Some vector commands involve the DSP reading or writing data to and
// from the comm page; if you send one of these commands to the DSP,
// it will complete the command and then write a non-zero value to
// the Handshake field in the comm page.  This function waits for the
// handshake to show up.
//
//===========================================================================

static int wait_handshake(echoaudio_t *chip)
{
	int i;

	/* Wait up to 10ms for the handshake from the DSP */
	for (i = 0; i < HANDSHAKE_TIMEOUT; i++) {
		/* Look for the handshake value */
		if (chip->comm_page->handshake) {
			//if (i)  DE_ACT(("Handshake time: %d\n", i));
			return 0;
		}
		udelay(1);
	}

	snd_printk(KERN_ERR "wait_handshake(): Timeout waiting for DSP\n");
	return -EBUSY;
}



//===========================================================================
//
// Much of the interaction between the DSP and the driver is done via vector
// commands; send_vector writes a vector command to the DSP.  Typically,
// this causes the DSP to read or write fields in the comm page.
//
// * PCI posting is not required thanks to the handshake logic
//
// Returns 0 if sent OK.
//
//===========================================================================

static int send_vector(echoaudio_t *chip, u32 command)
{
	int i;

	wmb();	/* Flush all pending writes before sending the command */

	/* Wait up to 100ms for the "vector busy" bit to be off */
	for (i = 0; i < VECTOR_BUSY_TIMEOUT; i++) {
		if (!(get_dsp_register(chip, CHI32_VECTOR_REG) & CHI32_VECTOR_BUSY)) {
			set_dsp_register(chip, CHI32_VECTOR_REG, command);
			//if (i)  DE_ACT(("send_vector time: %d\n", i));
			return 0;
		}
		udelay(1);
	}

	DE_ACT((KERN_ERR "timeout on send_vector\n"));
	return -EBUSY;
}



//===========================================================================
//
// write_dsp writes a 32-bit value to the DSP; this is used almost
// exclusively for loading the DSP.
//
//===========================================================================

static int write_dsp(echoaudio_t *chip, u32 data)
{
	u32 status, i;

	for (i = 0; i < 10000000; i++) {	/* timeout = 10s */
		status = get_dsp_register(chip, CHI32_STATUS_REG);
		if ((status & CHI32_STATUS_HOST_WRITE_EMPTY) != 0) {
			set_dsp_register(chip, CHI32_DATA_REG, data);
			wmb();			/* write it immediately */
			return 0;
		}
		udelay(1);
		cond_resched();
	}

	chip->bad_board = TRUE;		/* Set TRUE until DSP re-loaded */
	DE_ACT((KERN_ERR "write_dsp: Set bad_board to TRUE\n"));
	return -EIO;
}



//===========================================================================
//
// read_dsp reads a 32-bit value from the DSP; this is used almost
// exclusively for loading the DSP and checking the status of the ASIC.
//
//===========================================================================

static int read_dsp(echoaudio_t *chip, u32 *data)
{
	u32 status, i;

	for (i = 0; i < READ_DSP_TIMEOUT; i++) {
		status = get_dsp_register(chip, CHI32_STATUS_REG);
		if ((status & CHI32_STATUS_HOST_READ_FULL) != 0) {
			*data = get_dsp_register(chip, CHI32_DATA_REG);
			return 0;
		}
		udelay(1);
		cond_resched();
	}

	chip->bad_board = TRUE;		/* Set TRUE until DSP re-loaded */
	DE_INIT((KERN_ERR "read_dsp: Set bad_board to TRUE\n"));
	return -EIO;
}



/****************************************************************************

	Firmware loading functions

 ****************************************************************************/

//===========================================================================
//
// This function is used to read back the serial number from the DSP;
// this is triggered by the SET_COMMPAGE_ADDR command.
//
// Only some early Echogals products have serial numbers in the ROM;
// the serial number is not used, but you still need to do this as
// part of the DSP load process.
//
//===========================================================================

static int read_sn(echoaudio_t *chip)
{
	int i;
	u32 sn[6];

	for (i = 0; i < 5; i++) {
		if (read_dsp(chip, &sn[i])) {
			snd_printk((KERN_ERR "Failed to read serial number\n"));
			return -EIO;
		}
	}
	DE_INIT(("Read serial number %08x %08x %08x %08x %08x\n", sn[0], sn[1], sn[2], sn[3], sn[4]));
	return 0;
}



#ifndef ECHOCARD_HAS_ASIC
/* This card has no ASIC, just return ok */
static inline int check_asic_status(echoaudio_t *chip)
{
	chip->asic_loaded = TRUE;
	return 0;
}

#endif /* !ECHOCARD_HAS_ASIC */



#ifdef ECHOCARD_HAS_ASIC

// Load ASIC code - done after the DSP is loaded
static int load_asic_generic(echoaudio_t *chip, u32 cmd, const struct firmware *asic)
{
	const struct firmware *fw;
	int err;
	u32 i, size;
	u8 *code;

	if ((err = get_firmware(&fw, asic, chip)) < 0) {
		snd_printk(KERN_WARNING "Firmware not found !\n");
		return err;
	}

	code = (u8 *)fw->data;
	size = fw->size;

	/* Send the "Here comes the ASIC" command */
	if (write_dsp(chip, cmd) < 0)
		return -EIO;

	/* Write length of ASIC file in bytes */
	if (write_dsp(chip, size) < 0)
		return -EIO;

	for (i = 0; i < size; i++) {
		if (write_dsp(chip, code[i]) < 0) {
			DE_ACT(("failed on write_dsp\n"));
			free_firmware(fw);
			return -EIO;
		}
	}

	DE_INIT(("ASIC loaded\n"));
	free_firmware(fw);
	return 0;
}

#endif /* ECHOCARD_HAS_ASIC */



//===========================================================================
//
// install_resident_loader
//
// Install the resident loader for 56361 DSPs;  The resident loader
// is on the EPROM on the board for 56301 DSP.
//
// The resident loader is a tiny little program that is used to load
// the real DSP code.
//
//===========================================================================

#ifdef DSP_56361

static int install_resident_loader(echoaudio_t *chip)
{
	u32 address;
	int index, words, i;
	u16 *code;
	u32 status;
	const struct firmware *fw;

	/* 56361 cards only!  This check is required by the old 56301-based
	Mona and Gina24 */
	if (chip->device_id != DEVICE_ID_56361)
		return 0;

	/* Look to see if the resident loader is present.  If the resident
	loader is already installed, host flag 5 will be on. */
	status = get_dsp_register(chip, CHI32_STATUS_REG);
	if (status & CHI32_STATUS_REG_HF5) {
		DE_INIT(("Resident loader already installed; status is 0x%x\n", status));
		return 0;
	}
	/* Set DSP format bits for 24 bit mode */
	set_dsp_register(chip, CHI32_CONTROL_REG, get_dsp_register(chip, CHI32_CONTROL_REG) | 0x900);

	//---------------------------------------------------------------------------
	//
	// Loader
	//
	// The DSP code is an array of 16 bit words.  The array is divided up into
	// sections.  The first word of each section is the size in words, followed
	// by the section type.
	//
	// Since DSP addresses and data are 24 bits wide, they each take up two
	// 16 bit words in the array.
	//
	// This is a lot like the other loader loop, but it's not a loop,
	// you don't write the memory type, and you don't write a zero at the end.
	//
	//---------------------------------------------------------------------------

	if ((i = get_firmware(&fw, &card_fw[FW_361_LOADER], chip)) < 0) {
		snd_printk(KERN_WARNING "Firmware not found !\n");
		return i;
	}

	code = (u16 *)fw->data;

	/* Skip the header section; the first word in the array is the size
	of the first section, so the first real section of code is pointed
	to by Code[0]. */
	index = code[0];

	/* Skip the section size, LRS block type, and DSP memory type */
	index += 3;

	/* Get the number of DSP words to write */
	words = code[index++];

	/* Get the DSP address for this block; 24 bits, so build from two words */
	address = ((u32)code[index] << 16) + code[index + 1];
	index += 2;

	/* Write the count to the DSP */
	if (write_dsp(chip, words)) {
		DE_INIT(("install_resident_loader: Failed to write word count!\n"));
		goto irl_error;
	}
	/* Write the DSP address */
	if (write_dsp(chip, address)) {
		DE_INIT(("install_resident_loader: Failed to write DSP address!\n"));
		goto irl_error;
	}
	/* Write out this block of code to the DSP */
	for (i = 0; i < words; i++) {
		u32 data;

		data = ((u32)code[index] << 16) + code[index + 1];
		if (write_dsp(chip, data)) {
			DE_INIT(("install_resident_loader: Failed to write DSP code\n"));
			goto irl_error;
		}
		index += 2;
	}

	/* Wait for flag 5 to come up */
	for (i = 0; i < 200; i++) {	/* Timeout is 50us * 200 = 10ms */
		udelay(50);
		status = get_dsp_register(chip, CHI32_STATUS_REG);
		if (status & CHI32_STATUS_REG_HF5)
			break;
	}

	if (i == 200) {
		DE_INIT(("Resident loader failed to set HF5\n"));
		goto irl_error;
	}

	DE_INIT(("Resident loader successfully installed\n"));
	free_firmware(fw);
	return 0;

irl_error:
	free_firmware(fw);
	return -EIO;
}

#endif /* DSP_56361 */


//===========================================================================
//
// load_dsp
//
// This loads the DSP code.
//
//===========================================================================

static int load_dsp(echoaudio_t *chip, u16 *code)
{
	u32 address, data;
	int index, words, i;

	if (chip->dsp_code == code) {
		DE_INIT(("DSP is already loaded!\n"));
		return 0;
	}
	chip->bad_board = TRUE;		/* Set TRUE until DSP loaded */
	chip->dsp_code = NULL;		/* Current DSP code not loaded */
	chip->asic_loaded = FALSE;	/* Loading the DSP code will reset the ASIC */

	DE_INIT(("load_dsp: Set bad_board to TRUE\n"));

	/* If this board requires a resident loader, install it. */
#ifdef DSP_56361
	if ((i = install_resident_loader(chip)) < 0)
		return i;
#endif

	/* Send software reset command */
	if (send_vector(chip, DSP_VC_RESET) < 0) {
		DE_INIT(("LoadDsp: send_vector DSP_VC_RESET failed, Critical Failure\n"));
		return -EIO;
	}
	/* Delay 10us */
	udelay(10);

	/* Wait 10ms for HF3 to indicate that software reset is complete */
	for (i = 0; i < 1000; i++) {	/* Timeout is 10us * 1000 = 10ms */
		if (get_dsp_register(chip, CHI32_STATUS_REG) & CHI32_STATUS_REG_HF3)
			break;
		udelay(10);
	}

	if (i == 1000) {
		DE_INIT(("load_dsp: Timeout waiting for CHI32_STATUS_REG_HF3\n"));
		return -EIO;
	}

	/* Set DSP format bits for 24 bit mode now that soft reset is done */
	set_dsp_register(chip, CHI32_CONTROL_REG, get_dsp_register(chip, CHI32_CONTROL_REG) | 0x900);

	//---------------------------------------------------------------------------
	// Main loader loop
	//---------------------------------------------------------------------------
	index = code[0];

	for (;;) {
		int block_type, mem_type;

		/* Total Block Size */
		index++;

		/* Block Type */
		block_type = code[index];
		if (block_type == 4)	/* We're finished */
			break;

		index++;

		/* Memory Type  P=0,X=1,Y=2 */
		mem_type = code[index++];

		/* Block Code Size */
		words = code[index++];
		if (words == 0)		/* We're finished */
			break;

		/* Start Address */
		address = ((u32)code[index] << 16) + code[index + 1];
		index += 2;

		if (write_dsp(chip, words) < 0) {
			DE_INIT(("load_dsp: failed to write number of DSP words\n"));
			return -EIO;
		}
		if (write_dsp(chip, address) < 0) {
			DE_INIT(("load_dsp: failed to write DSP address\n"));
			return -EIO;
		}
		if (write_dsp(chip, mem_type) < 0) {
			DE_INIT(("load_dsp: failed to write DSP memory type\n"));
			return -EIO;
		}
		/* Code */
		for (i = 0; i < words; i++, index+=2) {
			data = ((u32)code[index] << 16) + code[index + 1];
			if (write_dsp(chip, data) < 0) {
				DE_INIT(("load_dsp: failed to write DSP data\n"));
				return -EIO;
			}
		}
	}

	if (write_dsp(chip, 0) < 0) {	/* We're done!!! */
		DE_INIT(("load_dsp: Failed to write final zero\n"));
		return -EIO;
	}
	udelay(10);

	for (i = 0; i < 5000; i++) {	/* Timeout is 100us * 5000 = 500ms */
		/* Wait for flag 4 - indicates that the DSP loaded OK */
		if (get_dsp_register(chip, CHI32_STATUS_REG) & CHI32_STATUS_REG_HF4) {
			set_dsp_register(chip, CHI32_CONTROL_REG, get_dsp_register(chip, CHI32_CONTROL_REG) & ~0x1b00);

			if (write_dsp(chip, DSP_FNC_SET_COMMPAGE_ADDR) < 0) {
				DE_INIT(("load_dsp: Failed to write DSP_FNC_SET_COMMPAGE_ADDR\n"));
				return -EIO;
			}

			if (write_dsp(chip, chip->comm_page_phys) < 0) {
				DE_INIT(("load_dsp: Failed to write comm page address\n"));
				return -EIO;
			}

			/* Get the serial number via slave mode.
			 * This is triggered by the SET_COMMPAGE_ADDR command.
			 * We don't actually use the serial number but we have to get
			 * it as part of the DSP init voodoo. */
			if (read_sn(chip) < 0) {
				DE_INIT(("load_dsp: Failed to read serial number\n"));
				return -EIO;
			}

			chip->dsp_code = code;		/* Show which DSP code loaded */
			chip->bad_board = FALSE;	/* DSP OK */
			DE_INIT(("load_dsp: OK!\n"));
			return 0;
		}
		udelay(100);
	}

	DE_INIT(("load_dsp: DSP load timed out waiting for HF4\n"));
	return -EIO;
}



//===========================================================================
//
// load_firmware takes care of loading the DSP and any ASIC code.
//
//===========================================================================

static int load_firmware(echoaudio_t *chip)
{
	const struct firmware *fw;
	int err;

	snd_assert(chip->dsp_code_to_load && chip->comm_page, return -EPERM);

	/* See if the ASIC is present and working - only if the DSP is already loaded */
	if (chip->dsp_code) {
		if (check_asic_status(chip) == 0)
			return 0;
		/* ASIC check failed; force the DSP to reload */
		chip->dsp_code = NULL;
	}

	if ((err = get_firmware(&fw, chip->dsp_code_to_load, chip)) < 0)
		return err;
	err = load_dsp(chip, (u16 *)fw->data);
	free_firmware(fw);
	if (err < 0)
		return err;

	/* Load the ASIC if the DSP load succeeded */
	if ((err = load_asic(chip)) < 0)
		return err;

	return restore_dsp_rettings(chip);
}



/****************************************************************************

  Mixer functions

 ****************************************************************************/

#if defined(ECHOCARD_HAS_INPUT_NOMINAL_LEVEL) || defined(ECHOCARD_HAS_OUTPUT_NOMINAL_LEVEL)

/* Set the nominal level for an input or output bus (true = -10dBV, false = +4dBu) */
static int set_nominal_level(echoaudio_t *chip, u16 index, char consumer)
{
	snd_assert(index < NUM_BUSSES_OUT + NUM_BUSSES_IN, return -EINVAL);

	/* Wait for the handshake (OK even if ASIC is not loaded) */
	if (wait_handshake(chip))
		return -EIO;

	chip->nominal_level[index] = consumer;

	if (consumer)
		chip->comm_page->nominal_level_mask |= cpu_to_le32(1 << index);
	else
		chip->comm_page->nominal_level_mask &= ~cpu_to_le32(1 << index);

	return 0;
}

#endif /* ECHOCARD_HAS_*_NOMINAL_LEVEL */



/* Set the gain for a single physical output channel (dB).
ECHOGAIN_MINOUT <= gain <= ECHOGAIN_MAXOUT) */
static int set_output_gain(echoaudio_t *chip, u16 channel, signed short gain)
{
	snd_assert(channel < NUM_BUSSES_OUT, return -EINVAL);

	if (wait_handshake(chip))
		return -EIO;

	/* Save the new value */
	chip->output_gain[channel] = gain;
	chip->comm_page->line_out_level[channel] = gain;
	return 0;
}



#ifdef ECHOCARD_HAS_MONITOR
/* Set the monitor level from an input bus to an output bus.
ECHOGAIN_MINOUT <= gain <= ECHOGAIN_MAXOUT) */
static int set_monitor_gain(echoaudio_t *chip, u16 output, u16 input, signed short gain)
{
	snd_assert(output < NUM_BUSSES_OUT && input < NUM_BUSSES_IN, return -EINVAL);

	if (wait_handshake(chip))
		return -EIO;

	chip->monitor_gain[output][input] = gain;
	chip->comm_page->monitors[MONITOR_INDEX(output, input)] = gain;
	return 0;
}
#endif /* ECHOCARD_HAS_MONITOR */


/* Tell the DSP to read and update output, nominal & monitor levels in comm page. */
static int update_output_line_level(echoaudio_t *chip)
{
	if (wait_handshake(chip))
		return -EIO;
	clear_handshake(chip);
	return send_vector(chip, DSP_VC_UPDATE_OUTVOL);
}



/* Tell the DSP to read and update input levels in comm page */
static int update_input_line_level(echoaudio_t *chip)
{
	if (wait_handshake(chip))
		return -EIO;
	clear_handshake(chip);
	return send_vector(chip, DSP_VC_UPDATE_INGAIN);
}



/* set_meters_on turns the meters on or off.  If meters are turned on, the DSP
will write the meter and clock detect values to the comm page at about 30Hz */
static void set_meters_on(echoaudio_t *chip, char on)
{
	if (on && !chip->meters_enabled) {
		send_vector(chip, DSP_VC_METERS_ON);
		chip->meters_enabled = 1;
	} else if (!on && chip->meters_enabled) {
		send_vector(chip, DSP_VC_METERS_OFF);
		chip->meters_enabled = 0;
		memset((s8 *)chip->comm_page->vu_meter, ECHOGAIN_MUTED, DSP_MAXPIPES);
		memset((s8 *)chip->comm_page->peak_meter, ECHOGAIN_MUTED, DSP_MAXPIPES);
	}
}



//===========================================================================
//
// Fill out an the given array using the current values in the comm page.
//
// Meters are written in the comm page by the DSP in this order:
//
// Output busses
// Input busses
// Output pipes (vmixer cards only)
//
// This function assumes there are no more than 16 in/out busses or pipes
// (maximum is ECHO_MAXAUDIO*, but currently there no Echoaudio cards
// have so many channels. Meters is an array [3][16][2])
//
//===========================================================================
static void get_audio_meters(echoaudio_t *chip, long *meters)
{
	int i, m, n;

	m = 0;
	n = 0;
	for (i = 0; i < NUM_BUSSES_OUT; i++, m++) {
		meters[n++] = chip->comm_page->vu_meter[m];
		meters[n++] = chip->comm_page->peak_meter[m];
	}
	for (; n < 32; n++)
		meters[n] = 0;

	for (i = 0; i < NUM_BUSSES_IN; i++, m++) {
		meters[n++] = chip->comm_page->vu_meter[m];
		meters[n++] = chip->comm_page->peak_meter[m];
	}
	for (; n < 64; n++)
		meters[n] = 0;

#ifdef ECHOCARD_HAS_VMIXER
	for (i = 0; i < NUM_PIPES_OUT; i++, m++) {
		meters[n++] = chip->comm_page->vu_meter[m];
		meters[n++] = chip->comm_page->peak_meter[m];
	}
#endif
	for (; n < 96; n++)
		meters[n] = 0;
}



//===========================================================================
//
// This is called after load_firmware to restore old gains, meters on, monitors, etc.
//
//===========================================================================

static int restore_dsp_rettings(echoaudio_t *chip)
{
	int err;
	DE_INIT(("restore_dsp_settings\n"));

	if ((err = check_asic_status(chip)) < 0)
		return err;

	/* @ Gina20/Darla20 only. Should be harmless for other cards. */
	chip->comm_page->gd_clock_state = GD_CLOCK_UNDEF;
	chip->comm_page->gd_spdif_status = GD_SPDIF_STATUS_UNDEF;
	chip->comm_page->handshake = 0xffffffff;

	if ((err = set_sample_rate(chip, chip->sample_rate)) < 0)
		return err;

	if (chip->meters_enabled)
		if (send_vector(chip, DSP_VC_METERS_ON) < 0)
			return -EIO;

#ifdef ECHOCARD_HAS_EXTERNAL_CLOCK
	if (set_input_clock(chip, chip->input_clock) < 0)
		return -EIO;
#endif

#ifdef ECHOCARD_HAS_OUTPUT_CLOCK_SWITCH
	if (set_output_clock(chip, chip->output_clock) < 0)
		return -EIO;
#endif

	if (update_output_line_level(chip) < 0)
		return -EIO;

	if (update_input_line_level(chip) < 0)
		return -EIO;

#ifdef ECHOCARD_HAS_VMIXER
	if (update_vmixer_level(chip) < 0)
		return -EIO;
#endif

	if (wait_handshake(chip) < 0)
		return -EIO;
	clear_handshake(chip);

	DE_INIT(("restore_dsp_rettings done\n"));
	return send_vector(chip, DSP_VC_UPDATE_FLAGS);
}



/****************************************************************************

	Transport functions

 ****************************************************************************/

/* set_audio_format() sets the format of the audio data in host memory for
this pipe.  Note that _MS_ (mono-to-stereo) playback modes are not used by ALSA
but they are here because they are just mono while capturing */
static void set_audio_format(echoaudio_t *chip, u16 pipe_index, const audioformat_t *format)
{
	u16 dsp_format;

	dsp_format = DSP_AUDIOFORM_SS_16LE;

	/* Look for super-interleave (no big-endian and 8 bits) */
	if (format->interleave > 2) {
		switch (format->bits_per_sample) {
		case 16:
			dsp_format = DSP_AUDIOFORM_SUPER_INTERLEAVE_16LE;
			break;
		case 24:
			dsp_format = DSP_AUDIOFORM_SUPER_INTERLEAVE_24LE;
			break;
		case 32:
			dsp_format = DSP_AUDIOFORM_SUPER_INTERLEAVE_32LE;
			break;
		}
		dsp_format |= format->interleave;
	} else if (format->data_are_bigendian) {
		/* For big-endian data, only 32 bit samples are supported */
		switch (format->interleave) {
		case 1:
			dsp_format = DSP_AUDIOFORM_MM_32BE;
			break;
#ifdef ECHOCARD_HAS_STEREO_BIG_ENDIAN32
		case 2:
			dsp_format = DSP_AUDIOFORM_SS_32BE;
			break;
#endif
		}
	} else if (format->interleave == 1 && format->bits_per_sample == 32 && !format->mono_to_stereo) {
		/* 32 bit little-endian mono->mono case */
		dsp_format = DSP_AUDIOFORM_MM_32LE;
	} else {
		/* Handle the other little-endian formats */
		switch (format->bits_per_sample) {
		case 8:
			if (format->interleave == 2)
				dsp_format = DSP_AUDIOFORM_SS_8;
			else
				dsp_format = DSP_AUDIOFORM_MS_8;
			break;
		default:
		case 16:
			if (format->interleave == 2)
				dsp_format = DSP_AUDIOFORM_SS_16LE;
			else
				dsp_format = DSP_AUDIOFORM_MS_16LE;
			break;
		case 24:
			if (format->interleave == 2)
				dsp_format = DSP_AUDIOFORM_SS_24LE;
			else
				dsp_format = DSP_AUDIOFORM_MS_24LE;
			break;
		case 32:
			if (format->interleave == 2)
				dsp_format = DSP_AUDIOFORM_SS_32LE;
			else
				dsp_format = DSP_AUDIOFORM_MS_32LE;
			break;
		}
	}
	DE_ACT(("set_audio_format[%d] = %x\n", pipe_index, dsp_format));
	chip->comm_page->audio_format[pipe_index] = cpu_to_le16(dsp_format);
}



/* start_transport starts transport for a set of pipes.
   The bits 1 in channel_mask specify what pipes to start. Only the bit of the
   first channel must be set, regardless its interleave. Same for Pause and Stop. */
static int start_transport(echoaudio_t *chip, u32 channel_mask, u32 cyclic_mask)
{
	DE_ACT(("start_transport %x\n", channel_mask));

	/* Wait for the previous command to complete */
	if (wait_handshake(chip))
		return -EIO;

	/* Write the appropriate fields in the comm page */
	chip->comm_page->cmd_start |= cpu_to_le32(channel_mask);

	if (chip->comm_page->cmd_start) {
		clear_handshake(chip);
		send_vector(chip, DSP_VC_START_TRANSFER);
		/* Wait for transport to start */
		if (wait_handshake(chip))
			return -EIO;
		/* Keep track of which pipes are transporting */
		chip->active_mask |= channel_mask;
		chip->comm_page->cmd_start = 0;
		return 0;
	}

	DE_ACT(("start_transport: No pipes to start!\n"));
	return -EINVAL;
}



/* pause_transport pauses transport for a set of pipes */
static int pause_transport(echoaudio_t *chip, u32 channel_mask)
{
	DE_ACT(("pause_transport %x\n", channel_mask));

	/* Wait for the last command to finish */
	if (wait_handshake(chip))
		return -EIO;

	/* Write to the comm page */
	chip->comm_page->cmd_stop |= cpu_to_le32(channel_mask);
	chip->comm_page->cmd_reset = 0;
	if (chip->comm_page->cmd_stop) {
		/* Clear the handshake and send the vector command */
		clear_handshake(chip);
		send_vector(chip, DSP_VC_STOP_TRANSFER);
		/* Wait for transport to stop */
		if (wait_handshake(chip))
			return -EIO;
		/* Keep track of which pipes are transporting */
		chip->active_mask &= ~channel_mask;
		chip->comm_page->cmd_stop = 0;
		chip->comm_page->cmd_reset = 0;
		return 0;
	}

	DE_ACT(("pause_transport: No pipes to stop!\n"));
	return 0;
}



/* stop_transport resets transport for a set of pipes */
static int stop_transport(echoaudio_t *chip, u32 channel_mask)
{
	DE_ACT(("stop_transport %x\n", channel_mask));

	/* Wait for the last command to finish */
	if (wait_handshake(chip))
		return -EIO;

	/* Write to the comm page */
	chip->comm_page->cmd_stop |= cpu_to_le32(channel_mask);
	chip->comm_page->cmd_reset |= cpu_to_le32(channel_mask);
	if (chip->comm_page->cmd_reset) {
		/* Clear the handshake and send the vector command */
		clear_handshake(chip);
		send_vector(chip, DSP_VC_STOP_TRANSFER);
		/* Wait for transport to stop */
		if (wait_handshake(chip))
			return -EIO;
		/* Keep track of which pipes are transporting */
		chip->active_mask &= ~channel_mask;
		chip->comm_page->cmd_stop = 0;
		chip->comm_page->cmd_reset = 0;
		return 0;
	}

	DE_ACT(("stop_transport: No pipes to stop!\n"));
	return 0;
}



static inline int is_pipe_allocated(echoaudio_t *chip, u16 pipe_index)
{
	return (chip->pipe_alloc_mask & (1 << pipe_index));
}



/* Stops everything and turns off the DSP. All pipes should be already
   stopped and unallocated. */
static int rest_in_peace(echoaudio_t *chip)
{
	DE_ACT(("rest_in_peace() open=%x\n", chip->pipe_alloc_mask));

	/* Stops all active pipes (just to be sure) */
	stop_transport(chip, chip->active_mask);

	set_meters_on(chip, FALSE);

#ifdef ECHOCARD_HAS_MIDI
	enable_midi_input(chip, FALSE);
#endif

	/* Go to sleep */
	if (chip->dsp_code) {
		/* Make load_firmware do a complete reload */
		chip->dsp_code = NULL;
		/* Put the DSP to sleep */
		return send_vector(chip, DSP_VC_GO_COMATOSE);
	}
	return 0;
}



/* Fills the comm page with default values */
static int init_dsp_comm_page(echoaudio_t *chip)
{
	int i;

	/* Check if the compiler added extra padding inside the structure */
	if (offsetof(comm_page_t, midi_output) != 0xbe0) {
		DE_INIT(("init_dsp_comm_page() - Invalid comm_page_t structure\n"));
		return -EPERM;
	}

	/* Init all the basic stuff */
	chip->bad_board = TRUE;	/* Set TRUE until DSP loaded */
	chip->dsp_code = NULL;	/* Current DSP code not loaded */
	chip->digital_mode = DIGITAL_MODE_NONE;
	chip->input_clock = ECHO_CLOCK_INTERNAL;
	chip->output_clock = ECHO_CLOCK_WORD;
	chip->asic_loaded = FALSE;
	memset(chip->comm_page, 0, sizeof(comm_page_t));

	/* Init the comm page */
	chip->comm_page->comm_size = __constant_cpu_to_le32(sizeof(comm_page_t));
	chip->comm_page->handshake = 0xffffffff;
	chip->comm_page->midi_out_free_count = __constant_cpu_to_le32(DSP_MIDI_OUT_FIFO_SIZE);
	chip->comm_page->sample_rate = __constant_cpu_to_le32(44100);
	chip->sample_rate = 44100;
	for (i = 0; i < DSP_MAXAUDIOINPUTS; i++)
		chip->comm_page->line_in_level[i] = 0;

	/* Set line levels so we don't blast any inputs on startup */
	memset(chip->comm_page->monitors, ECHOGAIN_MUTED, MONITOR_ARRAY_SIZE);
	memset(chip->comm_page->vmixer, ECHOGAIN_MUTED, VMIXER_ARRAY_SIZE);

	return 0;
}



/*===========================================================================
//
// This function initializes the several volume controls for busses and pipes.
// This MUST be called after the DSP is up and running !
//
===========================================================================*/

static int init_line_levels(echoaudio_t *chip)
{
	int st, i, o;

	DE_INIT(("init_line_levels\n"));

	/* Mute output busses */
	for (i = 0; i < NUM_BUSSES_OUT; i++)
		if ((st = set_output_gain(chip, i, ECHOGAIN_MUTED)))
			return st;
	if ((st = update_output_line_level(chip)))
		return st;

#ifdef ECHOCARD_HAS_VMIXER
	/* Mute the Vmixer */
	for (i = 0; i < NUM_PIPES_OUT; i++)
		for (o = 0; o < NUM_BUSSES_OUT; o++)
			if ((st = set_vmixer_gain(chip, o, i, ECHOGAIN_MUTED)))
				return st;
	if ((st = update_vmixer_level(chip)))
		return st;
#endif /* ECHOCARD_HAS_VMIXER */

#ifdef ECHOCARD_HAS_MONITOR
	/* Mute the monitor mixer */
	for (o = 0; o < NUM_BUSSES_OUT; o++)
		for (i = 0; i < NUM_BUSSES_IN; i++)
			if ((st = set_monitor_gain(chip, o, i, ECHOGAIN_MUTED)))
				return st;
	if ((st = update_output_line_level(chip)))
		return st;
#endif /* ECHOCARD_HAS_MONITOR */

#ifdef ECHOCARD_HAS_INPUT_GAIN
	for (i = 0; i < NUM_BUSSES_IN; i++)
		if ((st = set_input_gain(chip, i, ECHOGAIN_MUTED)))
			return st;
	if ((st = update_input_line_level(chip)))
		return st;
#endif /* ECHOCARD_HAS_INPUT_GAIN */

	return 0;
}



/*===========================================================================
//
// This isn't the interrupt handler itself; rather, the OS-specific layer
// of the driver has an interrupt handler that calls this function.
//
// Returns -1 if the IRQ is not ours, 0 if ack, N > 0 if there are N midi data
//
===========================================================================*/

static int service_irq(echoaudio_t *chip)
{
	int st;

	/* Read the DSP status register and see if this DSP generated this interrupt */
	if (get_dsp_register(chip, CHI32_STATUS_REG) & CHI32_STATUS_IRQ) {
		st = 0;
#ifdef ECHOCARD_HAS_MIDI
		/* If this was a MIDI input interrupt, get the MIDI input data */
		if (chip->comm_page->midi_input[0])	/* The count is at index 0 */
			st = midi_service_irq(chip);	/* Returns how many midi bytes we received */
#endif
		/* Clear the hardware interrupt */
		chip->comm_page->midi_input[0] = 0;
		send_vector(chip, DSP_VC_ACK_INT);
		return st;
	}
	return -1;
}




/******************************************************************************

 Functions for opening and closing pipes

 ******************************************************************************/

/*===========================================================================
//
// allocate_pipes is used to reserve audio pipes for your exclusive use.
// The call will fail if some pipes are already allocated.
//
===========================================================================*/

static int allocate_pipes(echoaudio_t *chip, audiopipe_t *audiopipe, u16 pipe_index, u16 interleave)
{
	int i;
	u32 channel_mask;
	char is_cyclic;

	DE_ACT(("allocate_pipes: ch=%d int=%d\n", pipe_index, interleave));

	if (chip->bad_board)
		return -EIO;

	is_cyclic = 1;	/* This driver uses cyclic buffers only */

	/* Compute channel mask for this substream */
	for (channel_mask = i = 0; i < interleave; i++)
		channel_mask |= 1 << (pipe_index + i);
	/* Check if the specified pipes are already open */
	if (chip->pipe_alloc_mask & channel_mask) {
		DE_ACT(("allocate_pipes: channel already open\n"));
		return -EAGAIN;
	}

	chip->comm_page->position[pipe_index] = 0;
	chip->pipe_alloc_mask |= channel_mask;
	if (is_cyclic)
		chip->pipe_cyclic_mask |= channel_mask;
	audiopipe->pipe_index = pipe_index;
	audiopipe->interleave = interleave;
	audiopipe->state = PIPE_STATE_STOPPED;

	/* The counter register is where the DSP writes the 32 bit DMA
	position for a pipe.  The DSP is constantly updating this value as
	it moves data. The DMA counter is in units of bytes, not samples. */
	audiopipe->dma_counter = &chip->comm_page->position[pipe_index];
	*audiopipe->dma_counter = 0;
	DE_ACT(("allocate_pipes: ok\n"));
	return pipe_index;
}



/*===========================================================================
//
// free_pipes is, naturally, the inverse of allocate_pipes.
//
===========================================================================*/

static int free_pipes(echoaudio_t *chip, audiopipe_t *audiopipe)
{
	u32 channel_mask;
	int i;

	DE_ACT(("free_pipes: Pipe %d\n", audiopipe->pipe_index));
	snd_assert(is_pipe_allocated(chip, audiopipe->pipe_index), return -EINVAL);

	/* Compute channel mask for this substream */
	for (channel_mask = i = 0; i < audiopipe->interleave; i++)
		channel_mask |= 1 << (audiopipe->pipe_index + i);

	/* Audio should be already stopped here */
	snd_assert(audiopipe->state == PIPE_STATE_STOPPED, return -EINVAL);

	chip->pipe_alloc_mask &= ~channel_mask;
	chip->pipe_cyclic_mask &= ~channel_mask;
	return 0;
}



/******************************************************************************

 Functions for managing the scatter-gather list

******************************************************************************/

static int sglist_init(echoaudio_t *chip, audiopipe_t *audiopipe)
{
	audiopipe->sglist_head = 0;
	memset(audiopipe->sgpage.area, 0, PAGE_SIZE);
	chip->comm_page->sglist_addr[audiopipe->pipe_index].addr = cpu_to_le32(audiopipe->sgpage.addr);
	return 0;
}



static int sglist_add_mapping(echoaudio_t *chip, audiopipe_t *audiopipe, dma_addr_t address, size_t length)
{
	int head = audiopipe->sglist_head;
	sg_entry_t *list = (sg_entry_t *)audiopipe->sgpage.area;

	if (head < MAX_SGLIST_ENTRIES - 1) {
		list[head].addr = cpu_to_le32(address);
		list[head].size = cpu_to_le32(length);
		audiopipe->sglist_head++;
	} else {
		DE_ACT(("SGlist: too many fragments\n"));
		return -ENOMEM;
	}
	return 0;
}



static inline int sglist_add_irq(echoaudio_t *chip, audiopipe_t *audiopipe)
{
	return sglist_add_mapping(chip, audiopipe, 0, 0);
}



static inline int sglist_wrap(echoaudio_t *chip, audiopipe_t *audiopipe)
{
	return sglist_add_mapping(chip, audiopipe, audiopipe->sgpage.addr, 0);
}
