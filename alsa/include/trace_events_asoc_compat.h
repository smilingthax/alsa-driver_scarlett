#ifndef _TRACE_ASOC_H
#define _TRACE_ASOC_H
#define	trace_snd_soc_reg_read(codec, reg, ret)
#define	trace_snd_soc_reg_write(codec, reg, val)
#define trace_snd_soc_bias_level_start(card, level)
#define trace_snd_soc_bias_level_done(card, level)
#define trace_snd_soc_dapm_widget_event_start(w, event)
#define trace_snd_soc_dapm_widget_event_done(w, event)
#define trace_snd_soc_dapm_start(card)
#define	trace_snd_soc_dapm_done(card)
#define trace_snd_soc_dapm_widget_power(w, power)
#define trace_snd_soc_jack_report(jack, mask, status)
#define trace_snd_soc_jack_notify(jack, status)
#define trace_snd_soc_jack_irq(name)
#define trace_snd_soc_cache_sync(codec, type, status)
#endif /* _TRACE_ASOC_H */
