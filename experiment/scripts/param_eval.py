#! /usr/bin/env python3

import itertools
import multiprocessing as mp
import subprocess
import csv
import argparse
import glob
import os

bin_path = "../../release/EDC"
graph_path = "../graphs/real/"

phi_values = [0.001, 0.01]

options = {
    'flow-vectors': [1, 5, 10, 20],
    'krv-first': [False, True],
    'use-cut-heuristics': [False, True],
    'flow-fraction': [False, True],
    'adaptive': [False, True],
    'kahan-error': [False, True],
    'seed' : [1,2,3,4,5]
}


def edc_call(graph, phi, options):
    args = [bin_path]
    for key, val in options:
        if type(val) == bool:  # peculiarity of tlx (bool options can only be set to true)
            if val:
                options.append('--' + key)
        else:
            options.append('--' + key + ' ' + str(val))
    args.extend([graph, phi])
    result = subprocess.run(args, text=True, check=True, timeout=480, stdout=subprocess.PIPE)
    if result.returncode != 0:
        print(f'Failed cut: {result.stdout}')
        return None
    else:
        lines = result.stdout.strip().split('\n')

        result = {}
        result['graph'] = os.path.basename(graph)
        result['phi'] = phi

        ## TODO parse runtimes from stdout

        result.update(options) #copy the options into the result
        return result


def enum_options():
    keys, values = zip(*options.items())
    return [dict(zip(keys, v)) for v in itertools.product(*values)]


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-t', '--threads', type=int, default=1)
    parser.add_argument('-o', '--output', type=str, default='results.csv')
    args = parser.parse_args()

    graph_files = glob.glob(graph_path + '*.graph')
    configs = enum_options()

    jobs = itertools.product(graph_files, phi_values, configs)
    with mp.Pool(processes=args.threads) as pool:
        results = pool.starmap(edc_call, jobs, chunksize=1)

    with open(args.output, 'w') as f:
        writer = csv.DictWriter(f, fieldnames=results[0].keys())
        writer.writeheader()
        writer.writerows(results)   # TODO beware of failed runs
