/*
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
 * 2002-07 Benny Sjostrand benny@hostmobility.com
 */


#define __NO_VERSION__
#include <sound/driver.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/pci.h>
#include <linux/pm.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/control.h>
#include <sound/info.h>
#include <sound/cs46xx.h>

#include "cs46xx_lib.h"
#include "dsp_spos.h"

void cs46xx_dsp_remove_scb (cs46xx_t *chip, dsp_scb_descriptor_t * scb)
{
	dsp_spos_instance_t * ins = chip->dsp_spos_instance;
	int i;

	/* check integrety */
	snd_assert ( (scb->index >= 0 && 
		      scb->index < ins->nscb && 
		      (ins->scbs + scb->index) == scb), return );

	/* cant remove a SCB with childs before 
	   removing childs first  */
	snd_assert ( (scb->sub_list_ptr == ins->the_null_scb &&
		      scb->next_scb_ptr == ins->the_null_scb),
		     return);

	if ( scb->parent_scb_ptr ) {
		/* unlink parent SCB */
		snd_assert ( (scb->parent_scb_ptr->sub_list_ptr == scb ||
			      scb->parent_scb_ptr->next_scb_ptr == scb),
			     return);

		if (scb->parent_scb_ptr->sub_list_ptr == scb)
			scb->parent_scb_ptr->sub_list_ptr = ins->the_null_scb;
		else
			scb->parent_scb_ptr->next_scb_ptr = ins->the_null_scb;
    
		/* update entry in DSP RAM */
		snd_cs46xx_poke(chip,
				(scb->parent_scb_ptr->address + SCBsubListPtr) << 2,
				(scb->parent_scb_ptr->sub_list_ptr->address << 0x10) |
				(scb->parent_scb_ptr->next_scb_ptr->address));
	}
	for(i = scb->index + 1;i < ins->nscb; ++i) {
		ins->scbs[i - 1] = ins->scbs[i];
	}

	ins->nscb --;
}

dsp_scb_descriptor_t * 
cs46xx_dsp_create_generic_scb (cs46xx_t *chip, char * name, u32 * scb_data, u32 dest,
                               char * task_entry_name,
                               dsp_scb_descriptor_t * parent_scb,
                               int scb_child_type)
{
	dsp_spos_instance_t * ins = chip->dsp_spos_instance;
	dsp_scb_descriptor_t * scb;
	symbol_entry_t * task_entry;

	snd_assert (ins->the_null_scb != NULL,return NULL);

	task_entry = cs46xx_dsp_lookup_symbol (chip,task_entry_name,
					       SYMBOL_CODE);

	if (task_entry == NULL) {
		snd_printdd("dsp_spos: symbol %s not found\n",task_entry_name);
		return NULL;
	}

	/* fill the data that will be wroten to DSP */
	scb_data[SCBsubListPtr] = 
		(ins->the_null_scb->address << 0x10) | ins->the_null_scb->address;

	scb_data[SCBfuncEntryPtr] &= 0xFFFF0000;
	scb_data[SCBfuncEntryPtr] |= task_entry->address;

	snd_printdd("dsp_spos: creating SCB <%s>\n",name);

	scb = cs46xx_dsp_create_scb(chip,name,scb_data,dest);


	scb->sub_list_ptr = ins->the_null_scb;
	scb->next_scb_ptr = ins->the_null_scb;

	scb->parent_scb_ptr = parent_scb;
	scb->task_entry = task_entry;

  
	/* update parent SCB */
	if (scb->parent_scb_ptr) {
		snd_printdd("scb->parent_scb_ptr = %s\n",scb->parent_scb_ptr->scb_name);
		snd_printdd("scb->parent_scb_ptr->next_scb_ptr = %s\n",scb->parent_scb_ptr->next_scb_ptr->scb_name);
		snd_printdd("scb->parent_scb_ptr->sub_list_ptr = %s\n",scb->parent_scb_ptr->sub_list_ptr->scb_name);
		/* unlink parent SCB */
		if (scb_child_type == SCB_ON_PARENT_NEXT_SCB) {
			snd_assert ( (scb->parent_scb_ptr->next_scb_ptr == ins->the_null_scb),
				     return NULL);

			snd_printdd("[1]\n");
			scb->parent_scb_ptr->next_scb_ptr = scb;

		} else if (scb_child_type == SCB_ON_PARENT_SUBLIST_SCB) {
			snd_assert ( (scb->parent_scb_ptr->sub_list_ptr == ins->the_null_scb),
				     return NULL);

			snd_printdd("[2]\n");
			scb->parent_scb_ptr->sub_list_ptr = scb;
		} else {
			snd_printk("invalid child type %d\n", scb_child_type);
			return NULL;
		}

		snd_printdd("[3] %04X - %04X \n",scb->parent_scb_ptr->address + SCBsubListPtr,
			(scb->parent_scb_ptr->sub_list_ptr->address << 0x10) |
			(scb->parent_scb_ptr->next_scb_ptr->address));
		/* update entry in DSP RAM */
		snd_cs46xx_poke(chip,
				(scb->parent_scb_ptr->address + SCBsubListPtr) << 2,
				(scb->parent_scb_ptr->sub_list_ptr->address << 0x10) |
				(scb->parent_scb_ptr->next_scb_ptr->address));
	}

	return scb;
}

