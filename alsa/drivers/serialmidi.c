/*
 *  Generic driver for serial MIDI adapters
 *  Copyright (c) by Jaroslav Kysela <perex@suse.cz>
 *
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
 */
/*
 *  The snd_adaptor module parameter allows you to select:
 *	0 - Roland SoundCanvas (use snd_outs paratemer to specify count of output ports)
 */

#include <sound/driver.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/tty.h>
#include <asm/uaccess.h>
#include <sound/core.h>
#include <sound/rawmidi.h>
#define SNDRV_GET_ID
#include <sound/initval.h>
#include <linux/delay.h>

#define SNDRV_SERIAL_MAX_OUTS		16 /* min 16 */

#define SERIAL_ADAPTOR_SOUNDCANVAS 	0  /* Roland Soundcanvas; F5 NN selects part */
#define SERIAL_ADAPTOR_MS124T		1  /* Midiator MS-124T */
#define SERIAL_ADAPTOR_MS124W_SA	2  /* Midiator MS-124W in S/A mode */
#define SERIAL_ADAPTOR_MS124W_MB	3  /* Midiator MS-124W in M/B mode */
#define SERIAL_ADAPTOR_MAX		SERIAL_ADAPTOR_MS124W_MB

EXPORT_NO_SYMBOLS;

MODULE_AUTHOR("Jaroslav Kysela <perex@suse.cz>");
MODULE_DESCRIPTION("Serial MIDI");
MODULE_LICENSE("GPL");
MODULE_CLASSES("{sound}");

static int snd_index[SNDRV_CARDS] = SNDRV_DEFAULT_IDX;		/* Index 0-MAX */
static char *snd_id[SNDRV_CARDS] = SNDRV_DEFAULT_STR;		/* ID for this card */
static int snd_enable[SNDRV_CARDS] = SNDRV_DEFAULT_ENABLE;	/* Enable this card */
static char *snd_sdev[SNDRV_CARDS] = {"/dev/ttyS0", [1 ... (SNDRV_CARDS - 1)] = ""}; /* serial device */
static int snd_speed[SNDRV_CARDS] = {[0 ... (SNDRV_CARDS - 1)] = 38400}; /* 9600,19200,38400,57600,115200 */
static int snd_adaptor[SNDRV_CARDS] = {[0 ... (SNDRV_CARDS - 1)] = SERIAL_ADAPTOR_SOUNDCANVAS};
static int snd_outs[SNDRV_CARDS] = {[0 ... (SNDRV_CARDS - 1)] = 1};     /* 1 to 16 */

MODULE_PARM(snd_index, "1-" __MODULE_STRING(SNDRV_CARDS) "i");
MODULE_PARM_DESC(snd_index, "Index value for MPU-401 device.");
MODULE_PARM_SYNTAX(snd_index, SNDRV_INDEX_DESC);
MODULE_PARM(snd_id, "1-" __MODULE_STRING(SNDRV_CARDS) "s");
MODULE_PARM_DESC(snd_id, "ID string for MPU-401 device.");
MODULE_PARM_SYNTAX(snd_id, SNDRV_ID_DESC);
MODULE_PARM(snd_enable, "1-" __MODULE_STRING(SNDRV_CARDS) "i");
MODULE_PARM_DESC(snd_enable, "Enable MPU-401 device.");
MODULE_PARM_SYNTAX(snd_enable, SNDRV_ENABLE_DESC);
MODULE_PARM(snd_speed, "1-" __MODULE_STRING(SNDRV_CARDS) "i");
MODULE_PARM_DESC(snd_speed, "Speed in bauds.");
MODULE_PARM_SYNTAX(snd_speed, SNDRV_ENABLED ",allows:{9600,19200,38400,57600,115200},dialog:list");
MODULE_PARM(snd_adaptor, "1-" __MODULE_STRING(SNDRV_CARDS) "i");
MODULE_PARM_DESC(snd_adaptor, "Type of adaptor.");
MODULE_PARM_SYNTAX(snd_adaptor, SNDRV_ENABLED ",allows:{{0=Soundcanvas,1=MS-124T,2=MS-124W S/A,3=MS-124W M/B}},dialog:list");
MODULE_PARM(snd_outs, "1-" __MODULE_STRING(SNDRV_CARDS) "i");
MODULE_PARM_DESC(snd_outs, "Number of MIDI outputs.");
MODULE_PARM_SYNTAX(snd_outs, SNDRV_ENABLED ",allows:{{1,16}},dialog:list");

