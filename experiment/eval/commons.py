import pandas as pd
import seaborn as sb
import itertools


def construct_new_color_mapping(algos):
	assert(len(algos) <= 10)
	return dict(zip(algos, sb.color_palette()))

def infer_algorithms_from_dataframe(df):
	return list(df.name.unique())

def infer_instances_from_dataframe(df):
	phis = df.phi.unique()
	hgs = df.graph.unique()
	return list(itertools.product(hgs, phis))

def add_threads_to_algorithm_name(df):
	if "threads" in df.columns:
		df["algorithm"] = df["algorithm"] + " " + df["threads"].astype(str)

def add_column_if_missing(df, column, value):
	if not column in df.columns:
		df[column] = [value for i in range(len(df))]
