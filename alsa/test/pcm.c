#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "../include/asound.h"

#define DEVICE "/dev/snd/pcm00"

static void show_playback_status( int fd )
{
  snd_pcm_playback_status_t pstatus;

  if ( ioctl( fd, SND_PCM_IOCTL_PLAYBACK_STATUS, &pstatus ) < 0 ) { perror( "PLAYBACK_STATUS" ); return; }
  printf( "PCM playback status:\n" );
  printf( "  rate = %i\n", pstatus.rate );
  printf( "  fragments = %i\n", pstatus.fragments );
  printf( "  fragment_size = %i\n", pstatus.fragment_size );
  printf( "  count = %i\n", pstatus.count );
  printf( "  queue = %i\n", pstatus.queue );
  printf( "  underrun = %i\n", pstatus.underrun );
  printf( "  time = %i.%i\n", (int)pstatus.time.tv_sec, (int)pstatus.time.tv_usec );  
  printf( "  stime = %i.%i\n", (int)pstatus.stime.tv_sec, (int)pstatus.stime.tv_usec );  
  printf( "  scount = %i\n", pstatus.scount );
}

static void show_record_status( int fd )
{
  snd_pcm_record_status_t rstatus;

  if ( ioctl( fd, SND_PCM_IOCTL_RECORD_STATUS, &rstatus ) < 0 ) { perror( "RECORD_STATUS" ); return; }
  printf( "PCM record status:\n" );
  printf( "  rate = %i\n", rstatus.rate );
  printf( "  fragments = %i\n", rstatus.fragments );
  printf( "  fragment_size = %i\n", rstatus.fragment_size );
  printf( "  count = %i\n", rstatus.count );
  printf( "  free = %i\n", rstatus.free );
  printf( "  overrun = %i\n", rstatus.overrun );
  printf( "  time = %i.%i\n", (int)rstatus.time.tv_sec, (int)rstatus.time.tv_usec );  
  printf( "  stime = %i.%i\n", (int)rstatus.stime.tv_sec, (int)rstatus.stime.tv_usec );
  printf( "  scount = %i\n", rstatus.scount );
}

void main( void )
{
  int fd, i;
  snd_pcm_info_t info;
  snd_pcm_playback_info_t pinfo;
  snd_pcm_record_info_t rinfo;
  char buffer[ 256 * 1024 ];

  fd = open( DEVICE, O_WRONLY );
  if ( fd < 0 ) { perror( "open (playback)" ); return; }
  if ( ioctl( fd, SND_PCM_IOCTL_PVERSION, &i ) < 0 ) { perror( "PVERSION" ); return; }
  printf( "PCM protocol version %i.%i.%i\n", i >> 16, (i >> 8) & 0xff, i & 0xff ); 
  if ( ioctl( fd, SND_PCM_IOCTL_INFO, &info ) < 0 ) { perror( "INFO" ); return; }
  printf( "PCM info:\n" );
  printf( "  type = %i\n", info.type );
  printf( "  flags = 0x%x\n", info.flags );
  printf( "  id = '%s'\n", info.id );
  printf( "  name = '%s'\n", info.name );
  if ( ioctl( fd, SND_PCM_IOCTL_PLAYBACK_INFO, &pinfo ) < 0 ) { perror( "PLAYBACK_INFO" ); return; }
  printf( "PCM playback info:\n" );
  printf( "  formats = 0x%x\n", pinfo.formats );
  printf( "  min_rate = %i\n", pinfo.min_rate );
  printf( "  max_rate = %i\n", pinfo.max_rate );
  printf( "  min_channels = %i\n", pinfo.min_channels );
  printf( "  max_channels = %i\n", pinfo.max_channels );
  printf( "  buffer_size = %i\n", pinfo.buffer_size );
  printf( "  min_fragment_size = %i\n", pinfo.min_fragment_size );
  printf( "  max_fragment_size = %i\n", pinfo.max_fragment_size );
  printf( "  fragment_align = %i\n", pinfo.fragment_align );
  i = 1;
  if ( ioctl( fd, SND_PCM_IOCTL_PLAYBACK_TIME, &i ) < 0 ) { perror( "PLAYBACK TIME" ); return; }
  show_playback_status( fd );
#if 1
  printf( "Write %i bytes...\n", write( fd, buffer, 4096 ) );
  show_playback_status( fd );
  sleep( 2 );				/* force underrun */
  show_playback_status( fd );
  printf( "Write %i bytes...\n", write( fd, buffer, 4096 ) );
  usleep( 100 );
  show_playback_status( fd );
  if ( ioctl( fd, SND_PCM_IOCTL_DRAIN_PLAYBACK ) < 0 ) { perror( "DRAIN_PLAYBACK" ); return; }
  show_playback_status( fd );
  if ( ioctl( fd, SND_PCM_IOCTL_FLUSH_PLAYBACK ) < 0 ) { perror( "FLUSH_PLAYBACK" ); return; }
  show_playback_status( fd );
#endif
  close( fd );

  printf( "\n\n" );

  fd = open( DEVICE, O_RDONLY );
  if ( fd < 0 ) { perror( "open (record)" ); return; }
  if ( ioctl( fd, SND_PCM_IOCTL_PVERSION, &i ) < 0 ) { perror( "PVERSION" ); return; }
  printf( "PCM protocol version %i.%i.%i\n", i >> 16, (i >> 8) & 0xff, i & 0xff ); 
  if ( ioctl( fd, SND_PCM_IOCTL_INFO, &info ) < 0 ) { perror( "INFO" ); return; }
  printf( "PCM info:\n" );
  printf( "  type = %i\n", info.type );
  printf( "  flags = 0x%x\n", info.flags );
  printf( "  name = '%s'\n", info.name );
  if ( ioctl( fd, SND_PCM_IOCTL_RECORD_INFO, &rinfo ) < 0 ) { perror( "RECORD_INFO" ); return; }
  printf( "PCM record info:\n" );
  printf( "  formats = 0x%x\n", rinfo.formats );
  printf( "  min_rate = %i\n", rinfo.min_rate );
  printf( "  max_rate = %i\n", rinfo.max_rate );
  printf( "  min_channels = %i\n", rinfo.min_channels );
  printf( "  max_channels = %i\n", rinfo.max_channels );
  printf( "  buffer_size = %i\n", rinfo.buffer_size );
  printf( "  min_fragment_size = %i\n", rinfo.min_fragment_size );
  printf( "  max_fragment_size = %i\n", rinfo.max_fragment_size );
  printf( "  fragment_align = %i\n", rinfo.fragment_align );
  i = 1;
  if ( ioctl( fd, SND_PCM_IOCTL_RECORD_TIME, &i ) < 0 ) { perror( "RECORD TIME" ); return; }
  show_record_status( fd );
  printf( "Read %i bytes...\n", read( fd, buffer, 4096 ) );
  usleep( 200 );
  show_record_status( fd );
  close( fd );

}
