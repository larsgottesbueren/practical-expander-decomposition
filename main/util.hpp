#pragma once

#include <iostream>
#include <memory>
#include <set>
#include <string>

#include "lib/datastructures/undirected_graph.hpp"

std::unique_ptr<std::mt19937> configureRandomness() {
  srand(0);
  std::random_device rd;
  std::mt19937 randomGen(rd());

  return std::make_unique<std::mt19937>(randomGen);
}

/**
   Read an undirected graph from standard input. If 'chaco_format' is true, read
   graph as specified in 'https://chriswalshaw.co.uk/jostle/jostle-exe.pdf'.
   Otherwise assume edges are given as 'm' vertex pairs. Duplicate edges are
   ignored.
 */
std::unique_ptr<Undirected::Graph> readGraph(bool chaco_format) {
  int n, m;
  std::cin >> n >> m;

  std::vector<Undirected::Edge> es;
  if (chaco_format) {
    std::cin.ignore();
    for (int u = 0; u < n; ++u) {
      std::string line;
      std::getline(std::cin, line);
      std::stringstream ss(line);

      int v;
      while (ss >> v)
        if (u < --v)
          es.emplace_back(u, v);
    }
  } else {
    std::set<std::pair<int, int>> seen;
    for (int i = 0; i < m; ++i) {
      int u, v;
      std::cin >> u >> v;
      if (u > v)
        std::swap(u, v);
      if (seen.find({u, v}) == seen.end()) {
        seen.insert({u, v});
        es.emplace_back(u, v);
      }
    }
  }

  return std::make_unique<Undirected::Graph>(n, es);
}
