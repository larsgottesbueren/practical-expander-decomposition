#! /usr/bin/env python3

import itertools
import sys
import multiprocessing as mp
import subprocess
import csv
import numpy


def cut(edc_cut_path, graph, seed, phi, default_strategy):
    """Run 'edc-cut', assert expander is returned, and return the graph parameters,
    phi, and conductivity sampling.

    """
    graph_string, graph_params = graph

    result = subprocess.run([
        edc_cut_path, f'-seed={seed}', f'-phi={phi}', '-sample_potential',
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
        lines = result.stdout.strip().split('\n')
        resultType = lines[0].split()[0]
        if resultType != 'expander':
            print(f'Cut did not result in expander: {resultType}')
            exit(1)
        xlen, *xs = list(map(int, lines[1].split()))
        assert xlen == len(xs)
        ylen, *ys = list(map(int, lines[2].split()))
        assert ylen == len(ys)

        assert (xlen == 0 or ylen == 0)

        numSamples = int(lines[3].strip())
        samples = list(enumerate(map(float, lines[4].strip().split())))
        assert len(samples) == numSamples

        return (graph_params, phi, default_strategy, samples)


if __name__ == '__main__':
    if len(sys.argv) != 6:
        print('Expected five arguments')
        exit(1)
    _, _, edc_cut_path, seed, gen_graph, output_file = sys.argv
    seed = int(seed)

    graph_params = [{
        'name': 'clique',
        'n': 20,
        'k': 1,
        'r': 0,
    }, {
        'name': 'margulis',
        'n': 10,
        'k': 1,
        'r': 0,
    }, {
        'name': 'clique',
        'n': 50,
        'k': 1,
        'r': 0,
    }, {
        'name': 'margulis',
        'n': 20,
        'k': 1,
        'r': 0,
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
            for phi in [0.001]:
                for i in range(10):
                    jobs.append((edc_cut_path, g, seed+i, phi, True))
                    jobs.append((edc_cut_path, g, seed+i, phi, False))

        result = pool.starmap(cut, jobs, chunksize=1)

    with open(output_file, 'w') as f:
        writer = csv.DictWriter(f,
                                fieldnames=[
                                    'graph',
                                    'phi',
                                    'strategy_type',
                                    'iteration',
                                    'potential',
                                ])
        writer.writeheader()

        for p, phi, default_strategy, samples in result:
            for iteration, potential in samples:
                writer.writerow({
                    'graph': graphParamsToString(p),
                    'phi': phi,
                    'strategy_type': 'Default' if default_strategy else 'Balanced',
                    'iteration': iteration,
                    'potential': potential
                })
