#! /usr/bin/env python3

import itertools
import sys
import multiprocessing as mp
import subprocess
import csv
import numpy


def cut(edc_cut_path, graph, phi):
    """Run 'edc-cut', return graph parameters, phi, and conductivity sampling.

    """
    graph_string, graph_params = graph

    n = int(graph_string.strip().split('\n')[0].strip().split()[0])

    numSamples = 50
    result = subprocess.run(
        [edc_cut_path, f'-phi={phi}', f'-verify_expansion={numSamples}'],
        input=graph_string,
        text=True,
        check=True,
        timeout=480,
        stdout=subprocess.PIPE)
    if result.returncode != 0:
        print(f'edc-cut failed: {result.stdout}')
        exit(1)
    else:
        lines = result.stdout.strip().split('\n')
        resultType = lines[0].split()[0]

        xlen, *xs = list(map(int, lines[1].split()))
        assert xlen == len(xs)
        ylen, *ys = list(map(int, lines[2].split()))
        assert ylen == len(ys)

        if resultType == "balanced_cut" or resultType == "near_expander":
            assert max(xs) < min(ys) or max(ys) < min(xs)
            assert xlen > 0 and ylen > 0
        else:
            assert max(xlen,ylen) == n and min(xlen,ylen) == 0

        iterations, numSamples = list(map(int, lines[3].split()))
        assert len(lines) - 4 == iterations

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
        'name': 'clique',
        'n': 10,
        'k': 1,
        'r': 0,
        'p': 100,
    }, {
        'name': 'clique',
        'n': 500,
        'k': 1,
        'r': 0,
        'p': 100,
    }, {
        'name': 'clique',
        'n': 250,
        'k': 2,
        'r': 10,
        'p': 100,
    }, {
        'name': 'margulis',
        'n': 150,
        'k': 1,
        'r': 0,
    }, {
        'name': 'margulis',
        'n': 100,
        'k': 2,
        'r': 10,
    }]

    def graphParamsToString(p):
        ps = [p['name'], str(p['n']), str(p['k'])]
        if 'r' in p: ps.append(str(p['r']))
        if 'p' in p: ps.append(str(p['p']))
        return '-'.join(ps)

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
                                    'potential',
                                ])
        writer.writeheader()

        for p, phi, samples in result:
            for iteration, potentials in samples.items():
                for potential in potentials:
                    writer.writerow({
                        'graph': graphParamsToString(p),
                        'phi': phi,
                        'iteration': iteration,
                        'potential': potential
                    })
