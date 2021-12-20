#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <linux/unistd.h>

#include "sys/apparmor.h"

#define SD_ID_MAGIC 0xdeadbeef

int main(int argc, char *argv[])
{
	int fd, error;
	char *hat_name;
	int hat_magic;
	char *o_file = "/bin/ls";

	while (1) {
	hat_name = "/subprofile/foo";
	hat_magic = SD_ID_MAGIC + 1;
	if (argc > 1)
		hat_name = argv[1];

	printf("before entering change_hat\n");
	error = change_hat(hat_name, hat_magic);
	printf("change_hat(%s, 0x%x): %s\n", hat_name, hat_magic,
							strerror(errno));

	errno = 0;
	fd = open(o_file, O_RDONLY);
	printf("open(%s): %s\n", o_file, strerror(errno));
	if (fd != -1)
		close(fd);
	
	hat_name = NULL;
	printf("before leaving change_hat\n");
	errno = 0;
	error = change_hat(hat_name, hat_magic);
	printf("change_hat(%s, 0x%x): %s\n", "NULL", hat_magic,
							strerror(errno));

	errno = 0;
	fd = open(o_file, O_RDONLY);
	printf("open(%s): %s\n", o_file, strerror(errno));
	if (fd != -1)
		close(fd);
	}
}
