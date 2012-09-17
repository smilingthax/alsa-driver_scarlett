/*
 * Jack-detection handling for HD-audio
 *
 * Copyright (c) 2011 Takashi Iwai <tiwai@suse.de>
 *
 * This driver is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/init.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/control.h>
#include "hda_codec.h"
#include "hda_local.h"
#include "hda_jack.h"

/* execute pin sense measurement */
static u32 read_pin_sense(struct hda_codec *codec, hda_nid_t nid)
{
	u32 pincap;

	if (!codec->no_trigger_sense) {
		pincap = snd_hda_query_pin_caps(codec, nid);
		if (pincap & AC_PINCAP_TRIG_REQ) /* need trigger? */
			snd_hda_codec_read(codec, nid, 0,
					AC_VERB_SET_PIN_SENSE, 0);
	}
	return snd_hda_codec_read(codec, nid, 0,
				  AC_VERB_GET_PIN_SENSE, 0);
}

/**
 * snd_hda_jack_tbl_get - query the jack-table entry for the given NID
 */
struct hda_jack_tbl *
snd_hda_jack_tbl_get(struct hda_codec *codec, hda_nid_t nid)
{
	struct hda_jack_tbl *jack = codec->jacktbl.list;
	int i;

	if (!nid || !jack)
		return NULL;
	for (i = 0; i < codec->jacktbl.used; i++, jack++)
		if (jack->nid == nid)
			return jack;
	return NULL;
}
EXPORT_SYMBOL_HDA(snd_hda_jack_tbl_get);

/**
 * snd_hda_jack_tbl_new - create a jack-table entry for the given NID
 */
struct hda_jack_tbl *
snd_hda_jack_tbl_new(struct hda_codec *codec, hda_nid_t nid)
{
	struct hda_jack_tbl *jack = snd_hda_jack_tbl_get(codec, nid);
	if (jack)
		return jack;
	snd_array_init(&codec->jacktbl, sizeof(*jack), 16);
	jack = snd_array_new(&codec->jacktbl);
	if (!jack)
		return NULL;
	jack->nid = nid;
	jack->jack_dirty = 1;
	return jack;
}

void snd_hda_jack_tbl_clear(struct hda_codec *codec)
{
	snd_array_free(&codec->jacktbl);
}

/* update the cached value and notification flag if needed */
static void jack_detect_update(struct hda_codec *codec,
			       struct hda_jack_tbl *jack)
{
	if (jack->jack_dirty || !jack->jack_cachable) {
		unsigned int val = read_pin_sense(codec, jack->nid);
		jack->jack_dirty = 0;
		if (val != jack->pin_sense) {
			jack->need_notify = 1;
			jack->pin_sense = val;
		}
	}
}

/**
 * snd_hda_set_dirty_all - Mark all the cached as dirty
 *
 * This function sets the dirty flag to all entries of jack table.
 * It's called from the resume path in hda_codec.c.
 */
void snd_hda_jack_set_dirty_all(struct hda_codec *codec)
{
	struct hda_jack_tbl *jack = codec->jacktbl.list;
	int i;

	for (i = 0; i < codec->jacktbl.used; i++, jack++)
		if (jack->nid)
			jack->jack_dirty = 1;
}
EXPORT_SYMBOL_HDA(snd_hda_jack_set_dirty_all);

/**
 * snd_hda_pin_sense - execute pin sense measurement
 * @codec: the CODEC to sense
 * @nid: the pin NID to sense
 *
 * Execute necessary pin sense measurement and return its Presence Detect,
 * Impedance, ELD Valid etc. status bits.
 */
u32 snd_hda_pin_sense(struct hda_codec *codec, hda_nid_t nid)
{
	struct hda_jack_tbl *jack = snd_hda_jack_tbl_get(codec, nid);
	if (jack) {
		jack_detect_update(codec, jack);
		return jack->pin_sense;
	}
	return read_pin_sense(codec, nid);
}
EXPORT_SYMBOL_HDA(snd_hda_pin_sense);

/**
 * snd_hda_jack_detect - query pin Presence Detect status
 * @codec: the CODEC to sense
 * @nid: the pin NID to sense
 *
 * Query and return the pin's Presence Detect status.
 */
int snd_hda_jack_detect(struct hda_codec *codec, hda_nid_t nid)
{
	u32 sense = snd_hda_pin_sense(codec, nid);
	return !!(sense & AC_PINSENSE_PRESENCE);
}
EXPORT_SYMBOL_HDA(snd_hda_jack_detect);

/**
 * snd_hda_jack_detect_enable - enable the jack-detection
 */
int snd_hda_jack_detect_enable(struct hda_codec *codec, hda_nid_t nid,
			       unsigned int tag)
{
	struct hda_jack_tbl *jack = snd_hda_jack_tbl_new(codec, nid);
	if (!jack)
		return -ENOMEM;
	if (jack->jack_cachable)
		return 0; /* already registered */
	jack->jack_cachable = 1;
	return snd_hda_codec_write_cache(codec, nid, 0,
					 AC_VERB_SET_UNSOLICITED_ENABLE,
					 AC_USRSP_EN | tag);
}
EXPORT_SYMBOL_HDA(snd_hda_jack_detect_enable);

