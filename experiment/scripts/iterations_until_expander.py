#! /usr/bin/env python3

from math import log10
import itertools
import sys
import multiprocessing as mp
import subprocess
import csv
import numpy


def cut(edc_cut_path, graph_info, phi, default_strategy, configured):
    """Run 'edc-cut', assert expander is returned, and return the graph parameters,
    phi, and the number of iterations run until correct expansion reached.

    """
    graph_string, graph_params, graph_edges = graph_info

    t1 = 0
    t2 = 1
    if configured:
        if default_strategy:
            t1 = 200
            t2 = 23
        else:
            t1 = 30
            t2 = 6

    result = subprocess.run([
        edc_cut_path, f'-phi={phi}', '-sample_potential',
        f'-t1={t1}', f'-t2={t2}', '-min_iterations=500',
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
        if resultType != 'expander':
            print(
                f'Failed to find expander. Found {resultType} with params {graph_params}'
            )
            exit(1)

        return (graph_params, phi, default_strategy, configured, graph_edges,
                iterationsUntil)


if __name__ == '__main__':
    if len(sys.argv) != 6:
        print('Expected five arguments')
        exit(1)
    _, _, edc_cut_path, seed, gen_graph, output_file = sys.argv

    graph_params = [{
        'name': 'margulis',
        'n': i,
        'k': 1,
        'r': 0,
    } for i in range(3, 20)] + [{
        'name': 'clique',
        'n': i,
        'k': 1,
        'r': 0,
    } for i in range(10, 60)]

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
                for _ in range(8):
                    jobs.append((edc_cut_path, graph_info, phi, True, True))
                    jobs.append((edc_cut_path, graph_info, phi, True, False))
                    jobs.append((edc_cut_path, graph_info, phi, False, True))
                    jobs.append((edc_cut_path, graph_info, phi, False, False))

        result = pool.starmap(cut, jobs, chunksize=1)

    with open(output_file, 'w') as f:
        writer = csv.DictWriter(f,
                                fieldnames=[
                                    'graph',
                                    'graph_type',
                                    'phi',
                                    'strategy',
                                    'configured',
                                    'log10_squared_edges',
                                    'iterationsUntilValid',
                                ])
        writer.writeheader()

        for p, phi, default_strategy, configured, edges, iterations in result:
            writer.writerow({
                'graph': graphParamsToString(p),
                'graph_type': p['name'],
                'phi': phi,
                'strategy': 'Default' if default_strategy else 'Balanced',
                'configured': 'After' if configured else 'Before',
                'log10_squared_edges': log10(edges) * log10(edges),
                'iterationsUntilValid': iterations,
            })
