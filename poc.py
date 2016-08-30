#!/usr/bin/python3
import sys
from pwn import *

def main():
    # Get the address from the leak
    leaker = process(['./userspace', sys.argv[1]])
    leak = [l for l in leaker.recv().decode('utf-8') if 'vboxdrv: ' in l and sys.argv[1] in l]
    print(leak)
    # drv = DynELF(leak, sys.argv[1])

if __name__ == '__main__':
    main()
