#include "trimming.hpp"
#include <util.hpp>

#include <cmath>

namespace Trimming {

    void SaranurakWangTrimming(UnitFlow::Graph* graph, const double phi, bool trim_with_max_flow_first) {
        VLOG(2) << "Trimming partition with " << graph->size() << " vertices.";

        graph->reset();
        UnitFlow::Flow injected = 0;
        for (auto u : *graph) {
            const int removedEdges = graph->removedEdges(u);
            UnitFlow::Flow supply = std::ceil(removedEdges * 2.0 / phi);
            injected += supply;
            graph->addSource(u, supply);
            for (auto e = graph->beginEdge(u); e != graph->endEdge(u); ++e) {
                e->capacity = (UnitFlow::Flow) std::ceil(2.0 / phi);
            }
            UnitFlow::Flow d = (UnitFlow::Flow) graph->globalDegree(u);
            graph->addSink(u, d);
        }

        const int m = graph->edgeCount();
        const int h = ceil(40 * std::log(2 * m + 1) / phi);

        bool run_unit_flow = true;
        if (trim_with_max_flow_first) {
            size_t work_bound = size_t(m) * h * 500;
            work_bound = 0; // temporary to test
            auto flow = graph->StandardMaxFlow(work_bound);
            if (flow != -1) {
                run_unit_flow = false;
                if (flow < injected) {
                    auto cut = graph->MinCut();
                    VLOG(2) << V(cut.size()) << V(flow) << V(graph->size());
                    for (auto u : cut) {
                        graph->remove(u);
                    }
                } else {
                    VLOG(2) << "Trimming flow fully routed";
                }
            } else {
                VLOG(2) << "Canceled standard max flow --> transition to unit flow";
            }
        }

        while (run_unit_flow) {
            const bool has_excess_flow = graph->computeFlow(h).second;
            if (!has_excess_flow) {
                break;
            }

            const auto [levelCut, _] = graph->levelCut(h);
            VLOG(3) << "Found level cut of size: " << levelCut.size();
            if (levelCut.empty()) {
                break;
            }
            // NOTE I swapped this with the vertex removal. This is a bug in the baseline implementation.
            for (auto u : levelCut) {
                for (auto e = graph->beginEdge(u); e != graph->endEdge(u); ++e) {
                    graph->addSource(e->to, (UnitFlow::Flow) std::ceil(2.0 / phi));
                }
            }

            for (auto u : levelCut) {
                graph->remove(u);
            }
        }

        VLOG(2) << "After trimming partition has " << graph->size() << " vertices.";
    }

    void FakeEdgeTrimming(UnitFlow::Graph& graph, UnitFlow::Graph& subdiv_graph, std::vector<int>& subdiv_idx, const double phi, int cut_matching_iterations,
                          const std::vector<std::pair<UnitFlow::Vertex, UnitFlow::Vertex>>& fake_matching_edges) {

        const int capacity = (UnitFlow::Flow) std::ceil(2.0 / phi);
        graph.restoreRemoves();
        subdiv_graph.restoreRemoves(); // restore removes made during cut-matching

        graph.reset();
        int64_t drained = 0, injected = 0;
        for (UnitFlow::Vertex u : graph) {
            graph.addSink(u, graph.globalDegree(u));
            drained += graph.globalDegree(u);
        }
        for (UnitFlow::Vertex u : graph) {
            for (auto& e : graph.edgesOf(u)) {
                e.capacity = capacity;
            }
        }

        for (const auto& [a, b] : fake_matching_edges) {
            // we can put flow on arbitrary endpoint --> map a/b to endpoints of the edge
            UnitFlow::Vertex ea = subdiv_graph.edgesOf(a)[0].to;
            UnitFlow::Vertex eb = subdiv_graph.edgesOf(b)[1].to;
            graph.addSource(ea, cut_matching_iterations);
            graph.addSource(eb, cut_matching_iterations);
            injected += 2 * cut_matching_iterations;
        }

#if false
        const int m = graph.edgeCount();
        const int height = ceil(40 * std::log(2 * m + 1) / phi);
        while (true) {
            const bool has_excess = graph.computeFlow(height).second;
            if (!has_excess) {
                break;
            }

            // TODO pick the smaller side to remove... --> go for mincut.
            const auto [cut, _] = graph.levelCut(height);
            VLOG(2) << V(cut.size());
            if (cut.empty()) {
                break;
            }

            for (auto u : cut) {
                for (auto& e : graph.edgesOf(u)) {
                    graph.addSource(e.to, cut_matching_iterations);
                }
            }

            for (auto u : cut) {
                graph.remove(u);
            }
        }
#else
        size_t excess = 0;
        for (auto u : graph) {
            excess += graph.excess(u);
        }

        VLOG(2) << V(injected) << V(drained) << V(excess);
        auto flow = graph.StandardMaxFlow();
        // auto flow = graph.Dinitz(std::numeric_limits<int>::max());
        auto cut = graph.MinCut();
        VLOG(2) << V(flow) << V(cut.size()) << V(graph.size());
        for (auto u : cut) {
            graph.remove(u);
        }
#endif
    }


} // namespace Trimming
