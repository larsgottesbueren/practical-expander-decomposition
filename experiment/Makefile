EDC_PATH=../bazel-bin/main/edc
EDC_CUT_PATH=../bazel-bin/main/edc-cut
SEED = 0

.PHONY: clean
.PRECIOUS: gen/%.csv

all: \
  out/expander_potential.pdf \
  out/cut_potential.pdf \
  out/potential.pdf \
  out/iterations_until_cut.pdf \
  out/cut_balance.pdf \
  out/time_to_edges.pdf \
  out/edge_ratio_to_edges.pdf

clean:
	rm -f gen/*.csv out/*.pdf

gen_graph.py:

gen/bench_walshaw.csv: scripts/bench_walshaw.sh
	./scripts/bench_walshaw.sh > gen/bench_walshaw.csv

gen/%.csv: scripts/%.py gen_graph.py
	python3 $< $(EDC_PATH) $(EDC_CUT_PATH) $(SEED) gen_graph.py $@

out/time_to_edges.pdf: scripts/plot/time_to_edges.r gen/bench_walshaw.csv
	Rscript $^ $@

out/edge_ratio_to_edges.pdf: scripts/plot/edge_ratio_to_edges.r gen/bench_walshaw.csv
	Rscript $^ $@

out/%.pdf: scripts/plot/%.r gen/%.csv
	Rscript $^ $@
