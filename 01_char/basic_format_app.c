
#include <stdio.h>

#define DEVICE_NMAE "TestDrv"


int main(int argc, char *argv[])
{
    char command;

	fd = open(DEVICE_NAME, O_RDWR, 777);
	if (fd < 0) {
		return FALSE;
	}

	while (1) {
		printf("command >> ");
		scanf("%c", &command);

		switch (command) {
		case 'T':
			ioctl(fd, 0, 0);
			break;
		default:
			break;
		}
	}
	close(fd);
    return 0;
}


