#include "gtest/gtest.h"

#include "lib/unit_flow.hpp"

#include <algorithm>
#include <numeric>

TEST(UnitFlow, TwoVertexFlow) {
  UnitFlow uf(2, INT_MAX);
  uf.addSource(0, 10);
  uf.addSink(1, 10);
  uf.addEdge(0, 1, 10);

  auto cut = uf.compute();

  EXPECT_TRUE(cut.empty());
  EXPECT_EQ(uf.flowIn(0), 0);
  EXPECT_EQ(uf.flowIn(1), 10);
}

TEST(UnitFlow, TwoVertexFlowSmallEdge) {
  UnitFlow uf(2, INT_MAX);
  uf.addSource(0, 10);
  uf.addSink(1, 10);
  uf.addEdge(0, 1, 4);

  auto cut = uf.compute();

  EXPECT_EQ(cut, (std::vector<Vertex>{0}));
  EXPECT_EQ(uf.flowIn(0), 6);
  EXPECT_EQ(uf.flowIn(1), 4);
}

TEST(UnitFlow, TwoVertexFlowSmallSink) {
  UnitFlow uf(2, INT_MAX);
  uf.addSource(0, 10);
  uf.addSink(1, 2);
  uf.addEdge(0, 1, 9);

  auto cut = uf.compute();

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

  UnitFlow uf(n + m, INT_MAX);

  for (int u = 0; u < n; ++u)
    uf.addSink(u, 10);
  for (int u = 0; u < m; ++u)
    uf.addSink(n + u, 5);

  for (int u = 0; u < n; ++u)
    for (int v = 0; v < m; ++v)
      uf.addEdge(u, n + v, 2);

  auto cut = uf.compute();

  EXPECT_TRUE(cut.empty());
  for (int u = 0; u < n; ++u)
    EXPECT_EQ(uf.excess(u), 0) << "Left hand side should not have any excess";
  for (int u = 0; u < m; ++u)
    EXPECT_EQ(uf.excess(n + u), 0)
        << "Right hand side should not have any excess";
}

TEST(UnitFlow, CannotRouteBottleneck) {
  const int n = 10;

  UnitFlow uf(n, INT_MAX);

  for (int u = 0; u < 3; ++u) {
    uf.addSource(u, 10);
    uf.addEdge(u, 3, 5);
    for (int v = u + 1; v < 3; ++v)
      uf.addEdge(u, v, 10);
  }
  for (int u = 4; u < n; ++u) {
    uf.addSink(u, 10);
    uf.addEdge(3, u, 5);
    for (int v = u + 1; v < n; ++v)
      uf.addEdge(u, v, 10);
  }

  auto cut = uf.compute();
  std::sort(cut.begin(), cut.end());
  std::vector<Vertex> expected = {0, 1, 2};
  EXPECT_EQ(cut, expected) << "Expected source nodes be part of the level cut";

  for (int u = 0; u < 3; ++u)
    EXPECT_GT(uf.excess(u), 0) << "Expected positive excess on source node";
  for (int u = 4; u < n; ++u)
    EXPECT_EQ(uf.excess(u), 0)
        << "Expected no excess on other side of bottleneck";
}

TEST(UnitFlow, CanMatchSimple) {
  UnitFlow uf(2, 10);
  uf.addSource(0, 5);
  uf.addSink(1, 5);
  uf.addEdge(0, 1, 5);
  uf.compute();
  auto matches = uf.matching({0});
  EXPECT_EQ(matches, (std::vector<std::pair<Vertex, Vertex>>{{0, 1}}));
}

TEST(UnitFlow, WontMatchBeforeFlowComputed) {
  UnitFlow uf(2, 10);
  uf.addSource(0, 5);
  uf.addSink(1, 5);
  uf.addEdge(0, 1, 5);

  auto matches = uf.matching({0});
  EXPECT_TRUE(matches.empty());
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

  UnitFlow uf(n, INT_MAX);

  for (int u = 0; u < leftN; ++u) {
    uf.addSource(u, 2);
    for (int v = u + 1; v < leftN; ++v)
      uf.addEdge(u, v, 1000);
  }
  for (int u = leftN; u < leftN + rightN; ++u) {
    uf.addSink(u, 2);
    for (int v = u + 1; v < leftN + rightN; ++v)
      uf.addEdge(u, v, 1000);
  }
  uf.addEdge(0, leftN, 1000);

  uf.compute();

  for (int u = 0; u < leftN; ++u)
    ASSERT_TRUE(uf.flowOut(u) > 0) << "Expected flow out of u.";
  for (int u = 0; u < leftN; ++u)
    ASSERT_EQ(uf.flowIn(u), 0)
        << "Did not expect a left partition vertex absorbing flow.";

  std::vector<Vertex> sources(leftN);
  std::iota(sources.begin(), sources.end(), 0);

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
     (Source)--(Source)--()--()--()--(Sink)--(Sink)

   Assuming edge capacities are large enough it should be possible to find two
   matchings.
 */
TEST(UnitFlow, CanRoutePathGraph) {
  UnitFlow uf(5, INT_MAX);
  uf.addSource(0, 1);
  uf.addSource(1, 1);

  uf.addSink(3, 1);
  uf.addSink(4, 1);

  uf.addEdge(0, 1, 2);
  uf.addEdge(1, 2, 2);
  uf.addEdge(2, 3, 2);
  uf.addEdge(3, 4, 2);

  uf.compute();
  auto matches = uf.matching({0, 1});

  ASSERT_EQ((int)matches.size(), 2);

  std::set<Vertex> left, right;
  for (auto [u, v] : matches)
    left.insert(u), right.insert(v);

  EXPECT_EQ(left, (std::set<Vertex>{0, 1}));
  EXPECT_EQ(right, (std::set<Vertex>{3, 4}));
}
