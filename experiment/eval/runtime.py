import pandas as pd
import seaborn as sb
import matplotlib.pyplot as plt
import scipy.stats.mstats

keys = ['graph', 'name']

def relative_times(df, field = 'Total', baseline='Arv'):
  baseline_df = df[df.name == baseline].copy()
  baseline_df.set_index(keys, inplace=True)
  remaining_df = df[df.name != baseline].copy()

  def base_time(row):
    baseline_key = tuple([row[key] for key in ['graph']])
    return baseline_df.loc[baseline_key][field]

  def base_cut(row):
    baseline_key = tuple([row[key] for key in ['graph']])
    return baseline_df.loc[baseline_key]['cut']
  
  remaining_df['base_time'] = remaining_df.apply(base_time, axis='columns')
  remaining_df['base_cut'] = remaining_df.apply(base_cut, axis='columns')
  #remaining_df['relative_time'] = remaining_df[field] / remaining_df['base_time']
  remaining_df['relative_time'] = remaining_df['base_time'] / remaining_df[field]
  return remaining_df

def xy_plot(ax, rdf, algos, colors):
  sb.scatterplot(data=rdf[rdf['base_time'] <= 100], y='Total', x='base_time', ax=ax, 
        hue='name', color=colors, hue_order=algos, alpha=0.8, edgecolor='none', legend=True, s=7
    )
  max_val = rdf[rdf['base_time'] <= 100]['base_time'].max()
  ax.plot([0,max_val], [0,max_val], color='black')
  ax.set_xlabel('Arv (baseline) time [s]')
  ax.set_ylabel('Algo time [s]')

def rel_time_plot(ax, rdf, algos, colors):
  rdf.sort_values(by=["relative_time"], inplace=True)
  for algo in algos:
    sb.scatterplot(y=rdf[rdf.name == algo]['relative_time'], x=range(len(rdf.graph.unique())), ax=ax, 
          color=colors[algo], label=algo
          #hue='name',
          #linewidth=lw,
          #legend=True,
    )
  ax.set_xlabel('\#instances')
  ax.set_ylabel('speedup over base version')

  