# Reproducing experiments
Make sure you are in the 'experiment' directory and then run:
``` shell
./configure
make
```
The configure script will create directories "gen" and "out" if the do not exist. Next the "make" invocation will generate data in "gen" and place plots in "out".

To remove all plots and intermediate files:
``` shell
make clean
```
