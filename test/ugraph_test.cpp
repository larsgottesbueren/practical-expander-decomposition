#include "gtest/gtest.h"

#include "lib/ugraph.hpp"

TEST(UndirectedGraph, SingleVertex) {
  Undirected::Graph g(1);

  EXPECT_EQ(g.size(), 1);
  EXPECT_EQ(g.size(0), 1);
  EXPECT_EQ(g.edgeCount(), 0);
  EXPECT_EQ(g.edgeCount(0), 0);
  EXPECT_EQ(g.getPartition(0), 0);
}

/**
   Add an undirected edge between two vertices.
 */
TEST(UndirectedGraph, AddEdge) {
  Undirected::Graph g(2);

  g.addEdge(0, 1);

  EXPECT_EQ(g.edgeCount(), 1);
  EXPECT_EQ(g.edgeCount(0), 1);
  EXPECT_EQ(g.getPartition(0), 0);
  EXPECT_EQ(g.getPartition(1), 0);
  EXPECT_EQ(g.neighbors(0), (std::vector<int>{1}));
  EXPECT_EQ(g.neighbors(1), (std::vector<int>{0}));
}

/**
   Adding the same undirected edge twice results in two distinct undirected
   edges.
 */
TEST(UndirectedGraph, AddEdgeTwice) {
  Undirected::Graph g(2);

  g.addEdge(0, 1);
  g.addEdge(0, 1);

  EXPECT_EQ(g.edgeCount(), 2);
  EXPECT_EQ(g.edgeCount(0), 2);
  EXPECT_EQ(g.getPartition(0), 0);
  EXPECT_EQ(g.getPartition(1), 0);
  EXPECT_EQ(g.neighbors(0), (std::vector<int>{1, 1}));
  EXPECT_EQ(g.neighbors(1), (std::vector<int>{0, 0}));
}

/**
   Add an edge in a graph with two vertices, then partition it.
 */
TEST(UndirectedGraph, AddEdgeAndPartition) {
  Undirected::Graph g(2);

  EXPECT_EQ(g.size(), 2);
  EXPECT_EQ(g.size(0), 2);
  EXPECT_EQ(g.edgeCount(), 0);
  EXPECT_EQ(g.edgeCount(0), 0);

  g.addEdge(0, 1);

  EXPECT_EQ(g.edgeCount(), 1);
  EXPECT_EQ(g.edgeCount(0), 1);
  EXPECT_EQ(g.neighbors(0), (std::vector<int>{1}));
  EXPECT_EQ(g.neighbors(1), (std::vector<int>{0}));

  g.newPartition({0}, {0, 1});
  EXPECT_EQ(g.getPartition(0), 1);
  EXPECT_EQ(g.getPartition(1), 0);
  EXPECT_EQ(g.size(), 2);
  EXPECT_EQ(g.size(0), 1);
  EXPECT_EQ(g.size(1), 1);
  EXPECT_EQ(g.edgeCount(), 1);
  EXPECT_EQ(g.edgeCount(0), 0);
  EXPECT_EQ(g.edgeCount(1), 0);
  EXPECT_EQ(g.degree(0), 0);
  EXPECT_EQ(g.degree(1), 0);
  EXPECT_EQ(g.globalDegree(0), 1);
  EXPECT_EQ(g.globalDegree(1), 1);
  EXPECT_TRUE(g.neighbors(0).empty());
  EXPECT_TRUE(g.neighbors(1).empty());
}

/**
   Construct a graph with six vertices and partition it in two equal size sub
   graphs.
 */
TEST(UndirectedGraph, NewPartitionMaintainsEdges) {
  Undirected::Graph g(6);
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
    EXPECT_EQ(g.degree(u), 2);

  EXPECT_EQ(g.globalDegree(0), 3);
  EXPECT_EQ(g.globalDegree(1), 2);
  EXPECT_EQ(g.globalDegree(2), 2);

  EXPECT_EQ(g.globalDegree(3), 3);
  EXPECT_EQ(g.globalDegree(4), 2);
  EXPECT_EQ(g.globalDegree(5), 2);
}
