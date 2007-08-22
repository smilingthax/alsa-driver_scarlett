#define __NO_VERSION__
/*
 *  cmi_lib.c - Driver for C-Media CMI8788 PCI soundcards.
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
 */

#include <sound/driver.h>
#include <linux/delay.h>
#include <asm/io.h>
#include "cmi8788.h"


/* read/write operations for 32-bit register */
void snd_cmipci_write(struct cmi8788 *chip, unsigned int data, unsigned int cmd)
{
	outl(data, chip->addr + cmd);
}

unsigned int snd_cmipci_read(struct cmi8788 *chip, unsigned int cmd)
{
	return inl(chip->addr + cmd);
}

/* read/write operations for 16-bit register */
void snd_cmipci_write_w(struct cmi8788 *chip, unsigned short data, unsigned int cmd)
{
	outw(data, chip->addr + cmd);
}

unsigned short snd_cmipci_read_w(struct cmi8788 *chip, unsigned int cmd)
{
	return inw(chip->addr + cmd);
}

/* read/write operations for 8-bit register */
void snd_cmipci_write_b(struct cmi8788 *chip, unsigned char data, unsigned int cmd)
{
	outb(data, chip->addr + cmd);
}

unsigned char snd_cmipci_read_b(struct cmi8788 *chip, unsigned int cmd)
{
	return inb(chip->addr + cmd);
}


/*
 * Interface for send controller command to codec
 */

/* send a command by SPI interface
 * The data (which include address, r/w, and data bits) written to or read from the codec.
 * The bits in this register should be interpreted according to the individual codec.
 */
int snd_cmi_send_spi_cmd(struct cmi_codec *codec, u8 *data)
{
	struct cmi8788 *chip = codec->chip;
	u8 ctrl;

	snd_cmipci_write_b(chip, data[0], SPI_Data + 0);
	snd_cmipci_write_b(chip, data[1], SPI_Data + 1);
	if (codec->reg_len_flag) /* 3bytes */
		snd_cmipci_write_b(chip, data[2], SPI_Data + 2);

	ctrl = snd_cmipci_read_b(chip, SPI_Ctrl);

	/* codec select Bit 6:4 (0-5 XSPI_CEN0 - XSPI_CEN5) */
	ctrl &= 0x8f; /* Bit6:4 clear 0 */
	ctrl |= (codec->addr << 4) & 0x70; /* set Bit6:4 codec->addr. codec index XSPI_CEN 0-5 */

	/* SPI clock period */
	/* The data length of read/write */
	ctrl &= 0xfd; /* 1101 Bit-2 */
	if (codec->reg_len_flag) /* 3Byte */
		ctrl |= 0x02;

	/* Bit 0 Write 1 to trigger read/write operation */
	ctrl |= 0x01;

	snd_cmipci_write_b(chip, ctrl, SPI_Ctrl);
	udelay(50);
	return 0;
}

#if 0
/*
 * send a command by 2-wire interface
 */
static int snd_cmi_send_2wire_cmd(struct cmi_codec *codec, u8 reg_addr, u16 reg_data)
{
	struct cmi8788 *chip = codec->chip;
	u8 Status = 0;

	Status = snd_cmipci_read_b(chip, BusCtrlStatus);
	if ((Status & 0x01) == 1) /* busy */
		return -1;

	snd_cmipci_write_b(chip, reg_addr, MAPReg);
	snd_cmipci_write_w(chip, reg_data, DataReg);

	/* bit7-1 The target slave device address */
	/* bit0 1: read, 0: write */
	snd_cmipci_write_b(chip, codec->addr << 1, SlaveAddrCtrl);

	return 0;
}
#endif

/*
 * send AC'97 command, control AC'97 CODEC register
 */
void snd_cmi_send_ac97_cmd(struct cmi8788 *chip, u8 reg, u16 value)
{
	/* 31:25 Reserved */
	/* 24    R/W  Codec Command Select 1: Front-Panel Codec 0: On-Card Codec */
	/* 23    R/W  Read/Write Select 1:Read 0:Write */
	/* 22:16 R/W  CODEC Register Address */
	/* 15:0  R/W  CODEC Register Data This is the data that is written to the selected CODEC register */
	/*            when the write operation is selected. Reading this field will return the last value received from CODEC. */
	snd_cmipci_write(chip, (reg << 16) | value, AC97InChanCfg2);
	udelay(150);
}

#if 0
/* receive a response */
static unsigned int snd_cmi_get_response(struct cmi_codec *codec)
{
	/* ŽýÍêÉÆ */
	return 0;
}
#endif