dsp_scb_descriptor_t * 
cs46xx_dsp_create_timing_master_scb (cs46xx_t *chip)
{
	dsp_scb_descriptor_t * scb;
  
	timing_master_scb_t timing_master_scb = {
		{ 0,
		  0,
		  0,
		  0
		},
		{ 0,
		  0,
		  0,
		  0,
		  0
		},
		0,0,
		0,NULL_SCB_ADDR,
		0,0,             /* extraSampleAccum:TMreserved */
		0,0,             /* codecFIFOptr:codecFIFOsyncd */
		0x0001,0x8000,   /* fracSampAccumQm1:TMfrmsLeftInGroup */
		0x0001,0x0000,   /* fracSampCorrectionQm1:TMfrmGroupLength */
		0x00060000       /* nSampPerFrmQ15 */
	};    
  
	scb = cs46xx_dsp_create_generic_scb(chip,"TimingMasterSCBInst",(u32 *)&timing_master_scb,
					    TIMINGMASTER_SCB_ADDR,
					    "TIMINGMASTER",NULL,SCB_NO_PARENT);

	return scb;
}


dsp_scb_descriptor_t * 
cs46xx_dsp_create_codec_out_scb(cs46xx_t * chip,char * codec_name,
                                u16 channel_disp,u16 fifo_addr,
                                u16 child_scb_addr,
                                u32 dest,dsp_scb_descriptor_t * parent_scb,
                                int scb_child_type)
{
	dsp_scb_descriptor_t * scb;
  
	codec_output_scb_t codec_out_scb = {
		{ 0,
		  0,
		  0,
		  0
		},
		{
			0,
			0,
			0,
			0,
			0
		},
		0,0,
		0,NULL_SCB_ADDR,
		0,                      /* COstrmRsConfig */
		0,                      /* COstrmBufPtr */
		channel_disp,fifo_addr, /* leftChanBaseIOaddr:rightChanIOdisp */
		0x0000,0x0080,          /* (!AC97!) COexpVolChangeRate:COscaleShiftCount */
		0,child_scb_addr        /* COreserved - need child scb to work with rom code */
	};
  
  
	scb = cs46xx_dsp_create_generic_scb(chip,codec_name,(u32 *)&codec_out_scb,
					    dest,"S16_CODECOUTPUTTASK",parent_scb,
					    scb_child_type);
  
	return scb;
}

