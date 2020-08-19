#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <unordered_set>

#include "cut_matching.hpp"
#include "graph.hpp"
#include "unit_flow.hpp"

CutMatching::CutMatching(const Graph &g)
    : graph(g.size() + g.edgeCount()), numRegularNodes(g.size()),
      numSplitNodes(g.edgeCount()) {
  std::vector<std::pair<Vertex, Vertex>> edges;

  for (Vertex u = 0; u < g.size(); ++u)
    for (auto v : g.neighbors[u]) {
      assert(u != v &&
             "No loops allowed when constructing sub-division graph.");
      if (u < v)
        edges.push_back({u, v});
    }

  for (int i = 0; i < (int)edges.size(); ++i) {
    const Vertex w = i + g.size();
    const auto &[u, v] = edges[i];
    graph.addEdge(u, w);
    graph.addEdge(v, w);
  }
}

typedef Eigen::SparseMatrix<double> Mat;
typedef Eigen::VectorXd Vec;

/**
   Dot product of two vectors.

   Precondition: size(u) = size(v)
 */
double dotProduct(const Vec &u, const Vec &v) {
  assert(u.rows() == v.rows() && "Expected vectors of same size");

  double sum = 0;
  for (int i = 0; i < u.rows(); ++i)
    sum += u(i) * v(i);
  return sum;
}

/**
   Euclidian distance of a vector.
 */
double vectorLength(const Vec &u) {
  double sum = 0;
  for (int i = 0; i < u.rows(); ++i)
    sum += u(i);
  return std::sqrt(sum);
}

/**
   Scalar projection of u along v.
 */
double scalarProject(const Vec &u, const Vec &v) {
  return dotProduct(u, v) / vectorLength(v);
}

/**
   Fill a vector with random data such that it is a unit vector orthogonal to
   the all ones vector.
 */
void fillRandomUnitVector(Vec &r) {
  const int n = r.rows();
  std::vector<double> xs(n, 1);
  for (int i = 0; i < n / 2; ++i)
    xs[i] = -1;
  if (n % 2 != 0)
    xs[0] = -2;
  // TODO: It is slow to declare random device every function call.
  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(xs.begin(), xs.end(), g);
  for (int i = 0; i < n; ++i)
    r(i) = xs[i] / (double)n;
}

Vec projectFlow(const std::vector<Mat> &matchings, const Vec &r) {
  Vec v = r;
  for (auto it = matchings.rbegin(); it != matchings.rend(); ++it)
    v = (*it) * v;
  return v;
}

/**
   Convert a list of matched vertices to a sparse mxm matrix where the row sum
   on each matched vertex is 1.

   Parameters:
   - ms: vertices matched
   - n: number of regular vertices (i.e. not split vertices)
   - m: number of edges in original graph (i.e. split vertices)
 */
Mat matchingToMatrix(const std::vector<std::pair<Vertex, Vertex>> &ms, int n,
                     int m) {
  typedef Eigen::Triplet<double> T;
  std::vector<T> values;
  for (auto &[u, v] : ms) {
    values.push_back(T(u - n, u - n, 0.5));
    values.push_back(T(u - n, v - n, 0.5));

    values.push_back(T(v - n, v - n, 0.5));
    values.push_back(T(v - n, u - n, 0.5));
  }
  Mat mat(m, m);
  mat.setFromTriplets(values.begin(), values.end());
  return mat;
}

/*
  - A: all nodes in sub-division graph
  - R: union of S_i
  - AX: A \cap X_E, all sub-division vertices in A. Zero indexed to allow for
        quick access to flow projection.
 */
