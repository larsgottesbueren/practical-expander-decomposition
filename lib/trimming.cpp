#include <glog/logging.h>
#include <glog/stl_logging.h>

#include "absl/container/flat_hash_set.h"
#include "trimming.hpp"

namespace Trimming {

Solver::Solver(UnitFlow::Graph *g, const std::vector<int> &subset,
               const double phi)
    : flowGraph(g), subset(subset), phi(phi) {}

/*
  TODO: consider making 'rSet' and 'result' the same.
  TODO: what to do with partitions without edges? 'std::log(2 * m + 1)' really
  should be 'std::log(2 * m)' but that crashes if subgraph does not have edges.
 */
Result Solver::compute() {
  VLOG(1) << "Trimming partition with " << subset.size() << " vertices.";

  absl::flat_hash_set<int> aSet(subset.begin(), subset.end()), rSet;
  flowGraph->reset(subset.begin(), subset.end());

  for (const auto &u : aSet) {
    for (auto e = flowGraph->beginEdge(u); e != flowGraph->endEdge(u); ++e) {
      if (aSet.find(e->to) == aSet.end())
        flowGraph->addSource(u, (UnitFlow::Flow)ceil(2.0 / phi));
      e->capacity = (UnitFlow::Flow)ceil(2.0 / phi);
    }
    UnitFlow::Flow d = (UnitFlow::Flow)flowGraph->globalDegree(u);
    flowGraph->addSink(u, d);
  }

  const int m = flowGraph->edgeCount();
  const int h = ceil(40 * std::log(2 * m + 1) / phi);

  while (true) {
    const auto hasExcess = flowGraph->compute(h, aSet);
    VLOG(3) << "Found excess of size: " << hasExcess.size();
    if (hasExcess.empty())
      break;

    const auto levelCut = flowGraph->levelCut(h, aSet);
    VLOG(3) << "Found level cut of size: " << levelCut.size();
    if (levelCut.empty())
      break;

    for (const auto &u : levelCut)
      aSet.erase(u), rSet.insert(u);
    for (const auto &u : levelCut)
      for (auto e = flowGraph->beginEdge(u); e != flowGraph->endEdge(u); ++e)
        if (aSet.find(e->to) != aSet.end())
          flowGraph->addSource(e->to, (UnitFlow::Flow)ceil(2.0 / phi));
  }

  Result result;
  result.r = std::vector<int>(rSet.begin(), rSet.end());

  VLOG(1) << "Trimmed " << result.r.size() << " vertices.";

  return result;
}
}; // namespace Trimming
