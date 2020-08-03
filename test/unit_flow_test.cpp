#include "gtest/gtest.h"

#include "lib/unit_flow.hpp"

TEST(UnitFlow, TwoVertexFlow) {
  UnitFlow uf(2, INT_MAX);
  uf.addSource(0, 10);
  uf.addSink(1, 10);
  uf.addEdge(0, 1, 10);

  uf.compute();

  EXPECT_EQ(uf.flowIn(0), 0);
  EXPECT_EQ(uf.flowIn(1), 10);
}

TEST(UnitFlow, TwoVertexFlowSmallEdge) {
  UnitFlow uf(2, INT_MAX);
  uf.addSource(0, 10);
  uf.addSink(1, 10);
  uf.addEdge(0, 1, 4);

  uf.compute();

  EXPECT_EQ(uf.flowIn(0), 6);
  EXPECT_EQ(uf.flowIn(1), 4);
}

TEST(UnitFlow, TwoVertexFlowSmallSink) {
  UnitFlow uf(2, INT_MAX);
  uf.addSource(0, 10);
  uf.addSink(1, 2);
  uf.addEdge(0, 1, 9);

  uf.compute();

  EXPECT_EQ(uf.flowIn(0), 7);
  EXPECT_EQ(uf.flowIn(1), 3);
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

  uf.compute();

  for (int u = 0; u < n; ++u)
    EXPECT_EQ(uf.excess(u), 0) << "Left hand side should not have any excess";
  for (int u = 0; u < m; ++u)
    EXPECT_EQ(uf.excess(n + u), 0)
        << "Right hand side should not have any excess";
}
