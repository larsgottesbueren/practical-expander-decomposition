#include <algorithm>
#include <cmath>
#include <glog/logging.h>
#include <glog/stl_logging.h>
#include <numeric>
#include <random>
#include <unordered_set>

#include "cut_matching.hpp"
#include "unit_flow.hpp"

namespace CutMatching {

Solver::Solver(const UnitFlow::Graph *g, UnitFlow::Graph *subdivisionFlowGraph,
               const std::vector<int> &subset, const double phi)
    : graph(g), subdivisionFlowGraph(subdivisionFlowGraph), subset(subset),
      phi(phi) {
  std::random_device rd;
  randomGen = std::mt19937(0);
  //  randomGen = std::mt19937(rd());

  const int splitNodes = graph->edgeCount();
  const UnitFlow::Flow capacity =
      std::round(1.0 / phi / std::log(splitNodes) / std::log(splitNodes));
  for (const auto u : subset)
    for (auto &e : subdivisionFlowGraph->edges(u))
      e->capacity = capacity, e->reverse->capacity = capacity;
}

/**
   Given a number of matchings 'M_i' and a start state, compute the flow
   projection.

   Assumes no pairs of vertices in single round overlap.

   Time complexity: O(|rounds| + |start|)
 */
using Matching = std::vector<std::pair<int, int>>;
std::vector<double>
projectFlow(const std::vector<Matching> &rounds,
            const std::unordered_map<int, int> &fromSplitNode,
            std::vector<double> start) {
  for (auto it = rounds.begin(); it != rounds.end(); ++it) {
    for (const auto &[u, v] : *it) {
      auto uIt = fromSplitNode.find(u), vIt = fromSplitNode.find(v);
      assert(uIt != fromSplitNode.end() && vIt != fromSplitNode.end());
      int i = uIt->second, j = vIt->second;
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
void fillRandomUnitVector(std::mt19937 &gen, std::vector<double> &xs) {
  const int n = (int)xs.size();
  for (int i = 0; i < n / 2; ++i)
    xs[i] = -1;
  for (int i = n / 2; i < n; ++i)
    xs[i] = 1;
  if (n % 2 != 0)
    xs[0] = -2;
  std::shuffle(xs.begin(), xs.end(), gen);
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

Result Solver::compute() {
  std::vector<Matching> rounds;

  std::vector<int> splitNodes;
  std::unordered_set<int> splitNodeSet;
  for (const auto u : subset)
    for (const auto &e : subdivisionFlowGraph->edges(u))
      if (splitNodeSet.find(e->to) == splitNodeSet.end()) {
        splitNodes.push_back(e->to);
        splitNodeSet.insert(e->to);
      }

  const int numSplitNodes = splitNodeSet.size();
  const double T =
      1 + 0.9 * std::ceil(std::log(numSplitNodes) * std::log(numSplitNodes));

  if (numSplitNodes <= 1) {
    Result result;
    result.t = Expander;
    result.a = subset;
    return result;
  }

  // Maintain split node indices: 'fromSplitNode[splitNodes[i]] = i'
  std::unordered_map<int, int> fromSplitNode;
  for (size_t i = 0; i < splitNodes.size(); ++i)
    fromSplitNode[splitNodes[i]] = i;

  std::unordered_set<int> aSet, axSet, aAndAxSet, rSet;
  for (auto u : subset) {
    aSet.insert(u), aAndAxSet.insert(u);
    for (const auto &e : subdivisionFlowGraph->edges(u))
      axSet.insert(e->to), aAndAxSet.insert(e->to);
  }

  int iterations = 1;
  for (; iterations <= T &&
         subdivisionFlowGraph->globalVolume(rSet.begin(), rSet.end()) <=
             100 + numSplitNodes / 10.0 / T;
       ++iterations) {

    std::vector<double> r = randomUnitVector(randomGen, numSplitNodes);
    const auto flow = projectFlow(rounds, fromSplitNode, r);
    // double avgFlow = std::accumulate(flow.begin(), flow.end(), 0) /
    // flow.size();
    std::vector<int> axSetByFlow(axSet.begin(), axSet.end());
    std::sort(axSetByFlow.begin(), axSetByFlow.end(),
              [&flow, &fromSplitNode](int u, int v) {
                return flow[fromSplitNode[u]] < flow[fromSplitNode[v]];
              });
    const int middle = axSetByFlow.size() / 2;
    // const double eta = flow[fromSplitNode[axSetByFlow[middle]]];

    subdivisionFlowGraph->reset(aAndAxSet.begin(), aAndAxSet.end());
    for (int i = 0; i < (int)axSetByFlow.size(); ++i)
      if (i < middle)
        subdivisionFlowGraph->addSource(axSetByFlow[i], 1);
      else
        subdivisionFlowGraph->addSink(axSetByFlow[i], 1);

    const int h = (int)ceil(1.0 / phi / std::log(numSplitNodes));
    const auto levelCut = subdivisionFlowGraph->compute(h, aAndAxSet);

    std::unordered_set<int> removed;
    for (auto u : levelCut)
      removed.insert(u);
    auto isRemoved = [&removed](int u) {
      return removed.find(u) != removed.end();
    };
    for (auto u : axSet) {
      assert(subdivisionFlowGraph->degree(u) == 2 &&
             "Subdivision vertices should have degree two.");
      int count = 0;
      for (const auto &e : subdivisionFlowGraph->edges(u))
        if (isRemoved(e->to))
          count++;

      if (isRemoved(u)) {
        if (count == 0)
          removed.erase(u);
      } else {
        if (count == 2)
          removed.insert(u);
      }
    }

    assert(middle < (int)axSetByFlow.size() &&
           "Set of split vertices smaller than expected.");
    std::vector<int> sourcesLeft, targetsLeft;
    for (int i = 0; i < middle; ++i) {
      int u = axSetByFlow[i];
      if (!isRemoved(u))
        sourcesLeft.push_back(u);
    }
    for (int i = middle; i < (int)axSetByFlow.size(); ++i) {
      int u = axSetByFlow[i];
      if (!isRemoved(u))
        targetsLeft.push_back(u);
    }
    auto matching =
        subdivisionFlowGraph->matching(aAndAxSet, sourcesLeft, targetsLeft);
    rounds.push_back(matching);

    for (auto u : removed)
      aSet.erase(u), axSet.erase(u), aAndAxSet.erase(u), rSet.insert(u);
  }

  Result result;
  if (iterations <= T)
    // We have: graph.volume(R) > m / (10 * T)
    result.t = Balanced;
  else if (rSet.empty())
    result.t = Expander;
  else
    result.t = NearExpander;

  for (auto u : aSet)
    if (splitNodeSet.find(u) == splitNodeSet.end())
      result.a.push_back(u);
  for (auto u : rSet)
    if (splitNodeSet.find(u) == splitNodeSet.end())
      result.r.push_back(u);

  switch (result.t) {
  case Balanced: {
    VLOG(2) << "Cut matching ran " << iterations
            << " iterations and resulted in balanced cut with size ("
            << result.a.size() << ", " << result.r.size() << ").";
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
            << result.a.size() << ".";
    break;
  }
  }

  return result;
}
} // namespace CutMatching
