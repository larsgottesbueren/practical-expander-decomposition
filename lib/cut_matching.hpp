#pragma once

#include <random>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "datastructures/undirected_graph.hpp"
#include "datastructures/unit_flow.hpp"

namespace CutMatching {

enum ResultType { Balanced, Expander, NearExpander };

/**
   The result of running the cut-matching game is a balanced cut, an expander,
   or a near expander.
 */
struct Result {
  /**
     Type of cut-matching result.
   */
  ResultType type;

  /**
     If 'Solver::verifyExpansion' is positive, this a vector of expansion
     samples for each iteration. Vector should have length 'iterations+1'
     corresponding to one entry before every iteration plus one entry after the
     final iteration.
   */
  std::vector<std::vector<double>> certificateSamples;

  /**
     Number of iterations the cut-matching step ran.
   */
  int iterations;
};

using Matching = std::vector<std::pair<int, int>>;

class Solver {
private:
  UnitFlow::Graph *graph;
  UnitFlow::Graph *subdivGraph;

  const double phi;
  const int T;

  /**
     Number of subdivision vertices at beginning of computation.
   */
  const int numSplitNodes;

  /**
     Number of steps in random walk.
   */
  const int randomWalkSteps;

  /**
     Minimum cut balance required to terminate.
   */
  const double minBalance;

  /**
     Number of times the current graph embedding should be sampled each
     iteration.
   */
  const int verifyExpansion;

  std::mt19937 randomGen;

  /**
     Construct a random vector for the currently alive subdivision vertices with
     length 'numSplitNodes' normalized by the number of alive subdivision
     vertices.

     Based on normal distribution sampling: https://stackoverflow.com/a/8453514
   */
  std::vector<double> randomUnitVector();

  /**
     Project flow across the sparse matrices represented by the round matchings.
   */
  void projectFlow(const std::vector<Matching> &rounds,
                   std::vector<double> &start);

  /**
     Sample the expansion certificate 'verifyExpansion' times.
   */
  std::vector<double> sampleCertificate(const std::vector<Matching> &rounds);

public:
  /**
     Create a cut-matching problem from a graph.

     Precondition: graph should not contain loops.
   */
  Solver(UnitFlow::Graph *g, UnitFlow::Graph *subdivGraph, double phi,
         int tConst, double tFactor, int randomWalkSteps, double minBalance,
         int verifyExpansion);

  Result compute();
};
}; // namespace CutMatching
