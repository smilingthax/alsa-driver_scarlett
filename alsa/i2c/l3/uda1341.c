/*
 * Philips UDA1341 mixer device driver
 *
 * Copyright (c) 2002 Tomas Kasparek <tomas.kasparek@seznam.cz>
 *
 * Portions are Copyright (C) 2000 Lernout & Hauspie Speech Products, N.V.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License.
 *
 * History:
 *
 * 2002-03-13	Tomas Kasparek	Initial release - based on uda1341.c from OSS
 *
 */

#include <sound/driver.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/ioctl.h>
#include <linux/errno.h>

#include <asm/uaccess.h>

#include <sound/core.h>
#include <sound/control.h>
#include <sound/initval.h>

#include <linux/l3/l3.h>
#include <uda1341.h>

#undef DEBUG_MODE
#undef DEBUG_FUNCTION_NAMES

#define DEFAULT_VOLUME	20

/* {{{ HW regs definition */

/*
 * UDA1341 L3 address and command types
 */
#define UDA1341_L3ADDR		5
#define UDA1341_DATA0		(UDA1341_L3ADDR << 2 | 0)
#define UDA1341_DATA1		(UDA1341_L3ADDR << 2 | 1)
#define UDA1341_STATUS		(UDA1341_L3ADDR << 2 | 2)

#define STAT0                   0x00
#define STAT0_RST		(1 << 6)
#define STAT0_SC_MASK		(3 << 4)
#define STAT0_SC_512FS		(0 << 4)
#define STAT0_SC_384FS		(1 << 4)
#define STAT0_SC_256FS		(2 << 4)
#define STAT0_IF_MASK		(7 << 1)
#define STAT0_IF_I2S		(0 << 1)
#define STAT0_IF_LSB16		(1 << 1)
#define STAT0_IF_LSB18		(2 << 1)
#define STAT0_IF_LSB20		(3 << 1)
#define STAT0_IF_MSB		(4 << 1)
#define STAT0_IF_LSB16MSB	(5 << 1)
#define STAT0_IF_LSB18MSB	(6 << 1)
#define STAT0_IF_LSB20MSB	(7 << 1)
#define STAT0_DC_FILTER		(1 << 0)

#define STAT1			0x80
#define STAT1_DAC_GAIN		(1 << 6)	/* gain of DAC */
#define STAT1_ADC_GAIN		(1 << 5)	/* gain of ADC */
#define STAT1_ADC_POL		(1 << 4)	/* polarity of ADC */
#define STAT1_DAC_POL		(1 << 3)	/* polarity of DAC */
#define STAT1_DBL_SPD		(1 << 2)	/* double speed playback */
#define STAT1_ADC_ON		(1 << 1)	/* ADC powered */
#define STAT1_DAC_ON		(1 << 0)	/* DAC powered */

#define DATA0_0			0x00
#define DATA0_VOLUME_MASK	0x3f

#define DATA0_1			0x40
#define DATA1_BASS(x)		((x) << 2)
#define DATA1_BASS_MASK		(15 << 2)
#define DATA1_TREBLE(x)		(x)
#define DATA1_TREBLE_MASK	(3)

#define DATA0_2			0x80
#define DATA2_PEAKAFTER		(1 << 5)
#define DATA2_DEEMP_NONE	(0 << 3)
#define DATA2_DEEMP_32KHz	(1 << 3)
#define DATA2_DEEMP_44KHz	(2 << 3)
#define DATA2_DEEMP_48KHz	(3 << 3)
#define DATA2_MUTE		(1 << 2)
#define DATA2_FILTER_FLAT	(0 << 0)
#define DATA2_FILTER_MIN	(1 << 0)
#define DATA2_FILTER_MAX	(3 << 0)

#define EXTADDR(n)		(0xc0 | (n))
#define EXTDATA(d)		(0xe0 | (d))

#define EXT0			0
#define EXT0_CH1_GAIN(x)	(x)

#define EXT1			1
#define EXT1_CH2_GAIN(x)	(x)