dsp_scb_descriptor_t * 
cs46xx_dsp_create_codec_in_scb(cs46xx_t * chip,char * codec_name,
                                u16 channel_disp,u16 fifo_addr,
                                u16 sample_buffer_addr,
                                u32 dest,dsp_scb_descriptor_t * parent_scb,
                                int scb_child_type)
{

	dsp_scb_descriptor_t * scb;
	codec_input_scb_t codec_input_scb = {
		{ 0,
		  0,
		  0,
		  0
		},
		{
			0,
			0,
			0,
			0,
			0
		},
    
#if 0  /* cs4620 */
		SyncIOSCB,NULL_SCB_ADDR
#else
		0 , 0,
#endif
		0,0,

		RSCONFIG_SAMPLE_16STEREO + RSCONFIG_MODULO_64,  /* strmRsConfig */
		sample_buffer_addr << 0x10,       /* strmBufPtr; defined as a dword ptr, used as a byte ptr */
		channel_disp,fifo_addr,           /* (!AC97!) leftChanBaseINaddr=AC97primary 
						     link input slot 3 :rightChanINdisp=""slot 4 */
		0x0000,0x0000,                    /* (!AC97!) ????:scaleShiftCount; no shift needed 
						     because AC97 is already 20 bits */
		0x80008000                        /* ??clw cwcgame.scb has 0 */
	};
  
	scb = cs46xx_dsp_create_generic_scb(chip,codec_name,(u32 *)&codec_input_scb,
					    dest,"S16_CODECINPUTTASK",parent_scb,
					    scb_child_type);
	return scb;
}


dsp_scb_descriptor_t * 
cs46xx_dsp_create_pcm_reader_scb(cs46xx_t * chip,char * scb_name,
                                 u16 sample_buffer_addr,u32 dest,int virtual_channel,
                                 dsp_scb_descriptor_t * parent_scb,
                                 int scb_child_type)
{
	dsp_scb_descriptor_t * scb;

	generic_scb_t pcm_reader_scb = {
    
		/*
		  Play DMA Task xfers data from host buffer to SP buffer
		  init/runtime variables:
		  PlayAC: Play Audio Data Conversion - SCB loc: 2nd dword, mask: 0x0000F000L
		  DATA_FMT_16BIT_ST_LTLEND(0x00000000L)   from 16-bit stereo, little-endian
		  DATA_FMT_8_BIT_ST_SIGNED(0x00001000L)   from 8-bit stereo, signed
		  DATA_FMT_16BIT_MN_LTLEND(0x00002000L)   from 16-bit mono, little-endian
		  DATA_FMT_8_BIT_MN_SIGNED(0x00003000L)   from 8-bit mono, signed
		  DATA_FMT_16BIT_ST_BIGEND(0x00004000L)   from 16-bit stereo, big-endian
		  DATA_FMT_16BIT_MN_BIGEND(0x00006000L)   from 16-bit mono, big-endian
		  DATA_FMT_8_BIT_ST_UNSIGNED(0x00009000L) from 8-bit stereo, unsigned
		  DATA_FMT_8_BIT_MN_UNSIGNED(0x0000b000L) from 8-bit mono, unsigned
		  ? Other combinations possible from:
		  DMA_RQ_C2_AUDIO_CONVERT_MASK    0x0000F000L
		  DMA_RQ_C2_AC_NONE               0x00000000L
		  DMA_RQ_C2_AC_8_TO_16_BIT        0x00001000L
		  DMA_RQ_C2_AC_MONO_TO_STEREO     0x00002000L
		  DMA_RQ_C2_AC_ENDIAN_CONVERT     0x00004000L
		  DMA_RQ_C2_AC_SIGNED_CONVERT     0x00008000L
        
		  HostBuffAddr: Host Buffer Physical Byte Address - SCB loc:3rd dword, Mask: 0xFFFFFFFFL
		  aligned to dword boundary
		*/
		/* Basic (non scatter/gather) DMA requestor (4 ints) */
		{ DMA_RQ_C1_SOURCE_ON_HOST +        /* source buffer is on the host */
		  DMA_RQ_C1_SOURCE_MOD1024 +        /* source buffer is 1024 dwords (4096 bytes) */
		  DMA_RQ_C1_DEST_MOD32 +            /* dest buffer(PCMreaderBuf) is 32 dwords*/
		  DMA_RQ_C1_WRITEBACK_SRC_FLAG +    /* ?? */
		  DMA_RQ_C1_WRITEBACK_DEST_FLAG +   /* ?? */
		  15,                             /* DwordCount-1: picked 16 for DwordCount because Jim */
		  /*        Barnette said that is what we should use since */
		  /*        we are not running in optimized mode? */
		  DMA_RQ_C2_AC_NONE +
		  DMA_RQ_C2_SIGNAL_SOURCE_PINGPONG + /* set play interrupt (bit0) in HISR when source */
		  /*   buffer (on host) crosses half-way point */
		  virtual_channel,                   /* Play DMA channel arbitrarily set to 0 */
		  0x0,                               /* HostBuffAddr (source) */
		  DMA_RQ_SD_SP_SAMPLE_ADDR +         /* destination buffer is in SP Sample Memory */
		  sample_buffer_addr                 /* SP Buffer Address (destination) */
		},
		/* Scatter/gather DMA requestor extension   (5 ints) */
		{
			0,
			0,
			0,
			0,
			0 
		},
		/* Sublist pointer & next stream control block (SCB) link. */
		NULL_SCB_ADDR,NULL_SCB_ADDR,
		/* Pointer to this tasks parameter block & stream function pointer */
		0,NULL_SCB_ADDR,
		/* rsConfig register for stream buffer (rsDMA reg. is loaded from basicReq.daw */
		/*   for incoming streams, or basicReq.saw, for outgoing streams) */
		RSCONFIG_DMA_ENABLE +                 /* enable DMA */
		(19 << RSCONFIG_MAX_DMA_SIZE_SHIFT) + /* MAX_DMA_SIZE picked to be 19 since SPUD  */
		/*  uses it for some reason */
		((dest >> 4) << RSCONFIG_STREAM_NUM_SHIFT) + /* stream number = SCBaddr/16 */
		RSCONFIG_SAMPLE_16STEREO +
		RSCONFIG_MODULO_32,             /* dest buffer(PCMreaderBuf) is 32 dwords (256 bytes) */
		/* Stream sample pointer & MAC-unit mode for this stream */
		(sample_buffer_addr << 0x10),
		/* Fractional increment per output sample in the input sample buffer */
		0, 
		{
			/* Standard stereo volume control */
			0x8000,0x8000,
			0x8000,0x8000
		}
	};

	scb = cs46xx_dsp_create_generic_scb(chip,scb_name,(u32 *)&pcm_reader_scb,
					    dest,"NULLALGORITHM",parent_scb,
					    scb_child_type);
  
	return scb;
}