#define SERIAL_MODE_NOT_OPENED		(0)
#define SERIAL_MODE_BIT_INPUT		(0)
#define SERIAL_MODE_BIT_OUTPUT		(1)
#define SERIAL_MODE_BIT_INPUT_TRIGGERED	(2)
#define SERIAL_MODE_BIT_OUTPUT_TRIGGERED (3)

typedef struct _snd_serialmidi {
	snd_card_t *card;
	char *sdev;			/* serial device name (e.g. /dev/ttyS0) */
	unsigned int speed;		/* speed in bauds */
	unsigned int adaptor;		/* see SERIAL_ADAPTOR_ */
	unsigned int mode;		/* see SERIAL_MODE_* */
	unsigned int outs;		/* count of outputs */
	unsigned char prev_status[SNDRV_SERIAL_MAX_OUTS];
	snd_rawmidi_t *rmidi;		/* rawmidi device */
	snd_rawmidi_substream_t *substream_input;
	snd_rawmidi_substream_t *substream_output;
	struct file *file;
	struct tty_struct *tty;
	struct semaphore open_lock;
	void (*old_receive_buf)(struct tty_struct *, const unsigned char *cp, char *fp, int count);
	void (*old_write_wakeup)(struct tty_struct *);
	int old_exclusive;
	int old_low_latency;
} serialmidi_t;

static snd_card_t *snd_serialmidi_cards[SNDRV_CARDS] = SNDRV_DEFAULT_PTR;

static void ldisc_receive_buf(struct tty_struct *, const unsigned char *cp, char *fp, int count);
static void ldisc_write_wakeup(struct tty_struct *);

static int tty_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct inode * inode = file->f_dentry->d_inode;
	mm_segment_t fs;
	int retval;

	if (file->f_op == NULL)
		return -ENXIO;
	fs = get_fs();
	set_fs(get_ds());
	retval = file->f_op->ioctl(inode, file, cmd, arg);
	set_fs(fs);
	return retval;
}

static int open_tty(serialmidi_t *serial, unsigned int mode)
{
	int retval = 0;
	struct tty_struct *tty;
	struct termios old_termios, *ntermios;
	int ldisc, speed, cflag;

	down(&serial->open_lock);
	if (serial->tty) {
		set_bit(mode, &serial->mode);
		goto __end;
	}
	if (IS_ERR(serial->file = filp_open(serial->sdev, O_RDWR|O_NONBLOCK, 0))) {
		retval = PTR_ERR(serial->file);
		serial->file = NULL;
		goto __end;
	}
	tty = (struct tty_struct *)serial->file->private_data;
	if (tty == NULL || tty->magic != TTY_MAGIC) {
		snd_printk(KERN_ERR "device %s has not valid tty", serial->sdev);
		retval = -EIO;
		goto __end;
	}
	if (tty->driver.set_termios == NULL) {
		snd_printk(KERN_ERR "tty %s has not set_termios", serial->sdev);
		retval = -EIO;
		goto __end;
	}
	if (tty->count > 1) {
		snd_printk(KERN_ERR "tty %s is already used", serial->sdev);
		retval = -EBUSY;
		goto __end;
	}
	
	/* select N_TTY line discipline (for sure) */
	ldisc = N_TTY;
	if ((retval = tty_ioctl(serial->file, TIOCSETD, (unsigned long)&ldisc)) < 0) {
		snd_printk(KERN_ERR "TIOCSETD (N_TTY) failed for tty %s", serial->sdev);
		goto __end;
	}

	/* sanity check, we use disc_data for own purposes */
	if (tty->disc_data != NULL) {
		snd_printk(KERN_ERR "disc_data are used for tty %s", serial->sdev);
		goto __end;
	}
	
	switch (serial->speed) {
	case 9600:
		speed = B9600;
		break;
	case 19200:
		speed = B19200;
		break;
	case 38400:
		speed = B38400;
		break;
	case 57600:
		speed = B57600;
		break;
	case 115200:
	default:
		speed = B115200;
		break;
	}

	cflag = speed | CREAD | CSIZE | CS8 | CRTSCTS | HUPCL;

	switch (serial->adaptor) {
	case SERIAL_ADAPTOR_MS124W_SA:
	case SERIAL_ADAPTOR_MS124W_MB:
	case SERIAL_ADAPTOR_MS124T:
		
		break;
	}
	
	old_termios = *tty->termios;
	ntermios = tty->termios;
	ntermios->c_lflag = NOFLSH;
	ntermios->c_iflag = IGNBRK | IGNPAR;
	ntermios->c_oflag = 0;
	ntermios->c_cflag = cflag;
	ntermios->c_cc[VEOL] = 0; /* '\r'; */
	ntermios->c_cc[VERASE] = 0;
	ntermios->c_cc[VKILL] = 0;
	ntermios->c_cc[VMIN] = 0;
	ntermios->c_cc[VTIME] = 0;
	(*tty->driver.set_termios)(tty, &old_termios);
	serial->tty = tty;

	/* some magic here, we need own receive_buf */
	/* it would be probably better to create own line discipline */
	/* but this solution is sufficient at the time */
	tty->disc_data = serial;
	serial->old_receive_buf = tty->ldisc.receive_buf;
	tty->ldisc.receive_buf = ldisc_receive_buf;
	serial->old_write_wakeup = tty->ldisc.write_wakeup;
	tty->ldisc.write_wakeup = ldisc_write_wakeup;
	serial->old_low_latency = tty->low_latency;
	tty->low_latency = 1;
	serial->old_exclusive = test_bit(TTY_EXCLUSIVE, &tty->flags);
	set_bit(TTY_EXCLUSIVE, &tty->flags);

	set_bit(mode, &serial->mode);
	retval = 0;

      __end:
      	if (retval < 0) {
      		if (serial->file) {
      			filp_close(serial->file, 0);
      			serial->file = NULL;
      		}
      	}
	up(&serial->open_lock);
	return retval;
}

