import pandas as pd
import seaborn as sb
import matplotlib.pyplot as plt
import scipy.stats.mstats
import commons

def reorder(df, sort_field):
  df.sort_values(by=[sort_field + "_fraction"], inplace=True)

def compute_fractions(df, fields, tfield):
  for f in fields:
    df[f + "_fraction"] = df[f] / df[tfield]

def stacked_bars(ax, df, fields, sort_field, tfield="Total"):
  compute_fractions(df, fields, tfield)
  reorder(df, sort_field=sort_field)
  totals = [sum(x) for x in zip(*[df[f] for f in fields])]
  num_instances = len(df)
  x_values = [i for i in range(1, num_instances + 1)]
  colors = commons.construct_new_color_mapping(fields + ["other"])
  prev = [0.0 for x in range(num_instances)]
  for f in fields:
    fractions = [i/j for i,j in zip(df[f], df[tfield])]
    sb.barplot(x=x_values, y=fractions, bottom=prev, label=f, color=colors[f])
    prev = [p + f for p,f in zip(prev, fractions)]

  others = [(at - t)/at for t,at in zip(totals, df[tfield])]
  if any(x > 0 for x in others):
    sb.barplot(x=x_values, y=others, bottom=prev, label="other", color=colors["other"])
  #plt.legend()
  plt.legend(loc='lower right')
  ax.set_xticks([], minor=False)
  ax.set_xlabel('instances')
  ax.set_ylabel('running time share')


