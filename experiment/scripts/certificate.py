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
        lines = result.stdout.strip().split('\n')
        partitions = int(lines[0].strip().split()[1])
        if partitions != 1:
            return None
        certificate = float(lines[1].strip().split()[1])

        return (graph_params, phi, certificate)


if __name__ == '__main__':
    if len(sys.argv) != 6:
        print('Expected five arguments')
        exit(1)
    _, edc_path, _, seed, gen_graph, output_file = sys.argv

    graph_params = [{
        'name': 'clique',
        'n': 10,
        'k': 1,
    }, {
        'name': 'clique',
        'n': 50,
        'k': 1,
    }, {
        'name': 'margulis',
        'n': 11,
        'k': 1,
    }, {
        'name': 'margulis',
        'n': 43,
        'k': 1,
    }]

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
            for phi in numpy.linspace(0.0001,0.01,100):
                for i in range(8):
                    jobs.append((edc_path, graph_info, phi))

        result = pool.starmap(partition, jobs, chunksize=1)

    with open(output_file, 'w') as f:
        writer = csv.DictWriter(f,
                                fieldnames=[
                                    'graph',
                                    'phi',
                                    'certificate',
                                    'certificate_ratio'
                                ])
        writer.writeheader()

        for r in result:
            if r is not None:
                p, phi, certificate = r
                writer.writerow({
                    'graph': graphParamsToString(p),
                    'phi': phi,
                    'certificate': certificate,
                    'certificate_ratio': certificate/phi
                })
