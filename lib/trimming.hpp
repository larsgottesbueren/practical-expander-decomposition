#pragma once

#include <vector>

#include "datastructures/unit_flow.hpp"

namespace Trimming {

    /**
           Construct a trimming problem on the subgraph in 'g' induced by 'subset'.
         */
    void SaranurakWangTrimming(UnitFlow::Graph* graph, const double phi);

    void FakeEdgeTrimming(UnitFlow::Graph* graph, const double phi);


} // namespace Trimming