static int close_tty(serialmidi_t *serial, unsigned int mode)
{
	unsigned int imode = mode == SERIAL_MODE_BIT_INPUT ?
			SERIAL_MODE_BIT_OUTPUT : SERIAL_MODE_BIT_INPUT;
	struct tty_struct *tty;

	down(&serial->open_lock);
	clear_bit(mode, &serial->mode);
	if (test_bit(imode, &serial->mode))
		goto __end;
	tty = serial->tty;
	if (tty->disc_data == serial)
		tty->disc_data = NULL;
	if (tty && tty->ldisc.receive_buf == ldisc_receive_buf) {
		tty->low_latency = serial->old_low_latency;
		tty->ldisc.receive_buf = serial->old_receive_buf;
		if (serial->old_exclusive)
			set_bit(TTY_EXCLUSIVE, &tty->flags);
		else
			clear_bit(TTY_EXCLUSIVE, &tty->flags);
	}
	filp_close(serial->file, 0);
	serial->tty = NULL;
	serial->file = NULL;
      __end:
	up(&serial->open_lock);
	return 0;
}

static void ldisc_receive_buf(struct tty_struct *tty, const unsigned char *cp, char *fp, int count)
{
	serialmidi_t *serial = snd_magic_cast(serialmidi_t, tty->disc_data, return);

	if (serial == NULL)
		return;
	if (!test_bit(SERIAL_MODE_BIT_INPUT_TRIGGERED, &serial->mode))
		return;
	snd_rawmidi_receive(serial->substream_input, cp, count);
}

static void tx_loop(serialmidi_t *serial)
{
	struct tty_struct *tty;
	char buf[64];
	int count;

	tty = serial->tty;
	count = tty->driver.write_room(tty);
	while (count > 0) {
		printk("write_room: %i\n", count);
		count = count > sizeof(buf) ? sizeof(buf) : count;
		count = snd_rawmidi_transmit_peek(serial->substream_output, buf, count);
		printk("peek: %i\n", count);
		if (count > 0) {
			count = tty->driver.write(tty, 0, buf, count);
			printk("written: %i\n", count);
			snd_rawmidi_transmit_ack(serial->substream_output, count);
			count = tty->driver.write_room(tty);
		} else {
			clear_bit(TTY_DO_WRITE_WAKEUP, &tty->flags);
			break;
		}
	}
}

static void ldisc_write_wakeup(struct tty_struct *tty)
{
	serialmidi_t *serial = snd_magic_cast(serialmidi_t, tty->disc_data, return);

	tx_loop(serial);
}

static void snd_serialmidi_output_trigger(snd_rawmidi_substream_t * substream, int up)
{
	serialmidi_t *serial = snd_magic_cast(serialmidi_t, substream->rmidi->private_data, return);
	
	if (up) {
		set_bit(SERIAL_MODE_BIT_OUTPUT_TRIGGERED, &serial->mode);
		set_bit(TTY_DO_WRITE_WAKEUP, &serial->tty->flags);
		tx_loop(serial);
	} else {
		clear_bit(TTY_DO_WRITE_WAKEUP, &serial->tty->flags);
		clear_bit(SERIAL_MODE_BIT_OUTPUT_TRIGGERED, &serial->mode);
	}
}

