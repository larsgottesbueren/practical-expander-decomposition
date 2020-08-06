#include "gtest/gtest.h"

#include "lib/unit_flow.hpp"

#include <algorithm>

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
