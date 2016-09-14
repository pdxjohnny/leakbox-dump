#!/usr/bin/env python3
import sys
import subprocess

from_addr = int(sys.argv[1], 16)
to_addr = from_addr + int(sys.argv[2])

for i in range(from_addr, to_addr):
    subprocess.call(["./query_app", str(hex(i)).replace('L', '')])
