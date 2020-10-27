#include "gtest/gtest.h"

#include "lib/datastructures/linkcut.hpp"

#include <algorithm>

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
  constexpr int n = (1 << 5) - 1;
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
  constexpr int n = (1 << 8) - 1;
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

/**
   Each vertex in a disconnected forest should have return itself with
   'findPathMin'.
 */
TEST(LinkCut, FindPathMinOfDisconnectedForest) {
  constexpr int n = 10;
  LinkCut::Forest forest(n);

  for (int u = 0; u < n; ++u) {
    auto [w, r] = forest.findPathMin(u);
    EXPECT_EQ(w, 0);
    EXPECT_EQ(r, u);
  }

  for (int u = 0; u < n; ++u)
    forest.updatePath(u, u);

  for (int u = 0; u < n; ++u) {
    auto [w, r] = forest.findPathMin(u);
    EXPECT_EQ(w, u);
    EXPECT_EQ(r, u);
  }
}

/**
   Create single path, increment out of order, query 'findPathMin'.
 */
TEST(LinkCut, FindPathMinOfPath) {
  constexpr int n = 10;
  LinkCut::Forest path(n);

  for (int u = 1; u < n; ++u)
    path.link(u, u - 1, 0);

  path.updatePath(0, 5);
  {
    auto [w, r] = path.findPathMin(0);
    EXPECT_EQ(w, 5);
    EXPECT_EQ(r, 0);
  }
  path.updatePath(5, 3);
  {
    auto [w, r] = path.findPathMin(5);
    EXPECT_EQ(w, 3);
    EXPECT_EQ(r, 1);
  }
  {
    auto [w, r] = path.findPathMin(7);
    EXPECT_EQ(w, 0);
    EXPECT_EQ(r, 6);
  }
  {
    auto [w, r] = path.findPathMin(6);
    EXPECT_EQ(w, 0);
    EXPECT_EQ(r, 6);
  }
}

/**
   Call 'link' with non-zero weight and verify that the weight is placed on the
   'from' vertex.
 */
TEST(LinkCut, LinkNonZeroWeight) {
  LinkCut::Forest forest(2);
  forest.link(0, 1, 42);

  {
    auto [w, r] = forest.findPathMin(0);
    EXPECT_EQ(w, 0);
    EXPECT_EQ(r, 1);
  }
  {
    auto [w, r] = forest.findPathMin(1);
    EXPECT_EQ(w, 0);
    EXPECT_EQ(r, 1);
  }
}

/**
   Construct balanced tree with three vertices, perform 'findPathMin' queries.

     0
    / \
   1  2
 */
TEST(LinkCut, LinkTwiceNonZeroWeight) {
  LinkCut::Forest forest(3);
  forest.updatePath(0, 1000);
  forest.link(1, 0, 42);
  forest.link(2, 0, 314);

  {
    auto [w, r] = forest.findPathMin(0);
    EXPECT_EQ(w, 1000);
    EXPECT_EQ(r, 0);
  }
  {
    auto [w, r] = forest.findPathMin(1);
    EXPECT_EQ(w, 42);
    EXPECT_EQ(r, 1);
  }
  {
    auto [w, r] = forest.findPathMin(2);
    EXPECT_EQ(w, 314);
    EXPECT_EQ(r, 2);
  }
}

/**
   Create single path using non-zero weight to 'link' call. Query 'findPathMin'.
 */
TEST(LinkCut, FindPathMinOfLinkPath) {
  constexpr int n = 10;
  LinkCut::Forest path(n);
  path.updatePath(0, 1000);

  path.link(1, 0, 8);
  path.link(2, 1, 9);
  path.link(3, 2, 1);
  path.link(4, 3, 1);
  path.link(5, 4, 18);
  path.link(6, 5, -1);
  path.link(7, 6, 9);
  path.link(8, 7, 3);
  path.link(9, 8, 2);

  {
    auto [w, r] = path.findPathMin(6);
    EXPECT_EQ(w, -1);
    EXPECT_EQ(r, 6);
  }
  {
    auto [w, r] = path.findPathMin(4);
    EXPECT_EQ(w, 1);
    EXPECT_EQ(r, 3);
  }
  {
    auto [w, r] = path.findPathMin(9);
    EXPECT_EQ(w, -1);
    EXPECT_EQ(r, 6);
  }
  {
    auto [w, r] = path.findPathMin(0);
    EXPECT_EQ(w, 1000);
    EXPECT_EQ(r, 0);
  }
  {
    auto [w, r] = path.findPathMin(5);
    EXPECT_EQ(w, 1);
    EXPECT_EQ(r, 3);
  }
  {
    auto [w, r] = path.findPathMin(3);
    EXPECT_EQ(w, 1);
    EXPECT_EQ(r, 3);
  }
}

