#include <memory>

#include "cut_matching.hpp"
#include "expander_decomp.hpp"
#include "sparse_cut_heuristics.hpp"
#include "trimming.hpp"

namespace ExpanderDecomposition {

    std::unique_ptr<UnitFlow::Graph> constructFlowGraph(const std::unique_ptr<Undirected::Graph>& g) {
        std::vector<UnitFlow::Edge> es;
        for (auto u : *g)
            for (auto e = g->cbeginEdge(u); e != g->cendEdge(u); ++e)
                if (e->from < e->to)
                    es.emplace_back(e->from, e->to, 0);

        return std::make_unique<UnitFlow::Graph>(g->size(), es);
    }

    std::unique_ptr<UnitFlow::Graph> constructSubdivisionFlowGraph(const std::unique_ptr<Undirected::Graph>& g) {
        std::vector<UnitFlow::Edge> es;
        for (auto u : *g)
            for (auto e = g->cbeginEdge(u); e != g->cendEdge(u); ++e)
                if (e->from < e->to) {
                    UnitFlow::Vertex splitVertex = g->size() + int(es.size()) / 2;
                    es.emplace_back(e->from, splitVertex, 0);
                    es.emplace_back(e->to, splitVertex, 0);
                }
        return std::make_unique<UnitFlow::Graph>(g->size() + int(es.size()) / 2, es);
    }

    Solver::Solver(std::unique_ptr<Undirected::Graph> graph, double phi, std::mt19937* randomGen, CutMatching::Parameters params) :
        flowGraph(nullptr), subdivisionFlowGraph(nullptr), randomGen(randomGen), subdivisionIdx(nullptr), phi(phi), cutMatchingParams(params), numPartitions(0),
        partitionOf(graph->size(), -1) {
        flowGraph = constructFlowGraph(graph);
        subdivisionFlowGraph = constructSubdivisionFlowGraph(graph);

        subdivisionIdx = std::make_unique<std::vector<int>>(subdivisionFlowGraph->size(), -1);
        for (int u = flowGraph->size(); u < subdivisionFlowGraph->size(); ++u)
            (*subdivisionIdx)[u] = 0;

        VLOG(1) << "Preparing to run expander decomposition."
                << "\n\tGraph: " << graph->size() << " vertices and " << graph->edgeCount() << " edges."
                << "\n\tFlow graph: " << flowGraph->size() << " vertices and " << flowGraph->edgeCount() << " edges."
                << "\n\tSubdivision graph: " << subdivisionFlowGraph->size() << " vertices and " << subdivisionFlowGraph->edgeCount() << " edges.";

        graph.reset(nullptr);

        sparse_cut_heuristics.Allocate(*flowGraph);
    }