static void snd_serialmidi_input_trigger(snd_rawmidi_substream_t * substream, int up)
{
	serialmidi_t *serial = snd_magic_cast(serialmidi_t, substream->rmidi->private_data, return);

	if (up) {
		set_bit(SERIAL_MODE_BIT_INPUT_TRIGGERED, &serial->mode);
	} else {
		clear_bit(SERIAL_MODE_BIT_INPUT_TRIGGERED, &serial->mode);
	}
}

static int snd_serialmidi_output_open(snd_rawmidi_substream_t * substream)
{
	serialmidi_t *serial = snd_magic_cast(serialmidi_t, substream->rmidi->private_data, return -ENXIO);
	int err;

	if ((err = open_tty(serial, SERIAL_MODE_BIT_OUTPUT)) < 0)
		return err;
	serial->substream_output = substream;
	return 0;
}

static int snd_serialmidi_output_close(snd_rawmidi_substream_t * substream)
{
	serialmidi_t *serial = snd_magic_cast(serialmidi_t, substream->rmidi->private_data, return -ENXIO);

	serial->substream_output = NULL;
	return close_tty(serial, SERIAL_MODE_BIT_OUTPUT);
}

static int snd_serialmidi_input_open(snd_rawmidi_substream_t * substream)
{
	serialmidi_t *serial = snd_magic_cast(serialmidi_t, substream->rmidi->private_data, return -ENXIO);
	int err;

	if ((err = open_tty(serial, SERIAL_MODE_BIT_INPUT)) < 0)
		return err;
	serial->substream_input = substream;
	return 0;
}

static int snd_serialmidi_input_close(snd_rawmidi_substream_t * substream)
{
	serialmidi_t *serial = snd_magic_cast(serialmidi_t, substream->rmidi->private_data, return -ENXIO);

	serial->substream_input = NULL;
	return close_tty(serial, SERIAL_MODE_BIT_INPUT);
}

static snd_rawmidi_ops_t snd_serialmidi_output =
{
	.open =		snd_serialmidi_output_open,
	.close =	snd_serialmidi_output_close,
	.trigger =	snd_serialmidi_output_trigger,
};

static snd_rawmidi_ops_t snd_serialmidi_input =
{
	.open =		snd_serialmidi_input_open,
	.close =	snd_serialmidi_input_close,
	.trigger =	snd_serialmidi_input_trigger,
};

static int snd_serialmidi_free(serialmidi_t *serial)
{
	if (serial->sdev);
		kfree(serial->sdev);
	snd_magic_kfree(serial);
	return 0;
}

static int snd_serialmidi_dev_free(snd_device_t *device)
{
	serialmidi_t *serial = snd_magic_cast(serialmidi_t, device->device_data, return -ENXIO);
	return snd_serialmidi_free(serial);
}

static int __init snd_serialmidi_rmidi(serialmidi_t *serial)
{
        snd_rawmidi_t *rrawmidi;
        int err;

        if ((err = snd_rawmidi_new(serial->card, "UART Serial MIDI", 0, serial->outs, 1, &rrawmidi)) < 0)
                return err;
        snd_rawmidi_set_ops(rrawmidi, SNDRV_RAWMIDI_STREAM_INPUT, &snd_serialmidi_input);
        snd_rawmidi_set_ops(rrawmidi, SNDRV_RAWMIDI_STREAM_OUTPUT, &snd_serialmidi_output);
        sprintf(rrawmidi->name, "Serial MIDI #0");
        rrawmidi->info_flags = SNDRV_RAWMIDI_INFO_OUTPUT |
                               SNDRV_RAWMIDI_INFO_INPUT |
                               SNDRV_RAWMIDI_INFO_DUPLEX;
        rrawmidi->private_data = serial;
	serial->rmidi = rrawmidi;
        return 0;
}

