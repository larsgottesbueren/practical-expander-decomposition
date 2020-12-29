#include "gtest/gtest.h"

#include "lib/datastructures/unit_flow.hpp"

#include <algorithm>
#include <numeric>
#include <vector>

/**
   Make sure 'addEdge' adds an edge in both ways with the correct 'backIdx'.
 */
TEST(UnitFlow, AddEdge) {
  const int n = 3;
  const std::vector<UnitFlow::Edge> es = {{0, 1, 5}, {0, 2, 10}};
  UnitFlow::Graph uf(n, es);

  EXPECT_EQ(uf.neighbors(0), (std::vector<int>{1, 2}));
  EXPECT_EQ(uf.neighbors(1), (std::vector<int>{0}));
  EXPECT_EQ(uf.neighbors(2), (std::vector<int>{0}));

  const auto &e01 = uf.getEdge(0, 0);
  const auto &e02 = uf.getEdge(0, 1);
  const auto &e10 = uf.getEdge(1, 0);
  const auto &e20 = uf.getEdge(2, 0);

  EXPECT_EQ(e01.capacity, 5);
  EXPECT_EQ(e10.capacity, 5);
  EXPECT_EQ(e02.capacity, 10);
  EXPECT_EQ(e20.capacity, 10);

  EXPECT_EQ(uf.reverse(e01), e10);
  EXPECT_EQ(uf.reverse(e02), e20);
}

TEST(UnitFlow, SingleVertex) {
  UnitFlow::Graph uf(1, {});
  uf.addSource(0, 10);
  uf.addSink(0, 5);

  auto cut = uf.compute(INT_MAX);

  EXPECT_EQ(cut, std::vector<int>{0});
  EXPECT_EQ(uf.excess(0), 5);
}

TEST(UnitFlow, TwoVertexFlow) {
  UnitFlow::Graph uf(2, {{0, 1, 10}});
  uf.addSource(0, 10);
  uf.addSink(1, 10);

  auto cut = uf.compute(INT_MAX);

  EXPECT_TRUE(cut.empty());
  EXPECT_EQ(uf.flowIn(0), 0);
  EXPECT_EQ(uf.flowIn(1), 10);
}

TEST(UnitFlow, TwoVertexFlowSmallEdge) {
  UnitFlow::Graph uf(2, {{0, 1, 4}});
  uf.addSource(0, 10);
  uf.addSink(1, 10);

  auto cut = uf.compute(INT_MAX);

  EXPECT_EQ(cut, (std::vector<int>{0}));
  EXPECT_EQ(uf.flowIn(0), 6);
  EXPECT_EQ(uf.flowIn(1), 4);
}

TEST(UnitFlow, TwoVertexFlowSmallSink) {
  UnitFlow::Graph uf(2, {{0, 1, 9}});
  uf.addSource(0, 10);
  uf.addSink(1, 2);

  auto cut = uf.compute(INT_MAX);

  EXPECT_FALSE(cut.empty());
  EXPECT_GT(uf.excess(0), 0);
  EXPECT_GT(uf.excess(1), 0);
}

/**
   Let G=(X \cup Y, X \times Y) be a complete bipartite graph where x \in X are
   sources and y \in Y are sinks. Test that flow is possible to route.
 */
TEST(UnitFlow, CanRouteBipartite) {
  const int n = 5;
  const int m = 10;

  std::vector<UnitFlow::Edge> es;
  for (int u = 0; u < n; ++u)
    for (int v = 0; v < m; ++v)
      es.emplace_back(u, n + v, 2);

  UnitFlow::Graph uf(n + m, es);

  for (int u = 0; u < n; ++u)
    uf.addSink(u, 10);
  for (int u = 0; u < m; ++u)
    uf.addSink(n + u, 5);

  auto cut = uf.compute(INT_MAX);

  EXPECT_TRUE(cut.empty());
  for (int u = 0; u < n; ++u)
    EXPECT_EQ(uf.excess(u), 0) << "Left hand side should not have any excess";
  for (int u = 0; u < m; ++u)
    EXPECT_EQ(uf.excess(n + u), 0)
        << "Right hand side should not have any excess";
}

