#! /usr/bin/env python3

"""Downloads all graphs used in the experiments to the correct locations.

Query results are sparse matrices. Any matrix which isn't square is removed
since it does not define a graph.

Usage: python3 setyp.py

"""

import ssgetpy
import os

# Maximum results when querying website.
QUERY_LIMIT = 1000
# Max non-zero values in matrix allowed. This roughly corresponds to edges.
NON_ZERO_LIMIT = 200000
# Base location for ssgetpy to place graphs when downloading.
SAVE_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'real.mtx')

def search(group, max_edges):
    results = ssgetpy.search(group=group, nzbounds=(None, max_edges), limit=QUERY_LIMIT)
    return list(filter(lambda r: r.cols == r.rows, results))

groups = [
    "DIMACS10",
    "Hamm",
    "AG-Monien",
    "Nasa",
]

graph_entries = []
for g in groups:
    graph_entries += search(g, NON_ZERO_LIMIT)

print(f'Graphs found: {len(graph_entries)}')

for e in graph_entries:
    e.download(format="MM", destpath=SAVE_PATH, extract=True)
