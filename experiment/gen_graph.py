#!/usr/bin/env python3
"""Utility for creating various types of graphs.

"""

import argparse
import itertools
from random import randrange, seed


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
    r = args.r
    p = args.p

    es = set()
    for c in range(k):
        for i in range(n):
            u = c * n + i
            for j in range(i + 1, n):
                if randrange(100) < p:
                    v = c * n + j
                    es.add((min(u, v), max(u, v)))

    for _ in range(r):
        u = randrange(k * n)
        v = u
        while v == u or u // n == v // n or (min(u, v), max(u, v)) in es:
            v = randrange(k * n)
        es.add((min(u, v), max(u, v)))

    g = {}
    m = len(es)
    for u, v in es:
        if u not in g: g[u] = []
        g[u].append(v)
    output(args, g, k * n, m)


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


def gen_margulis(args):
    """See https://www.abelprize.no/c76018/binfil/download.php?tid=76133

    """
    n = args.n
    k = args.k
    r = args.r

    es = set()

    for comp in range(k):
        for x, y in itertools.product(range(n), range(n)):

            def S(a, b):
                return (a, a + b)

            def Sinv(a, b):
                return (a, a - b)  # Sinv(S(a,b)) = Sinv(a,a+b) = (a,b)

            def s(a, b):
                return (a + 1, b)

            def sinv(a, b):
                return (a - 1, b)  # sinv(s(a,b)) = sinv(a+1,b) = (a,b)

            def T(a, b):
                return (a + b, b)

            def Tinv(a, b):
                return (a - b, b)  # Tinv(T(a,b)) = Tinv(a+b,b) = (a,b)

            def t(a, b):
                return (a, b + 1)

            def tinv(a, b):
                return (a, b - 1)  # tinv(t(a,b)) = tinv(a,b+1) = (a,b)

            for z, w in [
                    S(x, y),
                    Sinv(x, y),
                    s(x, y),
                    sinv(x, y),
                    T(x, y),
                    Tinv(x, y),
                    t(x, y),
                    tinv(x, y)
            ]:
                z = (z + n) % n
                w = (w + n) % n

                offset = comp * n * n
                u = x * n + y + offset
                v = z * n + w + offset

                es.add((min(u, v), max(u, v)))

    for _ in range(r):
        u = randrange(k * n * n)
        v = u
        while v == u or u // (n * n) == v // (n * n) or (min(u, v), max(
                u, v)) in es:
            v = randrange(k * n * n)
        es.add((min(u, v), max(u, v)))

    g = {}
    m = len(es)
    for u, v in es:
        if u not in g: g[u] = []
        g[u].append(v)
    output(args, g, k * n * n, m)


parser = argparse.ArgumentParser(description='Utility for creating graphs')
parser.add_argument('--one_indexed',
                    action='store_true',
                    help='make vertex labels one-indexed (default=false)')
parser.add_argument('-s',
                    '--seed',
                    type=int,
                    help='Seed to use when generating random values')

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
clique_parser.add_argument(
    '-r',
    type=int,
    default=0,
    help='number of random edges between different cliques (default=0)')
clique_parser.add_argument(
    '-p',
    type=int,
    default=100,
    help=
    'value 0 <= p <= 100 such that every clique edge is added with probability p/100 (default=100)'
)

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

lattice_parser = subparsers.add_parser('lattice', help='n by n lattice')
lattice_parser.set_defaults(func=gen_lattice)
lattice_parser.add_argument('-n',
                            type=int,
                            default=10,
                            help='width and height of lattice (default=10)')

margulis_parser = subparsers.add_parser(
    'margulis', help='k margulis-components, each with n^2 vertices.')
margulis_parser.set_defaults(func=gen_margulis)
margulis_parser.add_argument(
    '-n',
    type=int,
    default=100,
    help=
    'integer n such that each margulis component has n^2 vertices (default=100)'
)
margulis_parser.add_argument('-k',
                             type=int,
                             default=1,
                             help='number of margulis-components (default=1)')
margulis_parser.add_argument(
    '-r',
    type=int,
    default=0,
    help='number of random extra edges between separate components (default=0)'
)

args = parser.parse_args()

if args.seed is not None:
    seed(args.seed)

args.func(args)
