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
  auto f =
      std::make_unique<UnitFlow::Graph>(g->size() + int(es.size()) / 2, es);
  for (int u = g->size(); u < f->size(); ++u)
    f->setSubdivision(u);
  return f;
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

  compute();
}

void Solver::compute() {
  VLOG(1) << "Attempting to find balanced cut with " << flowGraph->size()
          << " vertices.";
  if (flowGraph->size() == 0) {
    VLOG(2) << "Exiting early, partition was empty.";
    return;
  } else if (flowGraph->size() == 1) {
    VLOG(2) << "Creating single vertex partition.";
    finalizePartition(flowGraph->begin(), flowGraph->end());
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

      compute();

      flowGraph->restoreSubgraph();
      subdivisionFlowGraph->restoreSubgraph();
    }
  } else {
    CutMatching::Solver cm(flowGraph.get(), subdivisionFlowGraph.get(), phi,
                           tConst, tFactor);
    auto resultType = cm.compute();
    std::vector<int> a,r;
    std::copy(flowGraph->cbegin(), flowGraph->cend(), std::back_inserter(a));
    std::copy(flowGraph->cbeginRemoved(), flowGraph->cendRemoved(), std::back_inserter(r));

    switch (resultType) {
    case CutMatching::Balanced: {
      assert(!a.empty() && "Cut should be balanced but A was empty.");
      assert(!r.empty() && "Cut should be balanced but R was empty.");

      flowGraph->restoreRemoves();
      subdivisionFlowGraph->restoreRemoves();

      auto subA = subdivisionFlowGraph->subdivisionVertices(a.begin(), a.end());
      auto subR = subdivisionFlowGraph->subdivisionVertices(r.begin(), r.end());

      flowGraph->subgraph(a.begin(), a.end());
      subdivisionFlowGraph->subgraph(subA.begin(), subA.end());
      compute();
      flowGraph->restoreSubgraph();
      subdivisionFlowGraph->restoreSubgraph();

      flowGraph->subgraph(r.begin(), r.end());
      subdivisionFlowGraph->subgraph(subR.begin(), subR.end());
      compute();
      flowGraph->restoreSubgraph();
      subdivisionFlowGraph->restoreSubgraph();
      break;
    }
    case CutMatching::NearExpander: {
      assert(!a.empty() && "Near expander should have non-empty A.");
      assert(!r.empty() && "Near expander should have non-empty R.");

      Trimming::Solver trimming(flowGraph.get(), phi);
      trimming.compute();

      assert(flowGraph->size() > 0 && "Should not trim all vertices from graph.");
      finalizePartition(flowGraph->cbegin(), flowGraph->cend());

      r.clear();
      std::copy(flowGraph->cbeginRemoved(), flowGraph->cendRemoved(), std::back_inserter(r));

      flowGraph->restoreRemoves();
      subdivisionFlowGraph->restoreRemoves();

      auto subR = subdivisionFlowGraph->subdivisionVertices(r.begin(), r.end());
      flowGraph->subgraph(r.begin(), r.end());
      subdivisionFlowGraph->subgraph(subR.begin(), subR.end());
      compute();
      flowGraph->restoreSubgraph();
      subdivisionFlowGraph->restoreSubgraph();
      break;
    }
    case CutMatching::Expander: {
      assert(!a.empty() && "Expander should not be empty graph.");
      assert(r.empty() && "Expander should not remove vertices.");

      flowGraph->restoreRemoves();
      subdivisionFlowGraph->restoreRemoves();

      VLOG(3) << "Finalizing " << a.size() << " vertices as partition "
              << numPartitions << ".";
      finalizePartition(a.begin(), a.end());
      break;
    }
    }
  }
}

std::vector<std::vector<int>> Solver::getPartition() const {
  std::vector<std::vector<int>> result(numPartitions);
  std::vector<int> tmp;
  for (auto u : *flowGraph) {
    tmp.push_back(u);
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
    assert(!p.empty() && "Partitions should not be empty.");
    flowGraph->subgraph(p.begin(), p.end());

    int pId = partitionOf[p[0]], edgesCut = 0, volume = flowGraph->globalVolume();
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
    assert(!p.empty() && "Partitions should not be empty.");
    flowGraph->subgraph(p.begin(), p.end());

    for (auto u : *flowGraph)
      count += flowGraph->globalDegree(u) - flowGraph->degree(u);

    flowGraph->restoreSubgraph();
  }

  return count / 2;
}

} // namespace ExpanderDecomposition
