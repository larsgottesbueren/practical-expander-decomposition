#!/usr/bin/env python3
"""Utility for creating various types of graphs.

"""

import argparse

def output(g,n,m):
    problemInput = '{} {}'.format(n, m)
    for u, vs in g.items():
        for v in vs:
            problemInput += '\n{} {}'.format(u, v)
    print(problemInput)

def gen_clique(args):
    g = {}
    n = args.n
    k = args.k
    m = 0

    for c in range(k):
        for i in range(n):
            u = c * n + i
            g[u] = []
            for j in range(i+1,n):
                v = c * n + j
                g[u].append(v)
                m += 1

    output(g,n*k,m)

def gen_dumbbell(args):
    g = {}
    n = args.n
    k = args.k
    m = 0

    for c in range(k):
        for i in range(n):
            u = c * n + i
            g[u] = []
            for j in range(i+1,n):
                v = c * n + j
                g[u].append(v)
                m += 1

    for c1 in range(k):
        u = c1 * n
        for c2 in range(c1+1,k):
            v = c2 * n
            g[u].append(v)
            m += 1

    output(g,n*k,m)

def gen_clique_path(args):
    g = {}
    n = args.n
    k = args.k
    m = 0

    for c in range(k):
        for i in range(n):
            u = c * n + i
            g[u] = []
            for j in range(i+1,n):
                v = c * n + j
                g[u].append(v)
                m += 1

    for c in range(k-1):
        u = c * n
        v = (c + 1) * n
        g[u].append(v)
        m += 1

    output(g,n*k,m)

parser = argparse.ArgumentParser(description='Utility for creating graphs')
subparsers = parser.add_subparsers()

clique_parser = subparsers.add_parser(
    'clique',
    help='k cliques, each with n vertices.')
clique_parser.set_defaults(func=gen_clique)
clique_parser.add_argument(
    '-n',
    type=int,
    default=100,
    help='number of vertices in each clique (default=100)')
clique_parser.add_argument(
    '-k',
    type=int,
    default=1,
    help='number of cliques (default=1)')

dumbbell_parser = subparsers.add_parser(
    'dumbbell',
    help='k subgraphs, each with n vertices, forming a clique'
    )
dumbbell_parser.set_defaults(func=gen_dumbbell)
dumbbell_parser.add_argument(
    '-n',
    type=int,
    default=100,
    help='number of vertices in each clique (default=100)')
dumbbell_parser.add_argument(
    '-k',
    type=int,
    default=2,
    help='number of cliques (default=2)')

clique_path_parser = subparsers.add_parser(
    'clique-path',
    help='k cliques, each with n vertices, forming a path'
    )
clique_path_parser.set_defaults(func=gen_clique_path)
clique_path_parser.add_argument(
    '-n',
    type=int,
    default=100,
    help='number of vertices in each clique (default=100)')
clique_path_parser.add_argument(
    '-k',
    type=int,
    default=10,
    help='number of cliques in path (default=10)')

args = parser.parse_args()
args.func(args)
