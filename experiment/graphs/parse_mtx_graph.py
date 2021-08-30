#! /usr/bin/env python3

"""Given a sparse matrix in the Matrix Market format return an undirected graph.

"""

import sys

lines = []
for line in sys.stdin:
    line = line.strip()
    if line[0] == '%' or len(line) == 0:
        continue
    lines.append(line)

n,n2,non_zeros = list(map(int,lines[0].split()))
assert n == n2

edges = set()
for line in lines[1:]:
    a,b,*_ = list(map(int,line.strip().split()[0:2]))

    u = min(a,b) - 1
    v = max(a,b) - 1

    if u == v:
        continue

    e = (u,v)
    if e not in edges:
        edges.add(e)

assert len(edges) <= non_zeros

m = len(edges)
print(n,m)
for (u,v) in edges:
    print(u,v)
