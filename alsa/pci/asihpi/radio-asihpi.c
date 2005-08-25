/* radio-asihpi.c
 *
 * A device driver for AudioScience radio cards.
 *
 * Copyright 2004 Fred Gleason <fredg@salemradiolabs.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public
 *   License along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <hpi.h>

#include <linux/autoconf.h>
#include <linux/module.h>
#include <linux/videodev.h>

#if LINUX_VERSION_CODE >= 0x20500

#include <linux/config.h>

#define MOD_INC_USE_COUNT do { } while (0)
#define MOD_DEC_USE_COUNT do { } while (0)

#endif

/*
 * This will eventually go in videodev.h
 */
#define VID_HARDWARE_ASIHPI 63


/*
 * Globals
 */
HPI_HSUBSYS *hpi_subsys=NULL;
struct radio_asihpi_tuner {
  int active;
  int bands;   /* 0 = TV/FM, 1 = AM/FM */
  struct video_device videodev;
};

struct radio_asihpi_tuner radio_asihpi_tuner[HPI_MAX_ADAPTERS][8];


int radio_asihpi_getband(int card,int tuner)
{
  HPI_HMIXER hpi_mixer;
  HPI_HCONTROL hpi_control;
  HW16 hpi_band=0;

  if(HPI_AdapterOpen(hpi_subsys,card)!=0) {
    return -1;
  }
  if(HPI_MixerOpen(hpi_subsys,card,&hpi_mixer)!=0) {
    HPI_AdapterClose(hpi_subsys,card);
    return -1;
  }
  if(HPI_MixerGetControl(hpi_subsys,hpi_mixer,HPI_SOURCENODE_TUNER,tuner,
			 0,0,HPI_CONTROL_TUNER,&hpi_control)!=0) {
    HPI_MixerClose(hpi_subsys,hpi_mixer);
    HPI_AdapterClose(hpi_subsys,card);
    return -1;
  }
  if(HPI_Tuner_GetBand(hpi_subsys,hpi_control,&hpi_band)!=0) {
    HPI_MixerClose(hpi_subsys,hpi_mixer);
    HPI_AdapterClose(hpi_subsys,card);
    return -1;
  }
  HPI_MixerClose(hpi_subsys,hpi_mixer);
  HPI_AdapterClose(hpi_subsys,card);

  switch(radio_asihpi_tuner[card][tuner].bands) {
      case 0:  /* FM/TV */
	switch(hpi_band) {
	    case HPI_TUNER_BAND_FM:
	      return 0;
	    case HPI_TUNER_BAND_FM_STEREO:
	      return 1;
	    case HPI_TUNER_BAND_TV:
	      return 2;
	}
	break;
      case 1:  /* FM/AM */
	switch(hpi_band) {
	    case HPI_TUNER_BAND_FM:
	      return 0;
	    case HPI_TUNER_BAND_FM_STEREO:
	      return 1;
	    case HPI_TUNER_BAND_AM:
	      return 2;
	}
	break;
  }
  return -1;
}


int radio_asihpi_setband(int card,int tuner,unsigned band)
{
  HPI_HMIXER hpi_mixer;
  HPI_HCONTROL hpi_control;
  HW16 hpi_band=0;

  switch(radio_asihpi_tuner[card][tuner].bands) {
      case 0:  /* FM/TV */
	switch(band) {
	    case 0:
	      hpi_band=HPI_TUNER_BAND_FM;
	      break;

	    case 1:
	      hpi_band=HPI_TUNER_BAND_FM_STEREO;
	      break;

	    case 2:
	      hpi_band=HPI_TUNER_BAND_TV;
	      break;
	}
	break;

      case 1:  /* FM/AM */
	switch(band) {
	    case 0:
	      hpi_band=HPI_TUNER_BAND_FM;
	      break;

	    case 1:
	      hpi_band=HPI_TUNER_BAND_FM_STEREO;
	      break;

	    case 2:
	      hpi_band=HPI_TUNER_BAND_AM;
	      break;
	}
	break;
  }
  if(HPI_AdapterOpen(hpi_subsys,card)!=0) {
    return -1;
  }
  if(HPI_MixerOpen(hpi_subsys,card,&hpi_mixer)!=0) {
    HPI_AdapterClose(hpi_subsys,card);
    return -1;
  }
  if(HPI_MixerGetControl(hpi_subsys,hpi_mixer,HPI_SOURCENODE_TUNER,tuner,
			 0,0,HPI_CONTROL_TUNER,&hpi_control)!=0) {
    HPI_MixerClose(hpi_subsys,hpi_mixer);
    HPI_AdapterClose(hpi_subsys,card);
    return -1;
  }
  if(HPI_Tuner_SetBand(hpi_subsys,hpi_control,hpi_band)!=0) {
    HPI_MixerClose(hpi_subsys,hpi_mixer);
    HPI_AdapterClose(hpi_subsys,card);
    return -1;
  }
  HPI_MixerClose(hpi_subsys,hpi_mixer);
  HPI_AdapterClose(hpi_subsys,card);
  return 0;
}


