#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <linux/unistd.h>

#define MAX_LOOP	1000000
int main(int argc, char *argv[])
{
	int fd, i, success, fail;
	char *o_file = "/bin/ls";

	if (argc > 1)
		o_file = argv[1];

	for (i=0, success=0, fail=0; i<MAX_LOOP; i++) {
//	for (i=0, success=0, fail=0; !i; i++) {
		fd = open(o_file, O_RDONLY);
		if (fd != -1) {
			success++;
			close(fd);
		} else {
			printf("open: %s\n", strerror(errno));
			fail++;
		}
	}
	printf("Iterations: %d\tSuccess: %d\t Fail: %d\n",
			MAX_LOOP, success, fail);
	
	return 0;
}
