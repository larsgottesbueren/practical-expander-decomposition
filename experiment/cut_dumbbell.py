#!/usr/bin/env python3

"""
Create two cliques, connect them with a single edge and find a sparse cut.
"""

import multiprocessing as mp
import subprocess

executable = '/Users/isaac/kth/thesis/code/bazel-bin/main/sparse-cut'

phi = 0.0001
tConst = 100
tFactor = 1.0

n = 100
n1 = n // 2
n2 = n - n1
m = (n1 * (n1 - 1) // 2) + (n2 * (n2 - 1) // 2) + 1

problemInput = '{} {} {} {} {}'.format(phi, tConst, tFactor, n, m)

for u in range(n):
    for v in range(u+1,n):
        if (u < n1 and v < n1) or (u >= n1 and v >= n1):
            problemInput += '\n{} {}'.format(u, v)
problemInput += '\n{} {}'.format(0,n1)

print(problemInput)
exit(0)

class Result(object):
    pass
class Fail(Result):
    pass
class Expander(Result):
    pass
class Cut(Result):
    def __init__(self,cutSize):
        self.cutSize = cutSize

def cut(iteration):
  result = subprocess.run('{}'.format(executable),
                          input=problemInput,
                          text=True,
                          check=True,
                          timeout=120,
                          stdout=subprocess.PIPE)
  if result.returncode != 0:
      print ('{}: Fail'.format(iteration))
      return Fail()
  else:
      xs = list(map(int, result.stdout.split()))

      k1 = xs[0]
      k2 = xs[k1+1]
      if (k1 == 0 and k2 == n) or (k1 == n and k2 == 0):
          print('{}: Expander'.format(iteration))
          return Expander()
      else:
          print('{}: Cut ({}, {})'.format(iteration,k1,k2))
          return Cut(min(k1,k2))

if __name__ == '__main__':
    fails = 0
    expanders = 0
    cuts = 0
    cutSizes = []

    with mp.Pool() as p:
        results = p.map(cut, range(100))
        for r in results:
            if isinstance(r, Fail):
                fails += 1
            elif isinstance(r, Expander):
                expanders += 1
            elif isinstance(r, Cut):
                cuts += 1
                cutSizes.append(r.cutSize)

    print('Cuts: {}\nExpanders: {}\nTotal: {}'.format(cuts, expanders, fails + cuts + expanders))
    if cuts > 0:
        print('Avg cut size: {}'.format(sum(cutSizes) / cuts))
