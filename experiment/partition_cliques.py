#!/usr/bin/env python3

"""Create several cliques and find an expander decomposition.

"""

import multiprocessing as mp
import subprocess
from math import log

executable = '/Users/isaac/kth/thesis/code/bazel-bin/main/expander-decomp'

compSize = 50
comps = 10

g = {}

n = compSize * comps
m = 0
for comp in range(comps):
    for i in range(compSize):
        u = comp * compSize + i
        g[u] = []
        for j in range(i+1, compSize):
            v = comp * compSize + j
            g[u].append(v)
            m += 1

for c in range(comps-1):
    u = c * compSize
    v = (c + 1) * compSize + 1
    g[u].append(v)
    m += 1

problemInput = '{} {}'.format(n, m)
for u,vs in g.items():
    for v in vs:
        problemInput += '\n{} {}'.format(u, v)

class Result(object):
    pass
class Fail(Result):
    pass
class Partitioning(Result):
    def __init__(self, partitionCount, edgesCut):
        self.partitionCount = partitionCount
        self.edgesCut = edgesCut

def run(iteration):
  result = subprocess.run('{}'.format(executable),
                          input=problemInput,
                          text=True,
                          check=True,
                          timeout=120,
                          stdout=subprocess.PIPE)
  if result.returncode != 0:
      print ('{}: Fail: {}'.format(iteration, result.stdout))
      return Fail()
  else:
      xs = list(map(int, result.stdout.split()))
      print('{}: Partitions: {} Edges cut: {}'.format(iteration, xs[1], xs[0]))
      return Partitioning(xs[1], xs[0])

if __name__ == '__main__':
    fails = 0
    partitionCounts = []
    edgesCut = []

    with mp.Pool() as p:
        results = p.map(run, range(100))
        for r in results:
            if isinstance(r, Fail):
                fails += 1
            elif isinstance(r, Partitioning):
                partitionCounts.append(r.partitionCount)
                edgesCut.append(r.edgesCut)

    print('Avg partition count: {}'.format(sum(partitionCounts) / 100))
    print('Avg edges cut: {}'.format(sum(edgesCut) / 100))
