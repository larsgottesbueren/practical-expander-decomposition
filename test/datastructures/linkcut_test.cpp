#include "gtest/gtest.h"

#include "lib/datastructures/linkcut.hpp"

TEST(LinkCut, ConstructEmpty) {
  constexpr int n = 10;
  LinkCut::Forest forest(n);
  for (int u = 0; u < n; ++u)
    for (int v = u + 1; v < n; ++v)
      EXPECT_FALSE(forest.connected(u, v));
}

TEST(LinkCut, LinkSingle) {
  constexpr int n = 10;
  LinkCut::Forest forest(n);

  forest.link(0, 1, 50);
  EXPECT_TRUE(forest.connected(0, 1));
  EXPECT_EQ(forest.findRoot(0), 1);
  EXPECT_EQ(forest.findRoot(1), 1);
}

TEST(LinkCut, LinkThree) {
  constexpr int n = 3;
  LinkCut::Forest forest(n);

  forest.link(0, 2, 50), forest.link(1, 2, 30);
  EXPECT_TRUE(forest.connected(0, 1));
  EXPECT_TRUE(forest.connected(0, 2));
  EXPECT_TRUE(forest.connected(1, 2));
}

TEST(LinkCut, LinkPath) {
  constexpr int n = 10;
  LinkCut::Forest forest(n);

  for (int u = 0; u < n - 1; ++u)
    forest.link(u, u + 1, 50);

  for (int u = 0; u < n; ++u)
    EXPECT_EQ(forest.findRoot(u), n - 1);

  for (int u = 0; u < n; ++u)
    for (int v = 0; v < n; ++v)
      EXPECT_TRUE(forest.connected(u, v));
}

TEST(LinkCut, LinkThenCutSingle) {
  LinkCut::Forest forest(2);

  forest.link(0, 1, 50);
  EXPECT_TRUE(forest.connected(0, 1));
  forest.cut(0);
  EXPECT_FALSE(forest.connected(0, 1));
}

TEST(LinkCut, LinkThenCutPath) {
  constexpr int n = 10;
  LinkCut::Forest forest(n);

  for (int u = 0; u < n - 1; ++u)
    forest.link(u, u + 1, 42);

  forest.cut(n / 2);

  for (int u = 0; u <= n / 2; ++u)
    for (int v = n / 2 + 1; v < n; ++v)
      EXPECT_FALSE(forest.connected(u, v));
}

/**
   Construct a balanced binary tree. Check that all vertices share the same
   root.
 */
TEST(LinkCut, LinkBinaryTree) {
  constexpr int n = 1 << 5 - 1;
  LinkCut::Forest forest(n);

  // Use 1-indexing => children of node 'p' are '2p' and '2p+1'
  for (int u = 2; u <= n; ++u) {
    int p = u / 2;
    forest.link(u - 1, p - 1, 42);
  }

  for (int u = n - 1; u >= 0; --u)
    EXPECT_EQ(forest.findRoot(u), 0);
}

/**
   Construct a balanced binary tree, then remove edges in middle layer.
 */
TEST(LinkCut, LinkThenCutBinaryTree) {
  constexpr int n = 1 << 8 - 1;
  LinkCut::Forest forest(n);

  // Use 1-indexing => children of node 'p' are '2p' and '2p+1'
  for (int u = 2; u <= n; ++u) {
    int p = u / 2;
    forest.link(u - 1, p - 1, 42);
  }

  // Remove layers between layer 2 and 3.
  for (int u = 1 << 3; u < 1 << 4; ++u)
    forest.cut(u);

  EXPECT_TRUE(forest.connected((1 << 3) * 2, ((1 << 3) * 2 + 1) * 2));
  EXPECT_FALSE(forest.connected((1 << 3) * 2, (1 << 3) + 2));
  EXPECT_TRUE(forest.connected(1 << 2, (1 << 2) + 1));
}
