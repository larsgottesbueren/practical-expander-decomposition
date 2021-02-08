#include "gtest/gtest.h"

#include "lib/datastructures/undirected_graph.hpp"
#include "lib/expander_decomp.hpp"

TEST(ConstructFlowGraph, EmptyGraph) {
  const auto g =
      std::make_unique<Undirected::Graph>(0, std::vector<Undirected::Edge>());
  const auto f = ExpanderDecomposition::constructFlowGraph(g);
  EXPECT_EQ(f->size(), 0);
  EXPECT_EQ(f->edgeCount(), 0);
}

TEST(ConstructFlowGraph, SingleVertex) {
  const auto g =
      std::make_unique<Undirected::Graph>(1, std::vector<Undirected::Edge>());
  const auto f = ExpanderDecomposition::constructFlowGraph(g);
  EXPECT_EQ(f->size(), 1);
  EXPECT_EQ(f->edgeCount(), 0);
}

TEST(ConstructFlowGraph, SingleEdge) {
  auto g = std::make_unique<Undirected::Graph>(
      2, std::vector<Undirected::Edge>({{0, 1}}));
  const auto f = ExpanderDecomposition::constructFlowGraph(g);
  EXPECT_EQ(f->size(), 2);
  EXPECT_EQ(f->edgeCount(), 1);
  EXPECT_EQ(f->degree(0), 1);
  EXPECT_EQ(f->degree(1), 1);

  const auto &e01 = f->getEdge(0, 0);
  const auto &e10 = f->getEdge(1, 0);

  EXPECT_EQ(e01.from, 0);
  EXPECT_EQ(e01.to, 1);
  EXPECT_EQ(e01.flow, 0);
  EXPECT_EQ(e01.capacity, 0);

  EXPECT_EQ(e10.from, 1);
  EXPECT_EQ(e10.to, 0);
  EXPECT_EQ(e10.flow, 0);
  EXPECT_EQ(e10.capacity, 0);
}

TEST(ConstructFlowGraph, BasicGraph) {
  const int n = 10;
  const std::vector<Undirected::Edge> es = {
      {0, 1}, {0, 2}, {2, 5}, {3, 2}, {6, 1}, {6, 7}, {7, 8}, {9, 2}, {9, 8}};
  auto g = std::make_unique<Undirected::Graph>(n, es);
  const auto f = ExpanderDecomposition::constructFlowGraph(g);
  EXPECT_EQ(f->size(), n);
  EXPECT_EQ(f->edgeCount(), g->edgeCount());

  for (int u = 0; u < f->size(); ++u)
    EXPECT_EQ(f->neighbors(u), g->neighbors(u));
}

TEST(ConstructSubdivisionFlowGraph, EmptyGraph) {
  const auto g =
      std::make_unique<Undirected::Graph>(0, std::vector<Undirected::Edge>());
  const auto f = ExpanderDecomposition::constructSubdivisionFlowGraph(g);
  EXPECT_EQ(f->size(), 0);
  EXPECT_EQ(f->edgeCount(), 0);
}

TEST(ConstructSubdivisionFlowGraph, SingleVertex) {
  const auto g =
      std::make_unique<Undirected::Graph>(1, std::vector<Undirected::Edge>());
  const auto f = ExpanderDecomposition::constructSubdivisionFlowGraph(g);
  EXPECT_EQ(f->size(), 1);
  EXPECT_EQ(f->edgeCount(), 0);
}

TEST(ConstructSubdivisionFlowGraph, SingleEdge) {
  auto g = std::make_unique<Undirected::Graph>(
      2, std::vector<Undirected::Edge>({{0, 1}}));
  const auto f = ExpanderDecomposition::constructSubdivisionFlowGraph(g);
  EXPECT_EQ(f->size(), 3);
  EXPECT_EQ(f->edgeCount(), 2);
  EXPECT_EQ(f->degree(0), 1);
  EXPECT_EQ(f->degree(1), 1);
  EXPECT_EQ(f->degree(2), 2); // Split vertex

  const auto &e02 = f->getEdge(0, 0);
  const auto &e12 = f->getEdge(1, 0);

  EXPECT_EQ(e02.from, 0);
  EXPECT_EQ(e02.to, 2);
  EXPECT_EQ(e02.flow, 0);
  EXPECT_EQ(e02.capacity, 0);

  EXPECT_EQ(e12.from, 1);
  EXPECT_EQ(e12.to, 2);
  EXPECT_EQ(e12.flow, 0);
  EXPECT_EQ(e12.capacity, 0);
}

TEST(ConstructSubdivisionFlowGraph, BasicGraph) {
  const int n = 10;
  const std::vector<Undirected::Edge> es = {
      {0, 1}, {0, 2}, {2, 5}, {3, 2}, {6, 1}, {6, 7}, {7, 8}, {9, 2}, {9, 8}};
  auto g = std::make_unique<Undirected::Graph>(n, es);
  const auto f = ExpanderDecomposition::constructSubdivisionFlowGraph(g);
  EXPECT_EQ(f->size(), n + g->edgeCount());
  EXPECT_EQ(f->edgeCount(), 2 * g->edgeCount());
}

/**
   Run expander decomposition on a basic graph. Verify that every vertex is in
   some partition.
 */
TEST(ExpanderDecomposition, BasicGraph) {
  const int n = 10;
  const std::vector<Undirected::Edge> es = {
      {0, 1}, {0, 2}, {2, 5}, {3, 2}, {6, 1}, {6, 7}, {7, 8}, {9, 2}, {9, 8}};
  auto g = std::make_unique<Undirected::Graph>(n, es);

  const double phi = 0.1;
  const int tConst = 100;
  const double tFactor = 20.0;
  const auto solver = ExpanderDecomposition::Solver(std::move(g), phi, tConst,
                                                    tFactor, 10, 0.45, 0);
  for (const auto &partition : solver.getPartition())
    for (const auto &u : partition)
      EXPECT_TRUE(u >= 0 && u < n);
}
