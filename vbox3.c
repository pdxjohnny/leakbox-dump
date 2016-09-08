#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <asm/uaccess.h>

#include "query_ioctl.h"

#define INFO KERN_INFO "vbox_poc: "
#define FIRST_MINOR 0
#define MINOR_CNT 1

#define OF_SIZE 256

static dev_t dev;
static struct cdev c_dev;
static struct class *cl;

void vulnerable_func(const char *msg, ssize_t msg_size, short call_times);

static int my_open(struct inode *i, struct file *f) { return 0; }
static int my_close(struct inode *i, struct file *f) { return 0; }
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 35))
static int my_ioctl(struct inode *i, struct file *f, unsigned int cmd,
                    unsigned long arg)
#else
static long my_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
#endif
{
  struct poc_msg msg;

  printk(INFO "cmd: %d\n", cmd);

  switch (cmd) {
  case VBOX_POC_SEND_MSG:
    if (copy_from_user(&msg, (struct poc_msg *)arg, sizeof(struct poc_msg))) {
      return -EACCES;
    }

    printk(INFO "msg.length: %d\n", msg.length);
    printk(INFO "msg.buffer: %s\n", msg.buffer);

    // Call it multiple times to make sure we dont overwrite the return on ioctl
    // handler
    vulnerable_func(msg.buffer, msg.length, 10);
  }

  return 0;
}

void vulnerable_func(const char *msg, ssize_t msg_size, short call_times) {
  // The buffer we will overflow
  char overflow_me[OF_SIZE];
  printk(INFO "called vulnerable_func\n");
  if (call_times > 0) {
    // Doh! Used size of attacker controlled not defender controlled for copy!
    // Stack overflow eminent!
    memcpy(overflow_me, msg, msg_size);
    // If we succeed in the memcpy then say so
    printk(INFO "vulnerable_func finished memcpy\n");
  } else {
    vulnerable_func(msg, msg_size, --call_times);
  }
  printk(INFO "exit vulnerable_func\n");
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
MODULE_AUTHOR("Anil Kumar Pugalia <email_at_sarika-pugs_dot_com>");
MODULE_DESCRIPTION("Query ioctl() Char Driver");
