#include "gtest/gtest.h"

#include "lib/cut_matching.hpp"

/**
   Test that sub-division graph is constructed correctly for the following small
   graph:

   0 -- 1
   | \  |
   |  \ |
   3 -- 2
 */
TEST(CutMatching, ConstructSimple) {
  Graph g(4);
  g.addEdge(0, 1);
  g.addEdge(1, 2);
  g.addEdge(2, 3);
  g.addEdge(3, 0);
  g.addEdge(0, 2);

  CutMatching cm(g);

  EXPECT_EQ(cm.numRegularNodes, 4);
  EXPECT_EQ(cm.numSplitNodes, 5);
}

/**
   Test that duplicate edges aren't forgotten.
 */
TEST(CutMatching, ConstructSimpleDuplicateEdges) {
  Graph g(4);
  g.addEdge(0, 1);
  g.addEdge(1, 2);
  g.addEdge(2, 3);
  g.addEdge(2, 3);
  g.addEdge(3, 0);
  g.addEdge(0, 3);
  g.addEdge(0, 2);

  CutMatching cm(g);

  EXPECT_EQ(cm.numRegularNodes, 4);
  EXPECT_EQ(cm.numSplitNodes, 7);
}

/**
   Test that graph containing loops hits an assertion.
 */
TEST(CutMatching, ConstructSimpleLoop) {
  Graph g(2);
  g.addEdge(0, 0);
  g.addEdge(0, 1);
  g.addEdge(1, 1);

  EXPECT_DEATH((CutMatching(g)),
               "No loops allowed when constructing sub-division graph.");
}

TEST(CutMatching, ConstructLargeRandom) {
  const int n = 100;
  const int m = 3 * n;

  Graph g(n);
  for (int i = 0; i < m; ++i) {
    int u = std::rand() % n, v = u;
    do {
      v = std::rand() % n;
    } while (u == v);
    g.addEdge(u, v);
  }

  CutMatching cm(g);

  EXPECT_EQ(cm.numRegularNodes, n);
  EXPECT_EQ(cm.numSplitNodes, m);
}

/*
TEST(CutMatching, PartitionsSimplePath) {
  const int n = 10;
  Graph g(n);
  for (int i = 0; i < n - 1; ++i)
    g.addEdge(i, i + 1);

  CutMatching cm(g);

  EXPECT_EQ(cm.numRegularNodes, n);
  EXPECT_EQ(cm.numSplitNodes, n - 1);

  auto [left, right] = cm.compute(0.5);
  EXPECT_FALSE(left.empty());
  EXPECT_FALSE(right.empty());
}
*/
/**
   Two large complete graphs with a couple edges in-between should be easy to
   partition. Make sure 'most' vertices are partitioned correctly.
*/
/*
TEST(CutMatching, PartitionTwoClusters) {
  const int n = 100, m = 100;

  Graph g(n+m);
  for (int i = 0; i < n; ++i)
    for (int j = i+1; j < n; ++j)
      g.addEdge(i,j);

  for (int i = n; i < n+m; ++i)
    for (int j = i+1; j < n+m; ++j)
      g.addEdge(i,j);

  g.addEdge(0,n);
  g.addEdge(1,n+1);

  CutMatching cm(g);

  EXPECT_EQ(cm.numRegularNodes, n+m);
  EXPECT_EQ(cm.numSplitNodes, (n-1)*n/2 + (m-1)*m/2 + 2);

  auto [left, right] = cm.compute(0.5);
  std::vector<Vertex> leftCorrect, rightCorrect;
  for (auto u : left)
    if (u < n) leftCorrect.push_back(u);
  for (auto u : right)
    if (u >= n) rightCorrect.push_back(u);

  EXPECT_GE((double)leftCorrect.size() / (double)left.size(), 0.9);
  EXPECT_GE((double)rightCorrect.size() / (double)right.size(), 0.9);
}
*/
