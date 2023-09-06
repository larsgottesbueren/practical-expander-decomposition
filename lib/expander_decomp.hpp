#pragma once

#include <vector>

#include "cut_matching.hpp"
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
     Randomness engine.
   */
  std::mt19937 *randomGen;

  /**
     Map to subdivision vertices.

     Value associated with each vertex such that 'subdivisionIdx[u] == -1' if
     'u' is not a subdivision vertex and 'subdivisionIdx[u] >= 0' if 'u' is a
     subdivision vertex.
   */
  std::unique_ptr<std::vector<int>> subdivisionIdx;

  const double phi;

  /**
     Parameters given to cut-matching game.
   */
  const CutMatching::Parameters cutMatchingParams;

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
     Congestion of expander embedding in each partition.
   */
  std::vector<long long> congestionOf;

  /**
     Compute expander decomposition for subset of vertices 'xs'.
   */
  void compute();

  /**
     Create a partition with the given vertices.
   */
  template <typename It> void finalizePartition(It begin, It end, long long c) {
    congestionOf.push_back(c);
    assert(congestionOf.size() == numPartitions + 1);

    for (auto it = begin; it != end; ++it)
      partitionOf[*it] = numPartitions;
    numPartitions++;
  }

public:
  /**
     Create a decomposition problem on graph 'g'.
   */
  Solver(std::unique_ptr<Undirected::Graph> g, double phi,
         std::mt19937 *randomGen, CutMatching::Parameters params);

  /**
     Return the computed partition as a vector of disjoint vertex vectors.
   */
  std::vector<std::vector<int>> getPartition() const;

  /**
     Compute lower bound on conductance using congestion from cut-matching game.
   */
  std::vector<double> getConductance() const;

  /**
     Return the number of edges which are cut. An edge is cut if it's two
     endpoints are in separate partitions.
   */
  int getEdgesCut() const;

  Duration time_balanced_cut = Duration(0.0), time_expander = Duration(0.0);
};
} // namespace ExpanderDecomposition
