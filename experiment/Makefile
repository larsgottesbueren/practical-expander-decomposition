EDC_PATH=../bazel-bin/main/edc
EDC_CUT_PATH=../bazel-bin/main/edc-cut
SEED = 1

.PHONY: clean
.PRECIOUS: gen/%.csv

all: \
  out/iterations_until_expander_default.pdf \
  out/iterations_until_expander_balanced.pdf \
  out/expander_potential.pdf \
  out/cut_potential.pdf \
  out/cut_balance.pdf \
  out/cut_balance_with_heuristic.pdf \
  out/real_cut_balance.pdf \
  out/real_cut_conductance.pdf \
  out/real_cut_conductance_large_phi.pdf \
  out/real_cut_iterations.pdf \
  out/real_cut_time.pdf \
  out/expected_edges_cut.pdf \
  out/time_to_edges.pdf \
  out/edge_ratio_to_edges.pdf \
  out/certificate.pdf

clean:
	rm -f gen/*.csv out/*.pdf

gen_graph.py:

gen/edc_header.csv: scripts/edc_header.sh
	./scripts/edc_header.sh > gen/edc_header.csv

gen/edc_real.csv: scripts/edc_real.sh
	./scripts/edc_real.sh > gen/edc_real.csv

gen/edc.csv: gen/edc_header.csv gen/edc_real.csv
	cat $^ > $@

gen/cut_header.csv: scripts/cut_header.sh
	./scripts/cut_header.sh > gen/cut_header.csv

gen/cut_real.csv: scripts/cut_real.sh
	./scripts/cut_real.sh > gen/cut_real.csv

gen/cut.csv: gen/cut_header.csv gen/cut_real.csv
	cat $^ > $@

gen/%.csv: scripts/%.py gen_graph.py
	python3 $< $(EDC_PATH) $(EDC_CUT_PATH) $(SEED) gen_graph.py $@

out/iterations_until_expander_default.pdf: \
  scripts/plot/iterations_until_expander_default.r \
  gen/iterations_until_expander.csv
	Rscript $^ $@

out/iterations_until_expander_balanced.pdf: \
  scripts/plot/iterations_until_expander_balanced.r \
  gen/iterations_until_expander.csv
	Rscript $^ $@

out/cut_balance.pdf: \
  scripts/plot/cut_balance.r \
  gen/cut.csv
	Rscript $^ $@

out/cut_balance_with_heuristic.pdf: \
  scripts/plot/cut_balance_with_heuristic.r \
  gen/cut.csv
	Rscript $^ $@

out/time_to_edges.pdf: scripts/plot/time_to_edges.r gen/edc.csv
	Rscript $^ $@

out/edge_ratio_to_edges.pdf: scripts/plot/edge_ratio_to_edges.r gen/edc.csv
	Rscript $^ $@

out/real_cut_conductance.pdf: scripts/plot/real_cut_conductance.r gen/cut.csv
	Rscript $^ $@

out/real_cut_conductance_large_phi.pdf: scripts/plot/real_cut_conductance_large_phi.r gen/cut.csv
	Rscript $^ $@

out/real_cut_balance.pdf: scripts/plot/real_cut_balance.r gen/cut.csv
	Rscript $^ $@

out/real_cut_time.pdf: scripts/plot/real_cut_time.r gen/cut.csv
	Rscript $^ $@

out/real_cut_iterations.pdf: scripts/plot/real_cut_iterations.r gen/cut.csv
	Rscript $^ $@

out/%.pdf: scripts/plot/%.r gen/%.csv
	Rscript $^ $@
