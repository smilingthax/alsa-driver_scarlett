
#ifndef __SND_CMI8788_CONTROLLER__H__
#define __SND_CMI8788_CONTROLLER__H__

#define MAX_CODEC_NUM       5
#define MAX_AC97_CODEC_NUM  2

/*
 * Structures
 */


/* bus operators */
typedef struct stru_cmi8788_bus_ops {
    /* send a single command */
    int (*spi_cmd)    (cmi_codec *codec, u8* data);
    int (*twowire_cmd)(cmi_codec *codec, u8 reg_addr, u16 reg_data);
    int (*ac97_cmd)   (cmi_codec *codec, u8 reg_addr, u16 reg_data);
    /* get a response from the last command */
    unsigned int (*get_response)(cmi_codec *codec);
    /* free the private data */
    void (*private_free)(cmi8788_controller *);
}cmi8788_bus_ops;

/* template to pass to the bus constructor */
typedef struct stru_cmi_bus_template {
    void               *private_data;
    struct pci_dev     *pci;
    cmi8788_bus_ops     ops;
}cmi_bus_template;

struct stru_cmi8788_controller {
    struct snd_card *card;

    /* copied from template */
    void            *private_data;
    struct pci_dev  *pci;
    cmi8788_bus_ops  ops;

    /* codec linked list */
    int              codec_num;                          /* 实际上 CODEC 的个数 */
    cmi_codec        codec_list[MAX_CODEC_NUM];          /* cmi_codec type */
    int              ac97_cocde_num;
    cmi_codec        ac97_codec_list[MAX_AC97_CODEC_NUM];/* 最多支持 2 个 AC97 CODEC */
    struct semaphore cmd_mutex;
};

/*
 * constructors
 */
int snd_cmi8788_controller_new(snd_cmi8788 *chip, const cmi_bus_template *temp, cmi8788_controller **busp );

#endif

