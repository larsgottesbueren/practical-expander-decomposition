#include "gtest/gtest.h"

#include "lib/partition_graph.hpp"

TEST(PartitionGraph, SingleVertex) {
  PartitionGraph<int, Edge> g(1);

  EXPECT_EQ(g.size(), 1);
  EXPECT_EQ(g.size(0), 1);
  EXPECT_EQ(g.edgeCount(), 0);
  EXPECT_EQ(g.edgeCount(0), 0);
  EXPECT_EQ(g.getPartition(0), 0);
}

TEST(PartitionGraph, AddEdge) {
  PartitionGraph<int, Edge> g(2);

  EXPECT_EQ(g.size(), 2);
  EXPECT_EQ(g.size(0), 2);
  EXPECT_EQ(g.edgeCount(), 0);
  EXPECT_EQ(g.edgeCount(0), 0);

  g.addEdge(0, 1);

  EXPECT_EQ(g.edgeCount(), 1);
  EXPECT_EQ(g.edgeCount(0), 1);
  EXPECT_EQ(g.getPartition(0), 0);
  EXPECT_EQ(g.getPartition(1), 0);
}

TEST(PartitionGraph, AddEdgeAndPartition) {
  PartitionGraph<int, Edge> g(2);

  EXPECT_EQ(g.size(), 2);
  EXPECT_EQ(g.size(0), 2);
  EXPECT_EQ(g.edgeCount(), 0);
  EXPECT_EQ(g.edgeCount(0), 0);

  g.addEdge(0, 1);

  EXPECT_EQ(g.edgeCount(), 1);
  EXPECT_EQ(g.edgeCount(0), 1);
  EXPECT_EQ(g.neighbors(0), (std::vector<int>{1}));
  EXPECT_EQ(g.neighbors(1), (std::vector<int>{0}));
  EXPECT_EQ(g.partitionNeighbors(0), (std::vector<int>{1}));
  EXPECT_EQ(g.partitionNeighbors(1), (std::vector<int>{0}));

  g.newPartition({0}, {0, 1});
  EXPECT_EQ(g.getPartition(0), 1);
  EXPECT_EQ(g.getPartition(1), 0);
  EXPECT_EQ(g.size(), 2);
  EXPECT_EQ(g.size(0), 1);
  EXPECT_EQ(g.size(1), 1);
  EXPECT_EQ(g.edgeCount(), 1);
  EXPECT_EQ(g.edgeCount(0), 0);
  EXPECT_EQ(g.edgeCount(1), 0);
  EXPECT_EQ(g.degree(0), 1);
  EXPECT_EQ(g.degree(1), 1);
  EXPECT_EQ(g.partitionDegree(0), 0);
  EXPECT_EQ(g.partitionDegree(1), 0);
  EXPECT_EQ(g.neighbors(0), (std::vector<int>{1}));
  EXPECT_EQ(g.neighbors(1), (std::vector<int>{0}));
  EXPECT_TRUE(g.partitionNeighbors(0).empty());
  EXPECT_TRUE(g.partitionNeighbors(1).empty());
}

TEST(PartitionGraph, NewPartitionMaintainsEdges) {
  PartitionGraph<int, Edge> g(6);
  g.addEdge(0, 1), g.addEdge(1, 2), g.addEdge(2, 0);
  g.addEdge(3, 4), g.addEdge(4, 5), g.addEdge(5, 3);
  g.addEdge(0, 3);

  g.newPartition({3, 4, 5}, {0, 1, 2, 3, 4, 5});
  EXPECT_EQ(g.size(), 6);
  EXPECT_EQ(g.size(0), 3);
  EXPECT_EQ(g.size(1), 3);

  EXPECT_EQ(g.edgeCount(), 7);
  EXPECT_EQ(g.edgeCount(0), 3);
  EXPECT_EQ(g.edgeCount(1), 3);

  for (int u = 0; u < 3; ++u)
    EXPECT_EQ(g.getPartition(u), 0);

  for (int u = 3; u < 6; ++u)
    EXPECT_EQ(g.getPartition(u), 1);

  for (int u = 0; u < 6; ++u)
    EXPECT_EQ(g.partitionDegree(u), 2);

  EXPECT_EQ(g.degree(0), 3);
  EXPECT_EQ(g.degree(1), 2);
  EXPECT_EQ(g.degree(2), 2);

  EXPECT_EQ(g.degree(3), 3);
  EXPECT_EQ(g.degree(4), 2);
  EXPECT_EQ(g.degree(5), 2);
}
