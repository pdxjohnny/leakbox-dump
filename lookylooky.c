#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <asm/uaccess.h>

#include "query_ioctl.h"

#define INFO KERN_INFO "lookylooky: "
#define FIRST_MINOR 0
#define MINOR_CNT 1

#define OF_SIZE 256

static dev_t dev;
static struct cdev c_dev;
static struct class *cl;

unsigned long long strtoull(const char *nptr, char **endptr, int base);

static int my_open(struct inode *i, struct file *f) { return 0; }
static int my_close(struct inode *i, struct file *f) { return 0; }
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 35))
static int my_ioctl(struct inode *i, struct file *f, unsigned int cmd,
                    unsigned long arg)
#else
static long my_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
#endif
{
  void * ptr = 0x0;
  struct poc_msg msg;

    if (copy_from_user(&msg, (struct poc_msg *)arg, sizeof(struct poc_msg))) {
      return -EACCES;
    }

    msg.buffer[msg.length] = '\0';
    printk(INFO "msg.length: %d\n", msg.length);
    printk(INFO "msg.buffer: %s\n", msg.buffer);

    ptr = strtoull(msg.buffer, NULL, 16);

    printk(INFO "%p: %x\n", ptr, (unsigned int)ptr);

  return 0;
}

static struct file_operations lookylooky_fops = {.owner = THIS_MODULE,
                                            .open = my_open,
                                            .release = my_close,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 35))
                                            .ioctl = my_ioctl
#else
                                            .unlocked_ioctl = my_ioctl
#endif
};

static int __init lookylooky_ioctl_init(void) {
  int ret;
  struct device *dev_ret;

  if ((ret = alloc_chrdev_region(&dev, FIRST_MINOR, MINOR_CNT, "lookylooky_ioctl")) <
      0) {
    return ret;
  }

  cdev_init(&c_dev, &lookylooky_fops);

  if ((ret = cdev_add(&c_dev, dev, MINOR_CNT)) < 0) {
    return ret;
  }

  if (IS_ERR(cl = class_create(THIS_MODULE, "lookylookychar"))) {
    cdev_del(&c_dev);
    unregister_chrdev_region(dev, MINOR_CNT);
    return PTR_ERR(cl);
  }
  if (IS_ERR(dev_ret = device_create(cl, NULL, dev, NULL, "lookylooky"))) {
    class_destroy(cl);
    cdev_del(&c_dev);
    unregister_chrdev_region(dev, MINOR_CNT);
    return PTR_ERR(dev_ret);
  }

  printk(INFO "Loaded\n");
  return 0;
}

static void __exit lookylooky_ioctl_exit(void) {
  device_destroy(cl, dev);
  class_destroy(cl);
  cdev_del(&c_dev);
  unregister_chrdev_region(dev, MINOR_CNT);
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

module_init(lookylooky_ioctl_init);
module_exit(lookylooky_ioctl_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(":)");
MODULE_DESCRIPTION("lookylooky");
