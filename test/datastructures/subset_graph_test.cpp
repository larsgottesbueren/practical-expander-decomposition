#include "gtest/gtest.h"

#include "lib/datastructures/subset_graph.hpp"

#include <iostream>

/**
   Basic undirected edge.
 */
struct Edge {
  int from, to, revIdx;

  /**
     Construct an edge 'from->to'. 'revIdx' remains undefined.
   */
  Edge(int from, int to) : from(from), to(to), revIdx(-1) {}

  /**
     Construct the reverse of this edge. 'revIdx' remains undefined since it is
     maintained by the graph representation.
   */
  Edge reverse() const {
    Edge e{to, from};
    return e;
  }
};

using Graph = SubsetGraph::Graph<int, Edge>;

TEST(SubsetGraph, ConstructEmpty) {
  Graph g(0, {});

  EXPECT_EQ(g.size(), 0);
  EXPECT_EQ(g.volume(), 0);
}

/**
   Construct a small graph and verify that all edges and vertices are present.
 */
TEST(SubsetGraph, ConstructSmall) {
  const int n = 10;
  const std::vector<Edge> es = {{0, 1}, {0, 2}, {1, 2}, {2, 3}, {3, 4}, {4, 5},
                                {0, 5}, {6, 7}, {6, 8}, {7, 8}, {7, 9}};
  Graph g(n, es);

  ASSERT_EQ(g.size(), n);
  ASSERT_EQ(g.edgeCount(), int(es.size()));

  std::set<int> vsLeft;
  for (int u = 0; u < n; ++u)
    vsLeft.insert(u);
  for (auto u : g) {
    ASSERT_FALSE(vsLeft.find(u) == vsLeft.end());
    vsLeft.erase(u);
  }
  ASSERT_TRUE(vsLeft.empty());

  std::set<std::pair<int, int>> esLeft;
  for (auto e : es)
    esLeft.insert({e.from, e.to}), esLeft.insert({e.to, e.from});
  for (auto u : g) {
    for (auto e = g.beginEdge(u); e != g.endEdge(u); ++e) {
      ASSERT_FALSE(esLeft.find({e->from, e->to}) == esLeft.end());
      esLeft.erase({e->from, e->to});
    }
  }
  ASSERT_TRUE(esLeft.empty());
}

/**
   Test that 'reverse' returns the correct edge.
 */
TEST(SubsetGraph, Reverse) {
  const int n = 4;
  const std::vector<Edge> es = {{0, 1}, {1, 2}, {0, 2}, {0, 3}};

  Graph g(n, es);

  for (auto u : g) {
    for (auto e = g.beginEdge(u); e != g.endEdge(u); ++e) {
      ASSERT_NE(e->revIdx, -1);
      auto re = g.reverse(*e);
      ASSERT_EQ(e->to, re.from);
      ASSERT_EQ(e->from, re.to);
    }
  }
}

/**
   Test 'connectedComponents' finds all three components in a small graph.
 */
TEST(SubsetGraph, ConnectedComponents) {
  const int n = 10;
  const std::vector<Edge> es = {{0, 1}, {0, 2}, {0, 3}, {1, 2},
                                {4, 5}, {5, 6}, {6, 7}, {7, 8}};
  Graph g(n, es);

  auto comps = g.connectedComponents();
  ASSERT_EQ(int(comps.size()), 3);

  for (auto comp : comps) {
    std::sort(comp.begin(), comp.end());
    if (comp.size() == 1)
      ASSERT_EQ(comp, std::vector<int>({9}));
    else if (comp.size() == 4)
      ASSERT_EQ(comp, std::vector<int>({0, 1, 2, 3}));
    else
      ASSERT_EQ(comp, std::vector<int>({4, 5, 6, 7, 8}));
  }
}

/**
   Remove vertex from graph, test that the graph is now disconnected.
 */
TEST(SubsetGraph, RemoveSingle) {
  const int n = 5;
  const std::vector<Edge> es = {{0, 1}, {0, 2}, {1, 2}, {2, 3}, {2, 4}, {3, 4}};
  Graph g(n, es);

  ASSERT_EQ(int(g.connectedComponents().size()), 1);
  g.remove(2);
  ASSERT_EQ(int(g.connectedComponents().size()), 2);

  EXPECT_EQ(g.degree(0), 1);
  EXPECT_EQ(g.degree(1), 1);
  EXPECT_EQ(g.degree(2), 0);
  EXPECT_EQ(g.degree(3), 1);
  EXPECT_EQ(g.degree(4), 1);
}

/**
   Remove every other vertex in path.
 */
TEST(SubsetGraph, RemoveSeveralInPath) {
  const int n = 10;
  std::vector<Edge> es;
  for (int i = 0; i < n - 1; ++i)
    es.emplace_back(i, i + 1);
  Graph g(n, es);

  EXPECT_EQ(int(g.connectedComponents().size()), 1);
  g.remove(0);
  EXPECT_EQ(int(g.connectedComponents().size()), 1);
  g.remove(2);
  EXPECT_EQ(int(g.connectedComponents().size()), 2);
  g.remove(8);
  EXPECT_EQ(int(g.connectedComponents().size()), 3);
  g.remove(6);
  EXPECT_EQ(int(g.connectedComponents().size()), 4);
  g.remove(4);
  EXPECT_EQ(int(g.connectedComponents().size()), 5);

  std::set<int> us;
  for (auto u : g)
    us.insert(u);

  EXPECT_EQ(us, std::set<int>({1, 3, 5, 7, 9}));

  for (auto u : g)
    EXPECT_EQ(g.degree(u), 0);
}

/**
   Remove vertices from small graph.
 */
TEST(SubsetGraph, RemoveSeveral) {
  const int n = 6;
  const std::vector<Edge> es = {{0, 1}, {0, 2}, {1, 2}, {2, 3},
                                {2, 4}, {3, 4}, {4, 5}};
  Graph g(n, es);

  EXPECT_EQ(int(g.connectedComponents().size()), 1);
  g.remove(0);
  EXPECT_EQ(int(g.connectedComponents().size()), 1);
  g.remove(4);
  EXPECT_EQ(int(g.connectedComponents().size()), 2);
  g.remove(2);
  EXPECT_EQ(int(g.connectedComponents().size()), 3);
}

TEST(SubsetGraph, SubgraphEmpty) {
  const int n = 4;
  const std::vector<Edge> es = {{0, 1}, {0, 2}, {2, 3}};

  Graph g(n, es);

  std::vector<int> subset;
  g.subgraph(subset.begin(), subset.end());

  EXPECT_EQ(g.size(), 0);
  EXPECT_EQ(g.volume(), 0);
}

TEST(SubsetGraph, SubgraphSimple) {
  const int n = 6;
  const std::vector<Edge> es = {{0, 1}, {0, 2}, {1, 2}, {2, 3},
                                {2, 4}, {3, 4}, {4, 5}};
  Graph g(n, es);

  std::vector<int> subset = {0, 1, 2, 3};
  g.subgraph(subset.begin(), subset.end());

  EXPECT_EQ(g.size(), 4);
  EXPECT_EQ(g.edgeCount(), 4);
  std::set<int> seen;
  for (auto u : g)
    seen.insert(u);
  EXPECT_EQ(seen, std::set<int>(subset.begin(), subset.end()));
}