int radio_asihpi_getsignal(int card,int tuner)
{
  HPI_HMIXER hpi_mixer;
  HPI_HCONTROL hpi_control;
  HW16 hpi_signal=0;

  if(HPI_AdapterOpen(hpi_subsys,card)!=0) {
    return -1;
  }
  if(HPI_MixerOpen(hpi_subsys,card,&hpi_mixer)!=0) {
    HPI_AdapterClose(hpi_subsys,card);
    return -1;
  }
  if(HPI_MixerGetControl(hpi_subsys,hpi_mixer,HPI_SOURCENODE_TUNER,tuner,
			 0,0,HPI_CONTROL_TUNER,&hpi_control)!=0) {
    HPI_MixerClose(hpi_subsys,hpi_mixer);
    HPI_AdapterClose(hpi_subsys,card);
    return -1;
  }
  if(HPI_Tuner_GetRFLevel(hpi_subsys,hpi_control,&hpi_signal)!=0) {
    HPI_MixerClose(hpi_subsys,hpi_mixer);
    HPI_AdapterClose(hpi_subsys,card);
    return -1;
  }
  HPI_MixerClose(hpi_subsys,hpi_mixer);
  HPI_AdapterClose(hpi_subsys,card);

  return hpi_signal;
}


int radio_asihpi_getfreq(int card,int tuner)
{
  HPI_HMIXER hpi_mixer;
  HPI_HCONTROL hpi_control;
  HW16 hpi_band=0;
  HW32 hpi_freq=0;

  if(HPI_AdapterOpen(hpi_subsys,card)!=0) {
    return -1;
  }
  if(HPI_MixerOpen(hpi_subsys,card,&hpi_mixer)!=0) {
    HPI_AdapterClose(hpi_subsys,card);
    return -1;
  }
  if(HPI_MixerGetControl(hpi_subsys,hpi_mixer,HPI_SOURCENODE_TUNER,tuner,
			 0,0,HPI_CONTROL_TUNER,&hpi_control)!=0) {
    HPI_MixerClose(hpi_subsys,hpi_mixer);
    HPI_AdapterClose(hpi_subsys,card);
    return -1;
  }
  if(HPI_Tuner_GetFrequency(hpi_subsys,hpi_control,&hpi_freq)!=0) {
    HPI_MixerClose(hpi_subsys,hpi_mixer);
    HPI_AdapterClose(hpi_subsys,card);
    return -1;
  }
  if(HPI_Tuner_GetBand(hpi_subsys,hpi_control,&hpi_band)!=0) {
    HPI_MixerClose(hpi_subsys,hpi_mixer);
    HPI_AdapterClose(hpi_subsys,card);
    return -1;
  }
  HPI_MixerClose(hpi_subsys,hpi_mixer);
  HPI_AdapterClose(hpi_subsys,card);
  switch(hpi_band) {
      case HPI_TUNER_BAND_FM:
      case HPI_TUNER_BAND_FM_STEREO:
      case HPI_TUNER_BAND_TV:
      case HPI_TUNER_BAND_AM:
	return 16*hpi_freq;
  }
  return -1;
}


