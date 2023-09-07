# Expander Decomposition & Clustering (EDC)

This is an implementation of [Expander decomposition and pruning: Faster,
stronger, and simpler.](https://arxiv.org/pdf/1812.08958.pdf) by Thatchaphol
Saranurak and Di Wang.

## Build

``` shell
mkdir release && cd release && cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4
```

## Run

First generate a synthetic toy-graph consisting of 10 cliques of size 100, connected by a path.

``` shell
./experiment/gen_graph.py clique-path -n=100 -k=10 > graph.txt
```

Then run the program on the generated graph.

``` shell
./release/EDC < graph.txt
```

Use the LOG_LEVEL environment variable (0-4) to set the output verbosity
``` shell
export LOG_LEVEL=1
```

or 

``` shell
LOG_LEVEL=1 ./release/EDC < graph.txt
```
