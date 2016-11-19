
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
#include "key_vault.h"

#define  MAX_USERS 20
#define  BUF_SIZE  80

#define KV_MOD_IOC_MAGIC  'k'
#define KV_MOD_IOCSKEY     _IOW (KV_MOD_IOC_MAGIC,   1, char)
#define KV_MOD_IOCGKEY     _IOR (KV_MOD_IOC_MAGIC,   5, char)

int main () {
	char buf[BUF_SIZE];
   char key[MAX_KEY_SIZE] = "";
   char val[MAX_VAL_SIZE];
	int  fd;
	int  count = BUF_SIZE;

   if ((fd = open ("/dev/kv_mod", O_RDWR)) == -1) {
     perror("opening file");
     return -1;
   }

	printf("Printing keys in vault:\n");
	int i  = 1;
	int rc;
	while (rc = read(fd, buf, count)) {
		if (count > 0) {
			printf("Key %2d:  %s\n", i, buf);
			if (i == 1) {
				sscanf(buf, "%s %s", key, val);
			}
			i++;
		} else {
			perror("read");
			break;
		}
	}

	/* seek to first key */
	sprintf(buf, "%s %s", key, val);
	ioctl(fd, KV_MOD_IOCSKEY, buf);
	rc = lseek(fd, 0, 0);
	if (rc) fprintf(stderr, "Resetting to key '%s'\n", buf);

	/* delete odd keys, print even keys */
	i  = 1;
	while (rc = read(fd, buf, count)) {
		if (count > 0) {
			if (i % 2) {
				ioctl(fd, KV_MOD_IOCSKEY, buf);
				rc = lseek(fd, 0, 0);
				if (rc) write(fd, "", 1);
			} else {
				printf("Key %2d:  %s\n", i, buf);
			}
			i++;
		} else {
			perror("read");
			break;
		}
	}

   close(fd);

   return 0;
}