/* queue the notification when needed */
static void jack_detect_report(struct hda_codec *codec,
			       struct hda_jack_tbl *jack)
{
	jack_detect_update(codec, jack);
	if (jack->need_notify) {
		snd_ctl_notify(codec->bus->card, SNDRV_CTL_EVENT_MASK_VALUE,
			       &jack->kctl->id);
		jack->need_notify = 0;
	}
}

/**
 * snd_hda_jack_report - notify kctl when the jack state was changed
 */
void snd_hda_jack_report(struct hda_codec *codec, hda_nid_t nid)
{
	struct hda_jack_tbl *jack = snd_hda_jack_tbl_get(codec, nid);

	if (jack)
		jack_detect_report(codec, jack);
}
EXPORT_SYMBOL_HDA(snd_hda_jack_report);

/**
 * snd_hda_jack_report_sync - sync the states of all jacks and report if changed
 */
void snd_hda_jack_report_sync(struct hda_codec *codec)
{
	struct hda_jack_tbl *jack = codec->jacktbl.list;
	int i;

	for (i = 0; i < codec->jacktbl.used; i++, jack++)
		if (jack->nid) {
			jack_detect_update(codec, jack);
			jack_detect_report(codec, jack);
		}
}
EXPORT_SYMBOL_HDA(snd_hda_jack_report_sync);

/*
 * jack-detection kcontrols
 */

#define jack_detect_kctl_info	snd_ctl_boolean_mono_info

static int jack_detect_kctl_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct hda_codec *codec = snd_kcontrol_chip(kcontrol);
	hda_nid_t nid = kcontrol->private_value;

	ucontrol->value.integer.value[0] = snd_hda_jack_detect(codec, nid);
	return 0;
}

static struct snd_kcontrol_new jack_detect_kctl = {
	/* name is filled later */
	.iface = SNDRV_CTL_ELEM_IFACE_CARD,
	.access = SNDRV_CTL_ELEM_ACCESS_READ,
	.info = jack_detect_kctl_info,
	.get = jack_detect_kctl_get,
};

/**
 * snd_hda_jack_add_kctl - Add a kctl for the given pin
 *
 * This assigns a jack-detection kctl to the given pin.  The kcontrol
 * will have the given name and index.
 */
int snd_hda_jack_add_kctl(struct hda_codec *codec, hda_nid_t nid,
			  const char *name, int idx)
{
	struct hda_jack_tbl *jack;
	struct snd_kcontrol *kctl;

	jack = snd_hda_jack_tbl_get(codec, nid);
	if (!jack)
		return 0;
	if (jack->kctl)
		return 0; /* already created */
	kctl = snd_ctl_new1(&jack_detect_kctl, codec);
	if (!kctl)
		return -ENOMEM;
	snprintf(kctl->id.name, sizeof(kctl->id.name), "%s Jack", name);
	kctl->id.index = idx;
	kctl->private_value = nid;
	if (snd_hda_ctl_add(codec, nid, kctl) < 0)
		return -ENOMEM;
	jack->kctl = kctl;
	return 0;
}

static int add_jack_kctl(struct hda_codec *codec, hda_nid_t nid, int idx,
			 const struct auto_pin_cfg *cfg)
{
	if (!nid)
		return 0;
	if (!is_jack_detectable(codec, nid))
		return 0;
	return snd_hda_jack_add_kctl(codec, nid,
				     snd_hda_get_pin_label(codec, nid, cfg),
				     idx);
}

/**
 * snd_hda_jack_add_kctls - Add kctls for all pins included in the given pincfg
 *
 * As of now, it assigns only to the pins that enabled the detection.
 * Usually this is called at the end of build_controls callback.
 */
int snd_hda_jack_add_kctls(struct hda_codec *codec,
			   const struct auto_pin_cfg *cfg)
{
	const hda_nid_t *p;
	int i, err;

	for (i = 0, p = cfg->line_out_pins; i < cfg->line_outs; i++, p++) {
		err = add_jack_kctl(codec, *p, i, cfg);
		if (err < 0)
			return err;
	}
	for (i = 0, p = cfg->hp_pins; i < cfg->hp_outs; i++, p++) {
		if (*p == *cfg->line_out_pins) /* might be duplicated */
			break;
		err = add_jack_kctl(codec, *p, i, cfg);
		if (err < 0)
			return err;
	}
	for (i = 0, p = cfg->speaker_pins; i < cfg->speaker_outs; i++, p++) {
		if (*p == *cfg->line_out_pins) /* might be duplicated */
			break;
		err = add_jack_kctl(codec, *p, i, cfg);
		if (err < 0)
			return err;
	}
	for (i = 0; i < cfg->num_inputs; i++) {
		err = add_jack_kctl(codec, cfg->inputs[i].pin, 0, cfg);
		if (err < 0)
			return err;
	}
	for (i = 0, p = cfg->dig_out_pins; i < cfg->dig_outs; i++, p++) {
		err = add_jack_kctl(codec, *p, i, cfg);
		if (err < 0)
			return err;
	}
	err = add_jack_kctl(codec, cfg->dig_in_pin, 0, cfg);
	if (err < 0)
		return err;
	err = add_jack_kctl(codec, cfg->mono_out_pin, 0, cfg);
	if (err < 0)
		return err;
	return 0;
}
EXPORT_SYMBOL_HDA(snd_hda_jack_add_kctls);
