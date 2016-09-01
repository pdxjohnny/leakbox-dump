/*
 *  hello-5.c - Demonstrates command line argument passing to a module.
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/moduleparam.h>
#include <linux/stat.h>
#include <asm/uaccess.h>	// for get_ds(), set_fs()
#include <asm/io.h>		// for inb(), outb()

MODULE_LICENSE("GPL");
MODULE_AUTHOR("John Andersen, Fredric Carl");

#define INFO KERN_INFO "lookylooky: "

#ifndef OF_SIZE
#define OF_SIZE 128
#endif

static int length = 0x010;
static char *start_addr = "0x0";

module_param(start_addr, charp, 0000);
MODULE_PARM_DESC(start_addr, "Address to start looking at");
module_param(length, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(length, "Length to look");

unsigned long long strtoull(const char *nptr, char **endptr, int base);

static int __init vbox_poc_init(void) {
  int i;
  unsigned int * addr = 0x0;

  printk(INFO "Loaded\n");

  addr = (unsigned int *)strtoull(start_addr, NULL, 16);
  for (i = 0; i < length; ++i, ++addr) {
    printk(INFO "%p: %016x\n", addr, *addr);
  }

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
