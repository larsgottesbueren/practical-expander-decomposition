# Expander Decomposition & Clustering (EDC)

This is an implementation of [Expander decomposition and pruning: Faster, stronger, and simpler.](https://arxiv.org/pdf/1812.08958.pdf) by Thatchaphol Saranurak and Di Wang.
Originally developed by [Isaac Arvestad](https://github.com/isaacarvestad).
This repository adds speedup techniques, such as sparse cut heuristics, a dynamic stopping criterion for cut-matching and partial flow routing. 

## Build

``` shell
git submodule --update --init
mkdir release && cd release && cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4
```

## Run

First generate a synthetic toy-graph consisting of 10 cliques of size 100, connected by a path.

``` shell
./experiment/gen_graph.py clique-path -n=100 -k=10 > graph.txt
```

Then run the program on the generated graph with ```phi = 0.001```.

``` shell
./release/EDC graph.txt 0.001
```

Use the ```--log [0-4]``` flag to set the output verbosity (lower = less verbose)
``` shell
./release/EDC --log 1 graph.txt 0.001
```

and use the ```-h``` or ```--help``` flag to display a list of options.
