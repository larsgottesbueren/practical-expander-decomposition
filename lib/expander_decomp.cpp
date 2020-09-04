#include <numeric>

#include "expander_decomp.hpp"

void ExpanderDecomp::compute(const std::vector<PartitionGraph::Vertex> &xs,
                             int partition) {
  CutMatching cm(graph, xs, partition, phi);
  auto result = cm.compute();

  switch (result.t) {
  case CutMatching::Balanced: {
    int newPartition = graph.newPartition(result.a, xs);
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

ExpanderDecomp::ExpanderDecomp(PartitionGraph &g, const double phi)
    : graph(std::move(g)), phi(phi) {
  std::vector<PartitionGraph::Vertex> vertices(g.size());
  std::iota(vertices.begin(), vertices.end(), 0);
  compute(vertices, 0);
}

std::vector<std::vector<PartitionGraph::Vertex>>
ExpanderDecomp::getPartition() const {
  std::vector<std::vector<PartitionGraph::Vertex>> result(
      graph.partitionCount());
  for (int u = 0; u < graph.size(); ++u)
    result[graph.getPartition(u)].push_back(u);
  return result;
}