static int __init snd_serialmidi_create(snd_card_t *card, const char *sdev,
					unsigned int speed, unsigned int adaptor,
					unsigned int outs, serialmidi_t **rserial)
{
	static snd_device_ops_t ops = {
		.dev_free =	snd_serialmidi_dev_free,
	};
	serialmidi_t *serial;
	int err;

	if (outs < 1)
		outs = 1;
	if (outs > 16)
		outs = 16;
	switch (adaptor) {
	case SERIAL_ADAPTOR_SOUNDCANVAS:
		break;
	case SERIAL_ADAPTOR_MS124T:
	case SERIAL_ADAPTOR_MS124W_SA:
		outs = 1;
		break;
	case SERIAL_ADAPTOR_MS124W_MB:
		outs = 16;
	default:
		snd_printk(KERN_ERR "Adaptor type is out of range 0-%d (%d)\n",
				SERIAL_ADAPTOR_MAX, adaptor);
		return -ENODEV;
	}

	if ((serial = snd_magic_kcalloc(serialmidi_t, 0, GFP_KERNEL)) == NULL)
		return -ENOMEM;

	init_MUTEX(&serial->open_lock);
	serial->card = card;
	serial->sdev = snd_kmalloc_strdup(sdev, GFP_KERNEL);
	if (serial->sdev == NULL) {
		snd_serialmidi_free(serial);
		return -ENOMEM;
	}
	serial->adaptor = adaptor;
	serial->speed = speed;
	serial->outs = outs;
	memset(serial->prev_status, 0x80, sizeof(unsigned char) * SNDRV_SERIAL_MAX_OUTS);
	
	/* Register device */
	if ((err = snd_device_new(card, SNDRV_DEV_LOWLEVEL, serial, &ops)) < 0) {
		snd_serialmidi_free(serial);
		return err;
	}
	
	if ((err = snd_serialmidi_rmidi(serial)) < 0) {
		snd_device_free(card, serial);
		return err;
	}
	
	if (rserial)
		*rserial = serial;
	return 0;
}

static int __init snd_card_serialmidi_probe(int dev)
{
	snd_card_t *card;
	serialmidi_t *serial;
	int err;

	card = snd_card_new(snd_index[dev], snd_id[dev], THIS_MODULE, 0);
	if (card == NULL)
		return -ENOMEM;
	strcpy(card->driver, "Serial MIDI");
	strcpy(card->shortname, card->driver);

	if ((err = snd_serialmidi_create(card,
					 snd_sdev[dev],
					 snd_speed[dev],
					 snd_adaptor[dev],
					 snd_outs[dev],
					 &serial)) < 0) {
		snd_card_free(card);
		return err;
	}

	sprintf(card->longname, "%s at %s", card->shortname, snd_sdev[dev]);
	if ((err = snd_card_register(card)) < 0) {
		snd_card_free(card);
		return err;
	}
	snd_serialmidi_cards[dev] = card;
	return 0;
}

static int __init alsa_card_serialmidi_init(void)
{
	int dev, cards = 0;

	for (dev = 0; dev < SNDRV_CARDS; dev++) {
		if (!snd_enable[dev])
			continue;
		if (snd_card_serialmidi_probe(dev) >= 0)
			cards++;
	}
	if (!cards) {
#ifdef MODULE
		printk(KERN_ERR "serial MIDI device not found or device busy\n");
#endif
		return -ENODEV;
	}
	return 0;
}

static void __exit alsa_card_serialmidi_exit(void)
{
	int idx;

	for (idx = 0; idx < SNDRV_CARDS; idx++)
		snd_card_free(snd_serialmidi_cards[idx]);
}

module_init(alsa_card_serialmidi_init)
module_exit(alsa_card_serialmidi_exit)

#ifndef MODULE

/* format is: snd-serialmidi=snd_enable,snd_index,snd_id,
			     snd_sdev,snd_speed,snd_adaptor,snd_outs */

static int __init alsa_card_serialmidi_setup(char *str)
{
	static unsigned __initdata nr_dev = 0;

	if (nr_dev >= SNDRV_CARDS)
		return 0;
	(void)(get_option(&str,&snd_enable[nr_dev]) == 2 &&
	       get_option(&str,&snd_index[nr_dev]) == 2 &&
	       get_id(&str,&snd_id[nr_dev]) == 2 &&
	       get_id(&str,&snd_sdev[nr_dev]) == 2 &&
	       get_option(&str,&snd_speed[nr_dev]) == 2 &&
	       get_option(&str,&snd_adaptor[nr_dev]) == 2 &&
	       get_option(&str,&snd_outs[nr_dev]) == 2);
	nr_dev++;
	return 1;
}

__setup("snd-serialmidi=", alsa_card_serialmidi_setup);

#endif /* ifndef MODULE */
