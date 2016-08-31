#!/usr/bin/python3
import sys

start = sys.argv[1]
end   = sys.argv[2]

r = []
appending = False

for l in sys.stdin:
    if start in l:
        appending = []
    if '5d' in l:
        appending = False
    if appending != False:
        appending.append(l)
    if end in l:
        if appending != False:
            r.append(appending)
            appending = False

if appending != False:
    r.append(appending)

for l in r:
    if len(l) > 6:
        continue
    print(''.join(l))
