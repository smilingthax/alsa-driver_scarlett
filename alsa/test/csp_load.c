/* PLEASE CHECK http:/www.kss-loka.si/~uros/CSP.html */
/* FOR NEW VERSIONS OF THIS APPLICATION AND CSP DRIVER */

#include "asound.h"
#include "sb16_csp.h"

#include <errno.h>
#include <string.h>

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#define ERR 1;


int main(int argc, char *argv[])
{

	char *device = "/dev/snd/hwC0D2";
	int fd;

	FILE *fp;

	snd_sb_csp_microcode_t microcode;

	if (argc < 3) {
		printf("Usage: %s csp_file func_nr\n", argv[0]);
		return -ERR;
	}
	/* open CSP hw_dep device */
	if ((fd = open(device, O_WRONLY)) == -1) {
		printf("Can't open CSP device (%s)\n", device);
		return -ERR;
	}

		/* open csp microcode file */
		if ((fp = fopen(argv[1], "rb")) == NULL) {
			printf("Can't open %s microcode file!\n", argv[2]);
			goto __close_device_exit;
		}
		/* read file to data buffer */
		if (fread(&microcode.data, sizeof(char),
			  SND_SB_CSP_MAX_MICROCODE_FILE_SIZE, fp) == SND_SB_CSP_MAX_MICROCODE_FILE_SIZE) {
			printf("File too long to fit in buffer!\n");
			goto __close_file_exit;
		}
		/* load microcode to CSP */
		strcpy(microcode.info.codec_name, "QSOUND");
		microcode.info.func_req = atoi(argv[2]);
		if (ioctl(fd, SND_SB_CSP_IOCTL_LOAD_CODE, &microcode) != 0) {
			printf("Error loading microcode to CSP!\n");
			goto __close_file_exit;
		}

		fclose(fp);
		close(fd);
		return 0;

	      __close_file_exit:
		fclose(fp);

	      __close_device_exit:
		close(fd);
		return -ERR;
}
