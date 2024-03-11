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

            for (auto u : levelCut)
                graph->remove(u);

            // TODO this doesn't do anything...the vertex was just removed, so it has no edges left
            for (auto u : levelCut)
                for (auto e = graph->beginEdge(u); e != graph->endEdge(u); ++e)
                    graph->addSource(e->to, (UnitFlow::Flow) std::ceil(2.0 / phi));
        }

        VLOG(2) << "After trimming partition has " << graph->size() << " vertices.";
    }

    void FakeEdgeTrimming(UnitFlow::Graph& graph, UnitFlow::Graph& subdiv_graph, std::vector<int>& subdiv_idx, const double phi, int cut_matching_iterations,
                          const std::vector<std::pair<UnitFlow::Vertex, UnitFlow::Vertex>>& fake_matching_edges) {
        const int m = graph.edgeCount();
        const int height = ceil(40 * std::log(2 * m + 1) / phi);
        const int capacity = (UnitFlow::Flow) std::ceil(2.0 / phi);

        // TODO can we use the normal graph somehow?
        graph.restoreRemoves(); // restore removes made during cut-matching
        subdiv_graph.restoreRemoves(); // restore removes made during cut-matching

        subdiv_graph.reset(); // have to use subdiv_graph to place flow on fake matching edges
        for (UnitFlow::Vertex u : graph) {
            subdiv_graph.addSink(u, graph.globalDegree(u));
        }
        for (UnitFlow::Vertex u : subdiv_graph) {
            for (auto e = subdiv_graph.beginEdge(u); e != subdiv_graph.endEdge(u); ++e) {
                e->capacity = capacity;
            }
        }
        for (const auto& [a, b] : fake_matching_edges) {
            // TODO is deduplication correct?
            if (!subdiv_graph.isSource(a)) {
                // If T > 2 * 2/phi then the flow already gets congested at the source
                subdiv_graph.addSource(a, cut_matching_iterations);
            }
            if (!subdiv_graph.isSource(b)) {
                subdiv_graph.addSource(b, cut_matching_iterations);
            }
        }

        while (true) {
            const bool has_excess = subdiv_graph.computeFlow(height).second;
            if (!has_excess) {
                break;
            }

            // TODO pick the smaller side to remove...
            const auto [cut, _] = subdiv_graph.levelCut(height);
            if (cut.empty()) {
                break;
            }

            for (auto u : cut) {
                for (auto e = subdiv_graph.beginEdge(u); e != subdiv_graph.endEdge(u); ++e) {
                    subdiv_graph.addSource(e->to, capacity);
                }
            }

            for (auto u : cut) {
                subdiv_graph.remove(u);
                if (subdiv_idx[u] == -1) {
                    graph.remove(u); // regular vertex
                }
            }
        }
    }


} // namespace Trimming
