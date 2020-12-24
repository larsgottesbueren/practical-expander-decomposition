#pragma once

#include <vector>

#include "datastructures/unit_flow.hpp"

namespace Trimming {

/**
   The result of trimming is a vector of removed vertices 'r'.
 */
struct Result {
  std::vector<int> r;
};

class Solver {
private:
  UnitFlow::Graph *graph;
  const double phi;

public:
  /**
     Construct a trimming problem on the subgraph in 'g' induced by 'subset'.
   */
  Solver(UnitFlow::Graph *g, const double phi);

  Result compute();
};
}; // namespace Trimming
