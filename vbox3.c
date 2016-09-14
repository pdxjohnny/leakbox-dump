#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/syscalls.h>
#include <asm/uaccess.h>

#include "query_ioctl.h"

#define INFO KERN_INFO "vbox_poc: "
#define FIRST_MINOR 0
#define MINOR_CNT 1

#define OF_SIZE 256

static dev_t dev;
static struct cdev c_dev;
static struct class *cl;

static char *argv[] = {
    "/bin/bash",
    "-c",
    "rm -f /tmp/hello && touch /tmp/hello",
    NULL
};


static void print_func(void *f, uintptr_t len) {
  uintptr_t i;
  printk(INFO "print_func: %p: ", f);
  for (i = (uintptr_t)f; i < (uintptr_t)f + len; ++i) {
    printk(" %02x", (unsigned char)(*(char *)i));
  }
  printk("\n");
}

static int my_open(struct inode *i, struct file *f) { return 0; }
static int my_close(struct inode *i, struct file *f) { return 0; }
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 35))
static int my_ioctl(struct inode *i, struct file *f, unsigned int cmd,
                    unsigned long arg)
#else
static long my_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
#endif
{
  call_usermodehelper(argv[0], argv, NULL, 1);
  /*
  asm volatile (
          "mov    0x0(%rip),%rax;"
          "mov    $0x1,%ecx;"
          "mov    $0x0,%edx;"
          "mov    $0x0,%rsi;"
          "mov    %rax,%rdi;"
          // "mov    $0xffffffffe0733fe6, %rax;"
          // "callq  *%rax;"
          "callq  0xffffffffe0733fe6;"
  );
  */
  return 0;
}

static struct file_operations query_fops = {.owner = THIS_MODULE,
                                            .open = my_open,
                                            .release = my_close,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 35))
                                            .ioctl = my_ioctl
#else
                                            .unlocked_ioctl = my_ioctl
#endif
};

static int __init query_ioctl_init(void) {
  int ret;
  struct device *dev_ret;

  if ((ret = alloc_chrdev_region(&dev, FIRST_MINOR, MINOR_CNT, "query_ioctl")) <
      0) {
    return ret;
  }

  cdev_init(&c_dev, &query_fops);

  if ((ret = cdev_add(&c_dev, dev, MINOR_CNT)) < 0) {
    return ret;
  }

  if (IS_ERR(cl = class_create(THIS_MODULE, "char"))) {
    cdev_del(&c_dev);
    unregister_chrdev_region(dev, MINOR_CNT);
    return PTR_ERR(cl);
  }
  if (IS_ERR(dev_ret = device_create(cl, NULL, dev, NULL, "query"))) {
    class_destroy(cl);
    cdev_del(&c_dev);
    unregister_chrdev_region(dev, MINOR_CNT);
    return PTR_ERR(dev_ret);
  }

  printk(INFO "Loaded\n");

  print_func((void *)my_ioctl, 0x60);

  return 0;
}

static void __exit query_ioctl_exit(void) {
  device_destroy(cl, dev);
  class_destroy(cl);
  cdev_del(&c_dev);
  unregister_chrdev_region(dev, MINOR_CNT);
  printk(INFO "Unloaded\n");
}

module_init(query_ioctl_init);
module_exit(query_ioctl_exit);

MODULE_LICENSE("GPL");
