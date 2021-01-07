#! /usr/bin/env python3

import itertools
import sys
import multiprocessing as mp
import subprocess
import csv

def partition(edc_path, graph_string, t1, t2):
    """ Run 'edc' and return number of edges cut and partitions created """
    result = subprocess.run(
        [edc_path, f'-t1={t1}', f'-t2={t2}'],
        input=graph_string,
        text=True,
        check=True,
        timeout=120,
        stdout=subprocess.PIPE)
    if result.returncode != 0:
        print('Failed partitioning: {}'.format(result.stdout))
        exit(1)
    else:
        xs = list(map(int, result.stdout.split()[0:2]))
        return (xs[0], xs[1])

if __name__ == '__main__':
    if len(sys.argv) != 6:
        print('Expected five arguments')
        exit(1)
    _, edc_path, _, seed, gen_graph, output_file = sys.argv

    graph_params = [('dumbbell', 10, 2), ('dumbbell', 100, 2)]
    graphs = []
    for (name, n, k) in graph_params:
        result = subprocess.run([
            f'./{gen_graph}', f'--seed={seed}', f'{name}', f'-n={n}', f'-k={k}'
        ],
                                text=True,
                                check=True,
                                timeout=60,
                                stdout=subprocess.PIPE)
        if result.returncode != 0:
            print(f'Generating graph with type {name} failed')
            exit(1)
        graphs.append(result.stdout)

    def test(t1, t2):
        iterations = 4
        with mp.Pool() as pool:
            results = pool.starmap(partition, [
                (edc_path, g, t1, t2)
                for g, _ in itertools.product(graphs, list(range(iterations)))
            ])
            for eCount, pCount in results:
                if eCount != 1 or pCount != 2:
                    return False
            return True

    points = {}
    for t1 in range(100):
        t2 = 1.0
        while t2 < 20:
            print(f'Searching for t2 when t1 = {t1}', end='\r', file=sys.stderr)
            if test(t1, t2):
                lo, hi = 0, t2
                while hi - lo > 1e-2:
                    mid = (lo + hi) / 2.0
                    if test(t1, mid): hi = mid
                    else: lo = mid
                points[t1] = hi
                break
            else:
                t2 *= 2

    with open(output_file, 'w') as f:
        writer = csv.DictWriter(f, fieldnames=['t1', 't2'])
        writer.writeheader()

        for t1, t2 in points.items():
            writer.writerow({'t1': t1, 't2': t2})
