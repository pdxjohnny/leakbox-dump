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
  const size_t buf_size = 10000;
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

static char *binary = NULL;
static char *exploit = NULL;

void handler(int sig, siginfo_t *si, void *unused) {
  printf("Got SIGSEGV at address: %p\n", si->si_addr);
  if (binary != NULL) free(binary);
  if (exploit != NULL) free(exploit);
  exit(EXIT_FAILURE);
}

void vulnerable_func(const char * msg, ssize_t msg_size) {
  // The buffer we will overflow
  const size_t overflow_me_size = 1024U;
  char overflow_me[overflow_me_size];
  // Doh! Used size of attacker controlled not defender controlled for copy!
  // Stack overflow eminent!
  memcpy(overflow_me, msg, msg_size);
  // If we succeed then say so
  printf("vulnerable_func finished memcpy\n");
}

int main(int argc, char *argv[]) {
  char *p = NULL;
  char *buffer = NULL;
  char *msg = NULL;
  int pages = 0;
  int pagesize = 0;
  struct sigaction sa;
  ssize_t msg_size = 0;

  if (argc < 3) {
    printf("Usage %s binary_file payload_file\n", argv[0]);
    return EXIT_FAILURE;
  }

  // Add a handy dandy SIGSEGV handler
  sa.sa_flags = SA_SIGINFO;
  sigemptyset(&sa.sa_mask);
  sa.sa_sigaction = handler;
  if (sigaction(SIGSEGV, &sa, NULL) == -1)
    handle_error("sigaction");

  // Find out the pagesize
  pagesize = sysconf(_SC_PAGE_SIZE);
  if (pagesize == -1)
    handle_error("sysconf");

  // Get load the file to the msg pointer
  msg = file_to_string(argv[1], &msg_size);
  if (msg == NULL) {
    return EXIT_FAILURE;
  }

  // Determine number of pages to allocate so that binary fits in them
  pages = msg_size / pagesize;
  if (msg_size % pagesize != 0) {
    ++pages;
  }

  // Allocate pages * pagesize worth of memory
  buffer = memalign(pagesize, pages * pagesize);
  if (buffer == NULL)
    handle_error("memalign");

  // Make that memory writable
  if (mprotect(buffer, pages * pagesize, PROT_WRITE) == -1)
    handle_error("mprotect");

  // Write from the msg into buffer
  memcpy(buffer, msg, msg_size);

  // Leak the address hur dur (not to be mistaken with the noble Hodor)
  printf("vboxdrv: %p %s\n", buffer, argv[1]);
  // Free the msg and set it to NULL, its now page aligned so loose the orig
  free(msg);
  msg = NULL;
  // Make binary point to the buffer so that we can free it at the end of this
  binary = buffer;

  // Make the memory containing the binary executable so was can ROP on it
  if (mprotect(buffer, pages * pagesize, PROT_WRITE|PROT_READ|PROT_EXEC) == -1)
    handle_error("mprotect");

  // Close stdin to let us know when to read in the payload file
  msg = fd_to_string(STDIN_FILENO, &msg_size);
  if (msg != NULL) {
    free(msg);
  }
  msg = file_to_string(argv[2], &msg_size);
  if (msg == NULL) {
    return EXIT_FAILURE;
  }
  // Set exploit to msg so we remember to free it
  exploit = msg;

  // Pass the payload (msg) to the vulnerable function
  vulnerable_func(msg, msg_size);

  // If we end up back here then clean up
  free(binary);
  free(exploit);

  // We made it out alive
  return EXIT_SUCCESS;
}
