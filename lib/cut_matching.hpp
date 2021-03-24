#pragma once

#include <random>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "datastructures/undirected_graph.hpp"
#include "datastructures/unit_flow.hpp"

namespace CutMatching {

enum ResultType { Balanced, Expander, NearExpander };

/**
   Parameters configuring how the cut-matching game should run.
 */
struct Parameters {
  /**
     Value 't_1' in 'T = t_1 + t_2 \log^2 m'
   */
  int tConst;

  /**
     Value 't_2' in 'T = t_1 + t_2 \log^2 m'
   */
  double tFactor;

  /**
     If true, a new random unit vector is chosen every iteration. This results
     in 'O(m log^4 m)' time complexity since matchings have to be maintained
     during entire game.
   */
  bool resampleUnitVector;

  /**
     The minimum volume balance '0 <= b <= 0.5' the algorithm should reach
     before terminating with sparse cut.
  */
  double minBalance;

  /**
     An integer 'k > 0' in the flow projection 'F^k r'. Normal behavior is 'k =
     1'. Since 'F' is a transition matrix 'F^k' is transition matrix after 'k'
     steps. Does nothing if 'resampleUnitVector' is false.
   */
  int randomWalkSteps;

  /**
     True if the potential function should be sampled each iteration. This
     requires maintaining the entire 'O(m^2)' flow matrix.
  */
  bool samplePotential;
};

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
     Number of iterations the cut-matching step ran.
   */
  int iterations;

  /**
     Congestion of the embedding. If result is an expander, then conductance of
     graph is '1/congestion'.
   */
  long long congestion;

  /**
     Vector of potential function at the start of the cut-matching game and
     after each iteration.
   */
  std::vector<double> sampledPotentials;
};

/**
   A matching is a vector of pairs '(a,b)'. The cut-matching game guarantees
   that no vertex occurs in more than one pair.
 */
using Matching = std::vector<std::pair<int, int>>;

class Solver {
private:
  UnitFlow::Graph *graph;
  UnitFlow::Graph *subdivGraph;

  std::vector<int> *subdivisionIdx;
  std::vector<int> *fromSubdivisionIdx;

  const double phi;
  const int T;

  /**
     Number of subdivision vertices at beginning of computation.
   */
  const int numSplitNodes;

  std::mt19937 randomGen;

  /**
     Matrix representing multi-commodity flow. Only constructed if potential is
     sampled.
   */
  std::vector<std::vector<double>> flowMatrix;

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
     Sample the potential function using the current state of the flow matrix.
   */
  double samplePotential() const;

public:
  /**
     Create a cut-matching problem.

     Parameters:

     - g: Original graph.

     - subdivGraph: Subdivision graph of g

     - subdivisionIdx: Vector used to associate an index with each subdivision

       vertex.

     - fromSubdivisionIdx: Vector used as inverse to 'subdivisionIdx'.

     - phi: Conductance value.

     - params: Algorithm configuration.
   */
  Solver(UnitFlow::Graph *g, UnitFlow::Graph *subdivGraph,
         std::vector<int> *subdivisionIdx, std::vector<int> *fromSubdivisionIdx,
         double phi, Parameters params);

  /**
     Compute a sparse cut.
   */
  Result compute(Parameters params);
};
}; // namespace CutMatching
