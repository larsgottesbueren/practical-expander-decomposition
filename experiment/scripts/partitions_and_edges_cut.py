#! /usr/bin/env python3

import itertools
import sys
import multiprocessing as mp
import subprocess
import csv
import numpy


def partition(edc_path, graph, phi):
    """Run 'edc' and return the graph parameters and phi along with number of edges
cut and partitions created

    """
    graph_string, graph_params = graph
    result = subprocess.run([edc_path, f'-phi={phi}'],
                            input=graph_string,
                            text=True,
                            check=True,
                            timeout=240,
                            stdout=subprocess.PIPE)
    if result.returncode != 0:
        print('Failed partitioning: {}'.format(result.stdout))
        exit(1)
    else:
        edges_cut, num_partitions = list(map(int, result.stdout.split()[0:2]))
        return (graph_params, phi, edges_cut, num_partitions)


if __name__ == '__main__':
    if len(sys.argv) != 6:
        print('Expected five arguments')
        exit(1)
    _, edc_path, _, seed, gen_graph, output_file = sys.argv

    graph_params = [{
        'name': 'clique',
        'n': 100,
        'k': 1,
        'expected_partitions': 1,
        'expected_edges_cut': 0,
    }, {
        'name': 'clique',
        'n': 100,
        'k': 2,
        'r': 1,
        'expected_partitions': 2,
        'expected_edges_cut': 1,
    }, {
        'name': 'clique',
        'n': 100,
        'k': 10,
        'r': 100,
        'expected_partitions': 10,
        'expected_edges_cut': 100,
    }, {
        'name': 'clique-path',
        'n': 50,
        'k': 100,
        'expected_partitions': 100,
        'expected_edges_cut': 99,
    }, {
        'name': 'clique',
        'n': 30,
        'k': 100,
        'r': 1000,
        'p': 50,
        'expected_partitions': 100,
        'expected_edges_cut': 1000,
    }, {
        'name': 'margulis',
        'n': 10,
        'k': 100,
        'r': 1000,
        'expected_partitions': 100,
        'expected_edges_cut': 1000,
    }]

    def graphParamsToString(p):
        result = p['name']
        for k, v in p.items():
            if k == 'name': continue
            result += f'-{k}={v}'
        return result

    graphs = []
    for ps in graph_params:
        cmd = [
            f'./{gen_graph}', f'--seed={seed}', ps['name'],
            '-n={}'.format(ps['n']), '-k={}'.format(ps['k'])
        ]
        if 'r' in ps:
            cmd.append('-r={}'.format(ps['r']))
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
        iterations = 8
        phis = numpy.logspace(0, -4, 10)
        jobs = [(edc_path, g, phi) for g, phi, _ in itertools.product(
            graphs, phis, list(range(iterations)))]
        results = pool.starmap(partition, jobs, chunksize=1)

    with open(output_file, 'w') as f:
        writer = csv.DictWriter(f,
                                fieldnames=[
                                    'graph', 'phi', 'partitions', 'edges_cut',
                                    'expected_partitions', 'expected_edges_cut'
                                ])
        writer.writeheader()

        for p, phi, edges_cut, num_partitions in results:
            writer.writerow({
                'graph': graphParamsToString(p),
                'phi': phi,
                'partitions': num_partitions,
                'edges_cut': edges_cut,
                'expected_partitions': p['expected_partitions'],
                'expected_edges_cut': p['expected_edges_cut'],
            })
