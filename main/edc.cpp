#include <cmath>
#include <cstring>
#include <iostream>
#include <numeric>
#include <vector>
#include <memory>
#include <set>
#include <string>
#include <sstream>

#include <util.hpp>
#include <cut_matching.hpp>
#include <datastructures/undirected_graph.hpp>
#include <expander_decomp.hpp>
#include <fstream>

#include <tlx/cmdline_parser.hpp>

int Logger::LOG_LEVEL = 0;

std::unique_ptr<std::mt19937> configureRandomness(unsigned int seed)
{
    std::mt19937 randomGen(seed);
    return std::make_unique<std::mt19937>(randomGen);
}

std::unique_ptr<Undirected::Graph> readGraph(const std::string& path)
{
    int n, m;
    std::ifstream in(path);
    in >> n >> m;

    std::vector<Undirected::Edge> es;

    std::set<std::pair<int, int>> seen;
    for (int i = 0; i < m; ++i)
    {
        int u, v;
        in >> u >> v;
        if (u > v)
            std::swap(u, v);
        if (seen.find({u, v}) == seen.end())
        {
            seen.insert({u, v});
            es.emplace_back(u, v);
        }
    }

    return std::make_unique<Undirected::Graph>(n, es);
}

int main(int argc, char* argv[])
{
    CutMatching::Parameters params = {
        .tConst = 22, .tFactor = 5.0, .minIterations = 0, .minBalance = 0.45,
        .samplePotential = false,
        .balancedCutStrategy = true,
        .use_cut_heuristics = true,
        .use_potential_based_dynamic_stopping_criterion = true,
        .num_flow_vectors = 20,
        .tune_num_flow_vectors = false,
    };
    double phi = 0.001;
    std::string graph_file;
    int seed = 555;

    tlx::CmdlineParser cp;
    cp.set_description("Expander Decomposition");
    cp.set_author("Isaac Arvestad and Lars Gottesbüren");
    cp.add_int("log", Logger::LOG_LEVEL, "log level");
    cp.add_double("phi", phi, "The conductance value");
    cp.add_string('G', "graph", graph_file, "Path to the graph");
    cp.add_int('S', "seed", seed, "Seed");

    cp.add_bool("sample-potential", params.samplePotential, "Sample potentials [for debugging]");
    cp.add_bool("tune-flow-vectors", params.tune_num_flow_vectors,
        "Tune the number of flow vectors needed for good convergence speed [for debugging]");
    cp.add_bool("use-cut-heuristics", params.use_cut_heuristics, "Try heuristic cut procedures before cut-matching.");
    cp.add_bool("adaptive", params.use_potential_based_dynamic_stopping_criterion,
        "Perform dynamic number of cut-matching rounds based on how well the flow vectors are mixing.");


    if (!cp.process(argc, argv))
    {
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

    // Timings::GlobalTimings().Print();
    // std::cout << "--- Total time " << total_time << " --- " << std::endl;
    // std::cout << "--- Time for balanced cuts " << solver.time_balanced_cut << " time for expanders " << solver.time_expander << " ---" << std::endl;

    // std::cout << "Time pre excess " << solver.subdivisionFlowGraph->pre_excess << " time post excess " << solver.subdivisionFlowGraph->post_excess << std::endl;
    if (params.tune_num_flow_vectors)
    {
        std::cout << "Num flow vectors" << solver.num_flow_vectors_needed << std::endl;
    }

    return 0;

    std::cout << solver.getEdgesCut() << " " << partitions.size() << std::endl;
    for (int i = 0; i < int(partitions.size()); ++i)
    {
        std::cout << partitions[i].size() << " " << conductances[i];
        for (auto p : partitions[i])
            std::cout << " " << p;
        std::cout << std::endl;
    }
}