#define EXT2			2
#define EXT2_MIC_GAIN_MASK	(7 << 2)
#define EXT2_MIC_GAIN(x)	((x) << 2)
#define EXT2_MIXMODE_DOUBLEDIFF	(0)
#define EXT2_MIXMODE_CH1	(1)
#define EXT2_MIXMODE_CH2	(2)
#define EXT2_MIXMODE_MIX	(3)

#define EXT4			4
#define EXT4_AGC_ENABLE		(1 << 4)
#define EXT4_INPUT_GAIN_MASK	(3)
#define EXT4_INPUT_GAIN(x)	((x) & 3)

#define EXT5			5
#define EXT5_INPUT_GAIN(x)	((x) >> 2)

#define EXT6			6
#define EXT6_AGC_CONSTANT_MASK	(7 << 2)
#define EXT6_AGC_CONSTANT(x)	((x) << 2)
#define EXT6_AGC_LEVEL_MASK	(3)
#define EXT6_AGC_LEVEL(x)	(x)

#define REC_MASK	(SOUND_MASK_LINE | SOUND_MASK_MIC)
#define DEV_MASK	(REC_MASK | SOUND_MASK_VOLUME | SOUND_MASK_BASS | SOUND_MASK_TREBLE)

#define ADD_FIELD(p,reg,field)				\
		*p++ = reg | uda->regs[field]

#define ADD_EXTFIELD(p, reg,field)			\
		*p++ = EXTADDR(reg);			\
		*p++ = EXTDATA(uda->regs[field]);

#define IS_DATA0(x)     ((x) >= data0_0 && (x) <= data0_2)
#define IS_DATA1(x)     ((x) == data1)
#define IS_STATUS(x)    ((x) >= stat0 && (x) <= stat1)
#define IS_EXTEND(x)   ((x) >= ext0 && (x) <= ext6)

/* }}} */

enum uda1341_regs_names {
        stat0=0,
        stat1,
        data0_0,
        data0_1,
        data0_2,
        data1,
        ext0,
        ext1,
        ext2,
        ext4,
        ext5,
        ext6,
        rec_source,
};

const char *uda1341_reg_names[] = {
        "stat 0 ",
        "stat 1 ",
        "data 00",
        "data 01",
        "data 02",
        "data 1 ",
        "ext 0",
        "ext 1",
        "ext 2",
        "ext 4",
        "ext 5",
        "ext 6",
        "rec src",
};

#define UDA1341_MUTE    data0_2
#define UDA1341_VOLUME  data0_0
#define UDA1341_BASS    data0_1
#define UDA1341_TREBLE  data0_1

#define UDA1341_REC_SEL rec_source

#define UDA1341_FIRST   stat0
#define UDA1341_LAST    rec_source

#define EXT_OFFSET      ext0
#define EXT_MASK        0x7

typedef struct uda1341 uda1341_t;

struct uda1341{
        void (*write) (struct l3_client *uda1341, unsigned short reg, unsigned short val);
	unsigned short (*read) (struct l3_client *uda1341, unsigned short reg);
        
	unsigned char   regs[UDA1341_LAST+1];
	int		active;

        spinlock_t reg_lock;        
};

//HACK
typedef struct l3_client l3_client_t;
#define chip_t l3_client_t      

static struct l3_client *uda1341=NULL;

void int2str_bin8(uint8_t val, char *buf){
        const int size = sizeof(val) * 8;
        int i;

        for (i= 0; i < size; i++){
                *(buf++) = (val >> (size - 1)) ? '1' : '0';
                val <<= 1;
        }
        *buf = '\0'; //end the string with zero
}

void int2str_bin16(int16_t val, char *buf){
        const int size = sizeof(val) * 8;
        int i;

        for (i= 0; i < size; i++){
                *(buf++) = (val >> (size - 1)) ? '1' : '0';
                val <<= 1;
        }
        *buf = '\0'; //end the string with zero
}

void int2str_bin32(uint32_t val, char *buf){
        const int size = sizeof(val) * 8;
        int i;

        for (i= 0; i < size; i++){
                *(buf++) = (val >> (size - 1)) ? '1' : '0';
                val <<= 1;
        }
        *buf = '\0'; //end the string with zero
}

