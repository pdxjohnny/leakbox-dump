#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include "query_ioctl.h"

char *file_to_string(char *const filename, size_t *string_size);
char *fd_to_string(int fd, size_t *string_size);

char *file_to_string(char *const filename, size_t *string_size) {
  char *string = NULL;
  int fd = open(filename, O_RDONLY);
  if (fd == -1) {
    *string_size = 0;
    return NULL;
  }
  string = fd_to_string(fd, string_size);
  close(fd);
  return string;
}

char *fd_to_string(int fd, size_t *string_size) {
  const size_t buf_size = 1024;
  ssize_t bytes_read = 0;
  size_t buf_length = 0;
  char buf[buf_size];
  char *string = NULL;
  char *tmp = NULL;
  *string_size = 0;
  memset(buf, 0, buf_size);
  bytes_read = read(fd, buf, buf_size);
  if (bytes_read > 0) {
    buf_length = (size_t)bytes_read;
  }
  while (bytes_read > 0) {
    tmp = malloc(*string_size + buf_length);
    if (string != NULL) {
      memcpy(tmp, string, *string_size);
    }
    memcpy(tmp + *string_size, buf, buf_length);
    *string_size += buf_length;
    if (string != NULL) {
      free(string);
    }
    string = tmp;
    tmp = NULL;
    memset(buf, 0, buf_size);
    bytes_read = read(fd, buf, buf_size);
    if (bytes_read > 0) {
      buf_length = (size_t)bytes_read;
    }
  }
  return string;
}

int main(int argc, char *argv[]) {
  int fd;
  char ioctl_device[] = "/dev/lookylooky";
  struct poc_msg msg;

  memcpy(msg.buffer, argv[1], strlen(argv[1]));
  msg.length = (__u32) strlen(argv[1]);

  fd = open(ioctl_device, O_RDWR);
  if (fd < 0) {
    perror("open /dev/lookylooky: ");
    return EXIT_FAILURE;
  }

  if (ioctl(fd, VBOX_POC_SEND_MSG, &msg) == -1) {
    perror("when sending: ");
  }

  close(fd);

  return 0;
}