dsp_scb_descriptor_t * 
cs46xx_dsp_create_src_task_scb(cs46xx_t * chip,char * scb_name,
                               u16 src_buffer_addr,
                               u16 src_delay_buffer_addr,u32 dest,
                               dsp_scb_descriptor_t * parent_scb,
                               int scb_child_type)
{
	dsp_scb_descriptor_t * scb;
  
	src_task_scb_t src_task_scb = {
		0x0028,0x00c8,
		0x5555,0x0000,
		0x0000,0x0000,
		src_buffer_addr,1,
		0x0028,0x00c8,
		RSCONFIG_SAMPLE_16STEREO + RSCONFIG_MODULO_32,  
		0x0000,src_delay_buffer_addr,                  
		0x0,                                            
		0x80,(src_delay_buffer_addr + (24 * 4)),
		0,0, /* next_scb, sub_list_ptr */
		0,0, /* entry, this_spb */
		RSCONFIG_SAMPLE_16STEREO + RSCONFIG_MODULO_8,
		src_buffer_addr << 0x10,
		0x04000000,
		{ 
			0x8000,0x8000,
			0xFFFF,0xFFFF
		}
	};

	scb = cs46xx_dsp_create_generic_scb(chip,scb_name,(u32 *)&src_task_scb,
					    dest,"S16_UPSRC",parent_scb,
					    scb_child_type);

	return scb;
}


