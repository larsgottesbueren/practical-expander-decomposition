#include <cmath>
#include <iostream>
#include <vector>

#include "lib/expander_decomp.hpp"
#include "lib/partition_graph.hpp"

using namespace std;

int main() {
  int n;
  cin >> n;
  string graphType;
  cin >> graphType;

  PartitionGraph<int,Edge> g(n);

  if (graphType == "random") {
    cerr << "Generating random graph with O(n) edges" << endl;

    int m = n + rand() % (2 * n);
    for (int i = 0; i < m; ++i) {
      int u = rand() % n;
      int v = u;
      do {
        v = rand() % n;
      } while (u == v);
      g.addEdge({u, v});
    }
  } else if (graphType == "clusters") {
    cerr
        << "Generating two unconnected clusters where the balanced cut is clear"
        << endl;

    int leftN = n / 2;

    for (int i = 0; i < leftN; ++i)
      for (int j = i + 1; j < leftN; ++j)
        if (rand() % 100 < 50)
          g.addEdge({i, j});
    for (int i = leftN; i < n; ++i)
      for (int j = i + 1; j < n; ++j)
        if (rand() % 100 < 50)
          g.addEdge({i, j});

    g.addEdge({0, leftN});
    g.addEdge({1, leftN + 1});
    g.addEdge({2, leftN + 2});
    g.addEdge({3, leftN + 3});
    g.addEdge({4, leftN + 4});
  } else if (graphType == "path") {
    cerr << "Generating path" << endl;
    for (int i = 0; i < n - 1; ++i)
      g.addEdge({i, i + 1});
  } else {
    int m;
    cin >> m;

    for (int i = 0; i < m; ++i) {
      int u, v;
      cin >> u >> v;
      g.addEdge({u, v});
    }
  }

  double phi;
  cin >> phi;
  ExpanderDecomp decomp(g, phi);

  cout << "Partition with phi = " << phi << endl;
  auto partitions = decomp.getPartition();
  for (int i = 0; i < (int)partitions.size(); ++i) {
    cout << "i:";
    for (int j = 0; j < (int)partitions[i].size(); ++j)
      cout << " " << partitions[i][j];
    cout << endl;
  }
}
