#! /usr/bin/env python3

from math import log10
import itertools
import sys
import multiprocessing as mp
import subprocess
import csv
import numpy


def is_expander(edc_cut_path, graph_info, seed, phi, default_strategy, t):
    """Run 'edc-cut' and return true if an expander was certified.

    """
    graph_string, graph_params, graph_edges = graph_info

    result = subprocess.run([
        edc_cut_path, f'-seed={seed}', f'-phi={phi}', '-sample_potential',
        f'-t1={t}', f'-t2=0',
        f'-balanced_cut_strategy={not(default_strategy)}'
    ],
                            input=graph_string,
                            text=True,
                            check=True,
                            timeout=480,
                            stdout=subprocess.PIPE)
    if result.returncode != 0:
        print(f'Failed cut: {result.stdout}')
        exit(1)
    else:
        lines = result.stdout.split('\n')
        resultType = lines[0].split()[0]
        iterationsUntil = int(lines[0].split()[1])

        return resultType == 'expander' and iterationsUntil <= t

def test_all_ts(edc_cut_path, graph_info, start_seed, phi, default_strategy):
    """Binary search for the smallest number of iterations required for the
    cut-matching game to reach the required potential threshold of an expander.

    Since the cut-matching game is randomized we evaluate the same point
    multiple times before being satisfied.

    """

    def test(t,seed):
        return is_expander(edc_cut_path, graph_info, seed, phi, default_strategy, t)

    t = 0
    while True:
        t += 1
        fail = False
        for i in range(10):
            if not test(t,start_seed+i):
                fail = True
                break
        if not fail:
            break

    _, graph_params, graph_edges = graph_info
    return (graph_params, phi, default_strategy, graph_edges, t)

if __name__ == '__main__':
    if len(sys.argv) != 6:
        print('Expected five arguments')
        exit(1)
    _, _, edc_cut_path, seed, gen_graph, output_file = sys.argv
    seed = int(seed)

    graph_params = [{
        'name': 'margulis',
        'n': i,
        'k': 1,
        'r': 0,
    } for i in range(3, 15)] + [{
        'name': 'clique',
        'n': i,
        'k': 1,
        'r': 0,
    } for i in numpy.linspace(10,45,20,dtype=int)]

    def graphParamsToString(p):
        ps = [p['name'], str(p['n']), str(p['k']), str(p['r'])]
        if 'p' in p:
            ps.append(str(p['p']))
        return '-'.join(ps)

    # contains tuples (graph_string, parameters for generating graph, edges in graph)
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
            print('Generating graph with type {} failed'.format(ps['name']))
            exit(1)
        lines = result.stdout.strip().split('\n')
        m = int(lines[0].split()[1])
        assert (len(lines) == m + 1)
        graphs.append((result.stdout, ps, m))

    with mp.Pool() as pool:
        jobs = []
        for graph_info in graphs:
            for phi in [0.001]:
                jobs.append((edc_cut_path, graph_info, seed, phi, True))
                jobs.append((edc_cut_path, graph_info, seed, phi, False))

        result = pool.starmap(test_all_ts, jobs, chunksize=1)

    with open(output_file, 'w') as f:
        writer = csv.DictWriter(f,
                                fieldnames=[
                                    'graph',
                                    'graph_type',
                                    'phi',
                                    'strategy',
                                    'log10_squared_edges',
                                    'iterations',
                                ])
        writer.writeheader()

        for p, phi, default_strategy, edges, t in result:
            writer.writerow({
                'graph': graphParamsToString(p),
                'graph_type': p['name'],
                'phi': phi,
                'strategy': 'Default' if default_strategy else 'Balanced',
                'log10_squared_edges': log10(edges) * log10(edges),
                'iterations': t,
            })
