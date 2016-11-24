
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
#include <string.h>
//#include "key_vault.h"

#define  MAX_USERS 20
#define  BUF_SIZE  80

int main () {
	char buf[BUF_SIZE];
	int  fd;
	int  count = BUF_SIZE;

   if ((fd = open ("/dev/kv_mod", O_RDONLY)) == -1) {
     perror("opening file");
     return -1;
   }

	printf("Printing keys in vault:\n");
	int i  = 1;
	int rc;
	while (rc = read(fd, buf, count)) {
		if (count > 0) {
			printf("Key %2d:  %s\n", i, buf);
			i++;
		} else {
			perror("read");
			break;
		}
	}

   close(fd);

   return 0;
}
