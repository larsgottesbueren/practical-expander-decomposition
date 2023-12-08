#pragma once

#include <random>
#include <vector>

#include "datastructures/unit_flow.hpp"
#include "util.hpp"

namespace CutMatching {

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
     Minimum iterations algorithm should run. Actual iterations is
     'min(minIterations, T)'. Forcing more than 'T' iterations is useful for
     testing but will reduce conductance certificate since the congestion bound
     becomes larger.
   */
  int minIterations;

  /**
     The minimum volume balance '0 <= b <= 0.5' the algorithm should reach
     before terminating with sparse cut.
  */
  double minBalance;

  /**
     True if the potential function should be sampled each iteration. This
     requires maintaining the entire 'O(m^2)' flow matrix.
  */
  bool samplePotential;

  /**
     If true, use a cut strategy which proposes perfectly balanced cuts.
     Otherwise use the original cut strategy from Lemma 3.3 in RST.
   */
  bool balancedCutStrategy;


  bool use_cut_heuristics = true;

  bool use_potential_based_dynamic_stopping_criterion = false;

  int num_flow_vectors = 20;

  /**
   * use in combination with 'samplePotential' to determine how many flow vectors are needed to not converge faster
   * than the full multi-commodity flow matrix.
   */
  bool tune_num_flow_vectors = false;
};

/**
   The result of running the cut-matching game is a balanced cut, an expander,
   or a near expander.
 */
struct Result {
  enum Type { Balanced, Expander, NearExpander };
  /**
     Type of cut-matching result.
   */
  Type type = Expander;

  /**
     Number of iterations the cut-matching step ran.
   */
  int iterations = 0;

  /**
     If potentials are sampled, the number of iterations until the potential
     threshold '1/(16m^2)' is reached. If potentials are not sampled or the
     threshold isn't reached value defaults to INT_MAX.
   */
  int iterationsUntilValidExpansion = std::numeric_limits<int>::max();
  int iterationsUntilValidExpansion2 = std::numeric_limits<int>::max();

  /**
     Congestion of the embedding. If result is an expander, then conductance of
     graph is '1/congestion'.
   */
  long long congestion = 1;

  int num_flow_vectors_needed = -1;
};

struct FlowVector {
    std::vector<double> entries;
    double sum;
    size_t num_entries;
    void RemoveEntry(size_t ind) {
        sum -= entries[ind];
        entries[ind] = 0.0;
    }
    double AvgFlow() const { return sum / num_entries; }
};

class Solver {
private:
  UnitFlow::Graph *graph;
  UnitFlow::Graph *subdivGraph;

  /**
     Randomness generator.
   */
  std::mt19937 *randomGen;

  std::vector<int> *subdivisionIdx;

  const double phi;
  int T;

  /**
     Number of subdivision vertices at beginning of computation.
   */
  const int numSplitNodes;

  /**
     Matrix representing multi-commodity flow. Only constructed if potential is
     sampled.
   */
  std::vector<std::vector<double>> flowMatrix;

  std::vector<int> num_matched_steps;

  /**
     Construct a semi-random vector for the currently alive subdivision vertices
     with length 'numSplitNodes' normalized by the number of alive subdivision
     vertices.
   */
  std::vector<double> randomUnitVector();

  /**
     Sample the potential function using the current state of the flow matrix.
   */
  double samplePotential() const;

  double ProjectedPotential(const std::vector<double>& flow) const;
  double AvgFlow(const std::vector<double>& flow) const;

  std::pair<std::vector<int>, std::vector<int>> KRVCutStep(
      const std::vector<double> &flow, const Parameters &params) const;
  std::pair<std::vector<int>, std::vector<int>> RSTCutStep(
      const std::vector<double> &flow, const Parameters &params) const;

  std::pair<size_t, double> SelectHighestPotentialFlowVector(const std::vector<std::vector<double>>& flows) const;

  /**
     Create a cut according to the cut player strategy given the current flow.
   */
  std::pair<std::vector<int>, std::vector<int>>
  proposeCut(const std::vector<double> &flow, const Parameters &params) const;

  double ProjectedPotentialConvergenceThreshold() const;

  void RemoveCutSide(const std::vector<UnitFlow::Vertex>& cutLeft, const std::vector<UnitFlow::Vertex>& cutRight,
      std::vector<UnitFlow::Vertex>& axLeft, std::vector<UnitFlow::Vertex>& axRight);

  void Initialize(Parameters params);

public:
  /**
     Create a cut-matching problem.

     Parameters:

     - g: Original graph.

     - subdivGraph: Subdivision graph of g

     - randomGen: Randomness generator.

     - subdivisionIdx: Vector used to associate an index with each subdivision

       vertex.

     - phi: Conductance value.

     - params: Algorithm configuration.
   */
  Solver(UnitFlow::Graph *g, UnitFlow::Graph *subdivGraph,
         std::mt19937 *randomGen, std::vector<int> *subdivisionIdx, double phi,
         Parameters params);

  /**
     Compute a sparse cut.
   */
  Result computeInternal(Parameters params);
  Result compute(Parameters params);
};
}; // namespace CutMatching
