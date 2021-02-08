#pragma once

#include <vector>

#include "datastructures/undirected_graph.hpp"
#include "datastructures/unit_flow.hpp"

namespace ExpanderDecomposition {

/**
   Construct a flow graph equivalent to 'g' with all edge capacities set to 0.
 */
std::unique_ptr<UnitFlow::Graph>
constructFlowGraph(const std::unique_ptr<Undirected::Graph> &g);

/**
   Construct a subdivision flow graph from 'g' with all edge capacities set to
   0.
 */
std::unique_ptr<UnitFlow::Graph>
constructSubdivisionFlowGraph(const std::unique_ptr<Undirected::Graph> &g);

/**
   Constructs and solves a expander decomposition problem.
 */
class Solver {
private:
  /**
     Two flow graphs are maintained. Let 'graph = (V,E)'. Then '{e.id + |V| | e
     \in E}' is the vertex ids of the split vertices in 'subdivisionFlowGraph'.
   */
  std::unique_ptr<UnitFlow::Graph> flowGraph, subdivisionFlowGraph;

  /**
     Map to and from subdivision vertices.

     Value associated with each vertex such that 'subdivision[u] == -1' if 'u'
     is not a subdivision vertex and 'subdivision[u] >= 0' if 'u' is a
     subdivision vertex.
   */
  std::unique_ptr<std::vector<int>> subdivisionIdx, fromSubdivisionIdx;

  const double phi;
  /**
      In Sanurak and Wang, the parameter 'T' is bounded by '\Theta(\log^2 m)'.
      This is a constant factor which is added.
   */
  const int tConst;
  /**
      In Sanurak and Wang, the parameter 'T' is bounded by '\Theta(\log^2 m)'.
      This is a constant factor such that 'T = tConst + tFactor (\log^2 m)'.
   */
  const double tFactor;

  /**
     Number of steps of the random walk in cut-matching game.
   */
  const int randomWalkSteps;

  /**
     Minimum volume balance before cut-matching game should terminate.
   */
  const double minBalance;

  /**
     Number of times expansion certificate should be sampled when cut-matching
     game results in an expander.
   */
  const int verifyExpansion;

  /**
     Number of finalized partitions.
   */
  int numPartitions;

  /**
     Vector of indices such that 'partitionOf[u]' is the partition index of
     vertex 'u'.
   */
  std::vector<int> partitionOf;

  /**
     Compute expander decomposition for subset of vertices 'xs'.
   */
  void compute();

  /**
     Create a partition with the given vertices.
   */
  template <typename It> void finalizePartition(It begin, It end) {
    for (auto it = begin; it != end; ++it)
      partitionOf[*it] = numPartitions;
    numPartitions++;
  }

public:
  /**
     Create a decomposition problem with n vertices.
   */
  Solver(std::unique_ptr<Undirected::Graph> g, double phi, int tConst,
         double tFactor, int randomWalkSteps, double minBalance,
         int verifyExpansion);

  /**
     Return the computed partition as a vector of disjoint vertex vectors.
   */
  std::vector<std::vector<int>> getPartition() const;

  /**
     Compute the conductance of each partition relative the rest of the graph.
   */
  std::vector<double> getConductance() const;

  /**
     Return the number of edges which are cut. An edge is cut if it's two
     endpoints are in separate partitions.
   */
  int getEdgesCut() const;
};
} // namespace ExpanderDecomposition
