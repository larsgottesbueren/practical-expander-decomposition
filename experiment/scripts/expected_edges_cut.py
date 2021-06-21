#! /usr/bin/env python3

from math import log10
import itertools
import sys
import multiprocessing as mp
import subprocess
import csv
import numpy


def partition(edc_path, graph_info, phi):
    """Run 'edc' and count edges cut.

    """
    graph_string, graph_params = graph_info
    result = subprocess.run([edc_path, f'-phi={phi}'],
                            input=graph_string,
                            text=True,
                            check=True,
                            timeout=480,
                            stdout=subprocess.PIPE)
    if result.returncode != 0:
        print(f'Failed edc: {result.stdout}')
        exit(1)
    else:
        edges_expected = graph_params['r']

        lines = result.stdout.split('\n')
        edges_cut = int(lines[0].split()[0])

        return (graph_params, phi, edges_expected, edges_cut)


if __name__ == '__main__':
    if len(sys.argv) != 6:
        print('Expected five arguments')
        exit(1)
    _, edc_path, _, seed, gen_graph, output_file = sys.argv

    graph_params = [{
        'name': 'margulis',
        'n': 11,
        'k': k,
        'r': 5*k
    } for k in [i for i in range(2,50)]]

    def graphParamsToString(p):
        ps = [p['name'], str(p['n']), str(p['k'])]
        if 'r' in p:
            ps.append(str(p['r']))
        if 'p' in p:
            ps.append(str(p['p']))
        return '-'.join(ps)

    # contains tuples (graph_string, parameters for generating graph, edges in graph)
    graphs = []
    for ps in graph_params:
        cmd = [
            f'./{gen_graph}', f'--seed={seed}', ps['name'],
            '-n={}'.format(ps['n']),
            '-k={}'.format(ps['k']),
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
        graphs.append((result.stdout, ps))

    with mp.Pool() as pool:
        jobs = []
        for graph_info in graphs:
            for phi in [0.01, 0.005, 0.0025, 0.00125]:
                jobs.append((edc_path, graph_info, phi))

        result = pool.starmap(partition, jobs, chunksize=1)

    with open(output_file, 'w') as f:
        writer = csv.DictWriter(f,
                                fieldnames=[
                                    'graph',
                                    'phi',
                                    'edges_expected',
                                    'edges_cut',
                                ])
        writer.writeheader()

        for p, phi, edges_expected, edges_cut in result:
            writer.writerow({
                'graph': graphParamsToString(p),
                'phi': phi,
                'edges_expected': edges_expected,
                'edges_cut': edges_cut,
            })
