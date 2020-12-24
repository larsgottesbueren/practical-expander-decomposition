#include <glog/logging.h>
#include <glog/stl_logging.h>

#include "absl/container/flat_hash_set.h"
#include "trimming.hpp"

namespace Trimming {

Solver::Solver(UnitFlow::Graph *g, const double phi) : graph(g), phi(phi) {}

/*
  TODO: consider making 'rSet' and 'result' the same.
  TODO: what to do with partitions without edges? 'std::log(2 * m + 1)' really
  should be 'std::log(2 * m)' but that crashes if subgraph does not have edges.
 */
Result Solver::compute() {
  VLOG(2) << "Trimming partition with " << graph->size() << " vertices.";

  graph->reset();

  for (auto u : *graph) {
    const int removedEdges = graph->globalDegree(u) - graph->degree(u);
    graph->addSource(u, (UnitFlow::Flow)std::ceil(removedEdges * 2.0 / phi));
    for (auto e = graph->beginEdge(u); e != graph->endEdge(u); ++e)
      e->capacity = (UnitFlow::Flow)ceil(2.0 / phi);

    UnitFlow::Flow d = (UnitFlow::Flow)graph->globalDegree(u);
    graph->addSink(u, d);
  }

  const int m = graph->edgeCount();
  const int h = ceil(40 * std::log(2 * m + 1) / phi);

  while (true) {
    const auto hasExcess = graph->compute(h);
    VLOG(3) << "Found excess of size: " << hasExcess.size();
    if (hasExcess.empty())
      break;

    const auto levelCut = graph->levelCut(h);
    VLOG(3) << "Found level cut of size: " << levelCut.size();
    if (levelCut.empty())
      break;

    for (auto u : levelCut)
      graph->remove(u);

    for (auto u : levelCut)
      for (auto e = graph->beginEdge(u); e != graph->endEdge(u); ++e)
        graph->addSource(e->to, (UnitFlow::Flow)ceil(2.0 / phi));
  }

  Result result;
  std::copy(graph->cbeginRemoved(), graph->cendRemoved(),
            std::back_inserter(result.r));

  VLOG(1) << "Trimmed " << result.r.size() << " vertices.";

  return result;
}
}; // namespace Trimming
