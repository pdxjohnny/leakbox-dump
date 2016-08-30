#include <errno.h>
#include <malloc.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>

#define handle_error(msg)                                                      \
  do {                                                                         \
    perror(msg);                                                               \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

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

void handler(int sig, siginfo_t *si, void *unused) {
  printf("Got SIGSEGV at address: 0x%lx\n", (long)si->si_addr);
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
  char *p;
  char *buffer;
  int pages;
  int pagesize;
  struct sigaction sa;
  ssize_t msg_size = 0;
  char *msg = NULL;

  if (argc < 2) {
    return EXIT_FAILURE;
  }

  msg = file_to_string(argv[1], &msg_size);
  if (msg == NULL) {
    return EXIT_FAILURE;
  }

  sa.sa_flags = SA_SIGINFO;
  sigemptyset(&sa.sa_mask);
  sa.sa_sigaction = handler;
  if (sigaction(SIGSEGV, &sa, NULL) == -1)
    handle_error("sigaction");

  pagesize = sysconf(_SC_PAGE_SIZE);
  if (pagesize == -1)
    handle_error("sysconf");

  /* Allocate a buffer aligned on a page boundary;
     initial protection is PROT_READ | PROT_WRITE */

  pages = msg_size / pagesize;
  if (msg_size % pagesize != 0) {
    ++pages;
  }

  buffer = memalign(pagesize, pages * pagesize);
  if (buffer == NULL)
    handle_error("memalign");

  printf("vboxdrv: %p %s\n", buffer, argv[1]);

  if (mprotect(buffer, pages * pagesize, PROT_WRITE) == -1)
    handle_error("mprotect");

  memcpy(buffer, msg, msg_size);

  if (mprotect(buffer, pages * pagesize, PROT_WRITE|PROT_READ|PROT_EXEC) == -1)
    handle_error("mprotect");

  /*
  // Now read in the exploit from stdin
  int exploit_size = 0;
  char *exploit = NULL;
  void (*call_exploit) = NULL;
  // Read in the exploit
  exploit = fd_to_string(STDIN_FILENO, exploit_size);
  // Push the address before the return EXIT_SUCCESS onto the stack and then
  // jmp to the last gadget of the rop chain, this is the subtraction
  call_exploit = exploit + exploit_size - 4;
  // Here we go
  call_exploit();
  */

  return EXIT_SUCCESS;
}
