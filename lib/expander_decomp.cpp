#include <glog/logging.h>
#include <glog/stl_logging.h>
#include <memory>
#include <numeric>

#include "cut_matching.hpp"
#include "expander_decomp.hpp"
#include "trimming.hpp"

namespace ExpanderDecomposition {

std::unique_ptr<UnitFlow::Graph>
constructFlowGraph(const std::unique_ptr<Undirected::Graph> &g) {
  std::vector<UnitFlow::Edge> es;
  for (auto u : *g)
    for (auto e = g->cbeginEdge(u); e != g->cendEdge(u); ++e)
      if (e->from < e->to)
        es.emplace_back(e->from, e->to, 0);

  return std::make_unique<UnitFlow::Graph>(g->size(), es);
}

std::unique_ptr<UnitFlow::Graph>
constructSubdivisionFlowGraph(const std::unique_ptr<Undirected::Graph> &g) {
  std::vector<UnitFlow::Edge> es;
  for (auto u : *g)
    for (auto e = g->cbeginEdge(u); e != g->cendEdge(u); ++e)
      if (e->from < e->to) {
        UnitFlow::Vertex splitVertex = g->size() + int(es.size()) / 2;
        es.emplace_back(e->from, splitVertex, 0);
        es.emplace_back(e->to, splitVertex, 0);
      }
  return std::make_unique<UnitFlow::Graph>(g->size() + int(es.size()) / 2, es);
}

Solver::Solver(std::unique_ptr<Undirected::Graph> graph, const double phi,
               const int tConst, const double tFactor)
    : flowGraph(nullptr), subdivisionFlowGraph(nullptr), phi(phi),
      tConst(tConst), tFactor(tFactor), numPartitions(0),
      partitionOf(graph->size(), -1) {
  flowGraph = constructFlowGraph(graph);
  subdivisionFlowGraph = constructSubdivisionFlowGraph(graph);

  VLOG(1) << "Preparing to run expander decomposition."
          << "\n\tGraph: " << graph->size() << " vertices and "
          << graph->edgeCount() << " edges."
          << "\n\tFlow graph: " << flowGraph->size() << " vertices and "
          << flowGraph->edgeCount() << " edges."
          << "\n\tSubdivision graph: " << subdivisionFlowGraph->size()
          << " vertices and " << subdivisionFlowGraph->edgeCount() << " edges.";

  std::vector<int> vertices(graph->size());
  std::iota(vertices.begin(), vertices.end(), 0);
  compute(vertices);
}

void Solver::compute(const std::vector<int> &xs) {
  VLOG(1) << "Attempting to find balanced cut with " << xs.size()
          << " vertices.";
  if (xs.empty()) {
    VLOG(2) << "Exiting early, partition was empty.";
    return;
  } else if (xs.size() == 1) {
    VLOG(2) << "Creating single vertex partition.";
    finalizePartition(xs.begin(), xs.end());
    return;
  }

  const auto &components = flowGraph->connectedComponents();

  if (components.size() > 1) {
    VLOG(2) << "Found " << components.size() << " connected components.";

    for (auto &comp : components) {
      auto subComp =
          subdivisionFlowGraph->subdivisionVertices(comp.begin(), comp.end());

      flowGraph->subgraph(comp.begin(), comp.end());
      subdivisionFlowGraph->subgraph(subComp.begin(), subComp.end());

      compute(comp);

      flowGraph->restoreSubgraph();
      subdivisionFlowGraph->restoreSubgraph();
    }
  } else {
    CutMatching::Solver cm(flowGraph.get(), subdivisionFlowGraph.get(), xs, phi,
                           tConst, tFactor);
    auto result = cm.compute();

    switch (result.t) {
    case CutMatching::Balanced: {
      assert(!result.a.empty() && "Cut should be balanced but A was empty.");
      assert(!result.r.empty() && "Cut should be balanced but R was empty.");

      auto subA = subdivisionFlowGraph->subdivisionVertices(result.a.begin(),
                                                            result.a.end());
      flowGraph->subgraph(result.a.begin(), result.a.end());
      subdivisionFlowGraph->subgraph(subA.begin(), subA.end());
      compute(result.a);
      flowGraph->restoreSubgraph();
      subdivisionFlowGraph->restoreSubgraph();

      auto subR = subdivisionFlowGraph->subdivisionVertices(result.r.begin(),
                                                            result.r.end());
      flowGraph->subgraph(result.r.begin(), result.r.end());
      subdivisionFlowGraph->subgraph(subR.begin(), subR.end());
      compute(result.r);
      flowGraph->restoreSubgraph();
      subdivisionFlowGraph->restoreSubgraph();
      break;
    }
    case CutMatching::NearExpander: {
      assert(!result.a.empty() && "Near expander should have non-empty A.");
      assert(!result.r.empty() && "Near expander should have non-empty R.");
      Trimming::Solver trimming(flowGraph.get(), result.a, phi);
      const auto trimmingResult = trimming.compute();

      finalizePartition(flowGraph->begin(), flowGraph->end());

      result.r.insert(result.r.end(), trimmingResult.r.begin(),
                      trimmingResult.r.end());
      if (result.r.size() > 0 && result.r.size() < xs.size()) {
        auto subR = subdivisionFlowGraph->subdivisionVertices(result.r.begin(),
                                                              result.r.end());
        flowGraph->subgraph(result.r.begin(), result.r.end());
        subdivisionFlowGraph->subgraph(subR.begin(), subR.end());
        compute(result.r);
        flowGraph->restoreSubgraph();
        subdivisionFlowGraph->restoreSubgraph();
      }
      break;
    }
    case CutMatching::Expander: {
      VLOG(3) << "Finalizing " << xs.size() << " vertices as partition "
              << numPartitions << ".";
      finalizePartition(xs.begin(), xs.end());
      break;
    }
    }
  }
}

std::vector<std::vector<int>> Solver::getPartition() const {
  std::vector<std::vector<int>> result(numPartitions);
  for (auto u : *flowGraph) {
    assert(partitionOf[u] != -1 && "Vertex not part of partition.");
    result[partitionOf[u]].push_back(u);
  }

  return result;
}

std::vector<double> Solver::getConductance() const {
  auto partitions = getPartition();
  std::vector<double> result(int(partitions.size()));

  const int totalVolume = flowGraph->volume();

  for (const auto &p : partitions) {
    flowGraph->subgraph(p.begin(), p.end());

    int pId = partitionOf[p[0]], edgesCut = 0, volume = flowGraph->volume();
    for (int u : *flowGraph)
      edgesCut += flowGraph->globalDegree(u) - flowGraph->degree(u);
    result[pId] = double(edgesCut) / std::min(volume, totalVolume);

    flowGraph->restoreSubgraph();
  }

  return result;
}

int Solver::getEdgesCut() const {
  int count = 0;
  auto partitions = getPartition();
  for (const auto &p : partitions) {
    flowGraph->subgraph(p.begin(), p.end());

    for (auto u : *flowGraph)
      count += flowGraph->globalDegree(u) - flowGraph->degree(u);

    flowGraph->restoreSubgraph();
  }

  return count / 2;
}

} // namespace ExpanderDecomposition