#ifdef DEBUG_MODE
static void uda1341_dump_state(struct uda1341 *uda)
{
        int i;
        char buf[10];
        
        printk("UDA1341 state:\n  -> active: %d\n", uda->active);
        for (i = UDA1341_FIRST; i <= UDA1341_LAST; i++){
                int2str_bin8(uda->regs[i], buf);
                printk("  -> reg[%s]: %s\n", uda1341_reg_names[i], buf);
        }
}
#else
static void uda1341_dump_state(struct uda1341 *uda)
{
        /* empty */
}
#endif

/* {{{ HW manipulation routines */

void snd_uda1341_codec_write(struct l3_client *clnt, unsigned short reg, unsigned short val)
{
	struct uda1341 *uda = clnt->driver_data;

        int err=0;
        unsigned short buf[2];

        DEBUG_NAME(KERN_DEBUG "codec_write: reg: %d val: %d ", reg, val);
        
        uda->regs[reg] = val;

        if (uda->active) {
                DEBUG("O");
                if (IS_DATA0(reg)) {
                        DEBUG(" D0 ");
                        err = l3_write(clnt, UDA1341_DATA0, (const char *)&val, 1);
                } else if (IS_DATA1(reg)) {
                        DEBUG(" D1 ");                        
                        err = l3_write(clnt, UDA1341_DATA1, (const char *)&val, 1);
                } else if (IS_STATUS(reg)) {
                        DEBUG(" S ");                        
                        err = l3_write(clnt, UDA1341_STATUS, (const char *)&val, 1);
                } else if (IS_EXTEND(reg)) {
                        DEBUG(" E ");                        
                        buf[0] = (reg - EXT_OFFSET) & EXT_MASK;
                        buf[1] = val;
                        err = l3_write(clnt, UDA1341_DATA0, (const char *)buf, 2);
                }
                
		if (err == 1)
                        DEBUG("K\n");
                else
                        DEBUG(" Error: %d\n", err);
        } else
                printk(KERN_ERR "UDA1341 codec not active!\n");
}

unsigned short snd_uda1341_codec_read(struct l3_client *clnt, unsigned short reg)
{
	struct uda1341 *uda = clnt->driver_data;
        return uda->regs[reg];
}

static int snd_uda1341_valid_reg(struct l3_client *clnt, unsigned short reg)
{
  return reg <= UDA1341_LAST;
}

int snd_uda1341_update(struct l3_client *clnt, unsigned short reg, unsigned short value)
{
	int change;
        struct uda1341 *uda = clnt->driver_data;
 
	if (!snd_uda1341_valid_reg(clnt, reg))
		return -EINVAL;
	spin_lock(&uda->reg_lock);
	change = uda->regs[reg] != value;
	if (change) {
		uda->write(clnt, reg, value);
		uda->regs[reg] = value;
	}
	spin_unlock(&uda->reg_lock);
	return change;
}


int snd_uda1341_update_bits(struct l3_client *clnt, unsigned short reg,
                            unsigned short mask, unsigned short value)
{
	int change;
	unsigned short old, new;
	struct uda1341 *uda = clnt->driver_data;

        DEBUG(KERN_DEBUG "update_bits: reg: %d mask: %d val: %d\n", reg, mask, value);
        
	if (!snd_uda1341_valid_reg(clnt, reg))
		return -EINVAL;
	spin_lock(&uda->reg_lock);
	old = uda->regs[reg];
	new = (old & ~mask) | value;
	change = old != new;
	if (change) {
		uda->write(clnt, reg, new);
		uda->regs[reg] = new;
	}
	spin_unlock(&uda->reg_lock);
	return change;
}

/* }}} */

/* {{{ Mixer controls setting */

/* {{{ UDA1341 single functions */

#define UDA1341_SINGLE(xname, reg, shift, mask, invert) \
{ iface: SNDRV_CTL_ELEM_IFACE_MIXER, name: xname, info: snd_uda1341_info_single, \
  get: snd_uda1341_get_single, put: snd_uda1341_put_single, \
  private_value: reg | (shift << 8) | (mask << 16) | (invert << 24) \
}

