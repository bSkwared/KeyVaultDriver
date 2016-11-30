
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#define MAX_KEY_SIZE 20
#define MAX_VAL_SIZE 20
int kv_mod_split_pair(char* pair, char* key, char* val) {

    const char SPLIT_CHAR = ',';

    int i;
    int splitIndex = -1;

    for (i = 0; i < MAX_KEY_SIZE; ++i) {
        if (pair[i] == SPLIT_CHAR) {
            splitIndex = i;
            break;
        }
    }

    if (splitIndex == -1) {
        return -1;
    }

    pair[splitIndex] = '\0';

    strncpy(key, pair, splitIndex+1);
    strncpy(val, pair+splitIndex+1, MAX_VAL_SIZE);

    pair[splitIndex] = SPLIT_CHAR;

    return 0;
}

int main(int argc, char* argv[]) {

    char pair[40];
    char key[20];
    char val[20];

    if (strlen(argv[1]) > 40) {
        printf("Too long\n");
        return -1;
    }
    strncpy(pair, argv[1], 40);

    printf("pair: %s\n", pair);

    
    if (kv_mod_split_pair(pair, key, val)) {
        printf("Key too long\n");
        return -1;
    }

    
    printf("key: %s\n", key);
    printf("val: %s\n", val);

    

    return 0;
}
