#pragma once

#include <vector>

#include "unit_flow.hpp"

namespace Trimming {

/**
   The result of trimming is an expander 'a' and the removed vertices 'r'.
 */
struct Result {
  std::vector<int> a, r;
};

class Solver {
private:
  const UnitFlow::Graph *flowGraph;
  const std::vector<int> &subset;
  const double phi;

public:
  /**
     Construct a trimming problem on the subgraph in 'g' induced by 'subset'.
   */
  Solver(const UnitFlow::Graph *g, const std::vector<int> &subset,
         const double phi);

  Result compute();
};
}; // namespace Trimming
