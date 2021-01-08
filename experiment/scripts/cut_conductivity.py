#! /usr/bin/env python3

import itertools
import sys
import multiprocessing as mp
import subprocess
import csv
import numpy


def cut(edc_cut_path, graph, phi):
    """Run 'edc-cut', assert balanced cut is returned, and return the graph
    parameters, phi, and conductivity sampling.

    """
    graph_string, graph_params = graph
    numSamples = 20
    result = subprocess.run(
        [edc_cut_path, f'-phi={phi}', f'-verify_expansion={numSamples}'],
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
        resultType = lines[0]
        if resultType != 'balanced_cut':
            print(f'Cut did not result in balanced_cut: {resultType}')
            exit(1)
        xlen, *xs = list(map(int, lines[1].split()))
        assert (xlen == len(xs))
        ylen, *ys = list(map(int, lines[2].split()))
        assert (ylen == len(ys))

        iterations, numSamples = list(map(int, lines[3].split()))
        assert (len(lines) - 4 - 1 == iterations)

        samples = {}
        for i in range(4, len(lines)):
            iteration = i - 4
            samples[iteration] = list(map(float, lines[i].split()))

        return (graph_params, phi, samples)


if __name__ == '__main__':
    if len(sys.argv) != 6:
        print('Expected five arguments')
        exit(1)
    _, _, edc_cut_path, seed, gen_graph, output_file = sys.argv

    graph_params = [{
        'name': 'dumbbell',
        'n': 100,
        'k': 2,
    }, {
        'name': 'dumbbell',
        'n': 100,
        'k': 3,
    }, {
        'name': 'clique',
        'n': 100,
        'k': 2,
    }, {
        'name': 'clique',
        'n': 250,
        'k': 2,
    }, {
        'name': 'dumbbell',
        'n': 250,
        'k': 2,
    }, {
        'name': 'clique-random',
        'n': 100,
        'k': 2,
        'r': 10,
    }, {
        'name': 'clique-random',
        'n': 250,
        'k': 2,
        'r': 10,
    }, {
        'name': 'clique-path',
        'n': 20,
        'k': 1000,
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
        phis = [0.001, 0.01]
        jobs = [(edc_cut_path, g, phi)
                for g, phi in itertools.product(graphs, phis)]
        result = pool.starmap(cut, jobs, chunksize=1)

    with open(output_file, 'w') as f:
        writer = csv.DictWriter(f,
                                fieldnames=[
                                    'graph',
                                    'phi',
                                    'iteration',
                                    'conductivity',
                                ])
        writer.writeheader()

        for p, phi, samples in result:
            for iteration, cs in samples.items():
                for c in cs:
                    writer.writerow({
                        'graph': graphParamsToString(p),
                        'phi': phi,
                        'iteration': iteration,
                        'conductivity': c
                    })