int radio_asihpi_setfreq(int card,int tuner,unsigned freq)
{
  HPI_HMIXER hpi_mixer;
  HPI_HCONTROL hpi_control;
  HW32 hpi_freq=0;

  if(HPI_AdapterOpen(hpi_subsys,card)!=0) {
    return -1;
  }
  if(HPI_MixerOpen(hpi_subsys,card,&hpi_mixer)!=0) {
    HPI_AdapterClose(hpi_subsys,card);
    return -1;
  }
  if(HPI_MixerGetControl(hpi_subsys,hpi_mixer,HPI_SOURCENODE_TUNER,tuner,
			 0,0,HPI_CONTROL_TUNER,&hpi_control)!=0) {
    HPI_MixerClose(hpi_subsys,hpi_mixer);
    HPI_AdapterClose(hpi_subsys,card);
    return -1;
  }
  switch(radio_asihpi_tuner[card][tuner].bands) {
      case 0:  /* FM/TV */
	switch(radio_asihpi_getband(card,tuner)) {
	    case 0:  /* FM */
	    case 1:  /* FM Stereo */
	      if((freq<1216000)||(freq>1728000)) {
		HPI_MixerClose(hpi_subsys,hpi_mixer);
		HPI_AdapterClose(hpi_subsys,card);
		return -1;
	      }
	      break;

	    case 2:  /* TV */
	      if((freq<884000)||(freq>12820000)) {
		HPI_MixerClose(hpi_subsys,hpi_mixer);
		HPI_AdapterClose(hpi_subsys,card);
		return -1;
	      }
	      break;
	}
	break;

      case 1:  /* FM/AM */
	switch(radio_asihpi_getband(card,tuner)) {
	    case 0:  /* FM */
	    case 1:  /* FM Stereo */
	      if((freq<1216000)||(freq>1728000)) {
		HPI_MixerClose(hpi_subsys,hpi_mixer);
		HPI_AdapterClose(hpi_subsys,card);
		return -1;
	      }
	      break;

	    case 2:  /* AM */
	      if((freq<8320)||(freq>28000)) {
		HPI_MixerClose(hpi_subsys,hpi_mixer);
		HPI_AdapterClose(hpi_subsys,card);
		return -1;
	      }
	      break;
	}
	break;
  }
  hpi_freq=freq/16;
  if(HPI_Tuner_SetFrequency(hpi_subsys,hpi_control,hpi_freq)!=0) {
    HPI_MixerClose(hpi_subsys,hpi_mixer);
    HPI_AdapterClose(hpi_subsys,card);
    return -1;
  }

  HPI_MixerClose(hpi_subsys,hpi_mixer);
  HPI_AdapterClose(hpi_subsys,card);
  return 0;
}


int radio_asihpi_open(struct inode *inode,struct file *filp)
{
  MOD_INC_USE_COUNT;

  return 0;
}


int radio_asihpi_release(struct inode *inode,struct file *filp)
{
  MOD_DEC_USE_COUNT;

  return 0;
}


