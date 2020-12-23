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
  UnitFlow::Graph *flowGraph;
  const std::vector<int> &subset;
  const double phi;

public:
  /**
     Construct a trimming problem on the subgraph in 'g' induced by 'subset'.
   */
  Solver(UnitFlow::Graph *g, const std::vector<int> &subset, const double phi);

  Result compute();
};
}; // namespace Trimming
