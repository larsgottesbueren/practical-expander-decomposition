#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <unordered_set>

#include "cut_matching.hpp"
#include "unit_flow.hpp"

namespace CutMatching {

Solver::Solver(const Undirected::Graph *g,
               UnitFlow::Graph *subdivisionFlowGraph,
               const std::vector<int> &subset, const int graphPartition,
               const double phi)
    : graph(g), subdivisionFlowGraph(subdivisionFlowGraph), subset(subset),
      graphPartition(graphPartition), phi(phi) {
  std::random_device rd;
  randomGen = std::mt19937(rd());

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
  if (n % 2 != 0)
    xs[0] = -2;
  std::shuffle(xs.begin(), xs.end(), gen);
}

Result Solver::compute() {
  std::vector<Matching> rounds;

  const int numSplitNodes = graph->edgeCount(graphPartition);
  const double T =
      1 + 0.9 * std::ceil(std::log(numSplitNodes) * std::log(numSplitNodes));

  std::vector<double> r(numSplitNodes);

  std::vector<int> splitNodes;
  std::unordered_set<int> splitNodeSet;
  splitNodes.reserve(numSplitNodes);
  for (const auto u : subset)
    for (const auto &e : subdivisionFlowGraph->edges(u))
      if (splitNodeSet.find(e->to) == splitNodeSet.end()) {
        splitNodes.push_back(e->to);
        splitNodeSet.insert(e->to);
      }
  assert((int)splitNodes.size() == numSplitNodes &&
         "The number of split nodes added did not match");

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

    fillRandomUnitVector(randomGen, r);
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
    std::vector<int> sourcesLeft;
    for (int i = 0; i < middle; ++i) {
      int u = axSetByFlow[i];
      if (!isRemoved(u))
        sourcesLeft.push_back(u);
    }
    auto matching = subdivisionFlowGraph->matching(sourcesLeft);
    rounds.push_back(matching);

    for (auto u : removed)
      aSet.erase(u), axSet.erase(u), aAndAxSet.erase(u), rSet.insert(u);
  }

  ResultType rType;
  if (iterations <= T)
    // We have: graph.volume(R) > m / (10 * T)
    rType = Balanced;
  else if (rSet.empty())
    rType = Expander;
  else
    rType = NearExpander;

  Result result;
  result.t = rType;
  for (auto u : aSet)
    if (splitNodeSet.find(u) == splitNodeSet.end())
      result.a.push_back(u);
  for (auto u : rSet)
    if (splitNodeSet.find(u) == splitNodeSet.end())
      result.r.push_back(u);

  return result;
}
} // namespace CutMatching
