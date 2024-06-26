#pragma once

#include <random>
#include <vector>

#include "datastructures/unit_flow.hpp"


namespace Trimming {

    /**
           Construct a trimming problem on the subgraph in 'g' induced by 'subset'.
         */
    void SaranurakWangTrimming(UnitFlow::Graph* graph, const double phi, bool trim_with_max_flow_first);

    void FakeEdgeTrimming(UnitFlow::Graph& graph, UnitFlow::Graph& subdiv_graph, std::vector<int>& subdiv_idx, double phi, int cut_matching_iterations,
                          const std::vector<std::pair<UnitFlow::Vertex, UnitFlow::Vertex>>& fake_matching_edges, std::mt19937& prng);


} // namespace Trimming
