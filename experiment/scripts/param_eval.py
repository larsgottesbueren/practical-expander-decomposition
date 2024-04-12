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


def edc_call(graph, phi, options, timelimit=1800):
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

    result = {'graph': os.path.basename(graph), 'phi': phi}
    result.update(options)  # copy the options into the result
    result['measured time'] = timelimit
    result['cut'] = -1
    result['partitions'] = -1
    result['timeout'] = False

    try:
        subproc_result = subprocess.run(args, text=True, check=False, capture_output=True, timeout=timelimit)
    except:
        print('Time out / Failed run: ', graph, phi, options)
        result['timeout'] = True
        return result
    

    with open('logs/' + graph + '.' + options['name'] + '.log', 'w') as log_file:
        log_file.write(subproc_result.stdout)
    lines = subproc_result.stdout.strip().split('\n')

    try:
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
    except:
        result['cut'] = -2
        result['partitions'] = -2
    
    return result


def enum_options():
    keys, values = zip(*options.items())
    return [dict(zip(keys, v)) for v in itertools.product(*values)]


def incremental_configs():
    configs = []

    base_config = {
        'flow-vectors': 1,
        'krv-first': False,
        'use-cut-heuristics': False,
        'use-balanced-partitions': False,
        'flow-fraction': False,
        'adaptive': False,
        'kahan-error': True,
        'flow-vectors' : 1,
        'seed': 1,
        'name': 'Arv',
        'base-config' : False   # yes...
    }
    config = copy.copy(base_config)
    base_config['base-config'] = True
    #configs.append(base_config)
    
    b20 = copy.copy(base_config)
    b20['flow-vectors'] = 20
    b20['name'] = 'Arv-20'
    #configs.append(b20)

    ada = copy.copy(config)
    ada['name'] = '+Ada'
    ada['flow-vectors'] = 20
    ada['adaptive'] = True
    #configs.append(ada)

    config['use-cut-heuristics'] = True
    config['name'] = '+Cut'
    #configs.append(copy.copy(config))

    config['use-balanced-partitions'] = True
    config['name'] = '+Cut+Bala'
    #configs.append(copy.copy(config))

    config['use-balanced-partitions'] = False
    config['adaptive'] = True
    config['flow-vectors'] = 20
    config['name'] = '+Cut+Ada'
    #configs.append(copy.copy(config))

    config['use-balanced-partitions'] = True
    config['name'] = '+Cut+Bala+Ada'
    #configs.append(copy.copy(config))

    frac = copy.copy(config)
    frac['flow-fraction'] = True
    frac['name'] = '+Cut+Ada+Frac'
    configs.append(frac) # leave out frac for now

    ada_frac = copy.copy(frac)
    ada_frac['name'] = '+Ada+Frac'
    ada_frac['use-cut-heuristics'] = False
    configs.append(ada_frac)

    cut_frac = copy.copy(frac)
    cut_frac['name'] = '+Cut+Frac'
    config['adaptive'] = False
    configs.append(cut_frac)

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

def large_graphs_configs():
    base_config = {
        'flow-vectors': 1,
        'krv-first': False,
        'use-cut-heuristics': False,
        'use-balanced-partitions': False,
        'flow-fraction': False,
        'adaptive': False,
        'kahan-error': True,
        'flow-vectors' : 1,
        'seed': 1,
        'name': 'Arv',
        'base-config' : True
    }

    our_config = {
        'flow-vectors': 20,
        'krv-first': False,
        'use-cut-heuristics': True,
        'use-balanced-partitions': True,
        'flow-fraction': False,
        'trim-with-max-flow-first' : True,
        'adaptive': True,
        'kahan-error': True,
        'seed': 1,
        'base-config' : False,
        'name': 'Ours'
    }

    frac = {
        'flow-vectors': 20,
        'krv-first': False,
        'use-cut-heuristics': True,
        'use-balanced-partitions': True,
        'flow-fraction': True,
        'adaptive': True,
        'trim-with-max-flow-first' : True,
        'kahan-error': True,
        'seed': 1,
        'base-config' : False,
        'name': 'Ours+Frac'
    }

    return [our_config, frac]

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
    parser.add_argument('-g', '--graphs', type=str, default='../graphs/real/')
    parser.add_argument('--timelimit', type=int, default=1800, help='Time limit in seconds')
    parser.add_argument('-c', '--configs', type=str, default='incremental', help='Which configs of EDC to run. [incremental, large-graphs]')
    args = parser.parse_args()

    graph_path = args.graphs

    graph_files = glob.glob(graph_path + '*.graph')
    # configs = enum_options()
    if args.configs == 'incremental':
        configs = incremental_configs()
        configs = add_more_seeds(configs)
    elif args.configs == 'large-graphs':
        configs = large_graphs_configs()

    jobs = list(itertools.product(graph_files, phi_values, configs, [args.timelimit]))  # list is necessary for len(jobs)
    with mp.Pool(processes=args.threads) as pool:
        results = pool.starmap(edc_call, tqdm.tqdm(jobs, total=len(jobs)), chunksize=1)

    with open(args.output, 'w') as f:
        writer = csv.DictWriter(f, fieldnames = set().union(*[r.keys() for r in results]), restval=args.timelimit)
        writer.writeheader()
        writer.writerows(results)
