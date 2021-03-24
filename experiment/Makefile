EDC_PATH=../bazel-bin/main/edc
EDC_CUT_PATH=../bazel-bin/main/edc-cut
SEED = 0

.PHONY: clean
.PRECIOUS: gen/%.csv

all: \
  out/iterations_until_cut.pdf \
  out/iterations_until_cut_final.pdf \
  out/iterations_until_cut_random_walk.pdf \
  out/expander_potential.pdf \
  out/cut_potential.pdf \
  out/cut_balance.pdf \
  out/expected_edges_cut.pdf \
  out/certificate.pdf \
  out/time_to_edges.pdf \
  out/edge_ratio_to_edges.pdf

clean:
	rm -f gen/*.csv out/*.pdf

gen_graph.py:

gen/bench_header.csv: scripts/bench_header.sh
	./scripts/bench_header.sh > gen/bench_header.csv

gen/bench_walshaw.csv: scripts/bench_walshaw.sh
	./scripts/bench_walshaw.sh > gen/bench_walshaw.csv

gen/bench_snap.csv: scripts/bench_snap.sh
	./scripts/bench_snap.sh > gen/bench_snap.csv

gen/bench_margulis.csv: scripts/bench_margulis.sh
	./scripts/bench_margulis.sh > gen/bench_margulis.csv

gen/bench.csv: \
  gen/bench_header.csv \
  gen/bench_walshaw.csv \
  gen/bench_snap.csv \
  gen/bench_margulis.csv
	cat $^ > $@

gen/%.csv: scripts/%.py gen_graph.py
	python3 $< $(EDC_PATH) $(EDC_CUT_PATH) $(SEED) gen_graph.py $@

out/time_to_edges.pdf: scripts/plot/time_to_edges.r gen/bench.csv
	Rscript $^ $@

out/edge_ratio_to_edges.pdf: scripts/plot/edge_ratio_to_edges.r gen/bench.csv
	Rscript $^ $@

out/%.pdf: scripts/plot/%.r gen/%.csv
	Rscript $^ $@
