
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
#define MAX_VAL_SIZE 30
#define MAX_KEY_SIZE 30

int main () {
	char buf[BUF_SIZE];
   char key[MAX_KEY_SIZE] = "";
   char val[MAX_VAL_SIZE];
	int  fd;
	int  count = BUF_SIZE;

   if ((fd = open ("/dev/kv_mod", O_WRONLY)) == -1) {
     perror("opening file");
     return -1;
   }

   while (strcmp(key, "q") != 0) {
      
      printf("Enter key-value pair:  ");
      scanf("%s %s", key, val);

		sprintf(buf, "%s %s", key, val);

		if (strcmp(key, "q") == 0) continue;

		int rc = write(fd, buf, count);
   }

   close(fd);

   return 0;
}
