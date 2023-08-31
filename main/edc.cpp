#include <cmath>
#include <cstring>
#include <iostream>
#include <numeric>
#include <vector>
#include <memory>
#include <set>
#include <string>
#include <sstream>

#include <util.hpp>
#include <cut_matching.hpp>
#include <datastructures/undirected_graph.hpp>
#include <expander_decomp.hpp>

int Logger::LOG_LEVEL = 3;

std::unique_ptr<std::mt19937> configureRandomness(unsigned int seed) {
    std::mt19937 randomGen(seed);
    return std::make_unique<std::mt19937>(randomGen);
}

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



int main(int argc, char *argv[]) {

  auto randomGen = configureRandomness(555);

  if(const char* env_log_level = std::getenv("LOG_LEVEL")) {
      int log_level = std::stoi(env_log_level);
      Logger::LOG_LEVEL = log_level;
  }

  auto g = readGraph(false);

  CutMatching::Parameters params = { .tConst = 22, .tFactor = 5.0, .minIterations = 0, .minBalance = 0.45, .samplePotential = false, .balancedCutStrategy = true };
  double phi = 0.01;

  ExpanderDecomposition::Solver solver(std::move(g), phi, randomGen.get(), params);
  auto partitions = solver.getPartition();
  auto conductances = solver.getConductance();

  return 0;

  std::cout << solver.getEdgesCut() << " " << partitions.size() << std::endl;
  for (int i = 0; i < int(partitions.size()); ++i) {
      std::cout << partitions[i].size() << " " << conductances[i];
      for (auto p : partitions[i])
          std::cout << " " << p;
      std::cout << std::endl;
  }
}