static int snd_uda1341_info_single(snd_kcontrol_t *kcontrol, snd_ctl_elem_info_t * uinfo)
{
	int mask = (kcontrol->private_value >> 16) & 0xff;

	uinfo->type = mask == 1 ? SNDRV_CTL_ELEM_TYPE_BOOLEAN : SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = mask;
	return 0;
}

static int snd_uda1341_get_single(snd_kcontrol_t * kcontrol, snd_ctl_elem_value_t * ucontrol)
{
	struct l3_client *clnt = snd_kcontrol_chip(kcontrol);
        uda1341_t *uda = clnt->driver_data;
	int reg = kcontrol->private_value & 0xff;
	int shift = (kcontrol->private_value >> 8) & 0xff;
	int mask = (kcontrol->private_value >> 16) & 0xff;
	int invert = (kcontrol->private_value >> 24) & 0xff;
	
	ucontrol->value.integer.value[0] = (uda->regs[reg] >> shift) & mask;
	if (invert)
		ucontrol->value.integer.value[0] = mask - ucontrol->value.integer.value[0];
	return 0;
}

static int snd_uda1341_put_single(snd_kcontrol_t * kcontrol, snd_ctl_elem_value_t * ucontrol)
{
	struct l3_client *clnt = snd_kcontrol_chip(kcontrol);
	int reg = kcontrol->private_value & 0xff;
	int shift = (kcontrol->private_value >> 8) & 0xff;
	int mask = (kcontrol->private_value >> 16) & 0xff;
	int invert = (kcontrol->private_value >> 24) & 0xff;
	unsigned short val;
	
	val = (ucontrol->value.integer.value[0] & mask);
	if (invert)
		val = mask - val;
	return snd_uda1341_update_bits(clnt, reg, mask << shift, val << shift);
}

/* }}} */

/* {{{ UDA1341 double functions */

#define UDA1341_DOUBLE(xname, reg, shift_left, shift_right, mask, invert) \
{ iface: SNDRV_CTL_ELEM_IFACE_MIXER, name: (xname), info: snd_uda1341_info_double, \
  get: snd_uda1341_get_double, put: snd_uda1341_put_double, \
  private_value: reg | (shift_left << 8) | (shift_right << 12) | (mask << 16) | (invert << 24) }


static int snd_uda1341_info_double(snd_kcontrol_t *kcontrol, snd_ctl_elem_info_t * uinfo)
{
	int mask = (kcontrol->private_value >> 16) & 0xff;

	uinfo->type = mask == 1 ? SNDRV_CTL_ELEM_TYPE_BOOLEAN : SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 2;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = mask;
	return 0;
}

static int snd_uda1341_get_double(snd_kcontrol_t * kcontrol, snd_ctl_elem_value_t * ucontrol)
{
  	struct l3_client *clnt = snd_kcontrol_chip(kcontrol);
        uda1341_t *uda = clnt->driver_data;
	int reg = kcontrol->private_value & 0xff;
	int shift_left = (kcontrol->private_value >> 8) & 0x0f;
	int shift_right = (kcontrol->private_value >> 12) & 0x0f;
	int mask = (kcontrol->private_value >> 16) & 0xff;
	int invert = (kcontrol->private_value >> 24) & 0xff;
	
	spin_lock(&uda->reg_lock);
	ucontrol->value.integer.value[0] = (uda->regs[reg] >> shift_left) & mask;
	ucontrol->value.integer.value[1] = (uda->regs[reg] >> shift_right) & mask;
	spin_unlock(&uda->reg_lock);
	if (invert) {
		ucontrol->value.integer.value[0] = mask - ucontrol->value.integer.value[0];
		ucontrol->value.integer.value[1] = mask - ucontrol->value.integer.value[1];
	}
	return 0;
}

static int snd_uda1341_put_double(snd_kcontrol_t * kcontrol, snd_ctl_elem_value_t * ucontrol)
{
	struct l3_client *clnt = snd_kcontrol_chip(kcontrol);
	int reg = kcontrol->private_value & 0xff;
	int shift_left = (kcontrol->private_value >> 8) & 0x0f;
	int shift_right = (kcontrol->private_value >> 12) & 0x0f;
	int mask = (kcontrol->private_value >> 16) & 0xff;
	int invert = (kcontrol->private_value >> 24) & 0xff;
	unsigned short val1, val2;
	
	val1 = ucontrol->value.integer.value[0] & mask;
	val2 = ucontrol->value.integer.value[1] & mask;
	if (invert) {
		val1 = mask - val1;
		val2 = mask - val2;
	}
	return snd_uda1341_update_bits(clnt, reg, 
				    (mask << shift_left) | (mask << shift_right),
				    (val1 << shift_left) | (val2 << shift_right));
}