int radio_asihpi_ioctl(struct inode *inode,struct file *filp,
		       unsigned cmd,unsigned long arg)
{
  unsigned freq;
  int i;
  int j;
  int card=-1;
  int tuner=-1;

  for(i=0;i<HPI_MAX_ADAPTERS;i++) {
    for(j=0;j<8;j++) {
      if(radio_asihpi_tuner[i][j].active&&
	 (radio_asihpi_tuner[i][j].videodev.minor==MINOR(inode->i_rdev))) {
	card=i;
	tuner=j;
	i=HPI_MAX_ADAPTERS;
	j=8;
      }
    }
  }
  if((card==-1)||(tuner==-1)) {
    return -EINVAL;    // Is this really the approriate return here?
  }

  switch(cmd)
  {
      case VIDIOCGCAP:
      {
	struct video_capability v;
	memset(&v, 0, sizeof(v));
	v.type=VID_TYPE_TUNER;
	v.channels=3;
	v.audios=1;
	strcpy(v.name,radio_asihpi_tuner[card][tuner].videodev.name);
	//sprintf(v.name,"AudioScience 8700[%d] Tuner %d",card+1,tuner+1);
	if(copy_to_user((unsigned *)arg,&v,sizeof(v))) {
	  return -EFAULT;
	}
	return 0;
      }

      case VIDIOCGTUNER:
      {
	struct video_tuner v;
	if(copy_from_user(&v,(unsigned *)arg,sizeof(v))!=0) {
	  return -EFAULT;
	}
	v.flags=VIDEO_TUNER_LOW;
	v.mode=VIDEO_MODE_AUTO;
	switch(radio_asihpi_tuner[card][tuner].bands) {
	    case 0:    /* FM/TV */
	      switch(v.tuner) {
		  case 0:
		    strcpy(v.name,"FM");
		    v.rangelow=1216000;     // 76.0 MHz 
		    v.rangehigh=1728000;    // 108.0 MHz 
		    break;
		  case 1:
		    strcpy(v.name,"FM Stereo");
		    v.rangelow=1216000;     // 76.0 MHz 
		    v.rangehigh=1728000;    // 108.0 MHz 
		    break;
		  case 2:
		    strcpy(v.name,"TV");
		    v.rangelow=884000;       // 55.25 MHz 
		    v.rangehigh=12820000;    // 801.25 MHz 
		    break;
		  default:
		    return -EINVAL;
	      }
	      break;
	    case 1:    /* FM/AM */
	      switch(v.tuner) {
		  case 0:
		    strcpy(v.name,"FM");
		    v.rangelow=1216000;     // 76.0 MHz 
		    v.rangehigh=1728000;    // 108.0 MHz 
		    break;
		  case 1:
		    strcpy(v.name,"FM Stereo");
		    v.rangelow=1216000;     // 76.0 MHz 
		    v.rangehigh=1728000;    // 108.0 MHz 
		    break;
		  case 2:
		    strcpy(v.name,"AM");
		    v.rangelow=8320;      // 520 kHz 
		    v.rangehigh=28000;    // 1750 kHz 
		    break;
		  default:
		    return -EINVAL;
	      }
	      break;

	    default:
	      return -EINVAL;
	}
	v.signal=radio_asihpi_getsignal(card,tuner);
	if(copy_to_user((unsigned *)arg,&v, sizeof(v))) {
	  return -EFAULT;
	}
	return 0;
      }

      case VIDIOCSTUNER:
      {
	struct video_tuner v;
	if(copy_from_user(&v,(unsigned *)arg,sizeof(v))) {
	  return -EFAULT;
	}
	if(radio_asihpi_setband(card,tuner,v.tuner)<0) {
	  return -EINVAL;
	}
	return 0;

      case VIDIOCGFREQ:
	if((freq=radio_asihpi_getfreq(card,tuner))<0) {
	  return -EINVAL;
	}
	if(copy_to_user((unsigned *)arg,&freq,sizeof(freq)))
	  return -EFAULT;
	return 0;
      }

      case VIDIOCSFREQ:
      {
	if(copy_from_user(&freq,(unsigned *)arg,sizeof(freq))) {
	  return -EFAULT;
	}
	if(radio_asihpi_setfreq(card,tuner,freq)<0) {
	  return -EINVAL;
	}
	return 0;
      }
	
      case VIDIOCGAUDIO:
      {
	struct video_audio v;
	memset(&v,0, sizeof(v));
	v.mode=VIDEO_SOUND_MONO;
	sprintf(v.name,"Tuner %d",8*card+tuner+1);
	if(copy_to_user((unsigned *)arg,&v, sizeof(v)))
	  return -EFAULT;
	return 0;			
      }
      case VIDIOCSAUDIO:
      {
	  struct video_audio v;
	  if(copy_from_user(&v, (unsigned *)arg, sizeof(v)))
	      return -EFAULT;
	  if(v.audio)
	      return -EINVAL;

	  /* handle muting, volume setting
	  cadet_setvol(v.volume);
	  if(v.flags&VIDEO_AUDIO_MUTE)
	      cadet_setvol(0);
	  else
	      cadet_setvol(0xffff);
	  */
	  if (v.mode & VIDEO_SOUND_STEREO)
	      {
		  /* set stereo mode */
	      }
	  if (v.mode & VIDEO_SOUND_MONO)
	      {
		  /* set mono mode */
	      }


	  return 0;
      }
     

      default:
	return -ENOIOCTLCMD;
  }
  return 0;
}


