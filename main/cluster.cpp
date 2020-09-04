#include <cmath>
#include <iostream>
#include <vector>

#include "lib/cut_matching.hpp"
#include "lib/graph.hpp"

using namespace std;

int main() {
  int n;
  cin >> n;
  string graphType;
  cin >> graphType;

  Graph g(n);

  if (graphType == "random") {
    cerr << "Generating random graph with O(n) edges" << endl;

    int m = n + rand() % (2 * n);
    for (int i = 0; i < m; ++i) {
      int u = rand() % n;
      int v = u;
      do {
        v = rand() % n;
      } while (u == v);
      g.addEdge(u, v);
    }
  } else if (graphType == "cluster") {
    cerr
        << "Generating two unconnected clusters where the balanced cut is clear"
        << endl;

    int leftN = n / 2;

    for (int i = 0; i < leftN; ++i)
      for (int j = i + 1; j < leftN; ++j)
        if (rand() % 100 < 50)
          g.addEdge(i, j);
    for (int i = leftN; i < n; ++i)
      for (int j = i + 1; j < n; ++j)
        if (rand() % 100 < 50)
          g.addEdge(i, j);

    g.addEdge(0, leftN);
    g.addEdge(1, leftN + 1);
    g.addEdge(2, leftN + 2);
    g.addEdge(3, leftN + 3);
    g.addEdge(4, leftN + 4);
  } else if (graphType == "path") {
    cerr << "Generating path" << endl;
    for (int i = 0; i < n - 1; ++i)
      g.addEdge(i, i + 1);
  } else {
    int m;
    cin >> m;

    for (int i = 0; i < m; ++i) {
      Vertex u, v;
      cin >> u >> v;
      g.addEdge(u, v);
    }
  }
  /*
  CutMatching cm(g);
  double phi;
  cin >> phi;

  cout << "Partition with phi = " << phi << endl;
  auto [type, left, right] = cm.compute(phi);
  cout << "Type: ";
  if (type == CutMatching::Balanced)
    cout << "balanced";
  else if (type == CutMatching::Expander)
    cout << "expander";
  else
    cout << "near expander";
  cout << endl;
  cout << "A:";
  for (auto u : left)
    cout << " " << u;
  cout << endl;
  cout << "R:";
  for (auto u : right)
    cout << " " << u;
  cout << endl;
  */
}