/* }}} */

/* {{{ UDA1341 multiplexer functions */

static int snd_uda1341_info_mux(snd_kcontrol_t *kcontrol, snd_ctl_elem_info_t * uinfo)
{
	static char *texts[2] = {
		"Mic", "Line"
	};

	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	uinfo->count = 2;
	uinfo->value.enumerated.items = 2;
	if (uinfo->value.enumerated.item > 1)
		uinfo->value.enumerated.item = 1;
	strcpy(uinfo->value.enumerated.name, texts[uinfo->value.enumerated.item]);
	return 0;
}

static int snd_uda1341_get_mux(snd_kcontrol_t * kcontrol, snd_ctl_elem_value_t * ucontrol)
{
  	struct l3_client *clnt = snd_kcontrol_chip(kcontrol);
        uda1341_t *uda = clnt->driver_data;
	unsigned short val = uda->regs[UDA1341_REC_SEL];
        
	ucontrol->value.enumerated.item[0] = (val >> 8) & 7;
	ucontrol->value.enumerated.item[1] = (val >> 0) & 7;
	return 0;
}

static int snd_uda1341_put_mux(snd_kcontrol_t * kcontrol, snd_ctl_elem_value_t * ucontrol)
{

	struct l3_client *clnt = snd_kcontrol_chip(kcontrol);
	unsigned short val;
	
	if (ucontrol->value.enumerated.item[0] > 7 ||
	    ucontrol->value.enumerated.item[1] > 7)
		return -EINVAL;
	val = (ucontrol->value.enumerated.item[0] << 8) |
	      (ucontrol->value.enumerated.item[1] << 0);
	return snd_uda1341_update(clnt, UDA1341_REC_SEL, val);
}

/* }}} */

#define UDA1341_CONTROLS (sizeof(snd_uda1341_controls)/sizeof(snd_kcontrol_new_t))

static snd_kcontrol_new_t snd_uda1341_controls[] = {
UDA1341_SINGLE("Master Playback Switch", UDA1341_MUTE, 2, 4, 0),
UDA1341_SINGLE("Master Playback Volume", UDA1341_VOLUME, 0, 63, 1),

UDA1341_SINGLE("Bass Playback Volume", UDA1341_BASS, 2, 15, 0),
UDA1341_SINGLE("Treble Playback Volume", UDA1341_TREBLE, 0, 3, 0),

/* these below are not working yet */
/*
UDA1341_SINGLE("Mic Playback Switch", UDA1341_MIC, 15, 1, 1),
UDA1341_SINGLE("Mic Playback Volume", UDA1341_MIC, 0, 15, 1),
UDA1341_SINGLE("Mic Boost (+20dB)", UDA1341_MIC, 6, 1, 0),
{
	iface: SNDRV_CTL_ELEM_IFACE_MIXER,
	name: "Capture Source",
	info: snd_uda1341_info_mux,
	get: snd_uda1341_get_mux,
	put: snd_uda1341_put_mux,
},
UDA1341_SINGLE("Gain Playback Switch", UDA1341_REC_GAIN, 15, 1, 1),
UDA1341_DOUBLE("Gain Playback Volume", UDA1341_REC_GAIN, 8, 0, 15, 0),
*/
};

int __init snd_chip_uda1341_mixer_new(snd_card_t *card, struct l3_client **clnt)
{
	int idx, err;

        
        snd_assert(card != NULL, return -EINVAL);

        uda1341 = snd_magic_kcalloc(l3_client_t, 0, GFP_KERNEL);
         if (uda1341 == NULL)
		return -ENOMEM;
         
        if ((err = l3_attach_client(uda1341, "l3-bit-sa1100-gpio", "snd-uda1341")))
                return -ENODEV;

        strcpy(card->mixername, "UDA1341 Mixer");
       
        for (idx = 0; idx < UDA1341_CONTROLS; idx++) {
		if ((err = snd_ctl_add(card, snd_ctl_new1(&snd_uda1341_controls[idx], uda1341))) < 0)
			return err;
	}

        *clnt = uda1341;
        return 0;
}

