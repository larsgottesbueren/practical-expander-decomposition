#!/usr/bin/env python3
"""Run expander decomposition multiple times on the same graph.

"""

import sys
import argparse
import multiprocessing as mp
import subprocess

def run(iteration, executable, problemInput):
    result = subprocess.run('{}'.format(executable),
                            input=problemInput,
                            text=True,
                            check=True,
                            timeout=120,
                            stdout=subprocess.PIPE)
    if result.returncode != 0:
        print('{}: Fail: {}'.format(iteration, result.stdout))
        return ('fail')
    else:
        xs = list(map(int, result.stdout.split()))
        print('{}: Partitions: {} Edges cut: {}'.format(
            iteration, xs[1], xs[0]))
        return ('partition',xs[1],xs[0])


parser = argparse.ArgumentParser(
    description=
    'Utility for running expander-decomp multiple times on the same graph')
parser.add_argument(
    '--executable',
    default='/Users/isaac/kth/thesis/code/bazel-bin/main/expander-decomp',
    help='location of expander-decomp binary')
parser.add_argument('-i',
                    '--iterations',
                    type=int,
                    default=100,
                    help='number of iterations (default=100)')

if __name__ == '__main__':
    args = parser.parse_args()

    problemInput = sys.stdin.read()

    fails = 0
    partitionCounts = []
    edgesCut = []

    with mp.Pool() as p:
        results = p.starmap(run, [(it, args.executable, problemInput)
                                  for it in range(args.iterations)])
        for r in results:
            if r[0] == 'fail':
                fails += 1
            else:
                partitionCounts.append(r[1])
                edgesCut.append(r[2])

    print('Avg partition count: {}'.format(sum(partitionCounts) / 100))
    print('Avg edges cut: {}'.format(sum(edgesCut) / 100))
