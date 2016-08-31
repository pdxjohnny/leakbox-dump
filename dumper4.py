#!/usr/bin/python3
import sys

start = sys.argv[1]
end   = sys.argv[2]

r = []
appending = False

for l in sys.stdin:
    if len(l) < 1:
        continue
    s = l.split(':')[-1]
    if start in s:
        appending = []
    if appending != False:
        appending.append(l)
    if end in s:
        if appending != False:
            r.append(appending)
            appending = False

if appending != False:
    r.append(appending)

for l in r:
    if len(l) > 2:
        continue
    print(''.join(l))
