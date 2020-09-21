#include <memory>
#include <numeric>

#include "expander_decomp.hpp"

namespace ExpanderDecomposition {

std::unique_ptr<UnitFlow::Graph>
constructFlowGraph(const std::unique_ptr<Undirected::Graph> &g) {
  auto f = std::make_unique<UnitFlow::Graph>(g->size());

  for (UnitFlow::Vertex u = 0; u < g->size(); ++u)
    for (const auto &e : g->edges(u))
      if (e->from < e->to)
        f->addEdge(e->from, e->to, 0);

  return f;
}

void Solver::compute(const std::vector<int> &xs, int partition) {
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

Solver::Solver(std::unique_ptr<Undirected::Graph> g, const double phi)
    : graph(std::move(g)), flowGraph(nullptr),
      subdivisionFlowGraph(std::make_unique<UnitFlow::Graph>(g->edgeCount())),
      phi(phi) {
  flowGraph = constructFlowGraph(graph);
  auto addSubdivisionFlowGraphEdges = [&]() {
    assert(false && "TODO: make subdivision graph");
  };
  addSubdivisionFlowGraphEdges();

  std::vector<int> vertices(g->size());
  std::iota(vertices.begin(), vertices.end(), 0);
  compute(vertices, 0);
}

std::vector<std::vector<int>> Solver::getPartition() const {
  std::vector<std::vector<int>> result(graph->partitionCount());
  for (int u = 0; u < graph->size(); ++u)
    result[graph->getPartition(u)].push_back(u);
  return result;
}

} // namespace ExpanderDecomposition