void __init snd_chip_uda1341_mixer_del(snd_card_t *card)
{
        l3_detach_client(uda1341);
        snd_magic_kfree(uda1341);
        
        uda1341 = NULL;
}

/* }}} */

/* {{{ L3 operations */

static void uda1341_sync(struct l3_client *clnt)
{
	struct uda1341 *uda = clnt->driver_data;
	char buf[24], *p = buf;

        DEBUG_NAME(KERN_DEBUG "uda1341 sync\n");
        
	ADD_FIELD(p, STAT0, stat0);
	ADD_FIELD(p, STAT1, stat1);

	if (p != buf)
		l3_write(clnt, UDA1341_STATUS, buf, p - buf);

	p = buf;
	ADD_FIELD(p, DATA0_0, data0_0);
	ADD_FIELD(p, DATA0_1, data0_1);
	ADD_FIELD(p, DATA0_2, data0_2);
	ADD_EXTFIELD(p, EXT0, ext0);
	ADD_EXTFIELD(p, EXT1, ext1);
	ADD_EXTFIELD(p, EXT2, ext2);
	ADD_EXTFIELD(p, EXT4, ext4);
	ADD_EXTFIELD(p, EXT5, ext5);
	ADD_EXTFIELD(p, EXT6, ext6);

	if (p != buf)
		l3_write(clnt, UDA1341_DATA0, buf, p - buf);
}

static void uda1341_cmd_init(struct l3_client *clnt)
{
	struct uda1341 *uda = clnt->driver_data;
	char buf[2];

        DEBUG_NAME(KERN_DEBUG "uda1341 cmd_init\n");
        
	uda->active = 1;

	buf[0] = uda->regs[stat0] | STAT0_RST;
	buf[1] = uda->regs[stat0];

	l3_write(clnt, UDA1341_STATUS, buf, 2);

	/* resend all parameters */
	uda1341_sync(clnt);
}

static int uda1341_configure(struct l3_client *clnt, struct uda1341_cfg *conf)
{
	struct uda1341 *uda = clnt->driver_data;
	int ret = 0;

        DEBUG_NAME(KERN_DEBUG "l3_configure fs: %d format: %d\n", conf->fs, conf->format);
        
	uda->regs[stat0] &= ~(STAT0_SC_MASK | STAT0_IF_MASK);

	switch (conf->fs) {
	case 512: uda->regs[stat0] |= STAT0_SC_512FS;	break;
	case 384: uda->regs[stat0] |= STAT0_SC_384FS;	break;
	case 256: uda->regs[stat0] |= STAT0_SC_256FS;	break;
	default:  ret = -EINVAL;			break;
	}

	switch (conf->format) {
	case FMT_I2S:		uda->regs[stat0] |= STAT0_IF_I2S;	break;
	case FMT_LSB16:		uda->regs[stat0] |= STAT0_IF_LSB16;	break;
	case FMT_LSB18:		uda->regs[stat0] |= STAT0_IF_LSB18;	break;
	case FMT_LSB20:		uda->regs[stat0] |= STAT0_IF_LSB20;	break;
	case FMT_MSB:		uda->regs[stat0] |= STAT0_IF_MSB;	break;
	case FMT_LSB16MSB:	uda->regs[stat0] |= STAT0_IF_LSB16MSB;	break;
	case FMT_LSB18MSB:	uda->regs[stat0] |= STAT0_IF_LSB18MSB;	break;
	case FMT_LSB20MSB:	uda->regs[stat0] |= STAT0_IF_LSB20MSB;	break;
	}

	if (ret == 0 && uda->active) {
		l3_write(clnt, UDA1341_STATUS, &uda->regs[stat0], 1);
                return 0;
	}

        if (!uda->active) {
                printk(KERN_ERR "UDA1341 codec not active!\n");                
        }
        
	return ret;
}