CutMatching::Result CutMatching::compute(double phi) const {
  std::vector<Mat> matchings;

  std::unordered_set<Vertex> A, AX, R;
  for (Vertex u = 0; u < graph.size(); ++u)
    A.insert(u);
  for (Vertex u = numRegularNodes; u < graph.size(); ++u)
    AX.insert(u - numRegularNodes);

  const double T =
      1 + 0.9 * std::ceil(std::log(numSplitNodes) * std::log(numSplitNodes));

  Vec r(numSplitNodes);

  // TODO: RST14 uses A as edge subdivision nodes left while Saranurak & Wang
  // uses A as all nodes left.
  int iterations = 1;
  for (; graph.volume(R.begin(), R.end()) <= 100 + numSplitNodes / (10.0 * T) &&
         iterations <= T;
       ++iterations) {

#define CUT_MATCHING_DEBUG
#ifdef CUT_MATCHING_DEBUG
    std::cerr << "Iteration: " << iterations
              << " volume(R) = " << graph.volume(R.begin(), R.end())
              << std::endl;
#endif
    fillRandomUnitVector(r);

    const Vec &projectedFlow = projectFlow(matchings, r);

    double avgFlow = 0;
    for (auto u : AX)
      avgFlow += projectedFlow(u);
    avgFlow /= AX.size();

    std::vector<Vertex> left, right;
    for (auto u : AX)
      if (projectedFlow(u) < avgFlow)
        left.push_back(u);
      else
        right.push_back(u);

#ifdef CUT_MATCHING_DEBUG
    std::cerr << "Average flow = " << avgFlow << std::endl
              << "|Left|=" << left.size() << " :";
    for (auto u : left)
      std::cerr << " " << u << ":" << projectedFlow(u);
    std::cerr << std::endl << "|Right|=" << right.size() << " :";
    for (auto u : right)
      std::cerr << " " << u << ":" << projectedFlow(u);
    std::cerr << std::endl;
#endif

    // TODO: It is unclear if average flow is computed correctly. RST14 says
    // |left| <= |right| must be true.
    //
    //    if (left.size() > right.size())
    //      std::swap(left, right);
    //    assert(left.size() <= right.size() &&
    //           "Expected right partition to be at least as large. See RST14
    //           lemma " "3.3.");

    auto potentialFunc = [avgFlow,
                          &projectedFlow](const std::vector<Vertex> &vertices) {
      double sum = 0;
      for (auto u : vertices) {
        double diff = projectedFlow(u) - avgFlow;
        sum += diff * diff;
      }
      return sum;
    };

    std::vector<Vertex> tmpAX;
    for (auto u : AX)
      tmpAX.push_back(u);
    double allP = potentialFunc(tmpAX), leftP = potentialFunc(left);

    std::vector<Vertex> leftAX, rightAX;
    double eta; // See RST14 lemma 3.3
    if (leftP >= 1.0 / 20.0 * allP) {
      eta = avgFlow;
      rightAX = right;

      leftAX = left;
      std::sort(leftAX.begin(), leftAX.end(), [&projectedFlow](int i1, int i2) {
        return projectedFlow(i1) < projectedFlow(i2);
      });
      if (leftAX.size() > 8)
        leftAX.resize(8);
    } else {
      double l = 0;
      for (auto idx : left)
        l += std::abs(projectedFlow(idx) - avgFlow);
      eta = avgFlow + 4 * l / (double)AX.size();

      for (auto u : AX) {
        if (projectedFlow(u) <= eta)
          rightAX.push_back(u);
        if (projectedFlow(u) >= avgFlow + 6 * l / (double)AX.size())
          // Element is in R' (RST14 lemma 3.3)
          leftAX.push_back(u);
      }
      std::sort(leftAX.begin(), leftAX.end(), [&projectedFlow](int i1, int i2) {
        return projectedFlow(i1) < projectedFlow(i2);
      });
      std::reverse(leftAX.begin(), leftAX.end());

      if (8 * leftAX.size() > AX.size())
        leftAX.resize(std::ceil((double)AX.size() / 8.0));
    }

#ifdef CUT_MATCHING_DEBUG
    std::cerr << "Left A U X:";
    for (auto u : leftAX)
      std::cerr << " " << u;
    std::cerr << std::endl << "Right A U X:";
    for (auto u : rightAX)
      std::cerr << " " << u;
    std::cerr << std::endl;
#endif

    const int h = (int)ceil(1.0 / phi / std::log(numSplitNodes));
    UnitFlow uf(graph.size(), h);
    for (auto u : leftAX)
      uf.addSource(u + numRegularNodes, 1);
    for (auto u : rightAX)
      uf.addSink(u + numRegularNodes, 1);

    const int cap = (int)std::ceil(1.0 / phi / std::log(numSplitNodes) /
                                   std::log(numSplitNodes));
    std::unordered_set<Vertex> alive;
    for (auto u : A)
      alive.insert(u);
    for (auto u : A)
      for (auto v : graph.neighbors[u])
        if (alive.find(v) != alive.end() && u < v)
          uf.addEdge(u, v, cap);

    auto levelCut = uf.compute();
    std::vector<bool> S(graph.size());
    for (auto u : levelCut)
      S[u] = true;
    for (Vertex u = numRegularNodes; u < graph.size(); ++u) {
      if (S[u])
        continue;
      bool both = true;
      assert(graph.neighbors[u].size() == 2 &&
             "Split nodes should have degree two");
      for (auto v : graph.neighbors[u])
        if (!S[v])
          both = false;
      if (both)
        S[u] = true;
    }
#ifdef CUT_MATCHING_DEBUG
    int count = 0;
    for (auto b : S)
      if (b)
        count++;
    std::cerr << "|S| = " << count << " size of level cut = " << levelCut.size()
              << std::endl;
#endif

    std::vector<Vertex> sourcesLeft;
    for (auto u : leftAX) {
      // Convert to regular indexing before matching in entire graph.
      const int idx = u + numRegularNodes;
      if (!S[idx])
        sourcesLeft.push_back(idx);
    }

    auto matching = uf.matching(sourcesLeft);
    matchings.push_back(
        matchingToMatrix(matching, numRegularNodes, numSplitNodes));

#ifdef CUT_MATCHING_DEBUG
    std::cerr << "Sources to match:";
    for (auto u : sourcesLeft)
      std::cerr << " " << u;
    std::cerr << std::endl;
    std::cerr << "Matching:";
    for (auto [u, v] : matching)
      std::cerr << " (" << u << "-" << v << ")";
    std::cerr << std::endl;
#endif

    for (auto it = A.begin(); it != A.end();)
      if (S[*it])
        it = A.erase(it);
      else
        ++it;
    for (auto it = AX.begin(); it != AX.end();)
      if (S[*it + numRegularNodes])
        it = AX.erase(it);
      else
        ++it;

    for (Vertex u = 0; u < graph.size(); ++u)
      if (S[u])
        R.insert(u);

    for (Vertex u = numRegularNodes; u < graph.size(); ++u) {
      assert(graph.neighbors[u].size() == 2 &&
             "Split vertices should have degree two");
      if (A.find(u) != A.end()) {
        bool both = true;
        for (auto v : graph.neighbors[u])
          if (R.find(v) == R.end())
            both = false;
        if (both)
          A.erase(u), AX.erase(u - numRegularNodes), R.insert(u);
      } else {
        bool both = true;
        for (auto v : graph.neighbors[u])
          if (A.find(v) == A.end())
            both = false;
        if (both)
          R.erase(u), A.insert(u), AX.insert(u - numRegularNodes);
      }
    }
  }

#ifdef CUT_MATCHING_DEBUG
  std::cerr << "Final volume(R) = " << graph.volume(R.begin(), R.end()) << std::endl;
#endif

  CutMatching::ResultType rType;
  if (iterations <= T)
    // We have: graph.volume(R) > m / (10 * T)
    rType = Balanced;
  else if (R.empty())
    rType = Expander;
  else
    rType = NearExpander;

  auto removeSubdivisionVertices = [this](std::unordered_set<Vertex> &us) {
    const int limit = numRegularNodes;
    for (auto it = us.begin(); it != us.end();)
      if (*it >= limit)
        it = us.erase(it);
      else
        ++it;
  };
  removeSubdivisionVertices(A);
  removeSubdivisionVertices(R);

  CutMatching::Result result;
  result.t = rType;
  for (auto u : A)
    result.a.push_back(u);
  for (auto u : R)
    result.r.push_back(u);

  return result;
}
