#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

int main() {
  int fd, result;
  char buf[20];

  if ((fd = open ("/dev/kv_mod0", O_WRONLY)) == -1) {
    perror("opening file");
    return -1;
  }

  close(fd);

  return 0;
}