TEST(UnitFlow, CannotRouteBottleneck) {
  const int n = 10;

  std::vector<UnitFlow::Edge> es;
  for (int u = 0; u < 3; ++u) {
    es.emplace_back(u, 3, 5);
    for (int v = u + 1; v < 3; ++v)
      es.emplace_back(u, v, 10);
  }
  for (int u = 4; u < n; ++u) {
    es.emplace_back(3, u, 5);
    for (int v = u + 1; v < n; ++v)
      es.emplace_back(u, v, 10);
  }

  UnitFlow::Graph uf(n, es);

  for (int u = 0; u < 3; ++u)
    uf.addSource(u, 10);
  for (int u = 4; u < n; ++u)
    uf.addSink(u, 10);

  auto cut = uf.compute(INT_MAX);
  std::sort(cut.begin(), cut.end());
  std::vector<int> expected = {0, 1, 2};
  EXPECT_EQ(cut, expected) << "Expected source nodes be part of the level cut";

  for (int u = 0; u < 3; ++u)
    EXPECT_GT(uf.excess(u), 0) << "Expected positive excess on source node";
  for (int u = 4; u < n; ++u)
    EXPECT_EQ(uf.excess(u), 0)
        << "Expected no excess on other side of bottleneck";
}

TEST(UnitFlow, CanMatchSimple) {
  UnitFlow::Graph uf(2, {{0, 1, 5}});
  uf.addSource(0, 5);
  uf.addSink(1, 5);
  uf.compute(10);
  auto matches = uf.matching({0});
  EXPECT_EQ(matches, (std::vector<std::pair<int, int>>{{0, 1}}));
}

TEST(UnitFlow, WontMatchBeforeFlowComputed) {
  UnitFlow::Graph uf(2, {{0, 1, 5}});
  uf.addSource(0, 5);
  uf.addSink(1, 5);

  auto matches = uf.matching({0});
  EXPECT_TRUE(matches.empty());
}

/**
    s(0) - 1 - 2
          /     \
     3 - 4 - 5 - 6
    /    \      /
  s(7)   8 - t(9) - 10 - t(11)
 */
TEST(UnitFlow, CanMatchMediumGraph) {
  std::vector<UnitFlow::Edge> es = {
      {0, 1, 1}, {1, 2, 1},  {1, 4, 1},  {2, 6, 1}, {3, 4, 1},
      {3, 7, 1}, {4, 5, 1},  {4, 8, 1},  {5, 6, 1}, {6, 9, 1},
      {8, 9, 1}, {9, 10, 1}, {10, 11, 1}};
  UnitFlow::Graph uf(12, es);
  uf.addSource(0, 1), uf.addSource(7, 1);
  uf.addSink(9, 1), uf.addSink(11, 1);

  uf.compute(10);
  auto matches = uf.matching({0,7});
  EXPECT_EQ(matches.size(), 2);

  std::set<int> left, right;
  for (auto [u,v] : matches)
    left.insert(u), right.insert(v);

  EXPECT_EQ(left, std::set<int>({0,7}));
  EXPECT_EQ(right, std::set<int>({9,11}));
}

/**
   Create two complete graphs and connect them with one vertex. Allow for high
   edge capacities and route flow from left component to right component through
   single edge. We expect all edges on left match with one on the right.
 */
