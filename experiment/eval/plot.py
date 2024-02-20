import pandas as pd
from collections import *
import seaborn as sb
import matplotlib.pyplot as plt
import matplotlib.gridspec as grd
import copy
import scipy.stats
import numpy as np
import commons

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

df = pd.read_csv('results.sequential.csv')

df = df[df.name != 'Arv']
df2 = pd.read_csv('results.arv.sequential.csv')
df2['CutHeuristics'] = 0
df = pd.concat([df, df2])

for phi in df.phi.unique():
  if phi == 0.01:
    continue

  if False:
    for algo in df.name.unique():
      if algo != "Arv":
        continue
      fig, ax = plt.subplots()
      print(algo)
      runtime_shares.stacked_bars(ax=ax, df=df[(df.seed == 1) & (df.name == algo) & (df.phi == phi)].copy(), 
        fields = ['FlowMatch', 'MatchDFS', 'ProposeCut', 'CutHeuristics', 'Components', 'Miscellaneous', 'FlowTrim'], 
        sort_field = 'FlowMatch', tfield="Total")
      fig.savefig('runtime_breakdown_' + algo + '.pdf', pad_inches=0.01, bbox_inches='tight')


  #mdf = aggregate_runtimes(df[df.phi == phi])
  mdf = df[df.seed == 1]
  rdf = runtime.relative_times(mdf)
  print(rdf[rdf['relative_time'] < 1.05][['graph', 'Total', 'base_time', 'CutHeuristics', 'FlowMatch', 'name', 'partitions', 'cut', 'base_cut']])
  
  #rdf = rdf[rdf['base_time'] <= 100]
  
  algos = list(rdf.name.unique())
  colors = commons.construct_new_color_mapping(algos)
  if False:
    fig, ax = plt.subplots()
    runtime.xy_plot(ax=ax, rdf=rdf, algos=algos, colors=colors)
    fig.savefig('xy.pdf', pad_inches=0.01, bbox_inches='tight')

  if False:
    fig, ax = plt.subplots()
    runtime.rel_time_plot(ax=ax, rdf=rdf, algos=algos, colors=colors)
    fig.savefig('speedup.pdf', pad_inches=0.01, bbox_inches='tight')
