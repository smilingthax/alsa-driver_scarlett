#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/soundcard.h>

#define DEVICE "/dev/dsp"

void main( void )
{
  int fd, i, r;

  fd = open( DEVICE, O_RDONLY );
  if ( fd < 0 ) { perror( "open (record)" ); return; }
  i = AFMT_U16_LE;
  if ( ( r = ioctl( fd, SNDCTL_DSP_SETFMT, &i ) ) < 0 ) { perror( "setfmt" ); return; }
  printf( "afmt = %i, result = %i (%i)\n", AFMT_U16_LE, i, r );
  i = 1;
  if ( ( r = ioctl( fd, SNDCTL_DSP_CHANNELS, &i ) ) < 0 ) { perror( "channels" ); return; }
  printf( "channels = 1, result = %i (%i)\n", i, r );
  i = 22050;
  if ( ( r = ioctl( fd, SNDCTL_DSP_SPEED, &i ) ) < 0 ) { perror( "rate" ); return; }
  printf( "rate = 22050, result = %i (%i)\n", i, r );
  i = 0x2000d;
  if ( ( r = ioctl( fd, SNDCTL_DSP_SETFRAGMENT, &i ) ) < 0 ) { perror( "setfragment" ); return; }
  printf( "fragment = 0x2000d, result = 0x%x (%i)\n", i, r );
  close( fd );
}
