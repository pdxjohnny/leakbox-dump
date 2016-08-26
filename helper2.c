#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

char *file_to_string(char *const filename, size_t *string_size);
char *fd_to_string(int fd, size_t *string_size);

char *file_to_string(char *const filename, size_t *string_size) {
  char *string = NULL;
  int fd = open(filename, O_RDONLY|O_NONBLOCK);
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

void *find_addr(const char *search, int *error) {
  ssize_t msg_size = 0;
  char *msg = file_to_string("/dev/kmsg", &msg_size);
  char *freeme = msg;
  char *tmp = msg;
  char *addr = NULL;
  if (msg != NULL) {
    char *newline = strchr(msg, '\n');
    while (newline != NULL && msg != NULL) {
      *newline = '\0';
	  printf("LINE: %s\n", msg);
      ++newline;
      tmp = newline;
      const char search_start[] = "vboxdrv: ";
      char *loc_start = strstr(msg, search_start);
      char *loc_end = strstr(msg, search);
      if (loc_start != NULL && loc_end != NULL) {
        --loc_end;
        *loc_end = '\0';
        addr = loc_start + strlen(search_start);
      }
      msg = tmp;
      newline = strchr(msg, '\n');
    }
    free(freeme);
  }
  if (addr != NULL) {
    *error = 0;
    return (void *)strtoull(addr, NULL, 16);
  }
  *error = 1;
  return 0U;
}

int main(int argc, char **argv) {
  if (argc > 1) {
    int error;
    void *addr = find_addr(argv[1], &error);
    if (error) {
      return EXIT_FAILURE;
    }
    printf("%p\n", addr);
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}
