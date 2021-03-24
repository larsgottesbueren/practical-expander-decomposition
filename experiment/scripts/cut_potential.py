#! /usr/bin/env python3

import itertools
import sys
import multiprocessing as mp
import subprocess
import csv
import numpy


def cut(edc_cut_path, graph, phi):
    """Run 'edc-cut', assert balanced cut is returned, and return the graph
    parameters, phi, and potential sampling.

    """
    graph_string, graph_params = graph
    result = subprocess.run(
        [edc_cut_path, f'-phi={phi}', '-sample_potential'],
        input=graph_string,
        text=True,
        check=True,
        timeout=480,
        stdout=subprocess.PIPE)
    if result.returncode != 0:
        print(f'Failed cut: {result.stdout}')
        exit(1)
    else:
        lines = result.stdout.strip().split('\n')
        resultType = lines[0].split()[0]
        if resultType != 'balanced_cut':
            print(f'Cut did not result in balanced_cut: {resultType}')
            exit(1)
        xlen, *xs = list(map(int, lines[1].split()))
        assert xlen == len(xs)
        ylen, *ys = list(map(int, lines[2].split()))
        assert ylen == len(ys)

        assert (max(xs) < min(ys) or max(ys) < min(xs))

        numSamples = int(lines[3].strip())
        samples = list(enumerate(map(float, lines[4].strip().split())))
        assert len(samples) == numSamples

        return (graph_params, phi, samples)


if __name__ == '__main__':
    if len(sys.argv) != 6:
        print('Expected five arguments')
        exit(1)
    _, _, edc_cut_path, seed, gen_graph, output_file = sys.argv

    graph_params = [{
        'name': 'clique',
        'n': 10,
        'k': 2,
        'r': 1,
    }, {
        'name': 'margulis',
        'n': 10,
        'k': 2,
        'r': 1,
    }, {
        'name': 'clique',
        'n': 100,
        'k': 2,
        'r': 1,
    }, {
        'name': 'margulis',
        'n': 20,
        'k': 2,
        'r': 1,
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
        jobs = []
        for g in graphs:
            for phi in [0.01]:
                for _ in range(10):
                    jobs.append((edc_cut_path, g, phi))

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
            for iteration, potential in samples:
                writer.writerow({
                    'graph': graphParamsToString(p),
                    'phi': phi,
                    'iteration': iteration,
                    'potential': potential
                })
