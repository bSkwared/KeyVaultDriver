#include <stdio.h>
#include "key_vault.h"

#define  MAX_USERS 20

int main () {
   char key[MAX_KEY_SIZE];
   char val[MAX_VAL_SIZE];
   int  uid = -1;

   init_vault(MAX_USERS);

   while (uid != 0) {
      printf("Enter a user id:  ");
      scanf("%d", &uid);

      if (uid < 0 || uid > MAX_USERS) {
         fprintf(stderr, "Error:  invalid uid (%d)\n", uid);
         continue;
      }
      
      if (uid == 0) continue;

      printf("Enter key-value pair:  ");
      scanf("%s %s", key, val);

      if (insert_pair(uid, key, val)) 
         printf("Success inserting [%s %s] for user %d\n", key, val, uid);
      else
         fprintf(stderr, "Error inserting [%s %s] for user %d\n", key,val,uid);
   }

   dump_vault();

   return 0;
}
