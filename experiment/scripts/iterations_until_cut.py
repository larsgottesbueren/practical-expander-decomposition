#! /usr/bin/env python3

from math import log10
import itertools
import sys
import multiprocessing as mp
import subprocess
import csv
import numpy


def cut(edc_cut_path, graph_info, phi):
    """Run 'edc-cut', assert balanced cut is returned, and return the graph
    parameters, phi, and the number of iterations run.

    """
    graph_string, graph_params, graph_edges = graph_info
    numSamples = 20
    result = subprocess.run([edc_cut_path, f'-phi={phi}'],
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
        iterations = int(lines[0].split()[1])
        if resultType != 'balanced_cut':
            print(f'Cut did not result in balanced_cut: {resultType}')
            exit(1)

        return (graph_params, phi, graph_edges, iterations)


if __name__ == '__main__':
    if len(sys.argv) != 6:
        print('Expected five arguments')
        exit(1)
    _, _, edc_cut_path, seed, gen_graph, output_file = sys.argv

    graph_params = [{
        'name': 'clique-random',
        'n': 30,
        'k': 2,
        'r': 1,
    }, {
        'name': 'clique-random',
        'n': 100,
        'k': 2,
        'r': 1,
    }, {
        'name': 'clique-random',
        'n': 200,
        'k': 2,
        'r': 1,
    }, {
        'name': 'clique-random',
        'n': 500,
        'k': 2,
        'r': 1,
    }, {
        'name': 'margulis',
        'n': 10,
        'k': 2,
        'r': 1,
    }, {
        'name': 'margulis',
        'n': 50,
        'k': 2,
        'r': 1,
    }, {
        'name': 'margulis',
        'n': 100,
        'k': 2,
        'r': 1,
    }]

    def graphParamsToString(p):
        return '-'.join([p['name'], str(p['n']), str(p['k']), str(p['r'])])

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
        phis = [0.001]
        numIterations = 8
        jobs = [(edc_cut_path, graph_info, phi)
                for graph_info, phi, _ in itertools.product(
                    graphs, phis, range(numIterations))]
        result = pool.starmap(cut, jobs, chunksize=1)

    with open(output_file, 'w') as f:
        writer = csv.DictWriter(f,
                                fieldnames=[
                                    'graph',
                                    'phi',
                                    'log10_squared_edges',
                                    'iterations',
                                ])
        writer.writeheader()

        for p, phi, edges, iterations in result:
            writer.writerow({
                'graph': graphParamsToString(p),
                'phi': phi,
                'log10_squared_edges': log10(edges) * log10(edges),
                'iterations': iterations,
            })
