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

Solver::Solver(std::unique_ptr<Undirected::Graph> graph, double phi,
               std::mt19937 *randomGen, CutMatching::Parameters params)
    : flowGraph(nullptr), subdivisionFlowGraph(nullptr), randomGen(randomGen),
      subdivisionIdx(nullptr), phi(phi), cutMatchingParams(params),
      numPartitions(0), partitionOf(graph->size(), -1) {
  flowGraph = constructFlowGraph(graph);
  subdivisionFlowGraph = constructSubdivisionFlowGraph(graph);

  subdivisionIdx =
      std::make_unique<std::vector<int>>(subdivisionFlowGraph->size(), -1);
  for (int u = flowGraph->size(); u < subdivisionFlowGraph->size(); ++u)
    (*subdivisionIdx)[u] = 0;

  VLOG(1) << "Preparing to run expander decomposition."
          << "\n\tGraph: " << graph->size() << " vertices and "
          << graph->edgeCount() << " edges."
          << "\n\tFlow graph: " << flowGraph->size() << " vertices and "
          << flowGraph->edgeCount() << " edges."
          << "\n\tSubdivision graph: " << subdivisionFlowGraph->size()
          << " vertices and " << subdivisionFlowGraph->edgeCount() << " edges.";

  graph.reset(nullptr);

  compute();
}

void Solver::compute() {
  VLOG(1) << "Attempting to find balanced cut with " << flowGraph->size()
          << " vertices.";
  if (flowGraph->size() == 0) {
    VLOG(1) << "Exiting early, partition was empty.";
    return;
  } else if (flowGraph->size() == 1) {
    VLOG(1) << "Creating single vertex partition.";
    finalizePartition(flowGraph->begin(), flowGraph->end(), 1);
    return;
  }

  Timer timer; timer.Start();
  const auto &components = flowGraph->connectedComponents();
  Timings::GlobalTimings().AddTiming(Timing::ConnectedComponents, timer.Stop());

  if (components.size() > 1) {
    VLOG(1) << "Found " << components.size() << " connected components.";

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

    // TODO call sparse cut heuristics here



    Timer cm_timer; cm_timer.Start();
    CutMatching::Solver cm(flowGraph.get(), subdivisionFlowGraph.get(),
                           randomGen, subdivisionIdx.get(), phi,
                           cutMatchingParams);
    auto result = cm.compute(cutMatchingParams);
    auto cm_dur = cm_timer.Stop();
    std::vector<int> a, r;
    std::copy(flowGraph->cbegin(), flowGraph->cend(), std::back_inserter(a));
    std::copy(flowGraph->cbeginRemoved(), flowGraph->cendRemoved(),
              std::back_inserter(r));

    switch (result.type) {
    case CutMatching::Result::Balanced: {
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
      time_balanced_cut += cm_dur;
      break;
    }
    case CutMatching::Result::NearExpander: {
      assert(!a.empty() && "Near expander should have non-empty A.");
      assert(!r.empty() && "Near expander should have non-empty R.");

      timer.Start();
      Trimming::Solver trimming(flowGraph.get(), phi);
      trimming.compute();
      Timings::GlobalTimings().AddTiming(Timing::FlowTrim, timer.Stop());

      assert(flowGraph->size() > 0 &&
             "Should not trim all vertices from graph.");
      finalizePartition(flowGraph->cbegin(), flowGraph->cend(), 0);

      r.clear();
      std::copy(flowGraph->cbeginRemoved(), flowGraph->cendRemoved(),
                std::back_inserter(r));

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
    case CutMatching::Result::Expander: {
      assert(!a.empty() && "Expander should not be empty graph.");
      assert(r.empty() && "Expander should not remove vertices.");

      flowGraph->restoreRemoves();
      subdivisionFlowGraph->restoreRemoves();

      VLOG(1) << "Finalizing " << a.size() << " vertices as partition "
              << numPartitions << "."
              << " Conductance: " << 1.0 / double(result.congestion) << ".";
      finalizePartition(a.begin(), a.end(), result.congestion);
      time_expander += cm_dur;
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
  std::vector<double> result(numPartitions);

  for (int i = 0; i < numPartitions; ++i)
    if (congestionOf[i] > 0)
      result[i] = 1.0 / double(congestionOf[i]);

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