TEST(UnitFlow, CanMatchMultiple) {
  const int leftN = 10;
  const int rightN = 20;
  const int n = leftN + rightN;

  std::vector<UnitFlow::Edge> es;
  for (int u = 0; u < leftN; ++u)
    for (int v = u + 1; v < leftN; ++v)
      es.emplace_back(u, v, 1000);
  for (int u = leftN; u < leftN + rightN; ++u)
    for (int v = u + 1; v < leftN + rightN; ++v)
      es.emplace_back(u, v, 1000);
  es.emplace_back(0, leftN, 1000);

  UnitFlow::Graph uf(n, es);

  for (int u = 0; u < leftN; ++u)
    uf.addSource(u, 2);
  for (int u = leftN; u < leftN + rightN; ++u)
    uf.addSink(u, 2);

  uf.compute(INT_MAX);

  for (int u = 0; u < leftN; ++u)
    ASSERT_TRUE(uf.flowOut(u) > 0) << "Expected flow out of u.";
  for (int u = 0; u < leftN; ++u)
    ASSERT_EQ(uf.flowIn(u), 0)
        << "Did not expect a left partition vertex absorbing flow.";

  std::vector<int> sources(leftN), targets(rightN);
  std::iota(sources.begin(), sources.end(), 0);
  std::iota(targets.begin(), targets.end(), leftN);

  auto matches = uf.matching(sources);

  ASSERT_EQ((int)matches.size(), leftN)
      << "Expected all vertices in left partition to be matched.";
  for (auto &[u, v] : matches) {
    EXPECT_TRUE(0 <= u && u < leftN) << "Expected u to be from left partition.";
    EXPECT_TRUE(leftN <= v && v < n)
        << "Expected v to be from right partition.";
  }
}

/**
   Consider a path graph where one side contains sources and the other sinks:
     (0: Source)--(1: Source)--(2)--(3)--(4)--(5: Sink)--(6: Sink)

   Assuming edge capacities are large enough it should be possible to find two
   matchings.
 */
TEST(UnitFlow, CanRouteAndMatchPathGraph) {
  const std::vector<UnitFlow::Edge> es = {{0, 1, 2}, {1, 2, 2}, {2, 3, 2},
                                          {3, 4, 2}, {4, 5, 2}, {5, 6, 2}};
  UnitFlow::Graph uf(7, es);
  uf.addSource(0, 1);
  uf.addSource(1, 1);

  uf.addSink(5, 1);
  uf.addSink(6, 1);

  auto levelCut = uf.compute(INT_MAX);
  ASSERT_TRUE(levelCut.empty());

  auto matches = uf.matching({0, 1});
  ASSERT_EQ((int)matches.size(), 2);

  std::set<int> left, right;
  for (auto [u, v] : matches)
    left.insert(u), right.insert(v);

  EXPECT_EQ(left, (std::set<int>{0, 1}));
  EXPECT_EQ(right, (std::set<int>{5, 6}));
}

/**
   Construct and match the following graph:

   s1 - t1 - s2
      \    /
        t2
 */
TEST(UnitFlow, CanRouteAndMatchDiamondGraph) {
  const std::vector<UnitFlow::Edge> es = {
      {0, 1, 2}, {0, 2, 8}, {1, 2, 10}, {1, 3, 1}, {2, 3, 10}};

  UnitFlow::Graph uf(4, es);
  uf.addSource(0, 10), uf.addSource(3, 10);
  uf.addSink(1, 10), uf.addSink(2, 10);

  auto levelCut = uf.compute(INT_MAX);
  ASSERT_TRUE(levelCut.empty());

  auto matches = uf.matching({0, 3});
  ASSERT_EQ((int)matches.size(), 2);
}

/**
   Can route flow from left to right hand side of a k-layered bipartite graph
   and then find a matching for all source vertices..
 */
