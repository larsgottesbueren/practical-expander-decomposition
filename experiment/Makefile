EDC_PATH=../bazel-bin/main/edc
EDC_CUT_PATH=../bazel-bin/main/edc-cut
SEED = 0

.PHONY: clean
.PRECIOUS: gen/%.csv

all: out/expander_potential.pdf out/cut_potential.pdf out/potential.pdf out/iterations_until_cut.pdf out/cut_balance.pdf out/bench_walshaw.csv

clean:
	rm -f gen/*.csv out/*.pdf

gen_graph.py:

gen/%.csv: scripts/%.py gen_graph.py
	python3 $< $(EDC_PATH) $(EDC_CUT_PATH) $(SEED) gen_graph.py $@

out/%.pdf: scripts/plot/%.r gen/%.csv
	Rscript $^ $@

out/%.csv: scripts/%.sh
	bash $^ > $@
