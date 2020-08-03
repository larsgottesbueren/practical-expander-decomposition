#include "gtest/gtest.h"

#include "lib/unit_flow.hpp"

TEST(UnitFlow, TwoVertexFlow) {
  UnitFlow uf(2, INT_MAX);
  uf.addSource(0, 10);
  uf.addSink(1, 10);
  uf.addEdge(0, 1, 10);

  uf.compute();

  EXPECT_EQ(uf.getAbsorbed(0), 0);
  EXPECT_EQ(uf.getAbsorbed(1), 10);
}

TEST(UnitFlow, TwoVertexFlowSmallEdge) {
  UnitFlow uf(2, INT_MAX);
  uf.addSource(0, 10);
  uf.addSink(1, 10);
  uf.addEdge(0, 1, 4);
  uf.compute();

  EXPECT_EQ(uf.getAbsorbed(0), 6);
  EXPECT_EQ(uf.getAbsorbed(1), 4);
}
