#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "../include/asound.h"

#define DEVICE "/dev/snd/mixer00"

void main( void )
{
  int fd, i;
  snd_mixer_info_t info;
  snd_mixer_channel_info_t cinfo;
  snd_mixer_channel_t r;

  fd = open( DEVICE, O_RDONLY );
  if ( fd < 0 ) { perror( "open" ); return; }
  if ( ioctl( fd, SND_MIXER_IOCTL_PVERSION, &i ) < 0 ) { perror( "PVERSION" ); return; }
  printf( "Mixer protocol version %i.%i.%i\n", i >> 16, (i >> 8) & 0xff, i & 0xff ); 
  if ( ioctl( fd, SND_MIXER_IOCTL_INFO, &info ) < 0 ) { perror( "INFO" ); return; }
  printf( "Mixer info:\n");
  printf( "  type = %i\n", info.type );
  printf( "  channels = %i\n", info.channels );
  printf( "  caps = 0x%x\n", info.caps );
  printf( "  id = '%s'\n", info.id );
  printf( "  name = '%s'\n", info.name );
  printf( "\n" );
  i = 1;
  if ( ioctl( fd, SND_MIXER_IOCTL_EXACT, &i ) < 0 ) { perror( "EXACT" ); return; }
  for ( i = 0; i < info.channels; i++ ) {
    cinfo.channel = i;
    if ( ioctl( fd, SND_MIXER_IOCTL_CHANNEL_INFO, &cinfo ) < 0 ) { perror( "CHANNEL_INFO" ); return; }
    printf( "Mixer channel %i:\n", i );
    printf( "  channel = %i\n", cinfo.channel );
    printf( "  parent = %i\n", cinfo.parent );
    printf( "  name = '%s'\n", cinfo.name );
    printf( "  caps = 0x%x\n", cinfo.caps );
    printf( "  min = %i\n", cinfo.min );
    printf( "  max = %i\n", cinfo.max );
    printf( "  min_dB = %.2f\n", ((float)cinfo.min_dB) / 100 );
    printf( "  max_dB = %.2f\n", ((float)cinfo.max_dB) / 100 );
    printf( "  step_dB = %.2f\n", ((float)cinfo.step_dB) / 100 );
  }
  printf( "\n" );
  for ( i = 0; i < info.channels; i++ ) {
    r.channel = i;
    if ( ioctl( fd, SND_MIXER_IOCTL_CHANNEL_READ, &r ) < 0 ) { perror( "CHANNEL_READ" ); return; }
    printf( "Mixer channel %i:\n", i );
    printf( "  channel = %i\n", r.channel );
    printf( "  flags = 0x%x\n", r.flags );
    printf( "  left = %i\n", r.left );
    printf( "  right = %i\n", r.right );
    printf( "  left_dB = %.2f\n", ((float)r.left_dB) / 100 );
    printf( "  right_dB = %.2f\n", ((float)r.right_dB) / 100 );
  }
  close( fd );
}