dsp_scb_descriptor_t * 
cs46xx_dsp_create_mix_only_scb(cs46xx_t * chip,char * scb_name,
                               u16 mix_buffer_addr,u32 dest,
                               dsp_scb_descriptor_t * parent_scb,
                               int scb_child_type)
{
	dsp_scb_descriptor_t * scb;
  
	mix_only_scb_t master_mix_scb = {
		/* 0 */ { 0,
			  /* 1 */   0,
			  /* 2 */  mix_buffer_addr,
			  /* 3 */  0
			  /*   */ },
		{
			/* 4 */  0,
			/* 5 */  0,
			/* 6 */  0,
			/* 7 */  0,
			/* 8 */  0x00000080
		},
		/* 9 */ 0,0,
		/* A */ 0,0,
		/* B */ RSCONFIG_SAMPLE_16STEREO + RSCONFIG_MODULO_64,
		/* C */ (mix_buffer_addr  + (32 * 4)) << 0x10, 
		/* D */ 0,
		{
			/* E */ 0x8000,0x8000,
			/* F */ 0xFFFF,0xFFFF
		}
	};


	scb = cs46xx_dsp_create_generic_scb(chip,scb_name,(u32 *)&master_mix_scb,
					    dest,"S16_MIX",parent_scb,
					    scb_child_type);
	return scb;
}


dsp_scb_descriptor_t * 
cs46xx_dsp_create_mix_to_ostream_scb(cs46xx_t * chip,char * scb_name,
                                     u16 mix_buffer_addr,u16 writeback_spb,u32 dest,
                                     dsp_scb_descriptor_t * parent_scb,
                                     int scb_child_type)
{
	dsp_scb_descriptor_t * scb;

	mix2_ostream_scb_t mix2_ostream_scb = {
		/* Basic (non scatter/gather) DMA requestor (4 ints) */
		{ 
			DMA_RQ_C1_SOURCE_MOD64 +
			DMA_RQ_C1_DEST_ON_HOST +
			DMA_RQ_C1_DEST_MOD1024 +
			DMA_RQ_C1_WRITEBACK_SRC_FLAG + 
			DMA_RQ_C1_WRITEBACK_DEST_FLAG +
			15,                            
      
			DMA_RQ_C2_AC_NONE +
			DMA_RQ_C2_SIGNAL_DEST_PINGPONG + 
      
			1,                                 
			DMA_RQ_SD_SP_SAMPLE_ADDR + 
			mix_buffer_addr, 
			0x0                   
		},
    
		{ 0, 0, 0, 0, 0, },
		0,0,
		0,writeback_spb,
    
		RSCONFIG_DMA_ENABLE + 
		(19 << RSCONFIG_MAX_DMA_SIZE_SHIFT) + 
    
		((dest >> 4) << RSCONFIG_STREAM_NUM_SHIFT) +
		RSCONFIG_DMA_TO_HOST + 
		RSCONFIG_SAMPLE_16STEREO +
		RSCONFIG_MODULO_64,    
		(mix_buffer_addr + (32 * 4)) << 0x10,
		1,0,            
		0x0001,0x0080,
		0xFFFF,0
	};


	scb = cs46xx_dsp_create_generic_scb(chip,scb_name,(u32 *)&mix2_ostream_scb,
					    dest,"S16_MIX_TO_OSTREAM",parent_scb,
					    scb_child_type);
  
	return scb;
}


dsp_scb_descriptor_t * 
cs46xx_dsp_create_vari_decimate_scb(cs46xx_t * chip,char * scb_name,
                                    u16 vari_buffer_addr0,
                                    u16 vari_buffer_addr1,
                                    u32 dest,
                                    dsp_scb_descriptor_t * parent_scb,
                                    int scb_child_type)
{

	dsp_scb_descriptor_t * scb;
  
	vari_decimate_scb_t vari_decimate_scb = {
		0x0028,0x00c8,
		0x5555,0x0000,
		0x0000,0x0000,
		vari_buffer_addr0,vari_buffer_addr1,
    
		0x0028,0x00c8,
		RSCONFIG_SAMPLE_16STEREO + RSCONFIG_MODULO_256, 
    
		0xFF800000,   
		0,
		0x0080,vari_buffer_addr1 + (25 * 4), 
    
		0,0, 
		0,0,

		RSCONFIG_SAMPLE_16STEREO + RSCONFIG_MODULO_8,
		vari_buffer_addr0 << 0x10,   
		0x04000000,                   
		{
			0x8000,0x8000, 
			0xFFFF,0xFFFF
		}
	};

	scb = cs46xx_dsp_create_generic_scb(chip,scb_name,(u32 *)&vari_decimate_scb,
					    dest,"VARIDECIMATE",parent_scb,
					    scb_child_type);
  
	return scb;
}


