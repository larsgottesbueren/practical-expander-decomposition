#include "trimming.hpp"
#include <util.hpp>

#include <cmath>

namespace Trimming {

    void SaranurakWangTrimming(UnitFlow::Graph* graph, const double phi) {
        VLOG(2) << "Trimming partition with " << graph->size() << " vertices.";

        graph->reset();

        for (auto u : *graph) {
            const int removedEdges =
                    graph->globalDegree(u) -
                    graph->degree(u); // TODO this is not the cut between A and R. especially deeper in the recursion. to test on the top level it suffices...
            graph->addSource(u, (UnitFlow::Flow) std::ceil(removedEdges * 2.0 / phi));
            for (auto e = graph->beginEdge(u); e != graph->endEdge(u); ++e) {
                e->capacity = (UnitFlow::Flow) std::ceil(2.0 / phi);
            }
            UnitFlow::Flow d = (UnitFlow::Flow) graph->globalDegree(u);
            graph->addSink(u, d);
        }

        const int m = graph->edgeCount();
        const int h = ceil(40 * std::log(2 * m + 1) / phi);

        while (true) {
            const bool has_excess_flow = graph->computeFlow(h).second;
            if (!has_excess_flow)
                break;

            const auto [levelCut, _] = graph->levelCut(h);
            VLOG(3) << "Found level cut of size: " << levelCut.size();
            if (levelCut.empty())
                break;

            // NOTE I swapped this with the vertex removal. This is a bug in the baseline implementation.
            for (auto u : levelCut)
                for (auto e = graph->beginEdge(u); e != graph->endEdge(u); ++e)
                    graph->addSource(e->to, (UnitFlow::Flow) std::ceil(2.0 / phi));

            for (auto u : levelCut)
                graph->remove(u);
        }

        VLOG(2) << "After trimming partition has " << graph->size() << " vertices.";
    }

    void FakeEdgeTrimming(UnitFlow::Graph& graph, UnitFlow::Graph& subdiv_graph, std::vector<int>& subdiv_idx, const double phi, int cut_matching_iterations,
                          const std::vector<std::pair<UnitFlow::Vertex, UnitFlow::Vertex>>& fake_matching_edges) {
        const int m = graph.edgeCount();
        const int height = ceil(40 * std::log(2 * m + 1) / phi);
        const int capacity = (UnitFlow::Flow) std::ceil(2.0 / phi);

        graph.restoreRemoves();
        subdiv_graph.restoreRemoves(); // restore removes made during cut-matching


        VLOG(2) << V(m) << V(height) << V(capacity) << V(cut_matching_iterations);
        VLOG(2) << V(graph.size());
        graph.reset();
        int64_t drained = 0, injected = 0;
        for (UnitFlow::Vertex u : graph) {
            graph.addSink(u, graph.degree(u));
            drained += graph.degree(u);
        }
        for (UnitFlow::Vertex u : graph) {
            for (auto& e : graph.edgesOf(u)) {
                e.capacity = capacity;
            }
        }
        VLOG(2) << V(cut_matching_iterations) << V(fake_matching_edges.size());
        for (const auto& [a, b] : fake_matching_edges) {
            // we can put flow on arbitrary endpoint --> map a/b to endpoints of the edge
            UnitFlow::Vertex ea = subdiv_graph.edgesOf(a)[0].to;
            UnitFlow::Vertex eb = subdiv_graph.edgesOf(b)[1].to;
            graph.addSource(ea, cut_matching_iterations);
            graph.addSource(eb, cut_matching_iterations);
            injected += 2 * cut_matching_iterations;
        }
        VLOG(2) << V(injected) << V(drained);

        size_t num_flooded_nodes = 0;
        std::vector<size_t> abs_vals;
        for (auto u : graph) {
            if (capacity * graph.degree(u) < graph.excess(u)) {
                num_flooded_nodes++;
            }
            abs_vals.push_back(graph.absorbed[u]);
        }
        VLOG(2) << V(num_flooded_nodes) << " / " << graph.size();
        std::sort(abs_vals.begin(), abs_vals.end(), std::greater<size_t>());
        std::cout << "abs vals = ";
        for (size_t i = 0; i < std::min<size_t>(20, abs_vals.size()); ++i) {
            std::cout << abs_vals[i] << " ";
        }
        std::cout << std::endl;

#if false
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
        graph.StandardMaxFlow();
        auto cut = graph.MinCut();
        VLOG(2) << V(cut.size()) << V(graph.size());
        for (auto u : cut) {
            graph.remove(u);
        }
#endif
    }


} // namespace Trimming
