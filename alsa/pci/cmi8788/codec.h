#ifndef __SND_CMI_CODEC__H__
#define __SND_CMI_CODEC__H__

struct stru_cmi_codec;
typedef struct stru_cmi_codec cmi_codec;

struct stru_codec_preset;
typedef struct stru_codec_preset codec_preset;

struct stru_cmi_codec_ops;
typedef struct stru_cmi_codec_ops cmi_codec_ops;

struct  stru_cmi8788_pcm_stream_ops;
typedef struct stru_cmi8788_pcm_stream_ops cmi8788_pcm_stream_ops;

struct stru_cmi8788_mixer_ops;
typedef struct stru_cmi8788_mixer_ops  cmi8788_mixer_ops;

struct stru_cmi8788_pcm_stream;
typedef struct stru_cmi8788_pcm_stream cmi8788_pcm_stream;

struct stru_ac97_volume;
typedef struct stru_ac97_volume ac97_volume;

/*
 * codec preset
 *
 * Known codecs have the patch to build and set up the controls/PCMs
 * better than the generic parser.
 */
struct stru_codec_preset {
    unsigned    int id;
    const char *name;
    unsigned    int mask;
    unsigned    int subs;
    unsigned    int subs_mask;
    unsigned    int rev;
    int (*patch)(cmi_codec *codec);
};
    
/* ops set by the preset patch */
struct stru_cmi_codec_ops
{
    int  (*build_controls)(cmi_codec *codec);
    int  (*build_pcms)    (cmi_codec *codec);
    int  (*init)          (cmi_codec *codec);
    void (*free)          (cmi_codec *codec);
};

/* PCM callbacks */
struct stru_cmi8788_pcm_stream_ops {
    int (*open)   (void *info, cmi_codec *codec,
                   struct snd_pcm_substream *substream);
    int (*close)  (void *info, cmi_codec *codec,
                   struct snd_pcm_substream *substream);
    int (*prepare)(void *info, cmi_codec *codec,
                   struct snd_pcm_substream *substream);
    int (*cleanup)(void *info, cmi_codec *codec,
                   struct snd_pcm_substream *substream);
};

/* Mixer callbacks */
struct stru_cmi8788_mixer_ops
{
    int (*get_info)   (cmi_codec *codec, int *min_vol, int *max_vol);
    int (*get_volume) (cmi_codec *codec, int *l_vol, int *r_vol);
    int (*set_volume) (cmi_codec *codec, int  l_vol, int  r_vol);
};

/* for PCM creation */
struct stru_cmi8788_pcm_stream
{
    unsigned int channels;     /* number of channels */
    u32          rates;        /* supported rates */
    u64          formats;      /* supported formats (SNDRV_PCM_FMTBIT_) */
    unsigned int maxbps;       /* supported max. bit per sample */
    cmi8788_pcm_stream_ops ops;
};

/* for AC97 CODEC volume*/
struct stru_ac97_volume
{
    s16  left_vol;
    s16  right_vol;
};

struct stru_cmi_codec {
    cmi8788_controller *controller;

    unsigned int addr;         /* codec addr*/
    int          valid_flag;   /* 0 invalid ,1 valid*/

    /* ids */
    u16 vendor_id;
    u16 subsystem_id;
    u16 revision_id;

    /* register length flag */
    u8  reg_len_flag; // 0 : 2bytes; 1: 3bytes

    /* detected preset */
    const codec_preset *preset;

    /* set by patch */
    cmi_codec_ops patch_ops;

    /* PCM to create, set by patch_ops.build_pcms callback */
    cmi8788_pcm_stream  pcm_substream[2]; // 0 playback ;1 capture

    /* Mixer */
    cmi8788_mixer_ops  mixer_ops;
    s16  left_vol;
    s16  right_vol;

    /* for AC97 CODEC */
    u8  volume_opera_source;
    ac97_volume volume[MAX_VOL_SLIDER];
};

/*
 * constructors
 */
void snd_cmi8788_codec_free(cmi_codec *codec );
int  snd_cmi8788_codec_new (cmi8788_controller *controller, cmi_codec *codec, u32 addr, codec_preset *preset);

#endif

