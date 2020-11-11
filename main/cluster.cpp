#include <cmath>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <glog/stl_logging.h>
#include <iostream>
#include <vector>

#include "lib/expander_decomp.hpp"
#include "lib/ugraph.hpp"

using namespace std;

int main(int argc, char *argv[]) {
  google::InitGoogleLogging(argv[0]);
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  DLOG(INFO) << "Waiting for input";

  //  srand(time(NULL));

  int n;
  cin >> n;
  string graphType;
  cin >> graphType;

  auto g = make_unique<Undirected::Graph>(n);

  if (graphType == "random") {
    int m = n + rand() % (2 * n);
    for (int i = 0; i < m; ++i) {
      int u = rand() % n;
      int v = u;
      do {
        v = rand() % n;
      } while (u == v);
      g->addEdge(u, v);
    }
  } else if (graphType == "clusters") {
    int leftN = n / 2;

    for (int i = 0; i < leftN; ++i)
      for (int j = i + 1; j < leftN; ++j)
        //        if (rand() % 100 < 50)
        g->addEdge(i, j);
    for (int i = leftN; i < n; ++i)
      for (int j = i + 1; j < n; ++j)
        //        if (rand() % 100 < 50)
        g->addEdge(i, j);

    g->addEdge(0, leftN);
    //    g->addEdge(1, leftN + 1);
    //    g->addEdge(2, leftN + 2);
    //    g->addEdge(3, leftN + 3);
    //    g->addEdge(4, leftN + 4);
  } else if (graphType == "path") {
    for (int i = 0; i < n - 1; ++i)
      g->addEdge(i, i + 1);
  } else {
    int m;
    cin >> m;

    for (int i = 0; i < m; ++i) {
      int u, v;
      cin >> u >> v;
      g->addEdge(u, v);
    }
  }

  double phi;
  int tConst;
  int tFactor;
  cin >> phi >> tConst >> tFactor;

  ExpanderDecomposition::Solver solver(move(g), phi, tConst, tFactor);
  auto partitions = solver.getPartition();

  cout << "Edges cut: " << solver.getEdgesCut() << endl;

  for (const auto &p : partitions)
    cout << p.size() << " ";
  cout << endl;

  if (n <= 200) {
    for (int p = 0; p < partitions.size(); ++p) {
      cout << p << ":";
      for (auto x : partitions[p])
        cout << " " << x;
      cout << endl;
    }
  }
}
