#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <algorithm>
#include <cmath>
#include <random>
#include <unordered_set>

#include "cut_matching.hpp"
#include "graph.hpp"
#include "unit_flow.hpp"

CutMatching::CutMatching(const Graph &g) : graph(0), numRegularNodes(g.size()) {
  const int n = g.size();
  int m = 0;
  std::vector<std::pair<Vertex, Vertex>> edges;

  for (Vertex u = 0; u < n; ++u)
    for (auto v : g.neighbors[u]) {
      assert(u != v &&
             "No loops allowed when constructing sub-division graph.");
      if (u < v)
        m++, edges.push_back({u, v});
    }

  graph.neighbors.resize(n + m);
  for (int i = 0; i < (int)edges.size(); ++i) {
    const Vertex w = i + n;
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
  g.seed(0);
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
    values.push_back(T(u - n, v - n, 0.5));
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
  const int m = numSplitNodes(); // Number of edges in original graph.

  std::vector<Mat> matchings;

  std::vector<Vertex> A, AX, R;
  for (Vertex u = 0; u < graph.size(); ++u)
    A.push_back(u);
  for (Vertex u = numRegularNodes; u < graph.size(); ++u)
    AX.push_back(u - numRegularNodes);

  const int Tconst = 10;
  const int T = Tconst + std::ceil(std::log(m) * std::log(m));

  // int c = std::ceil(1 / (phi * T));

  Vec r(m);

  // TODO: RST14 uses A as edge subdivision nodes left while Saranurak & Wang
  // uses A as all nodes left.
  int iterations = 1;
  for (; graph.volume(R) <= m / (10 * T) && iterations <= T; ++iterations) {
    fillRandomUnitVector(r);

    const Vec &projectedFlow = projectFlow(matchings, r);

    // TODO: is it correct to average flow _after_ projection?
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

    if (left.size() > right.size())
      std::swap(left, right);
    assert(left.size() <= right.size() &&
           "Expected right partition to be at least as large. See RST14 lemma "
           "3.3.");

    auto potentialFunc = [avgFlow,
                          &projectedFlow](const std::vector<Vertex> &vertices) {
      double sum = 0;
      for (auto u : vertices) {
        double diff = projectedFlow(u) - avgFlow;
        sum += diff * diff;
      }
      return sum;
    };

    double allP = potentialFunc(AX), leftP = potentialFunc(left);
    //           rightP = potentialFunc(right);

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

    const int h = (int)ceil(1.0 / phi / std::log(m));
    UnitFlow uf(graph.size(), h);
    for (auto u : leftAX)
      uf.addSource(u + numRegularNodes, 1);
    for (auto u : rightAX)
      uf.addSink(u + numRegularNodes, 1);

    const int cap = (int)std::ceil(1.0 / phi / std::log(m) / std::log(m));
    std::unordered_set<Vertex> alive;
    for (auto u : A)
      alive.insert(u);
    for (auto u : A)
      for (auto v : graph.neighbors[u])
        if (alive.find(v) != alive.end())
          uf.addEdge(u, v, cap); // TODO: are double edges being added?

    auto cutS = uf.compute();
    std::unordered_set<Vertex> removed;
    for (auto u : cutS)
      removed.insert(u);

    std::vector<Vertex> sourcesLeft;
    for (auto u : leftAX) {
      // Convert to regular indexing before matching in entire graph.
      const int idx = u + numRegularNodes;
      if (removed.find(idx) == removed.end())
        sourcesLeft.push_back(idx);
    }

    auto matching = uf.matching(sourcesLeft);
    matchings.push_back(matchingToMatrix(matching, numRegularNodes, m));

    A.erase(std::remove_if(A.begin(), A.end(),
                           [&removed](Vertex u) {
                             return removed.find(u) != removed.end();
                           }),
            A.end());

    AX.erase(std::remove_if(AX.begin(), AX.end(),
                            [this, &removed](Vertex u) {
                              const int idx = u + numRegularNodes;
                              return removed.find(idx) != removed.end();
                            }),
             AX.end());
    for (auto u : removed)
      R.push_back(u);
  }

  CutMatching::ResultType rType;
  if (iterations <= T)
    // We have: graph.volume(R) > m / (10 * T)
    rType = Balanced;
  else if (R.empty())
    rType = Expander;
  else
    rType = NearExpander;

  auto removeSubdivisionVertices = [this](std::vector<Vertex> &us) {
    const int limit = numRegularNodes;
    us.erase(std::remove_if(us.begin(), us.end(),
                            [limit](Vertex u) { return u >= limit; }),
             us.end());
  };
  removeSubdivisionVertices(A);
  removeSubdivisionVertices(R);

  return {rType, A, R};
}