struct file_operations radio_asihpi_fops = {
  owner:THIS_MODULE,
  open:radio_asihpi_open,
  release:radio_asihpi_release,
  ioctl:radio_asihpi_ioctl,
};


int init_module(void)
{
  int i;
  int j;
  int k;
  int found=0;
  HW16 adapters;
  HW16 adapter_list[HPI_MAX_ADAPTERS];
  HPI_HMIXER hpi_mixer;
  HPI_HCONTROL hpi_control;
  HW32 hpi_band;

  if((hpi_subsys=HPI_SubSysCreate())==NULL) {
    return -ENODEV;
  }
  if(HPI_SubSysFindAdapters(hpi_subsys,&adapters,adapter_list,
			    HPI_MAX_ADAPTERS)!=0) {
    HPI_SubSysFree(hpi_subsys);
    return -ENODEV;
  }
  memset(radio_asihpi_tuner,0,
	 HPI_MAX_ADAPTERS*8*sizeof(struct radio_asihpi_tuner));
  for(i=0;i<HPI_MAX_ADAPTERS;i++) {
      if((adapter_list[i]& 0xFF00) == 0x8700) {
      if(HPI_MixerOpen(hpi_subsys,i,&hpi_mixer)==0) {
	for(j=0;j<8;j++) {
	  if(HPI_MixerGetControl(hpi_subsys,hpi_mixer,HPI_SOURCENODE_TUNER,
				 j,0,0,HPI_CONTROL_TUNER,&hpi_control)==0) {
	    k=0;
	    while(HPI_ControlQuery(hpi_subsys,hpi_control,HPI_TUNER_BAND,
				   k,0,&hpi_band)==0) {
	      if(hpi_band==HPI_TUNER_BAND_AM) {
		radio_asihpi_tuner[i][j].bands=1;
	      }
	      k++;
	    }
	    radio_asihpi_tuner[i][j].videodev.owner=THIS_MODULE;
	    sprintf(radio_asihpi_tuner[i][j].videodev.name,
		    "AudioScience %4x[%d] Tuner %d",adapter_list[i],i+1,j+1);
	    radio_asihpi_tuner[i][j].videodev.type=VID_TYPE_TUNER;
	    radio_asihpi_tuner[i][j].videodev.hardware=VID_HARDWARE_ASIHPI;
	    radio_asihpi_tuner[i][j].videodev.fops=&radio_asihpi_fops;

#if LINUX_VERSION_CODE >= 0x20500

	    radio_asihpi_tuner[i][j].videodev.release=video_device_release;

#endif

	    if(video_register_device(&radio_asihpi_tuner[i][j].videodev,
				     VFL_TYPE_RADIO,-1)!=-1) {
	      radio_asihpi_tuner[i][j].active=1;
	      found=1;

	      radio_asihpi_setband(i,j,0); // set to FM band

	    }
	  }
	}
	HPI_MixerClose(hpi_subsys,hpi_mixer);
      }
    }
  }
  if(!found) {
    HPI_SubSysFree(hpi_subsys);
    return -ENODEV;
  }
  return 0;
}


void cleanup_module(void)
{
  int i;
  int j;

  for(i=0;i<HPI_MAX_ADAPTERS;i++) {
    for(j=0;j<8;j++) {
      if(radio_asihpi_tuner[i][j].active) {
	video_unregister_device(&radio_asihpi_tuner[i][j].videodev);
      }
    }
  }
  HPI_SubSysFree(hpi_subsys);
}


MODULE_AUTHOR("Fred Gleason <fredg@salemradiolabs.com>");
MODULE_DESCRIPTION("A V4L driver for AudioScience radio cards");
MODULE_LICENSE("GPL");

