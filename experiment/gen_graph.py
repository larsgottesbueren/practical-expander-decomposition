#!/usr/bin/env python3
"""Utility for creating various types of graphs.

"""

import argparse
from random import randrange


def output(args, g, n, m):
    problemInput = '{} {}'.format(n, m)
    for u, vs in g.items():
        for v in vs:
            a, b = u, v
            if args.one_indexed:
                a += 1
                b += 1
            problemInput += '\n{} {}'.format(a, b)
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
            for j in range(i + 1, n):
                v = c * n + j
                g[u].append(v)
                m += 1

    output(args, g, n * k, m)


def gen_dumbbell(args):
    g = {}
    n = args.n
    k = args.k
    m = 0

    for c in range(k):
        for i in range(n):
            u = c * n + i
            g[u] = []
            for j in range(i + 1, n):
                v = c * n + j
                g[u].append(v)
                m += 1

    for c1 in range(k):
        u = c1 * n
        for c2 in range(c1 + 1, k):
            v = c2 * n
            g[u].append(v)
            m += 1

    output(args, g, n * k, m)


def gen_clique_path(args):
    g = {}
    n = args.n
    k = args.k
    m = 0

    for c in range(k):
        for i in range(n):
            u = c * n + i
            g[u] = []
            for j in range(i + 1, n):
                v = c * n + j
                g[u].append(v)
                m += 1

    for c in range(k - 1):
        u = c * n
        v = (c + 1) * n
        g[u].append(v)
        m += 1

    output(args, g, n * k, m)


def gen_clique_random(args):
    g = {}
    n = args.n
    k = args.k
    m = 0
    for c in range(k):
        for i in range(n):
            u = c * n + i
            g[u] = []
            for j in range(i + 1, n):
                if randrange(100) < 50:
                    v = c * n + j
                    g[u].append(v)
                m += 1
    for r in range(args.r):
        u = randrange(n * k)
        v = u
        while v == u or v in g[u] or u in g[v]:
            v = randrange(n * k)
        g[u].append(v)
        m += 1

    output(args, g, n * k, m)


def gen_lattice(args):
    g = {}
    n = args.n
    m = 0

    for x in range(n):
        for y in range(n):
            u = x + y * n
            g[u] = []
            vs = []
            if x < n - 1: vs.append((x + 1) + y * n)
            if y < n - 1: vs.append(x + (y + 1) * n)
            if x < n - 1 and y < n - 1: vs.append((x + 1) + (y + 1) * n)
            for v in vs:
                g[u].append(v)
                m += 1
    output(args, g, n * n, m)


parser = argparse.ArgumentParser(description='Utility for creating graphs')
parser.add_argument('--one_indexed',
                    action='store_true',
                    help='make vertex labels one-indexed (default=false)')

subparsers = parser.add_subparsers()

clique_parser = subparsers.add_parser('clique',
                                      help='k cliques, each with n vertices.')
clique_parser.set_defaults(func=gen_clique)
clique_parser.add_argument(
    '-n',
    type=int,
    default=100,
    help='number of vertices in each clique (default=100)')
clique_parser.add_argument('-k',
                           type=int,
                           default=1,
                           help='number of cliques (default=1)')

dumbbell_parser = subparsers.add_parser(
    'dumbbell', help='k subgraphs, each with n vertices, forming a clique')
dumbbell_parser.set_defaults(func=gen_dumbbell)
dumbbell_parser.add_argument(
    '-n',
    type=int,
    default=100,
    help='number of vertices in each clique (default=100)')
dumbbell_parser.add_argument('-k',
                             type=int,
                             default=2,
                             help='number of cliques (default=2)')

clique_path_parser = subparsers.add_parser(
    'clique-path', help='k cliques, each with n vertices, forming a path')
clique_path_parser.set_defaults(func=gen_clique_path)
clique_path_parser.add_argument(
    '-n',
    type=int,
    default=100,
    help='number of vertices in each clique (default=100)')
clique_path_parser.add_argument('-k',
                                type=int,
                                default=10,
                                help='number of cliques in path (default=10)')

clique_random_parser = subparsers.add_parser(
    'clique-random',
    help=
    'k almost-cliques (each edge has 50% probability), each with n vertices, with r random edges'
)
clique_random_parser.set_defaults(func=gen_clique_random)
clique_random_parser.add_argument(
    '-n',
    type=int,
    default=100,
    help='number of vertices in each clique (default=100)')
clique_random_parser.add_argument('-k',
                                  type=int,
                                  default=10,
                                  help='number of cliques (default=10)')
clique_random_parser.add_argument(
    '-r',
    type=int,
    default=100,
    help='number of random edges added (default=100)')

lattice_parser = subparsers.add_parser('lattice', help='n by n lattice')
lattice_parser.set_defaults(func=gen_lattice)
lattice_parser.add_argument('-n',
                            type=int,
                            default=10,
                            help='width and height of lattice (default=10)')

args = parser.parse_args()
args.func(args)
