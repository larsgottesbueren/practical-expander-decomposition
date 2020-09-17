#include <memory>
#include <numeric>

#include "expander_decomp.hpp"

void ExpanderDecomp::compute(const std::vector<int> &xs, int partition) {
  CutMatching cm(graph.get(), subdivisionFlowGraph.get(), xs, partition, phi);
  auto result = cm.compute();

  switch (result.t) {
  case CutMatching::Balanced: {
    int newPartition = graph->newPartition(result.a, xs);
    compute(result.a, newPartition);
    compute(result.r, partition);
    break;
  }
  case CutMatching::NearExpander: {
    break;
  }
  case CutMatching::Expander: {
    break;
  }
  }
}


ExpanderDecomp::ExpanderDecomp(const PartitionGraph<int, Edge> &g,
                               const double phi)
    : graph(std::make_unique<PartitionGraph<int, Edge>>(g)),
      flowGraph(std::make_unique<UnitFlow>(g.size())),
      subdivisionFlowGraph(std::make_unique<UnitFlow>(g.edgeCount())),
      phi(phi) {
  auto addFlowGraphEdges =
    [&]() {
      assert(false && "TODO: copy g");
    };
  addFlowGraphEdges();
  auto addSubdivisionFlowGraphEdges =
    [&]() {
      assert(false && "TODO: make subdivision graph");
    };
  addSubdivisionFlowGraphEdges();

  std::vector<int> vertices(g.size());
  std::iota(vertices.begin(), vertices.end(), 0);
  compute(vertices, 0);
}

std::vector<std::vector<int>> ExpanderDecomp::getPartition() const {
  std::vector<std::vector<int>> result(graph->partitionCount());
  for (int u = 0; u < graph->size(); ++u)
    result[graph->getPartition(u)].push_back(u);
  return result;
}
