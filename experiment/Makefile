EDC_PATH=../bazel-bin/main/edc
EDC_CUT_PATH=../bazel-bin/main/edc-cut
SEED = 0

.PHONY: clean
.PRECIOUS: gen/%.csv

all: out/t1_vs_t2.pdf out/partitions_and_edges_cut.pdf out/expander_conductivity.pdf out/cut_conductivity.pdf out/iterations_until_cut.pdf

clean:
	rm -f gen/*.csv out/*.pdf

gen_graph.py:

gen/%.csv: scripts/%.py gen_graph.py
	python3 $< $(EDC_PATH) $(EDC_CUT_PATH) $(SEED) gen_graph.py $@

out/%.pdf: scripts/plot/%.r gen/%.csv
	Rscript $^ $@
