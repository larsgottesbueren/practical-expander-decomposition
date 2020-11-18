#!/usr/bin/env python3

"""Utility for creating various types of graphs.

"""

import argparse

parser = argparse.ArgumentParser(description='Utility for creating graphs')
parser.add_argument('graph_type', choices=['clique', 'dumbbell'], metavar='type')
parser.add_argument('-n', type=int, default=100, help='number of vertices in graph (default=100)')

args = parser.parse_args()

g = {}
n = args.n
m = 0

if args.graph_type == 'clique':
    for u in range(n):
        g[u] = []
        for v in range(u+1,n):
            g[u].append(v)
            m += 1
elif args.graph_type == 'dumbbell':
    n1 = n // 2
    for u in range(n):
        g[u] = []
        for v in range(u+1,n):
            if (u < n1 and v < n1) or (u >= n1 and v >= n1):
                g[u].append(v)
                m += 1
    g[0].append(n1)
    m += 1

problemInput = '{} {}'.format(n, m)
for u,vs in g.items():
    for v in vs:
        problemInput += '\n{} {}'.format(u, v)

print(problemInput)
