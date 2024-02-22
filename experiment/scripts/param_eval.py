#! /usr/bin/env python3

import itertools
import multiprocessing as mp
import subprocess
import csv
import argparse
import glob
import os
import copy
import tqdm

bin_path = "../../release/EDC"
graph_path = "../graphs/real/"

phi_values = [
    0.001,
    # 0.01
]

options = {
    'flow-vectors': [1, 5, 10, 20],
    'krv-first': [False, True],
    'use-cut-heuristics': [False, True],
    'flow-fraction': [False, True],
    'adaptive': [False, True],
    'kahan-error': [False, True],
    'seed': [1, 2, 3, 4, 5]
}


def edc_call(graph, phi, options):
    args = [bin_path]
    for key, val in options.items():
        if key == 'name':
            continue
        if type(val) == bool:  # peculiarity of tlx (bool options can only be set to true)
            if val:
                args.append('--' + key)
        else:
            args.append('--' + key)
            args.append(str(val))
    args.extend(['--log', '0'])
    args.extend([graph, str(phi)])
    result = subprocess.run(args, text=True, check=False, capture_output=True, timeout=1800)
    if result.returncode != 0:
        print('Failed run: ', graph, phi, options, 'stdout = ', result.stdout)
        return None
    else:
        lines = result.stdout.strip().split('\n')
        result = {'graph': os.path.basename(graph), 'phi': phi}

        for l in lines:
            s = l.split('\t\t')
            if len(s) == 3:
                # timer
                if s[0] != "Category":
                    result[s[0]] = float(s[1])
            elif "Total measured time" in l:
                s = l.replace('---', '')
                s = s.split(' ')
                result['measured time'] = float(s[-2])
            elif "Time " in l:
                s = l.split(' ')
                result[s[1]] = float(s[2].replace('s', ''))
            else:
                s = l.split(' ')
                result['cut'] = int(s[0])
                result['partitions'] = int(s[1])

        result.update(options)  # copy the options into the result
        return result


def enum_options():
    keys, values = zip(*options.items())
    return [dict(zip(keys, v)) for v in itertools.product(*values)]


def incremental_configs():
    base_config = {
        'flow-vectors': 20,
        'krv-first': False,
        'use-cut-heuristics': False,
        'flow-fraction': False,
        'adaptive': False,
        'kahan-error': True,
        'seed': 1,
        'name': 'Arv',
        'base-config' : False   # yes...
    }
    config = copy.copy(base_config)
    base_config['base-config'] = True
    configs = [base_config]

    ada = copy.copy(config)
    ada['name'] = '+Ada'
    ada['adaptive'] = True
    configs.append(ada)

    config['use-cut-heuristics'] = True
    config['name'] = '+Cut'
    configs.append(copy.copy(config))

    config['adaptive'] = True
    config['name'] = '+Cut+Ada'
    configs.append(copy.copy(config))

    #krv = copy.copy(config)
    #krv['krv-first'] = True
    #krv['name'] = '+Cut+Ada+KRV'
    #configs.append(krv)

    frac = copy.copy(config)
    frac['flow-fraction'] = True
    frac['name'] = '+Cut+Ada+Frac'
    # configs.append(frac) # leave out frac for now

    kahan = copy.copy(config)
    kahan['kahan-error'] = False
    kahan['name'] = '+Cut+Ada-Kahan'
    configs.append(kahan)

    our_config = {
        'flow-vectors': 20,
        'krv-first': False,
        'use-cut-heuristics': True,
        'flow-fraction': False,
        'adaptive': True,
        'kahan-error': True,
        'seed': 1,
        'base-config' : False,
        'name': 'Ours'
    }
    #configs.append(our_config)

    return configs

def add_more_seeds(configs):
    res = []
    for c in configs:
        for seed in range(1, 11):
            c['seed'] = seed
            res.append(copy.copy(c))
    return res


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-t', '--threads', type=int, default=1)
    parser.add_argument('-o', '--output', type=str, default='results.csv')
    args = parser.parse_args()

    graph_files = glob.glob(graph_path + '*.graph')
    # configs = enum_options()
    configs = incremental_configs()

    configs = add_more_seeds(configs)

    jobs = list(itertools.product(graph_files, phi_values, configs))  # list is necessary for len(jobs)
    with mp.Pool(processes=args.threads) as pool:
        results = pool.starmap(edc_call, tqdm.tqdm(jobs, total=len(jobs)), chunksize=1)

    with open(args.output, 'w') as f:
        writer = csv.DictWriter(f, fieldnames=results[0].keys())
        writer.writeheader()
        writer.writerows(results)  # TODO beware of failed runs