TEST(UnitFlow, CanRouteAndMatchKBipartite) {
  constexpr int layerSize = 100, k = 100;
  constexpr int n = layerSize * k;

  std::vector<UnitFlow::Edge> es;
  for (int l = 0; l < k - 1; ++l) {
    for (int i = 0; i < layerSize; ++i) {
      for (int j = 0; j < layerSize; ++j) {
        int u = l * layerSize + i, v = (l + 1) * layerSize + j;
        es.emplace_back(u, v, 1);
      }
    }
  }

  UnitFlow::Graph uf(n, es);

  std::vector<int> sources, targets;
  for (int i = 0; i < layerSize; ++i) {
    int s = i, t = (k - 1) * layerSize + i;

    uf.addSource(s, 1), sources.push_back(s);
    uf.addSink(t, 1), targets.push_back(t);
  }

  auto hasExcess = uf.compute(INT_MAX);
  ASSERT_TRUE(hasExcess.empty());

  auto matches = uf.matching(sources);
  ASSERT_EQ((int)matches.size(), layerSize);
  for (auto [u, v] : matches) {
    ASSERT_GE(u, 0);
    ASSERT_LT(u, layerSize);
    ASSERT_GE(v, (k - 1) * layerSize);
    ASSERT_LT(v, n);
  }
}

/**
   Construct random graph with random capacities and find a matching.
 */
TEST(UnitFlow, CanMatchLargeGraph) {
  for (int iteration = 0; iteration < 200; ++iteration) {
    std::srand(iteration);
    constexpr int n = 50, m = 1000, c = 100;

    std::vector<UnitFlow::Edge> es;
    for (int i = 0; i < m; ++i) {
      int u = rand() % n, v = rand() % n;
      es.emplace_back(u, v, rand() % c);
    }

    UnitFlow::Graph uf(n, es);

    std::vector<int> sources = {0, 1, 2, 3, 4},
                     targets = {n - 5, n - 4, n - 3, n - 2, n - 1};

    for (int u : sources)
      uf.addSource(u, 10);
    for (int u : targets)
      uf.addSink(u, 10);

    uf.compute(INT_MAX);
    uf.matching(sources);
  }
}

/**
   'reset' should set all flow, height, absorbtion and sinks to 0.
 */
TEST(UnitFlow, Reset) {
  const std::vector<UnitFlow::Edge> es = {{0, 1, 10}, {0, 2, 10}, {1, 2, 10},
                                          {1, 3, 10}, {1, 2, 10}, {2, 4, 10}};
  UnitFlow::Graph uf(5, es);
  uf.addSource(0, 5);
  uf.addSink(4, 5);

  uf.compute(INT_MAX);

  uf.reset();

  for (int u = 0; u < 5; ++u) {
    EXPECT_EQ(uf.getAbsorbed()[u], 0);
    EXPECT_EQ(uf.getSink()[u], 0);
    EXPECT_EQ(uf.getHeight()[u], 0);
    EXPECT_EQ(uf.getNextEdgeIdx()[u], 0);
    for (auto e = uf.beginEdge(u); e != uf.endEdge(u); ++e)
      EXPECT_EQ(e->flow, 0);
  }
}

/**
   'reset' should set all flow, height, absorbtion and sinks to 0.
 */
TEST(UnitFlow, ResetSubset) {
  const std::vector<UnitFlow::Edge> es = {{0, 1, 10}, {0, 2, 10}, {1, 2, 10},
                                          {1, 3, 10}, {1, 2, 10}, {2, 4, 10}};
  UnitFlow::Graph uf(5, es);
  uf.addSource(0, 5);
  uf.addSink(4, 5);

  uf.compute(INT_MAX);

  std::vector<int> subset = {1, 2, 3};
  uf.reset(subset.begin(), subset.end());

  for (auto u : subset) {
    EXPECT_EQ(uf.getAbsorbed()[u], 0);
    EXPECT_EQ(uf.getSink()[u], 0);
    EXPECT_EQ(uf.getHeight()[u], 0);
    EXPECT_EQ(uf.getNextEdgeIdx()[u], 0);
    for (auto e = uf.beginEdge(u); e != uf.endEdge(u); ++e)
      EXPECT_EQ(e->flow, 0);
  }
}