    void Solver::compute(int depth) {
        if (flowGraph->size() == 0) {
            VLOG(2) << "Exiting early, partition was empty.";
            return;
        } else if (flowGraph->size() == 1) {
            VLOG(2) << "Creating single vertex partition.";
            finalizePartition(flowGraph->begin(), flowGraph->end(), 1);
            return;
        }

        Timer timer;
        timer.Start();
        const auto& components = flowGraph->connectedComponents();
        Timings::GlobalTimings().AddTiming(Timing::ConnectedComponents, timer.Stop());

        if (components.size() > 1) {
            VLOG(2) << "Found " << components.size() << " connected components.";

            for (auto& comp : components) {
                auto subComp = subdivisionFlowGraph->subdivisionVertices(comp.begin(), comp.end());

                flowGraph->subgraph(comp.begin(), comp.end());
                subdivisionFlowGraph->subgraph(subComp.begin(), subComp.end());

                compute(depth + 1);

                flowGraph->restoreSubgraph();
                subdivisionFlowGraph->restoreSubgraph();
            }
        } else {
            Timer cut_timer;
            size_t cut_matching_rounds = cutMatchingParams.tConst + ceil(cutMatchingParams.tFactor * square(std::log10(flowGraph->globalVolume())));
            double vol_balance_lb = flowGraph->globalVolume() / 10.0 / cut_matching_rounds;
            bool heuristic_sparse_cut_found = false;
            if (cutMatchingParams.use_cut_heuristics) {
                cut_timer.Start();
                double conductance_goal = phi * square(std::log10(flowGraph->globalVolume())) * 1.7;
                heuristic_sparse_cut_found =
                        sparse_cut_heuristics.Compute(*flowGraph, conductance_goal, vol_balance_lb, cutMatchingParams.use_balanced_partitions);
                Timings::GlobalTimings().AddTiming(Timing::CutHeuristics, cut_timer.Stop());
            }

            CutMatching::Result cut_matching_result;
            Duration cm_dur(0.0);
            std::vector<int> a, r;
            bool restore_removes = true;

            if (heuristic_sparse_cut_found) {
                cut_matching_result.type = CutMatching::Result::Balanced;
                std::tie(a, r) = sparse_cut_heuristics.ExtractCutSides(*flowGraph);
                restore_removes = false;
            } else {
                Timer cm_timer;
                cm_timer.Start();
                CutMatching::Solver cm(flowGraph.get(), subdivisionFlowGraph.get(), randomGen, subdivisionIdx.get(), phi, cutMatchingParams);
                cut_matching_result = cm.compute(cutMatchingParams);

                if (cutMatchingParams.tune_num_flow_vectors) {
                    num_flow_vectors_needed = std::max(num_flow_vectors_needed, cut_matching_result.num_flow_vectors_needed);
                }

                std::copy(flowGraph->cbegin(), flowGraph->cend(), std::back_inserter(a));
                std::copy(flowGraph->cbeginRemoved(), flowGraph->cendRemoved(), std::back_inserter(r));
                cm_dur = cm_timer.Stop();
            }

            switch (cut_matching_result.type) {
                case CutMatching::Result::Balanced: {
                    assert(!a.empty() && "Cut should be balanced but A was empty.");
                    assert(!r.empty() && "Cut should be balanced but R was empty.");
                    if (restore_removes) {
                        flowGraph->restoreRemoves();
                        subdivisionFlowGraph->restoreRemoves();
                    }
                    auto subA = subdivisionFlowGraph->subdivisionVertices(a.begin(), a.end());
                    auto subR = subdivisionFlowGraph->subdivisionVertices(r.begin(), r.end());

                    flowGraph->subgraph(a.begin(), a.end());
                    subdivisionFlowGraph->subgraph(subA.begin(), subA.end());
                    compute(depth + 1);
                    flowGraph->restoreSubgraph();
                    subdivisionFlowGraph->restoreSubgraph();

                    flowGraph->subgraph(r.begin(), r.end());
                    subdivisionFlowGraph->subgraph(subR.begin(), subR.end());
                    compute(depth + 1);
                    flowGraph->restoreSubgraph();
                    subdivisionFlowGraph->restoreSubgraph();
                    time_balanced_cut += cm_dur;
                    break;
                }
                case CutMatching::Result::NearExpander: {
                    assert(!a.empty() && "Near expander should have non-empty A.");
                    assert(!r.empty() && "Near expander should have non-empty R.");

                    timer.Start();
                    Trimming::SaranurakWangTrimming(flowGraph.get(), phi, cutMatchingParams.trim_with_max_flow_first);
                    Timings::GlobalTimings().AddTiming(Timing::FlowTrim, timer.Stop());

                    assert(flowGraph->size() > 0 && "Should not trim all vertices from graph.");
                    finalizePartition(flowGraph->cbegin(), flowGraph->cend(), 0);

                    r.clear();
                    std::copy(flowGraph->cbeginRemoved(), flowGraph->cendRemoved(), std::back_inserter(r));

                    flowGraph->restoreRemoves();
                    subdivisionFlowGraph->restoreRemoves();

                    auto subR = subdivisionFlowGraph->subdivisionVertices(r.begin(), r.end());
                    flowGraph->subgraph(r.begin(), r.end());
                    subdivisionFlowGraph->subgraph(subR.begin(), subR.end());
                    compute(depth + 1);
                    flowGraph->restoreSubgraph();
                    subdivisionFlowGraph->restoreSubgraph();
                    break;
                }
                case CutMatching::Result::NearExpanderFakeEdges: {
                    timer.Start();
                    Trimming::FakeEdgeTrimming(*flowGraph, *subdivisionFlowGraph, *subdivisionIdx, phi, cut_matching_result.iterations,
                                               cut_matching_result.fake_matching_edges, *randomGen);
                    Timings::GlobalTimings().AddTiming(Timing::FlowTrim, timer.Stop());

                    assert(flowGraph->size() > 0 && "Should not trim all vertices from graph.");
                    finalizePartition(flowGraph->cbegin(), flowGraph->cend(), 0);

                    r.clear();
                    std::copy(flowGraph->cbeginRemoved(), flowGraph->cendRemoved(), std::back_inserter(r));

                    flowGraph->restoreRemoves();
                    // subdivisionFlowGraph->restoreRemoves(); not needed

                    auto subR = subdivisionFlowGraph->subdivisionVertices(r.begin(), r.end());
                    flowGraph->subgraph(r.begin(), r.end());
                    subdivisionFlowGraph->subgraph(subR.begin(), subR.end());
                    compute(depth + 1);
                    flowGraph->restoreSubgraph();
                    subdivisionFlowGraph->restoreSubgraph();
                    break;
                }
                case CutMatching::Result::Expander: {
                    assert(!a.empty() && "Expander should not be empty graph.");
                    assert(r.empty() && "Expander should not remove vertices.");

                    flowGraph->restoreRemoves();
                    subdivisionFlowGraph->restoreRemoves();

                    VLOG(1) << "Finalizing " << a.size() << " vertices as partition " << numPartitions << "."
                            << " Conductance: " << 1.0 / double(cut_matching_result.congestion) << ".";
                    finalizePartition(a.begin(), a.end(), cut_matching_result.congestion);
                    time_expander += cm_dur;
                    break;
                }
            }
        }
    }

    std::vector<std::vector<int>> Solver::getPartition() const {
        std::vector<std::vector<int>> result(numPartitions);
        std::vector<int> tmp;
        for (auto u : *flowGraph) {
            tmp.push_back(u);
            assert(partitionOf[u] != -1 && "Vertex not part of partition.");
            result[partitionOf[u]].push_back(u);
        }

        return result;
    }

    std::vector<double> Solver::getConductance() const {
        std::vector<double> result(numPartitions);

        for (int i = 0; i < numPartitions; ++i)
            if (congestionOf[i] > 0)
                result[i] = 1.0 / double(congestionOf[i]);

        return result;
    }

    int Solver::getEdgesCut() const {
        int count = 0;
        for (auto u : *flowGraph) {
            for (auto e = flowGraph->cbeginEdge(u); e != flowGraph->cendEdge(u); ++e) {
                count += static_cast<int>(partitionOf[u] != partitionOf[e->to]);
            }
        }
        return count / 2;
    }

} // namespace ExpanderDecomposition