static int uda1341_attach(struct l3_client *clnt)
{
	struct uda1341 *uda;

        DEBUG_NAME(KERN_DEBUG "uda1341 attach\n");
        
	uda = kmalloc(sizeof(*uda), GFP_KERNEL);
	if (!uda)
		return -ENOMEM;

	memset(uda, 0, sizeof(*uda));

	uda->regs[stat0]   = STAT0 | STAT0_SC_256FS | STAT0_IF_LSB16;
	uda->regs[stat1]   = STAT1 | STAT1_DAC_GAIN | STAT1_ADC_GAIN |
			     STAT1_ADC_ON | STAT1_DAC_ON;
	uda->regs[data0_0] = DATA0_0 | DEFAULT_VOLUME;
	uda->regs[data0_1] = DATA0_1 | DATA1_BASS(0) | DATA1_TREBLE(0);
	uda->regs[data0_2] = DATA0_2 | DATA2_PEAKAFTER | DATA2_DEEMP_NONE |
			     DATA2_FILTER_MAX;
	uda->regs[ext0]    = 0xFF & EXT0_CH1_GAIN(4);
	uda->regs[ext1]    = 0xFF & EXT1_CH2_GAIN(4);
	uda->regs[ext2]    = 0xFF & ( EXT2_MIXMODE_MIX | EXT2_MIC_GAIN(4) );
	uda->regs[ext4]    = 0xFF & ( EXT4_AGC_ENABLE | EXT4_INPUT_GAIN(0) );
	uda->regs[ext5]    = 0xFF & EXT5_INPUT_GAIN(0);
	uda->regs[ext6]    = 0xFF & ( EXT6_AGC_CONSTANT(3) | EXT6_AGC_LEVEL(0) );

        uda1341_dump_state(uda);
        
        uda->write = snd_uda1341_codec_write;
        uda->read = snd_uda1341_codec_read;
  
        spin_lock_init(&uda->reg_lock);
        
	clnt->driver_data = uda;

        l3_open(clnt);

	return 0;
}

static void uda1341_detach(struct l3_client *clnt)
{
        DEBUG_NAME(KERN_DEBUG "uda1341 detach\n");        
	kfree(clnt->driver_data);
}

static int
uda1341_command(struct l3_client *clnt, int cmd, void *arg)
{
        DEBUG_NAME(KERN_DEBUG "l3_command\n");
        
        if (cmd == L3_UDA1341_CONFIGURE)
                return uda1341_configure(clnt, arg);

        return -EINVAL;
}

static int uda1341_open(struct l3_client *clnt)
{
        struct uda1341 *uda = clnt->driver_data;

        DEBUG_NAME(KERN_DEBUG "uda1341 open\n");

	uda1341_cmd_init(clnt);
	return 0;
}

static void uda1341_close(struct l3_client *clnt)
{
	struct uda1341 *uda = clnt->driver_data;

        DEBUG_NAME(KERN_DEBUG "uda1341 close\n");
	uda->active = 0;
}

/* }}} */

/* {{{ Module and L3 initialization */

static struct l3_ops uda1341_ops = {
	open:		uda1341_open,
	command:	uda1341_command,
	close:		uda1341_close,
};

static struct l3_driver uda1341_driver = {
	name:		UDA1341_ALSA_NAME,
	attach_client:	uda1341_attach,
	detach_client:	uda1341_detach,
	ops:		&uda1341_ops,
	owner:		THIS_MODULE,
};

static int __init uda1341_init(void)
{
	return l3_add_driver(&uda1341_driver);
}

static void __exit uda1341_exit(void)
{
	l3_del_driver(&uda1341_driver);
}

module_init(uda1341_init);
module_exit(uda1341_exit);

MODULE_AUTHOR("Tomas Kasparek <tomas.kasparek@seznam.cz>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Philips UDA1341 CODEC driver for ALSA");
MODULE_CLASSES("{sound}");
MODULE_DEVICES("{{UDA1341,UDA1341TS}}");

EXPORT_SYMBOL(snd_chip_uda1341_mixer_new);
EXPORT_SYMBOL(snd_chip_uda1341_mixer_del);

/* }}} */
