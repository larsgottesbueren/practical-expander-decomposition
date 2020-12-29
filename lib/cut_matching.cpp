#include <algorithm>
#include <cmath>
#include <glog/logging.h>
#include <glog/stl_logging.h>
#include <numeric>
#include <random>

#include "absl/container/flat_hash_set.h"
#include "cut_matching.hpp"

namespace CutMatching {

Solver::Solver(UnitFlow::Graph *g, UnitFlow::Graph *subdivGraph,
               const double phi, const int tConst, const double tFactor,
               const int verifyExpansion)
    : graph(g), subdivGraph(subdivGraph), phi(phi),
      T(tConst + std::ceil(tFactor * std::log10(graph->edgeCount()) *
                           std::log10(graph->edgeCount()))),
      verifyExpansion(verifyExpansion) {
  assert(graph->size() != 0 && "Cut-matching expected non-empty subset.");

  std::random_device rd;
  // randomGen = std::mt19937(0);
  randomGen = std::mt19937(rd());

  const UnitFlow::Flow capacity = std::ceil(1.0 / phi / T);
  for (auto u : *graph)
    for (auto e = subdivGraph->beginEdge(u); e != subdivGraph->endEdge(u); ++e)
      e->capacity = capacity, subdivGraph->reverse(*e).capacity = capacity;
}

/**
   Given a number of matchings 'M_i' and a start state, compute the flow
   projection.

   Assumes no pairs of vertices in single round overlap.

   Time complexity: O(|rounds| + |start|)
 */
using Matching = std::vector<std::pair<int, int>>;
std::vector<double> projectFlow(const std::vector<Matching> &rounds,
                                const std::vector<int> &fromSplitNode,
                                std::vector<double> start) {
  for (auto it = rounds.begin(); it != rounds.end(); ++it) {
    for (const auto &[u, v] : *it) {
      int i = fromSplitNode[u], j = fromSplitNode[v];
      assert(i >= 0 && "Given vertex is not a subdivision vertex.");
      assert(j >= 0 && "Given vertex is not a subdivision vertex.");
      start[i] = 0.5 * (start[i] + start[j]);
      start[j] = start[i];
    }
  }

  return start;
}

/**
   Fill a vector of size 'n' with random data such that it is orthogonal to the
   all ones vector.
 */
std::vector<double> randomUnitVectorFast(std::mt19937 &gen, int n) {
  std::vector<double> xs(n);
  for (int i = 0; i < n / 2; ++i)
    xs[i] = -1;
  for (int i = n / 2; i < n; ++i)
    xs[i] = 1;
  if (n % 2 != 0)
    xs[0] = -2;
  std::shuffle(xs.begin(), xs.end(), gen);
  return xs;
}

/**
   Construct a random unit vector by sampling from normal distribution. Based
   on https://stackoverflow.com/a/8453514
 */
std::vector<double> randomUnitVector(std::mt19937 &gen, int n) {
  std::normal_distribution<> d(0, 1);
  std::vector<double> xs(n);
  double m = 0;
  for (auto &x : xs) {
    x = d(gen);
    m += x * x;
  }
  m = std::sqrt(m);
  for (auto &x : xs)
    x /= m;
  return xs;
}

template <typename It>
double potential(const double avgFlow, const std::vector<double> &flow,
                 const std::vector<int> &fromSplitNode, It begin, It end) {
  double p = 0;
  for (auto it = begin; it != end; it++) {
    double f = flow[fromSplitNode[*it]];
    p += (f - avgFlow) * (f - avgFlow);
  }
  return p;
}

/**
   Sample an expansion ceritificate 'numSamples' times by generating a random
   unit vector orthogonal to the all ones vector, project it on the matchings,
   and computing the distance to the uniform unit vector.
 */
std::vector<double> sampleCertificate(std::mt19937 &gen,
                                      const std::vector<Matching> &rounds,
                                      const std::vector<int> &fromSplitNode,
                                      int n, int numSamples) {
  std::vector<double> result;

  for (int sample = 0; sample < numSamples; ++sample) {
    auto xs = projectFlow(rounds, fromSplitNode, randomUnitVector(gen, n));
    const double invertN = 1.0 / double(n);
    double sum = 0;
    for (auto x : xs)
      sum += (invertN - x) * (invertN - x);
    result.push_back(std::sqrt(sum));
  }

  return result;
}

Result Solver::compute() {
  std::vector<Matching> rounds;

  const int numSplitNodes = subdivGraph->size() - graph->size();

  if (numSplitNodes <= 1) {
    VLOG(3) << "Cut matching exited early with " << numSplitNodes
            << " subdivision vertices.";
    Result result;
    result.type = Expander;
    return result;
  }

  {
    int count = 0;
    for (auto u : *subdivGraph)
      if (subdivGraph->isSubdivision(u))
        subdivGraph->setSubdivision(u, count++);
  }

  const int goodBalance = 0.45 * subdivGraph->globalVolume(),
            minBalance = numSplitNodes / (10 * T);

  int iterations = 1;
  for (; iterations <= T &&
         subdivGraph->globalVolume(subdivGraph->cbeginRemoved(),
                                   subdivGraph->cendRemoved()) <= goodBalance;
       ++iterations) {
    VLOG(3) << "Iteration " << iterations << " out of " << T << ".";

    auto flow = projectFlow(rounds, subdivGraph->getSubdivisionVector(),
                            randomUnitVectorFast(randomGen, numSplitNodes));
    double avgFlow =
        std::accumulate(flow.begin(), flow.end(), 0.0) / (double)flow.size();

    std::vector<int> axLeft, axRight;
    for (auto u : *subdivGraph) {
      if (subdivGraph->isSubdivision(u)) {
        if (flow[subdivGraph->getSubdivision(u)] < avgFlow)
          axLeft.push_back(u);
        else
          axRight.push_back(u);
      }
    }

    auto cmpFlow = [&flow, &subdivGraph = subdivGraph](int u, int v) {
      return flow[subdivGraph->getSubdivision(u)] <
             flow[subdivGraph->getSubdivision(v)];
    };
    std::sort(axLeft.begin(), axLeft.end(), cmpFlow);
    std::sort(axRight.begin(), axRight.end(), cmpFlow);
    std::reverse(axRight.begin(), axRight.end());

    const int numSubdivVertices = int(axLeft.size() + axRight.size());
    while (8 * axLeft.size() > numSubdivVertices)
      axLeft.pop_back();
    while (2 * axRight.size() > numSubdivVertices)
      axRight.pop_back();

    subdivGraph->reset();

    for (const auto u : axLeft)
      subdivGraph->addSource(u, 1);
    for (const auto u : axRight)
      subdivGraph->addSink(u, 1);

    const int h = std::max((int)round(1.0 / phi / std::log10(numSplitNodes)),
                           (int)std::log10(numSplitNodes));
    VLOG(3) << "Computing flow with |S| = " << axLeft.size()
            << " |T| = " << axRight.size() << " and max height " << h << ".";
    const auto hasExcess = subdivGraph->compute(h);

    absl::flat_hash_set<int> removed;
    if (hasExcess.empty()) {
      VLOG(3) << "\tAll flow routed.";
    } else {
      VLOG(3) << "\tHas " << hasExcess.size()
              << " vertices with excess. Computing level cut.";
      const auto levelCut = subdivGraph->levelCut(h);
      VLOG(3) << "\tHas level cut with " << levelCut.size() << " vertices.";

      for (auto u : levelCut)
        removed.insert(u);
    }

    VLOG(3) << "\tRemoving " << removed.size() << " vertices.";

    auto isRemoved = [&removed](int u) {
      return removed.find(u) != removed.end();
    };
    axLeft.erase(std::remove_if(axLeft.begin(), axLeft.end(), isRemoved),
                 axLeft.end());
    axRight.erase(std::remove_if(axRight.begin(), axRight.end(), isRemoved),
                  axRight.end());

    for (auto u : removed) {
      if (!subdivGraph->isSubdivision(u))
        graph->remove(u);
      subdivGraph->remove(u);
    }
    std::vector<int> zeroDegrees;
    for (auto it = subdivGraph->cbegin(); it != subdivGraph->cend(); ++it)
      if (subdivGraph->degree(*it) == 0)
        zeroDegrees.push_back(*it);
    for (auto u : zeroDegrees) {
      if (!subdivGraph->isSubdivision(u))
        graph->remove(u);
      subdivGraph->remove(u);
    }

    VLOG(3) << "Computing matching with |S| = " << axLeft.size()
            << " |T| = " << axRight.size() << ".";
    auto matching = subdivGraph->matching(axLeft);
    VLOG(3) << "Found matching of size " << matching.size() << ".";

    // TODO: Can we expect complete matching after removing level cut?
    // assert(matching.size() == axLeft.size() &&
    // "Expected all source vertices to be matched.");
    rounds.push_back(matching);
  }

  Result result;

  if (graph->size() != 0 && graph->removedSize() != 0 &&
      subdivGraph->globalVolume(subdivGraph->cbeginRemoved(),
                                subdivGraph->cendRemoved()) > minBalance)
    // We have: graph.volume(R) > m / (10 * T)
    result.type = Balanced;
  else if (graph->removedSize() == 0)
    result.type = Expander;
  else if (graph->size() == 0)
    graph->restoreRemoves(), result.type = Expander;
  else
    result.type = NearExpander;

  switch (result.type) {
  case Balanced: {
    VLOG(2) << "Cut matching ran " << iterations
            << " iterations and resulted in balanced cut with size ("
            << graph->size() << ", " << graph->removedSize() << ") and volume ("
            << graph->globalVolume(graph->cbegin(), graph->cend()) << ", "
            << graph->globalVolume(graph->cbeginRemoved(), graph->cendRemoved())
            << ").";
    break;
  }
  case Expander: {
    VLOG(2) << "Cut matching ran " << iterations
            << " iterations and resulted in expander.";
    result.certificateSamples = sampleCertificate(
        randomGen, rounds, subdivGraph->getSubdivisionVector(), numSplitNodes,
        verifyExpansion);
    break;
  }
  case NearExpander: {
    VLOG(2) << "Cut matching ran " << iterations
            << " iterations and resulted in near expander of size "
            << graph->size() << ".";
    result.certificateSamples = sampleCertificate(
        randomGen, rounds, subdivGraph->getSubdivisionVector(), numSplitNodes,
        verifyExpansion);
    break;
  }
  }

  return result;
}
} // namespace CutMatching