dsp_scb_descriptor_t * 
cs46xx_dsp_create_pcm_serial_input_scb(cs46xx_t * chip, char * scb_name, u32 dest,
                                       dsp_scb_descriptor_t * input_scb,
                                       dsp_scb_descriptor_t * parent_scb,
                                       int scb_child_type)
{

	dsp_scb_descriptor_t * scb;

	pcm_serial_input_scb_t pcm_serial_input_scb = {
		{ 0,
		  0,
		  0,
		  0
		},
		{
			0,
			0,
			0,
			0,
			0
		},

		0,0,
		0,0,

		0x000000c1,
		0,
		0,input_scb->address, 
		{
			0x8000,0x8000,
			0x8000,0x8000
		}
	};

	scb = cs46xx_dsp_create_generic_scb(chip, scb_name, (u32 *)&pcm_serial_input_scb,
					    dest, "PCMSERIALINPUTTASK", parent_scb,
					    scb_child_type);
	return scb;
}


dsp_scb_descriptor_t * 
cs46xx_dsp_create_asynch_fg_tx_scb(cs46xx_t * chip,char * scb_name,u32 dest,
                                   u16 hfg_scb_address,
                                   u16 asynch_buffer_address,
                                   dsp_scb_descriptor_t * parent_scb,
                                   int scb_child_type)
{

	dsp_scb_descriptor_t * scb;

	asynch_fg_tx_scb_t asynch_fg_tx_scb = {
		0xf800,0x07ff,      /*  Prototype sample buffer size of 512 dwords */
		0x0058,0x0028,      /* Min Delta 7 dwords == 28 bytes */
		/* : Max delta 25 dwords == 100 bytes */
		0,hfg_scb_address,  /* Point to HFG task SCB */
		0,0,				/* Initialize current Delta and Consumer ptr adjustment count */
		0,                  /* Initialize accumulated Phi to 0 */
		0,0x2aab,           /* Const 1/3 */
    
		{
			0,                /* Define the unused elements */
			0,
			0
		},
    
		0,0,
		0,dest + AFGTxAccumPhi,
    
		0x000000c6,                       /* Stereo, 512 dword */
		(asynch_buffer_address) << 0x10,  /* This should be automagically synchronized
						     to the producer pointer */
    
		/* There is no correct initial value, it will depend upon the detected
		   rate etc  */
		0x18000000,                     /* Phi increment for approx 32k operation */
		0x8000,0x8000,                  /* Volume controls are unused at this time */
		0x8000,0x8000
	};
  
	scb = cs46xx_dsp_create_generic_scb(chip,scb_name,(u32 *)&asynch_fg_tx_scb,
					    dest,"ASYNCHFGTXCODE",parent_scb,
					    scb_child_type);

	return scb;
}


dsp_scb_descriptor_t * 
cs46xx_dsp_create_asynch_fg_rx_scb(cs46xx_t * chip,char * scb_name,u32 dest,
                                   u16 hfg_scb_address,
                                   u16 asynch_buffer_address,
                                   dsp_scb_descriptor_t * parent_scb,
                                   int scb_child_type)
{

	dsp_scb_descriptor_t * scb;

	asynch_fg_rx_scb_t asynch_fg_rx_scb = {
		0xff00,0x00ff,      /* Prototype sample buffer size of 512 dwords */
		0x0064,0x001c,      /* Min Delta 7 dwords == 28 bytes */
		/* : Max delta 25 dwords == 100 bytes */
		0,hfg_scb_address,  /* Point to HFG task SCB */
		0,0,				/* Initialize current Delta and Consumer ptr adjustment count */
		{
			0,                /* Define the unused elements */
			0,
			0,
			0,
			0
		},
      
		0,0,
		0,dest,
    
		0x000000c3,                            /* Stereo, 512 dword */
		(asynch_buffer_address  << 0x10),      /* This should be automagically 
							  synchrinized to the producer pointer */
    
		/* There is no correct initial value, it will depend upon the detected
		   rate etc  */
		0,         
		0x8000,0x8000,       
		0xFFFF,0xFFFF
	};

	scb = cs46xx_dsp_create_generic_scb(chip,scb_name,(u32 *)&asynch_fg_rx_scb,
					    dest,"ASYNCHFGRXCODE",parent_scb,
					    scb_child_type);

	return scb;
}


