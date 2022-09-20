#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <termio.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/utsname.h>

//#define LOG_TAG "JHC-test-gpio"

int main(int argc, char *argv[])
{
	int fd;
	char write_buf[2] = { 0 };

	if (argc != 3)
	{
		printf("usage: %s filename\n", argv[0]);
		_exit(-1);
	}

	printf("%d %d\n",*argv[1], *argv[2]);

	fd = open("/dev/relay-gpio", O_RDWR);
	if(fd < 0)
	{
		printf("device open fail\n");
		return -1;
	}
	lseek(fd, 0, SEEK_SET);

	write_buf[0] = *argv[1];
	write_buf[1] = *argv[2];
	//printf("%d %d\n", write_buf[0],write_buf[1]);
	write(fd, write_buf, 2);

	return 0;
}
