#include "trimming.hpp"

namespace Trimming {

Solver::Solver(const UnitFlow::Graph *g, const std::vector<int> &subset,
               const double phi)
    : flowGraph(g), subset(subset), phi(phi) {}

Result Solver::compute() { return {}; }
}; // namespace Trimming