dsp_scb_descriptor_t * 
cs46xx_dsp_create_output_snoop_scb(cs46xx_t * chip,char * scb_name,u32 dest,
                                   u16 snoop_buffer_address,
                                   dsp_scb_descriptor_t * snoop_scb,
                                   dsp_scb_descriptor_t * parent_scb,
                                   int scb_child_type)
{

	dsp_scb_descriptor_t * scb;
  
	output_snoop_scb_t output_snoop_scb = {
		{ 0,	/*  not used.  Zero */
		  0,
		  0,
		  0,
		},
		{
			0, /* not used.  Zero */
			0,
			0,
			0,
			0
		},
    
		0,0,
		0,0,
    
		0x000000c3,
		snoop_buffer_address << 0x10,  
		0,0,
		0,
		0,snoop_scb->address
	};
  
	scb = cs46xx_dsp_create_generic_scb(chip,scb_name,(u32 *)&output_snoop_scb,
					    dest,"OUTPUTSNOOP",parent_scb,
					    scb_child_type);
	return scb;
}


dsp_scb_descriptor_t * 
cs46xx_dsp_create_spio_write_scb(cs46xx_t * chip,char * scb_name,u32 dest,
                                 dsp_scb_descriptor_t * parent_scb,
                                 int scb_child_type)
{
	dsp_scb_descriptor_t * scb;
  
	spio_write_scb_t spio_write_scb = {
		0,0,         /*   SPIOWAddress2:SPIOWAddress1; */
		0,           /*   SPIOWData1; */
		0,           /*   SPIOWData2; */
		0,0,         /*   SPIOWAddress4:SPIOWAddress3; */
		0,           /*   SPIOWData3; */
		0,           /*   SPIOWData4; */
		0,0,         /*   SPIOWDataPtr:Unused1; */
		{ 0,0 },     /*   Unused2[2]; */
    
		0,0,	     /*   SPIOWChildPtr:SPIOWSiblingPtr; */
		0,0,         /*   SPIOWThisPtr:SPIOWEntryPoint; */
    
		{ 
			0,
			0,
			0,
			0,
			0          /*   Unused3[5];  */
		}
	};

	scb = cs46xx_dsp_create_generic_scb(chip,scb_name,(u32 *)&spio_write_scb,
					    dest,"SPIOWRITE",parent_scb,
					    scb_child_type);

	return scb;
}

dsp_scb_descriptor_t *
cs46xx_dsp_create_magic_snoop_scb(cs46xx_t * chip,char * scb_name,u32 dest,
				  u16 snoop_buffer_address,
				  dsp_scb_descriptor_t * snoop_scb,
				  dsp_scb_descriptor_t * parent_scb,
				  int scb_child_type)
{
	dsp_scb_descriptor_t * scb;
  
	magic_snoop_task_t magic_snoop_scb = {
		/* 0 */ 0, /* i0 */
		/* 1 */ 0, /* i1 */
		/* 2 */ snoop_buffer_address << 0x10,
		/* 3 */ 0,snoop_scb->address,
		/* 4 */ 0, /* i3 */
		/* 5 */ 0, /* i4 */
		/* 6 */ 0, /* i5 */
		/* 7 */ 0, /* i6 */
		/* 8 */ 0, /* i7 */
		/* 9 */ 0,0, /* next_scb, sub_list_ptr */
		/* A */ 0,0, /* entry_point, this_ptr */
		/* B */ RSCONFIG_SAMPLE_16STEREO + RSCONFIG_MODULO_64,
		/* C */ (snoop_buffer_address + (48 * 4))  << 0x10,
		/* D */ 0,
		/* E */ { 0x8000,0x8000,
			  /* F */   0xffff,0xffff
		}
	};

	scb = cs46xx_dsp_create_generic_scb(chip,scb_name,(u32 *)&magic_snoop_scb,
					    dest,"MAGICSNOOPTASK",parent_scb,
					    scb_child_type);

	return scb;
}
