#include <cmath>
#include <iostream>
#include <vector>

#include "lib/cut_matching.hpp"
#include "lib/graph.hpp"

using namespace std;

int main() {
  int n, m;
  cin >> n >> m;
  Graph g(n);
  if (m == -1) {
    //    m = rand() % (n * (n + 1) / 2);
    m = n + rand() % (2 * n);
    for (int i = 0; i < m; ++i) {
      int u = rand() % n;
      int v = u;
      do {
        v = rand() % n;
      } while (u == v);
      g.addEdge(u, v);
    }
    cerr << "Generated graph" << endl;
  } else {
    for (int i = 0; i < m; ++i) {
      Vertex u, v;
      cin >> u >> v;
      g.addEdge(u, v);
    }
    cerr << "Read graph" << endl;
  }

  CutMatching cm(g);
  double phi = 0.1;
  while (phi < 1.0) {
    cout << "------------------------------------------------------" << endl;
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

    cout << "------------------------------------------------------" << endl;
    phi += 0.1;
  }
}
