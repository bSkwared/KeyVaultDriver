
/* Author:  Keith Shomper
 * Date:    8 Nov 2016
 * Purpose: Rudimentary testing for the key vault implementation
 *          that is embedded in a kernel module.  See main.c for
 *          for a test program that tests a user space version of
 *          the key_vault (Key-Vault-V3).
 */

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>

/*
 * Ioctl definitions
 */

/* Use 'k' as magic number - please use a different 8-bit number in your code */
#define KV_IOC_MAGIC 'B'
#define KV_SEEK_KEY  _IOW (KV_IOC_MAGIC,   0, char*)
#define KV_SEEK_PAIR _IOW (KV_IOC_MAGIC,   1, char*)
#define KV_NUM_KEYS  _IO  (KV_IOC_MAGIC,   2       )

int main (int argc, char **argv) {
	int  fd;
	int  rc;

	if (argc != 2) {
		fprintf(stderr, "Usage:  %s \"key value\"\n", argv[0]);
		return 1;
	}

   if ((fd = open ("/dev/kv_mod", O_WRONLY)) == -1) {
     perror("opening file");
     return -1;
   }
	
	printf("Setting current key is %s\n", argv[1]);
	ioctl(fd, KV_SEEK_PAIR, argv[1]);
	rc = llseek(fd, 0, 0);
	printf("lseek return code is %d\n", rc);

	if (rc) {
		write(fd, "", 1);
	} else {
		fprintf(stderr, "seek failed\n");
	}

   close(fd);

   return 0;
}
