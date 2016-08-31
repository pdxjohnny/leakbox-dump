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
  printk(INFO "Loaded\n");

  printk(INFO "Exploit payload: %s\n", exploit_payload);
  printk(INFO "Payload length: %d\n", exploit_length);

  printk(INFO "Calling vulnerable_func\n");
  vulnerable_func(exploit_payload, exploit_length);
  printk(INFO "Done with vulnerable_func\n");

  return 0;
}

static void __exit vbox_poc_exit(void) {
  printk(INFO "Unloaded\n");
}

module_init(vbox_poc_init);
module_exit(vbox_poc_exit);
