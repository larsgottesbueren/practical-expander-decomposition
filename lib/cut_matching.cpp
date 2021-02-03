#include <algorithm>
#include <cmath>
#include <glog/logging.h>
#include <glog/stl_logging.h>
#include <numeric>
#include <random>

#include "absl/container/flat_hash_set.h"
#include "cut_matching.hpp"

namespace CutMatching {

Solver::Solver(UnitFlow::Graph *g, UnitFlow::Graph *subdivG, double phi,
               int tConst, double tFactor, int randomWalkSteps,
               double minBalance, int verifyExpansion)
    : graph(g), subdivGraph(subdivG), phi(phi),
      T(tConst + std::ceil(tFactor * std::log10(graph->edgeCount()) *
                           std::log10(graph->edgeCount()))),
      numSplitNodes(subdivGraph->size() - graph->size()),
      randomWalkSteps(randomWalkSteps), minBalance(minBalance),
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
   projection in place.

   Assumes no pairs of vertices in single round overlap.

   Time complexity: O(|rounds| + |start|)
 */
void Solver::projectFlow(const std::vector<Matching> &rounds,
                         std::vector<double> &start) {
  for (auto it = rounds.begin(); it != rounds.end(); ++it) {
    for (const auto &[u, v] : *it) {
      if (subdivGraph->alive(u) && subdivGraph->alive(v)) {
        int i = subdivGraph->getSubdivision(u),
            j = subdivGraph->getSubdivision(v);
        assert(i >= 0 && "Given vertex is not a subdivision vertex.");
        assert(j >= 0 && "Given vertex is not a subdivision vertex.");
        start[i] = 0.5 * (start[i] + start[j]);
        start[j] = start[i];
      }
    }
  }
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

std::vector<double> Solver::randomUnitVector() {
  std::normal_distribution<> dist(0, 1);
  std::vector<double> result(numSplitNodes);
  double total = 0;

  for (auto it = subdivGraph->cbegin(); it != subdivGraph->cend(); ++it) {
    const auto u = *it;
    if (subdivGraph->isSubdivision(u)) {
      double x = dist(randomGen);
      result[subdivGraph->getSubdivision(u)] = x;
      total += x * x;
    }
  }

  total = std::sqrt(total);
  for (auto it = subdivGraph->cbegin(); it != subdivGraph->cend(); ++it) {
    const auto u = *it;
    if (subdivGraph->isSubdivision(u))
      result[subdivGraph->getSubdivision(u)] /= total;
  }

  return result;
}

std::vector<double>
Solver::sampleCertificate(const std::vector<Matching> &rounds) {
  std::vector<double> result;
  for (int sample = 0; sample < verifyExpansion; ++sample) {
    auto flow = randomUnitVector();
    projectFlow(rounds, flow);

    const int n = subdivGraph->size() - graph->size();
    const double avgFlow = 1.0 / double(n);
    double total = 0;
    for (auto it = subdivGraph->cbegin(); it != subdivGraph->cend(); ++it) {
      const auto u = *it;
      if (subdivGraph->isSubdivision(u)) {
        double f = flow[subdivGraph->getSubdivision(u)];
        total += (avgFlow - f) * (avgFlow - f);
      }
    }
    result.push_back(total);
  }
  return result;
}

Result Solver::compute() {
  std::vector<Matching> rounds;

  if (numSplitNodes <= 1) {
    VLOG(3) << "Cut matching exited early with " << numSplitNodes
            << " subdivision vertices.";
    Result result;
    result.type = Expander;
    result.iterations = 0;
    return result;
  }

  {
    int count = 0;
    for (auto u : *subdivGraph)
      if (subdivGraph->isSubdivision(u))
        subdivGraph->setSubdivision(u, count++);
  }

  /**
     Current number of subdivision vertices alive.
   */
  const auto curSubdivisionCount = [&graph = graph,
                                    &subdivGraph = subdivGraph] {
    return subdivGraph->size() - graph->size();
  };

  const int lowerVolumeBalance = numSplitNodes / (10 * T);
  const int targetVolumeBalance = std::max(
      lowerVolumeBalance, int(minBalance * subdivGraph->globalVolume()));

  Result result;

  int iterations = 0;
  for (; iterations < T &&
         subdivGraph->globalVolume(subdivGraph->cbeginRemoved(),
                                   subdivGraph->cendRemoved()) <=
             targetVolumeBalance;
       ++iterations) {
    VLOG(3) << "Iteration " << iterations << " out of " << T << ".";

    if (verifyExpansion > 0) {
      VLOG(4) << "Sampling conductance";
      result.certificateSamples.push_back(sampleCertificate(rounds));
      VLOG(4) << "Finished sampling conductance";
    }

    auto flow = randomUnitVector();
    for (int i = 0; i < randomWalkSteps; ++i)
      projectFlow(rounds, flow);

    double avgFlow = std::accumulate(flow.begin(), flow.end(), 0.0) /
                     (double)curSubdivisionCount();

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

  result.iterations = iterations;

  if (verifyExpansion > 0) {
    VLOG(4) << "Sampling conductance";
    result.certificateSamples.push_back(sampleCertificate(rounds));
    VLOG(4) << "Finished sampling conductance";
  }

  if (graph->size() != 0 && graph->removedSize() != 0 &&
      subdivGraph->globalVolume(subdivGraph->cbeginRemoved(),
                                subdivGraph->cendRemoved()) >
          lowerVolumeBalance)
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
    break;
  }
  case NearExpander: {
    VLOG(2) << "Cut matching ran " << iterations
            << " iterations and resulted in near expander of size "
            << graph->size() << ".";
    break;
  }
  }

  return result;
}
} // namespace CutMatching
