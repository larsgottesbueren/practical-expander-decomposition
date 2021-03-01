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
     If true, the 'm^2' sized flow matrix is maintained and returned along with
     other results. This does not impact result but is useful when verifying the
     algorithm.
  */
  bool computeFlowMatrix;

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
     An integer 'i >= 0' describing the number of times per iteration the
     potential function of the currently alive vertices should be computed. Just
     like 'resampleUnitVector' this requires maintaining matchings during the
     entire algorithm.
  */
  int samplePotential;
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
     If 'samplePotential' is greater than zero, this a vector of expansion
     samples for each iteration. Vector should have length 'iterations+1'
     corresponding to one entry before every iteration plus one entry after the
     final iteration.
   */
  std::vector<std::vector<double>> sampledPotentials;

  /**
     If 'computeFlowMatrix' is true, this matrix represents the flow at each
     subdivision vertex.
   */
  std::vector<std::vector<double>> flowMatrix;
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
     Sample the potential function 'k' times.
   */
  std::vector<double> samplePotential(const std::vector<Matching> &rounds,
                                      int k);

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
