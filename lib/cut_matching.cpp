#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>

#include "cut_matching.hpp"
#include "unit_flow.hpp"

CutMatching::CutMatching(const std::unique_ptr<PartitionGraph<int, Edge>> &g,
                         const std::vector<int> &subset,
                         const int graphPartition, const double phi)
    : graph(g), subset(subset), fromSubset(subset.size()),
      graphPartition(graphPartition), phi(phi),
      flowInstance(subset.size() + g->edgeCount(graphPartition)) {
  std::random_device rd;
  randomGen = std::mt19937(rd());

  for (int i = 0; i < (int)subset.size(); ++i)
    fromSubset[subset[i]] = i;

  int edgesAdded = 0;
  const int m = graph->edgeCount(graphPartition);
  const int cap = (int)std::ceil(1.0 / phi / std::log(m) / std::log(m));

  for (const auto u : subset)
    for (const auto v : graph->partitionNeighbors(u))
      if (u < v) {
        flowInstance.addEdge(fromSubset[u], m + edgesAdded, cap);
        flowInstance.addEdge(fromSubset[v], m + edgesAdded++, cap);
      }

  assert(edgesAdded == m &&
         "Unexpected number of edges added to flow instance.");
}

/**
   Given a number of matchings 'M_i' and a start state, compute the flow
   projection.

   Time complexity: O(|rounds| + |start|)
 */
using Matching = std::vector<std::pair<int, int>>;
std::vector<double> projectFlow(const std::vector<Matching> &rounds,
                                std::vector<double> start) {
  std::vector<double> result = start;
  for (auto it = rounds.rbegin(); it != rounds.rend(); ++it) {
    for (const auto [u, v] : *it) {
      start[u] = 0.5 * (result[u] + result[v]);
      start[v] = start[u];
    }
    std::swap(result, start);
  }

  return result;
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

CutMatching::Result CutMatching::compute() {
  std::vector<Matching> rounds;

  const int numSplitNodes = graph->edgeCount(graphPartition);
  const double T =
      1 + 0.9 * std::ceil(std::log(numSplitNodes) * std::log(numSplitNodes));

  std::vector<double> r(numSplitNodes);

  int iterations = 1;
  for (; graph->volume(subset.begin(), subset.end()) <=
             100 + numSplitNodes / 10.0 / T &&
         iterations <= T;
       ++iterations) {

    fillRandomUnitVector(randomGen, r);

    const auto flow = projectFlow(rounds, r);
    double avgFlow = std::accumulate(flow.begin(), flow.end(), 0) / flow.size();

    flowInstance.reset();

    const int h = (int)ceil(1.0 / phi / std::log(numSplitNodes));

    for (int u = 0; u < numSplitNodes; ++u)
      if (flow[u] < avgFlow)
        flowInstance.addSource(u + numSplitNodes, 1);
      else
        flowInstance.addSink(u + numSplitNodes, 1);
  }
}
