#include "../include/sb16_csp.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define ERR 1;

int main(int argc, char *argv[])
{
	char *device = "/dev/snd/hwC0D2";
	int fd;

	FILE *fp;
	snd_sb_csp_microcode_t microcode;


	/* open CSP hw_dep device */
	if ((fd = open(device, O_WRONLY | O_NONBLOCK)) == -1) {
		printf("can't open CSP device '%s'\n", device);
		return ERR;
	}
	if ((argc == 2) && !(strcmp(argv[1], "-u"))) {
		if (ioctl(fd, SNDRV_SB_CSP_IOCTL_UNLOAD_CODE) != 0) {
			printf("error unloading microcode\n");
		} else {
		        printf("microcode unloaded\n");
		}
		goto __close_device_exit;
	}
	if (argc < 3) {
		printf("Usage: %s csp_file func_nr\n", argv[0]);
		printf("       %s -u\n", argv[0]);
		goto __close_device_exit;
	}
	/* open csp microcode file */
	if ((fp = fopen(argv[1], "rb")) == NULL) {
		printf("can't open microcode file '%s'\n", argv[1]);
		goto __close_device_exit;
	}
	/* read file to data buffer */
	if (fread(&microcode.data, sizeof(char),
		  SNDRV_SB_CSP_MAX_MICROCODE_FILE_SIZE, fp) == SNDRV_SB_CSP_MAX_MICROCODE_FILE_SIZE) {
		printf("microcode file too long\n");
		goto __close_file_exit;
	}
	/* load microcode to CSP */
	strcpy(microcode.info.codec_name, "QSOUND");
	microcode.info.func_req = atoi(argv[2]);
	if (ioctl(fd, SNDRV_SB_CSP_IOCTL_LOAD_CODE, &microcode) != 0) {
		printf("error loading microcode to CSP device\n");
		goto __close_file_exit;
	}
	fclose(fp);
	close(fd);
	return 0;

      __close_file_exit:
	fclose(fp);

      __close_device_exit:
	close(fd);
	return ERR;
}
