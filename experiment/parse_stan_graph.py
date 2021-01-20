#! /usr/bin/env python3

"""Parse an undirected or directed graph as an undirected graph readable by
'edc'.

Graphs from the Stan archive are given as a list of edges. Vertices are not
necessarily in the range [0,n). This program remaps vertices such that they are
within the range.

"""

import sys

edges = set()
node_map = {}

for line in sys.stdin:
    line = line.strip()
    if len(line) == 0:
        break
    if line[0] == '#':
        continue
    i, j = list(map(int,line.split()))

    def get(x):
        if x not in node_map:
            node_map[x] = len(node_map)
        return node_map[x]

    u, v = get(i), get(j)
    edges.add((min(u, v), max(u, v)))

n, m = len(node_map), len(edges)
print(n,m)
for (u,v) in edges:
    print(f'{u} {v}')
