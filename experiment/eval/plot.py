import pandas as pd
from collections import *
import seaborn as sb
import matplotlib.pyplot as plt
import matplotlib.gridspec as grd
import copy
import scipy.stats
import numpy as np
import commons
import itertools

import runtime, runtime_shares

plt.rc('text', usetex=True)
plt.rc('font', family='libertine')
#plt.rc('font', size=6)
#plt.rc('legend',fontsize=5)
plt.rc('axes', axisbelow=True)

def aggregate_runtimes(df, field = 'Total', seed_aggregator = 'median'):
  def aggregate_func(series):
    if seed_aggregator == "mean":
      return np.mean(series)
    elif seed_aggregator == "median":
      return np.median(series)
    elif seed_aggregator == 'gmean':
      return scipy.stats.gmean(series)
    elif seed_aggregator == 'max':
      return np.max(series)

  df = df.groupby(runtime.keys)[field].agg(aggregate_func).reset_index()
  return df

#files = ['results.v3.parallel.csv', 'results.partial-flow.csv']
files = ['results.small.csv']

df = pd.concat([pd.read_csv(f) for f in files])

for phi in df.phi.unique():
  print("phi=", phi)
  if phi == 0.01:
    continue

  if False:
    for algo in df.name.unique():
      fig, ax = plt.subplots()
      print(algo)
      runtime_shares.stacked_bars(ax=ax, df=df[(df.seed == 1) & (df.name == algo) & (df.phi == phi)].copy(), 
        fields = ['FlowMatch', 'MatchDFS', 'ProposeCut', 'CutHeuristics', 'Components', 'Miscellaneous', 'FlowTrim'], 
        sort_field = 'Total', 
        tfield="Total")
      fig.savefig('runtime_breakdown_' + algo + '.pdf', pad_inches=0.01, bbox_inches='tight')
    exit()


  
  mdf = aggregate_runtimes(df, seed_aggregator='median')
  
  #mdf = df[df.seed == 9]
  rdf = runtime.relative_times(mdf)
  #print(rdf.iloc[rdf['base_time'].argmax()])
  #print(rdf[rdf['relative_time'] < 1.05][['graph', 'Total', 'base_time', 'CutHeuristics', 'FlowMatch', 'name', 'partitions', 'cut', 'base_cut']])
  
  #rdf = rdf[rdf['base_time'] <= 100]
  
  algos = list(rdf.name.unique())
  print(algos)
  for algo in itertools.chain(algos):
    '''
    print(algo)
    print(mdf[mdf.name == algo].iloc[mdf[mdf.name == algo]['Total'].argmax()])
    print(df[df.name == algo].iloc[df[df.name == algo]['Total'].argmax()])
    print('---------------------------')
    print('---------------------------')
    print('---------------------------')
    '''
  colors = commons.construct_new_color_mapping(algos)
  if True:
    fig, ax = plt.subplots()
    runtime.xy_plot(ax=ax, rdf=rdf, algos=algos, colors=colors)
    fig.savefig('xy.pdf', pad_inches=0.01, bbox_inches='tight')

  if True:
    fig, ax = plt.subplots()
    runtime.rel_time_plot(ax=ax, rdf=rdf, algos=algos, colors=colors)
    fig.savefig('speedup.pdf', pad_inches=0.01, bbox_inches='tight')
