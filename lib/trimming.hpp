#pragma once

#include <vector>

#include "datastructures/unit_flow.hpp"

namespace Trimming {

class Solver {
private:
  UnitFlow::Graph *graph;
  const double phi;

public:
  /**
     Construct a trimming problem on the subgraph in 'g' induced by 'subset'.
   */
  Solver(UnitFlow::Graph *g, const double phi);

  void compute();
};
}; // namespace Trimming
