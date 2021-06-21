#! /usr/bin/env python3

from math import log10
import itertools
import sys
import multiprocessing as mp
import subprocess
import csv
import numpy


def cut(edc_cut_path, graph_info, phi, default_strategy, min_balance):
    """Run 'edc-cut', assert balanced cut is returned, and return the graph
    parameters and the balance.

    """
    graph_string, graph_params, graph_edges = graph_info
    result = subprocess.run([edc_cut_path, f'-phi={phi}', f'-balanced_cut_strategy={not(default_strategy)}', f'-min_balance={min_balance}'],
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
        resultType = lines[0].split()[0]
        volA,volB = list(map(int, lines[0].split()[2:4]))
        assert volA + volB == 2 * graph_edges

        if resultType != 'balanced_cut':
            print(f'Cut resulted in: {resultType} with params {graph_params}')
            exit(1)
        xlen, *xs = list(map(int, lines[1].split()))
        assert xlen == len(xs)
        ylen, *ys = list(map(int, lines[2].split()))
        assert ylen == len(ys)

        balance = min(volA, volB) / (volA + volB)
        return (graph_params, graph_edges, default_strategy, min_balance, balance)


if __name__ == '__main__':
    if len(sys.argv) != 6:
        print('Expected five arguments')
        exit(1)
    _, _, edc_cut_path, seed, gen_graph, output_file = sys.argv

    graph_params = [{
        'name': 'clique-path',
        'n': 20,
        'k': k,
    } for k in [i*50 for i in range(1,20)]]

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
            '-n={}'.format(ps['n']), '-k={}'.format(ps['k'])
        ]
        result = subprocess.run(cmd,
                                text=True,
                                check=True,
                                timeout=60,
                                stdout=subprocess.PIPE)
        if result.returncode != 0:
            print('Generating graph with type {} failed'.format(ps['name']))
            exit(1)
        lines = result.stdout.strip().split('\n')
        m = int(lines[0].split()[1])
        assert (len(lines) == m + 1)
        graphs.append((result.stdout, ps, m))

    with mp.Pool() as pool:
        jobs = []
        for g in graphs:
            for phi in [0.001]:
                for balance in [0, 0.25, 0.45]:
                    for _ in range(8):
                        jobs.append((edc_cut_path, g, phi, True, balance))
                        jobs.append((edc_cut_path, g, phi, False, balance))
        result = pool.starmap(cut, jobs, chunksize=1)

    with open(output_file, 'w') as f:
        writer = csv.DictWriter(f,
                                fieldnames=[
                                    'graph',
                                    'edges',
                                    'default_strategy',
                                    'min_balance',
                                    'balance',
                                ])
        writer.writeheader()

        for p, edges, default_strategy, min_balance, balance in result:
            writer.writerow({
                'graph': graphParamsToString(p),
                'edges': edges,
                'default_strategy': 'Default' if default_strategy else 'Balanced',
                'min_balance': min_balance,
                'balance': balance,
            })
