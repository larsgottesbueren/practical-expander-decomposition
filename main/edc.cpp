#include <cmath>
#include <cstring>
#include <iostream>
#include <memory>
#include <numeric>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <cut_matching.hpp>
#include <datastructures/undirected_graph.hpp>
#include <expander_decomp.hpp>
#include <fstream>
#include <util.hpp>

#include <tlx/cmdline_parser.hpp>

int Logger::LOG_LEVEL = 0;

std::unique_ptr<std::mt19937> configureRandomness(unsigned int seed) {
    std::mt19937 randomGen(seed);
    return std::make_unique<std::mt19937>(randomGen);
}

std::unique_ptr<Undirected::Graph> readGraph(const std::string& path) {
    int n, m;
    std::ifstream in(path);
    in >> n >> m;

    std::vector<Undirected::Edge> es;

    std::set<std::pair<int, int>> seen;
    for (int i = 0; i < m; ++i) {
        int u, v;
        in >> u >> v;
        if (u > v)
            std::swap(u, v);
        if (seen.find({ u, v }) == seen.end()) {
            seen.insert({ u, v });
            es.emplace_back(u, v);
        }
    }

    return std::make_unique<Undirected::Graph>(n, es);
}

int main(int argc, char* argv[]) {
    CutMatching::Parameters params = {
        .tConst = 22,
        .tFactor = 5.0,
        .minIterations = 0,
        .minBalance = 0.45,
        .samplePotential = false,
        .balancedCutStrategy = true,
        .use_cut_heuristics = false,
        .use_potential_based_dynamic_stopping_criterion = false,
        .stop_flow_at_fraction = false,
        .krv_step_first = false,
        .num_flow_vectors = 20,
        .tune_num_flow_vectors = false,
    };
    double phi = 0.001;
    std::string graph_file;
    int seed = 555;

    tlx::CmdlineParser cp;
    cp.set_description("Expander Decomposition");
    // required
    cp.add_param_string("graph", graph_file, "Path to the graph");
    cp.add_param_double("phi", phi, "The conductance value");

    // standard options
    cp.add_int("log", Logger::LOG_LEVEL, "log level");
    cp.add_int('S', "seed", seed, "Seed");
    cp.add_int("flow-vectors", params.num_flow_vectors, "Number of flow vectors to use");

    // our optimizations
    cp.add_bool("use-cut-heuristics", params.use_cut_heuristics, "Try heuristic cut procedures before cut-matching.");
    cp.add_bool("adaptive", params.use_potential_based_dynamic_stopping_criterion,
                "Perform dynamic number of cut-matching rounds based on how well the flow vectors are mixing.");
    cp.add_bool("flow-fraction", params.stop_flow_at_fraction, "Stop flow computation once almost all flow is routed.");
    cp.add_bool("krv-first", params.krv_step_first, "Perform the matching step from KRV instead of RST as long as no cut was made.");
    cp.add_bool("kahan-error", params.kahan_error, "Use Kahan summation to reduce floating point issues.");

    // stuff for debugging
    cp.add_bool("sample-potential", params.samplePotential, "Sample potentials [for debugging]");
    cp.add_bool("tune-flow-vectors", params.tune_num_flow_vectors, "Tune the number of flow vectors needed for good convergence speed [for debugging]");


    if (!cp.process(argc, argv)) {
        std::exit(-1);
    }

    auto g = readGraph(graph_file);

    auto randomGen = configureRandomness(seed);

    Timer timer;
    timer.Start();
    ExpanderDecomposition::Solver solver(std::move(g), phi, randomGen.get(), params);
    auto total_time = timer.Stop();

    auto partitions = solver.getPartition();
    auto conductances = solver.getConductance();

    Timings::GlobalTimings().Print();
    std::cout << "--- Total time " << total_time << " --- " << std::endl;
    std::cout << "--- Time for balanced cuts " << solver.time_balanced_cut << " time for expanders " << solver.time_expander << " ---" << std::endl;

    // std::cout << "Time pre excess " << solver.subdivisionFlowGraph->pre_excess << " time post excess " << solver.subdivisionFlowGraph->post_excess <<
    // std::endl;
    if (params.tune_num_flow_vectors) {
        std::cout << "Num flow vectors" << solver.num_flow_vectors_needed << std::endl;
    }

    std::cout << solver.getEdgesCut() << " " << partitions.size() << std::endl;

#if false
    for (int i = 0; i < int(partitions.size()); ++i) {
        std::cout << partitions[i].size() << " " << conductances[i];
        for (auto p : partitions[i])
            std::cout << " " << p;
        std::cout << std::endl;
    }
#endif
}
