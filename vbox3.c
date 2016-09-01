/*
 *  hello-5.c - Demonstrates command line argument passing to a module.
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/moduleparam.h>
#include <linux/stat.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("John Andersen, Fredric Carl");

#define INFO KERN_INFO "vbox_poc: "

#ifndef OF_SIZE
#define OF_SIZE 128
#endif

static int exploit_length = 0;
static char *exploit_payload = "blah";

module_param(exploit_payload, charp, 0000);
MODULE_PARM_DESC(exploit_payload, "Exploit payload");
module_param(exploit_length, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(exploit_length, "Length of payload");

unsigned long long strtoull(const char *nptr, char **endptr, int base);

void vulnerable_func(const char * msg, ssize_t msg_size) {
  // The buffer we will overflow
  char overflow_me[OF_SIZE];
  // Doh! Used size of attacker controlled not defender controlled for copy!
  // Stack overflow eminent!
  memcpy(overflow_me, msg, msg_size);
  // If we succeed then say so
  printk(INFO "vulnerable_func finished memcpy\n");
  // asm("pop %rax; pop %rax; retq;");
}

static int __init vbox_poc_init(void) {
  int i;
  int j;
  int str_len;
  char buf[3] = {0, 0, 0};

  printk(INFO "Loaded\n");

  str_len = strlen(exploit_payload);
  for (i = 0, j = 0; i < str_len; i += 2, ++j) {
	buf[0] = exploit_payload[i];
	buf[1] = exploit_payload[i + 1];
    exploit_payload[j] = strtoull(buf, NULL, 16);
	printk(INFO "exploit_payload[%d]: %02x\n", j, exploit_payload[j]);
  }

  // printk(INFO "Exploit payload: %x\n", exploit_payload[exploit_length]);
  printk(INFO "Payload length: %d\n", exploit_length);

  printk(INFO "Calling vulnerable_func\n");
  vulnerable_func(exploit_payload, exploit_length);
  printk(INFO "Done with vulnerable_func\n");

  return 0;
}

static void __exit vbox_poc_exit(void) {
  printk(INFO "Unloaded\n");
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

module_init(vbox_poc_init);
module_exit(vbox_poc_exit);
