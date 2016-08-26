#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

unsigned long long strtoull(const char *nptr, char **endptr, int base);
char *file_to_string(char *const filename, size_t *string_size);
char *fd_to_string(struct file *fd, size_t *string_size);
void *find_addr(const char *search, int *error);

char *file_to_string(char *const filename, size_t *string_size) {
  int err = 0;
  char *string = NULL;
  struct file *fd = NULL;
  mm_segment_t oldfs;

  oldfs = get_fs();
  set_fs(get_ds());
  fd = filp_open(filename, O_RDONLY, 0);
  set_fs(oldfs);

  if (fd == NULL) {
    printk(KERN_INFO "wtf could not open file\n");
    *string_size = 0;
    return NULL;
  }
  string = fd_to_string(fd, string_size);
  filp_close(fd, NULL);
  return string;
}

char *fd_to_string(struct file *fd, size_t *string_size) {
  const size_t buf_size = 1024;
  ssize_t bytes_read = 0;
  size_t buf_length = 0;
  char buf[buf_size];
  char *string = NULL;
  char *tmp = NULL;
  *string_size = 0;
  memset(buf, 0, buf_size);
  bytes_read = fd->f_op->read(fd, buf, buf_size, &fd->f_pos);
  printk(KERN_INFO "bytes_read: %d\n", bytes_read);
  if (bytes_read > 0) {
    buf_length = (size_t)bytes_read;
  }
  while (bytes_read > 0) {
    tmp = kmalloc(*string_size + buf_length, GFP_KERNEL);
    if (string != NULL) {
      memcpy(tmp, string, *string_size);
    }
    memcpy(tmp + *string_size, buf, buf_length);
    *string_size += buf_length;
    if (string != NULL) {
      kfree(string);
    }
    string = tmp;
    tmp = NULL;
    memset(buf, 0, buf_size);
    bytes_read = fd->f_op->read(fd, buf, buf_size, &fd->f_pos);
    printk(KERN_INFO "bytes_read: %d\n", bytes_read);
    if (bytes_read > 0) {
      buf_length = (size_t)bytes_read;
    }
  }
  return string;
}

void *find_addr(const char *search, int *error) {
  char search_start[] = "vboxdrv: ";
  ssize_t msg_size = 0;
  char *msg = file_to_string("/dev/kmsg", &msg_size);
  char *kfreeme = msg;
  char *tmp = msg;
  char *addr = NULL;
  char *loc_start = NULL;
  char *loc_end = NULL;
  char *newline = NULL;
  if (msg != NULL) {
    newline = strchr(msg, '\n');
    while (newline != NULL && msg != NULL) {
      *newline = '\0';
      ++newline;
      tmp = newline;
      loc_start = strstr(msg, search_start);
      loc_end = strstr(msg, search);
      if (loc_start != NULL && loc_end != NULL) {
        --loc_end;
        *loc_end = '\0';
        addr = loc_start + strlen(search_start);
      }
      msg = tmp;
      newline = strchr(msg, '\n');
    }
    kfree(kfreeme);
  }
  if (addr != NULL) {
    *error = 0;
    return (void *)strtoull(addr, NULL, 16);
  }
  *error = 1;
  return 0U;
}

unsigned long long strtoull(const char *nptr, char **endptr, int base) {
  const char *s;
  unsigned long long acc;
  char c;
  unsigned long long cutoff;
  int neg, any, cutlim;

  /*
   * See strtoq for comments as to the logic used.
   */
  s = nptr;
  do {
    c = *s++;
  } while (c == ' ');
  if (c == '-') {
    neg = 1;
    c = *s++;
  } else {
    neg = 0;
    if (c == '+')
      c = *s++;
  }
  if ((base == 0 || base == 16) && c == '0' && (*s == 'x' || *s == 'X')) {
    c = s[1];
    s += 2;
    base = 16;
  }
  if (base == 0)
    base = c == '0' ? 8 : 10;
  acc = any = 0;

  cutoff = ULLONG_MAX / base;
  cutlim = ULLONG_MAX % base;
  for (;; c = *s++) {
    if (c >= '0' && c <= '9')
      c -= '0';
    else if (c >= 'A' && c <= 'Z')
      c -= 'A' - 10;
    else if (c >= 'a' && c <= 'z')
      c -= 'a' - 10;
    else
      break;
    if (c >= base)
      break;
    if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
      any = -1;
    else {
      any = 1;
      acc *= base;
      acc += c;
    }
  }
  if (any < 0) {
    acc = ULLONG_MAX;
  } else if (!any) {
  } else if (neg)
    acc = -acc;
  if (endptr != NULL)
    *endptr = (char *)(any ? s - 1 : nptr);
  return (acc);
}
