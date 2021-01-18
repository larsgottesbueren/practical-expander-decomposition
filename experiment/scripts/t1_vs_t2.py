#! /usr/bin/env python3

import itertools
import sys
import multiprocessing as mp
import subprocess
import csv


def cut(edc_cut_path, graph_string, t1, t2):
    """Run 'edc-cut' and return number of edges cut and partitions created"""
    result = subprocess.run([edc_cut_path, f'-t1={t1}', f'-t2={t2}'],
                            input=graph_string,
                            text=True,
                            check=True,
                            timeout=120,
                            stdout=subprocess.PIPE)
    if result.returncode != 0:
        print('Failed partitioning: {}'.format(result.stdout))
        exit(1)
    else:
        lines = result.stdout.strip().split('\n')
        resultType = lines[0].split()[0]

        xlen, *xs = list(map(int, lines[1].split()))
        assert xlen == len(xs)
        ylen, *ys = list(map(int, lines[2].split()))
        assert ylen == len(ys)

        return (resultType == "balanced_cut"  and (max(xs) < min(ys) or max(ys) < min(xs)) and xlen == ylen)


if __name__ == '__main__':
    if len(sys.argv) != 6:
        print('Expected five arguments')
        exit(1)
    _, _, edc_cut_path, seed, gen_graph, output_file = sys.argv

    graph_params = [{
        'name': 'clique',
        'n': 30,
        'k': 2,
        'r': 1,
    }, {
        'name': 'clique',
        'n': 100,
        'k': 2,
        'r': 1,
    }]
    graphs = []
    for ps in graph_params:
        cmd = [
            f'./{gen_graph}', f'--seed={seed}', ps['name'],
            '-n={}'.format(ps['n']), '-k={}'.format(ps['k']),
            '-r={}'.format(ps['r'])
        ]
        result = subprocess.run(cmd,
                                text=True,
                                check=True,
                                timeout=60,
                                stdout=subprocess.PIPE)
        if result.returncode != 0:
            print(f'Generating graph with type {name} failed')
            exit(1)
        graphs.append(result.stdout)

    def test(t1, t2):
        with mp.Pool() as pool:
            numIterations = 4
            jobs = [(edc_cut_path, g, t1, t2)
                    for g, _ in itertools.product(graphs, range(numIterations))
                    ]
            results = pool.starmap(cut, jobs, chunksize=1)
            return all(results)

    points = {}
    for t1 in range(20, 40):
        lo, hi = 2.0, 3.0
        print(f'Searching for t2 when t1 = {t1}', end='\r', file=sys.stderr)
        if test(t1, hi):
            while hi - lo > 1e-3:
                mid = (lo + hi) / 2.0
                if test(t1, mid): hi = mid
                else: lo = mid
            points[t1] = hi

    with open(output_file, 'w') as f:
        writer = csv.DictWriter(f, fieldnames=['t1', 't2'])
        writer.writeheader()

        for t1, t2 in points.items():
            writer.writerow({'t1': t1, 't2': t2})
