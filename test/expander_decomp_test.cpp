#include "gtest/gtest.h"

#include "lib/expander_decomp.hpp"
#include "lib/ugraph.hpp"

TEST(ConstructFlowGraph, EmptyGraph) {
  const auto g = std::make_unique<Undirected::Graph>(0);
  const auto f = ExpanderDecomposition::constructFlowGraph(g);
  EXPECT_EQ(f->size(), 0);
  EXPECT_EQ(f->edgeCount(), 0);
}

TEST(ConstructFlowGraph, SingleVertex) {
  const auto g = std::make_unique<Undirected::Graph>(1);
  const auto f = ExpanderDecomposition::constructFlowGraph(g);
  EXPECT_EQ(f->size(), 1);
  EXPECT_EQ(f->edgeCount(), 0);
}

TEST(ConstructFlowGraph, SingleEdge) {
  auto g = std::make_unique<Undirected::Graph>(2);
  g->addEdge(0, 1);
  const auto f = ExpanderDecomposition::constructFlowGraph(g);
  EXPECT_EQ(f->size(), 2);
  EXPECT_EQ(f->edgeCount(), 1);
  EXPECT_EQ(f->edges(0).size(), 1);
  EXPECT_EQ(f->edges(1).size(), 1);

  const auto &e01 = f->edges(0)[0];
  const auto &e10 = f->edges(1)[0];

  EXPECT_EQ(e01->from, 0);
  EXPECT_EQ(e01->to, 1);
  EXPECT_EQ(e01->flow, 0);
  EXPECT_EQ(e01->capacity, 0);

  EXPECT_EQ(e10->from, 1);
  EXPECT_EQ(e10->to, 0);
  EXPECT_EQ(e10->flow, 0);
  EXPECT_EQ(e10->capacity, 0);
}

TEST(ConstructFlowGraph, BasicGraph) {
  const int n = 10;
  auto g = std::make_unique<Undirected::Graph>(n);
  g->addEdge(0, 1);
  g->addEdge(0, 2);
  g->addEdge(2, 5);
  g->addEdge(3, 2);
  g->addEdge(6, 1);
  g->addEdge(6, 7);
  g->addEdge(7, 8);
  g->addEdge(9, 2);
  g->addEdge(9, 8);
  const auto f = ExpanderDecomposition::constructFlowGraph(g);
  EXPECT_EQ(f->size(), n);
  EXPECT_EQ(f->edgeCount(), g->edgeCount());

  for (int u = 0; u < f->size(); ++u)
    EXPECT_EQ(f->neighbors(u), g->neighbors(u));
}

TEST(ConstructSubdivisionFlowGraph, EmptyGraph) {
  const auto g = std::make_unique<Undirected::Graph>(0);
  const auto f = ExpanderDecomposition::constructSubdivisionFlowGraph(g);
  EXPECT_EQ(f->size(), 0);
  EXPECT_EQ(f->edgeCount(), 0);
}

TEST(ConstructSubdivisionFlowGraph, SingleVertex) {
  const auto g = std::make_unique<Undirected::Graph>(1);
  const auto f = ExpanderDecomposition::constructSubdivisionFlowGraph(g);
  EXPECT_EQ(f->size(), 1);
  EXPECT_EQ(f->edgeCount(), 0);
}

TEST(ConstructSubdivisionFlowGraph, SingleEdge) {
  auto g = std::make_unique<Undirected::Graph>(2);
  g->addEdge(0, 1);
  const auto f = ExpanderDecomposition::constructSubdivisionFlowGraph(g);
  EXPECT_EQ(f->size(), 3);
  EXPECT_EQ(f->edgeCount(), 2);
  EXPECT_EQ(f->edges(0).size(), 1);
  EXPECT_EQ(f->edges(1).size(), 1);
  EXPECT_EQ(f->edges(2).size(), 2); // Split vertex

  const auto &e02 = f->edges(0)[0];
  const auto &e12 = f->edges(1)[0];

  EXPECT_EQ(e02->from, 0);
  EXPECT_EQ(e02->to, 2);
  EXPECT_EQ(e02->flow, 0);
  EXPECT_EQ(e02->capacity, 0);

  EXPECT_EQ(e12->from, 1);
  EXPECT_EQ(e12->to, 2);
  EXPECT_EQ(e12->flow, 0);
  EXPECT_EQ(e12->capacity, 0);
}

TEST(ConstructSubdivisionFlowGraph, BasicGraph) {
  const int n = 10;
  auto g = std::make_unique<Undirected::Graph>(n);
  g->addEdge(0, 1);
  g->addEdge(0, 2);
  g->addEdge(2, 5);
  g->addEdge(3, 2);
  g->addEdge(6, 1);
  g->addEdge(6, 7);
  g->addEdge(7, 8);
  g->addEdge(9, 2);
  g->addEdge(9, 8);
  const auto f = ExpanderDecomposition::constructSubdivisionFlowGraph(g);
  EXPECT_EQ(f->size(), n + g->edgeCount());
  EXPECT_EQ(f->edgeCount(), 2 * g->edgeCount());
}