/**
   Supports link cut tree operations in linear time instead of logarithmic.
 */
struct NaiveTree {
  std::vector<int> parent;
  std::vector<int> weight;
  NaiveTree(int n) : parent(n, -1), weight(n) {}
  int get(int u) { return weight[u]; }
  void set(int u, int value) { weight[u] = value; }
  void link(int u, int v, int delta) {
    parent[u] = v;
    weight[u] += delta;
  }
  void cut(int u) { parent[u] = -1; }
  void updatePath(int u, int delta) {
    weight[u] += delta;
    u = parent[u];
    if (u != -1)
      updatePath(u, delta);
  }
  void updatePathEdges(int u, int delta) {
    while (parent[u] != -1) {
      weight[u] += delta;
      u = parent[u];
    }
  }
  std::pair<int, int> findPathMin(int u) {
    if (parent[u] == -1)
      return {weight[u], u};

    auto [w, v] = findPathMin(parent[u]);
    if (w <= weight[u])
      return {w, v};
    else
      return {weight[u], u};
  }
  int findRoot(int u) {
    if (parent[u] == -1)
      return u;
    else
      return findRoot(parent[u]);
  }
  int findRootEdge(int u) {
    while (parent[u] != -1 && parent[parent[u]] != -1)
      u = parent[u];
    return u;
  }
  int findParent(int u) {
    return parent[u];
  }
};

/**
   Maintain a link cut tree and a naive link cut tree simultaneously and run
   random operations on them using a fixed seed. Using the invariant u > v when
   linking u to v it is guaranteed that the graph is a forest.
 */
TEST(LinkCut, StressTest) {
  std::srand(0);
  constexpr int n = 1000, queries = 10000000;

  LinkCut::Forest forest(n);
  NaiveTree naive(n);

  for (int query = 0; query < queries; ++query) {
    int op = rand() % 9;

    switch (op) {
    case 0: { // Get
      int u = rand() % n;
      EXPECT_EQ(forest.get(u), naive.get(u));
      break;
    }
    case 1: { // Set
      int u = rand() % n, v = (rand() % 1000) - 500;
      forest.set(u, v), naive.set(u, v);
      break;
    }
    case 2: { // Link or cut depending on if 'u' already has a parent.
      int u = rand() % n;
      int v;
      do
        v = rand() % n;
      while (u == v);
      if (u < v)
        std::swap(u, v);

      if (naive.parent[u] == -1) {
        int w = (rand() % (1 << 10)) - (1 << 9);
        forest.link(u, v, w);
        naive.link(u, v, w);
      } else {
        forest.cut(u);
        naive.cut(u);
      }
      break;
    }
    case 3: { // Find root
      int u = rand() % n;
      EXPECT_EQ(forest.findRoot(u), naive.findRoot(u));
      break;
    }
    case 4: { // Find edge root
      int u = rand() % n;
      if (naive.parent[u] != -1)
        EXPECT_EQ(forest.findRootEdge(u), naive.findRootEdge(u));
      break;
    }
    case 5: { // Find path min
      int u = rand() % n;
      EXPECT_EQ(forest.findPathMin(u), naive.findPathMin(u));
      break;
    }
    case 6: { // Update path
      int u = rand() % n;
      int w = (rand() % (1 << 10)) - (1 << 9);
      forest.updatePath(u, w), naive.updatePath(u, w);
      break;
    }
    case 7: { // Update path edges
      int u = rand() % n;
      if (naive.parent[u] != -1) {
        int w = (rand() % (1 << 10)) - (1 << 9);
        forest.updatePath(u, w), naive.updatePath(u, w);
      }
      break;
    }
    case 8: { // Compare parents.
      int u = rand() % n;
      EXPECT_EQ(forest.findParent(u), naive.findParent(u));
      break;
    }
    default: {
      assert(false && "Default not implemented.");
    }
    }
  }
}
