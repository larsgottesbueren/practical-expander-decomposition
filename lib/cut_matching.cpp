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
      phi(phi), T(100 + std::ceil(std::log(graph->edgeCount()) *
                                  std::log(graph->edgeCount()))) {
  assert(!subset.empty() && "Cut-matching expected non-empty subset.");

  std::random_device rd;
  randomGen = std::mt19937(0);
  //  randomGen = std::mt19937(rd());

  const UnitFlow::Flow capacity = std::ceil(1.0 / phi / T);
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

template <typename It>
double potential(const double avgFlow, const std::vector<double> &flow,
                 const std::unordered_map<int, int> &fromSplitNode, It begin,
                 It end) {
  double p = 0;
  for (auto it = begin; it != end; it++) {
    double f = flow[fromSplitNode.at(*it)];
    p += (f - avgFlow) * (f - avgFlow);
  }
  return p;
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
             numSplitNodes / (10 * T);
       ++iterations) {
    VLOG(3) << "Iteration " << iterations << " out of " << T << ".";

    const auto flow = projectFlow(rounds, fromSplitNode,
                                  randomUnitVector(randomGen, numSplitNodes));
    double avgFlow =
        std::accumulate(flow.begin(), flow.end(), 0.0) / (double)flow.size();

    std::vector<int> axLeft, axRight;
    for (auto u : axSet) {
      if (flow[fromSplitNode[u]] < avgFlow)
        axLeft.push_back(u);
      else
        axRight.push_back(u);
    }
    // TODO: Is this what w.l.o.g in RST Lemma 3.3 refers to?
    if (axLeft.size() > axRight.size())
      while (axLeft.size() > axRight.size() && !axLeft.empty())
        axRight.push_back(axLeft.back()), axLeft.pop_back();

    double pAll = potential(avgFlow, flow, fromSplitNode, axSet.begin(),
                            axSet.end()),
           pLeft = potential(avgFlow, flow, fromSplitNode, axLeft.begin(),
                             axLeft.end());

    if (pLeft >= pAll / 20.0) {
      sort(axLeft.begin(), axLeft.end(), [&flow, &fromSplitNode](int u, int v) {
        return flow[fromSplitNode[u]] < flow[fromSplitNode[v]];
      });
      while (8 * axLeft.size() > axSet.size())
        axLeft.pop_back();
    } else {
      double leftL = 0;
      for (auto u : axLeft)
        leftL += std::abs(flow[fromSplitNode[u]] - avgFlow);
      double rightL = 0;
      for (auto u : axRight)
        rightL += std::abs(flow[fromSplitNode[u]] - avgFlow);
      assert(std::abs(leftL - rightL) < 1e-9 &&
             "Left and right sums should be equal.");
      const double l = leftL;
      const double mu = avgFlow + 4.0 * l / axSet.size();

      axRight.clear();
      for (auto u : axSet)
        if (flow[fromSplitNode[u]] <= mu)
          axRight.push_back(u);

      axLeft.clear();
      for (auto u : axSet)
        if (flow[fromSplitNode[u]] >= avgFlow + 6.0 * l / axSet.size())
          axLeft.push_back(u);
      sort(axLeft.begin(), axLeft.end(), [&flow, &fromSplitNode](int u, int v) {
        return flow[fromSplitNode[u]] > flow[fromSplitNode[v]];
      });
      while (8 * axLeft.size() > axSet.size())
        axLeft.pop_back();
    }

    subdivisionFlowGraph->reset(aAndAxSet.begin(), aAndAxSet.end());

    for (const auto u : axLeft)
      subdivisionFlowGraph->addSource(u, 1);
    for (const auto u : axRight)
      subdivisionFlowGraph->addSink(u, 1);

    const int h = (int)ceil(1.0 / phi / std::log(numSplitNodes));
    VLOG(3) << "Computing flow with |S| = " << axLeft.size()
            << " |T| = " << axRight.size() << " and max height " << h << ".";
    const auto hasExcess = subdivisionFlowGraph->compute(h, aAndAxSet);

    std::unordered_set<int> removed;
    if (hasExcess.empty()) {
      VLOG(3) << "\tAll flow routed.";
    } else {
      VLOG(3) << "\tHas " << hasExcess.size()
              << " vertices with excess. Computing level cut.";
      const auto levelCut = subdivisionFlowGraph->levelCut(h, aAndAxSet);
      VLOG(3) << "\tHas level cut with " << levelCut.size() << " vertices.";

      for (auto u : levelCut)
        removed.insert(u);
    }

    for (auto u : axSet) {
      assert(subdivisionFlowGraph->degree(u) == 2 &&
             "Subdivision vertices should have degree two.");

      bool allNeighborsRemoved = true, noNeighborsRemoved = true;
      for (const auto &e : subdivisionFlowGraph->edges(u)) {
        if (aSet.find(e->to) != aSet.end()) {
          if (removed.find(e->to) == removed.end())
            allNeighborsRemoved = false;
          else
            noNeighborsRemoved = false;
        }
      }

      if (removed.find(u) != removed.end()) {
        for (const auto &e : subdivisionFlowGraph->edges(u))
          if (aSet.find(e->to) != aSet.end())
            removed.insert(e->to);
        //        if (noNeighborsRemoved)
        //          removed.erase(u);
      } else {
        if (allNeighborsRemoved)
          removed.insert(u);
      }
    }
    VLOG(3) << "\tRemoving " << removed.size() << " vertices.";

    std::vector<int> sourcesLeft, targetsLeft;
    for (const auto u : axLeft)
      if (removed.find(u) == removed.end())
        sourcesLeft.push_back(u);
    for (const auto u : axRight)
      if (removed.find(u) == removed.end())
        targetsLeft.push_back(u);

    for (auto u : removed)
      aSet.erase(u), axSet.erase(u), aAndAxSet.erase(u), rSet.insert(u);

    VLOG(3) << "Computing matching with |S| = " << sourcesLeft.size()
            << " |T| = " << targetsLeft.size() << ".";
    auto matching =
        subdivisionFlowGraph->matching(aAndAxSet, sourcesLeft, targetsLeft);
    rounds.push_back(matching);
    VLOG(3) << "Found matching of size " << matching.size() << ".";
  }

  Result result;
  for (auto u : aSet)
    if (splitNodeSet.find(u) == splitNodeSet.end())
      result.a.push_back(u);
  for (auto u : rSet)
    if (splitNodeSet.find(u) == splitNodeSet.end())
      result.r.push_back(u);

  if (iterations <= T && !result.a.empty() && !result.r.empty())
    // We have: graph.volume(R) > m / (10 * T)
    result.t = Balanced;
  else if (result.r.empty())
    result.t = Expander;
  else
    result.t = NearExpander;

  switch (result.t) {
  case Balanced: {
    VLOG(2) << "Cut matching ran " << iterations
            << " iterations and resulted in balanced cut with size ("
            << result.a.size() << ", " << result.r.size() << ") and volume ("
            << graph->volume(result.a.begin(), result.a.end()) << ", "
            << graph->volume(result.r.begin(), result.r.end()) << ").";
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
