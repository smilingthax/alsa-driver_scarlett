/*
 */

#define USB_DT_CS_DEVICE                0x21
#define USB_DT_CS_CONFIG                0x22
#define USB_DT_CS_STRING                0x23
#define USB_DT_CS_INTERFACE             0x24
#define USB_DT_CS_ENDPOINT              0x25

#define CS_AUDIO_UNDEFINED		0x20
#define CS_AUDIO_DEVICE			0x21
#define CS_AUDIO_CONFIGURATION		0x22
#define CS_AUDIO_STRING			0x23
#define CS_AUDIO_INTERFACE		0x24
#define CS_AUDIO_ENDPOINT		0x25

#define HEADER				0x01
#define INPUT_TERMINAL			0x02
#define OUTPUT_TERMINAL			0x03
#define MIXER_UNIT			0x04
#define SELECTOR_UNIT			0x05
#define FEATURE_UNIT			0x06
#define PROCESSING_UNIT			0x07
#define EXTENSION_UNIT			0x08

#define AS_GENERAL			0x01
#define FORMAT_TYPE			0x02
#define FORMAT_SPECIFIC			0x03

#define EP_GENERAL			0x01

#define MAX_CHAN			9
#define MAX_FREQ			16
#define MAX_IFACE			8
#define MAX_FORMAT			8
#define MAX_ALT				32 	/* Sorry, we need quite a few for the Philips webcams */


/* Audio Class specific Request Codes */

#define SET_CUR    0x01
#define GET_CUR    0x81
#define SET_MIN    0x02
#define GET_MIN    0x82
#define SET_MAX    0x03
#define GET_MAX    0x83
#define SET_RES    0x04
#define GET_RES    0x84
#define SET_MEM    0x05
#define GET_MEM    0x85
#define GET_STAT   0xff

/* Terminal Control Selectors */

#define COPY_PROTECT_CONTROL       0x01

/* Endpoint Control Selectors */

#define SAMPLING_FREQ_CONTROL      0x01
#define PITCH_CONTROL              0x02

/*
 */

#define USB_FORMAT_TYPE_I	0x01
#define USB_FORMAT_TYPE_II	0x02
#define USB_FORMAT_TYPE_III	0x03

/* type I */
#define USB_AUDIO_FORMAT_PCM	0x01
#define USB_AUDIO_FORMAT_PCM8	0x02
#define USB_AUDIO_FORMAT_IEEE_FLOAT	0x03
#define USB_AUDIO_FORMAT_ALAW	0x04
#define USB_AUDIO_FORMAT_MU_LAW	0x05

/* type II */
#define USB_AUDIO_FORMAT_MPEG	0x1001
#define USB_AUDIO_FORMAT_AC3	0x1002

/* type III */
#define USB_AUDIO_FORMAT_IEC1937_AC3	0x2001
#define USB_AUDIO_FORMAT_IEC1937_MPEG1_LAYER1	0x2002
#define USB_AUDIO_FORMAT_IEC1937_MPEG2_NOEXT	0x2003
#define USB_AUDIO_FORMAT_IEC1937_MPEG2_EXT	0x2004
#define USB_AUDIO_FORMAT_IEC1937_MPEG2_LAYER1_LS	0x2005
#define USB_AUDIO_FORMAT_IEC1937_MPEG2_LAYER23_LS	0x2006


/*
 */

typedef struct snd_usb_audio snd_usb_audio_t;

#define snd_usb_audio_t_magic	0xa15a401	/* should be in sndmagic.h later */


struct snd_usb_audio {
	
	struct usb_device *dev;

	snd_card_t *card;

	int pcm_devs;

	snd_info_entry_t *proc_entry;

};  

/*
 */

#define combine_word(s)    ((*s) | ((unsigned int)(s)[1] << 8))
#define combine_triple(s)  (combine_word(s) | ((unsigned int)(s)[2] << 16))
#define combine_quad(s)    (combine_triple(s) | ((unsigned int)(s)[3] << 24))

unsigned int snd_usb_combine_bytes(unsigned char *bytes, int size);

void *snd_usb_find_desc(void *descstart, unsigned int desclen, void *after, u8 dtype, int iface, int altsetting);
void *snd_usb_find_csint_desc(void *descstart, unsigned int desclen, void *after, u8 dsubtype, int iface, int altsetting);

int snd_usb_create_mixer(snd_usb_audio_t *chip, unsigned char *buffer, unsigned int buflen, unsigned int ctrlif);


     
#if LINUX_VERSION < KERNEL_VERSION(2,5,0)
#define do_usb_alloc_urb(n,flags) usb_alloc_urb(n)
#define do_usb_submit_urb(p,flags) usb_submit_urb(p)
#else
#define do_usb_alloc_urb(n,flags) usb_alloc_urb(n,flags)
#define do_usb_submit_urb(p,flags) usb_submit_urb(p,flags)
#endif

