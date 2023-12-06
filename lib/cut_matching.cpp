#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <unordered_set>

#include "cut_matching.hpp"

namespace CutMatching {

Result::Result()
    : type(Result::Type::Expander), iterations(0),
      iterationsUntilValidExpansion(std::numeric_limits<int>::max()), congestion(1) {}

Solver::Solver(UnitFlow::Graph *g, UnitFlow::Graph *subdivG,
               std::mt19937 *randomGen, std::vector<int> *subdivisionIdx,
               double phi, Parameters params)
    : graph(g), subdivGraph(subdivG), randomGen(randomGen),
      subdivisionIdx(subdivisionIdx), phi(phi),
      T(std::max(1, params.tConst + int(ceil(params.tFactor *square(
                                        std::log10(graph->edgeCount())))))),
      numSplitNodes(subdivGraph->size() - graph->size()) {
  assert(graph->size() != 0 && "Cut-matching expected non-empty subset.");

  // Set edge capacities in subdivision flow graph.
  const UnitFlow::Flow capacity = std::ceil(1.0 / phi / T);   // TODO SW'19 says its log^2(m) not T (no hidden constants) page 29 top
  for (auto u : *graph)
    for (auto e = subdivGraph->beginEdge(u); e != subdivGraph->endEdge(u); ++e)
      e->capacity = capacity, subdivGraph->reverse(*e).capacity = capacity,
      e->congestion = 0, subdivGraph->reverse(*e).congestion = 0;


  // Give each 'm' subdivision vertex a unique index in the range '[0,m)'.
  int count = 0;
  for (auto u : *subdivGraph) {
    if ((*subdivisionIdx)[u] >= 0) {
      (*subdivisionIdx)[u] = count++;
    }
  }

  num_matched_steps.assign(count, 0);

  // If potential is sampled, set the flow matrix to the identity matrix.
  if (params.samplePotential) {
    flowMatrix.resize(count);
    for (int i : *subdivGraph) {
      int u = (*subdivisionIdx)[i];
      if (u >= 0) {
        flowMatrix[u].resize(count);
        flowMatrix[u][u] = 1.0;
      }
    }
  }
}

std::vector<double> Solver::randomUnitVector() {
  std::normal_distribution<> distr(0, 1);

  std::vector<double> result(numSplitNodes);
  for (auto &r : result)
    r = distr(*randomGen);

  double offset = std::accumulate(result.begin(), result.end(), 0.0) / double(numSplitNodes);
  double sumSq = 0;
  for (auto &r : result)
    r -= offset, sumSq += r * r;

  const double normalize = sqrt(sumSq);
  for (auto &r : result)
    r /= normalize;

  return result;
}

double Solver::samplePotential() const {
  // Subdivision vertices remaining.
  std::vector<int> alive;
  for (auto it = subdivGraph->cbegin(); it != subdivGraph->cend(); ++it) {
    const auto u = (*subdivisionIdx)[*it];
    if (u >= 0)
      alive.push_back(u);
  }

  std::vector<long double> avgFlowVector(numSplitNodes);

  for (int u : alive)
    for (int v : alive)
      avgFlowVector[v] += flowMatrix[u][v];
  for (auto &f : avgFlowVector)
    f /= (long double)alive.size();

  long double sum = 0, kahanError = 0;
  long double sum2 = 0.0, max_sq = 0.0;
  double max_entry = std::numeric_limits<double>::lowest();
  double min_entry = std::numeric_limits<double>::max();
  for (int u : alive) {
    for (int v : alive) {
      const long double sq = square(flowMatrix[u][v] - avgFlowVector[v]);
      const long double y = sq - kahanError;
      const long double t = sum + y;
      kahanError = t - sum - y;
      sum = t;

      sum2 += sq;
      max_sq = std::max(max_sq, sq);
      max_entry = std::max(max_entry, flowMatrix[u][v]);
      min_entry = std::min(min_entry, flowMatrix[u][v]);
    }
  }

  VLOG(3) << V(sum) << V(sum2) << V(max_sq) << V(max_entry) << V(min_entry);
  return (double)sum;
}

  // TODO cache the avg flow value. when edges get removed, update it.
  // should be faster than recomputing this all the time
  // can we also update the projected potential?
double Solver::AvgFlow(const std::vector<double>& flow) const {
  long double sum = 0.0;
  for (auto u : *subdivGraph) {
    const int idx = (*subdivisionIdx)[u];
    if (idx >= 0) {
      sum += flow[idx];
    }
  }
  const int curSubdivisionCount = subdivGraph->size() - graph->size();
  return sum / (double)curSubdivisionCount;
}

double Solver::ProjectedPotential(const std::vector<double>& flow) const {
  const double avg_flow = AvgFlow(flow);
  long double potential = 0.0;
  for (auto u : *subdivGraph) {
    const int idx = (*subdivisionIdx)[u];
    if (idx >= 0) {
      potential += square(flow[idx] - avg_flow);
    }
  }
  return potential;
}

std::pair<std::vector<int>, std::vector<int>> Solver::KRVCutStep(
      const std::vector<double> &flow, const Parameters &params) const {
  std::vector<std::pair<double, UnitFlow::Vertex>> perm;
  for (auto u : *subdivGraph) {
    const int idx = (*subdivisionIdx)[u];
    if (idx >= 0) {
      perm.emplace_back(flow[idx], u);
    }
  }
  size_t mid = perm.size() / 2;
  std::nth_element(perm.begin(), perm.begin() + mid, perm.end());
  std::vector<int> axLeft, axRight;
  for (size_t i = 0; i < mid; ++i) {
    axLeft.push_back(perm[i].second);
  }
  for (size_t i = mid; i < perm.size(); ++i) {
    axRight.push_back(perm[i].second);
  }
  return std::make_pair(std::move(axLeft), std::move(axRight));
}

std::pair<std::vector<int>, std::vector<int>> Solver::RSTCutStep(
    const std::vector<double> &flow, const Parameters &params) const {
  const int curSubdivisionCount = subdivGraph->size() - graph->size();
  double avgFlow = AvgFlow(flow);
  // Partition subdivision vertices into a left and right set.
  std::vector<int> axLeft, axRight;
  for (auto u : *subdivGraph) {
    const int idx = (*subdivisionIdx)[u];
    if (idx >= 0) {
      if (flow[idx] < avgFlow)
        axLeft.push_back(u);
      else
        axRight.push_back(u);
    }
  }

  // Sort by flow
  auto cmpFlow = [&flow, &subdivisionIdx = subdivisionIdx](int u, int v) {
    return flow[(*subdivisionIdx)[u]] < flow[(*subdivisionIdx)[v]];
  };
  std::sort(axLeft.begin(), axLeft.end(), cmpFlow);
  std::sort(axRight.begin(), axRight.end(), cmpFlow);

  // When removing vertices from either side, we want to do it from values
  // closer to the average. If left and right are swapped, then left should be
  // reversed instead of right.
  if (axLeft.size() > axRight.size()) {
    std::swap(axLeft, axRight);
    std::reverse(axLeft.begin(), axLeft.end());
  } else {
    std::reverse(axRight.begin(), axRight.end());
  }

  // Compute potentials
  long double totalPotential = 0.0, leftPotential = 0.0;
  for (auto u : *subdivGraph) {
    const int idx = (*subdivisionIdx)[u];
    if (idx >= 0)
      totalPotential += square(flow[idx] - avgFlow);
  }
  for (auto u : axLeft) {
    const int idx = (*subdivisionIdx)[u];
    assert(idx >= 0);
    leftPotential += square(flow[idx] - avgFlow);
  }

  if (leftPotential <= totalPotential / 20.0) {
    VLOG(3) << "repartition along mu";
    double l = 0.0;
    for (auto u : axLeft) {
      const int idx = (*subdivisionIdx)[u];
      assert(idx >= 0);
      l += std::abs(flow[idx] - avgFlow);
    }
    const double mu = avgFlow + 4.0 * l / (double)curSubdivisionCount;

    // Re-partition along '\mu'.
    axLeft.clear(), axRight.clear();
    for (auto u : *subdivGraph) {
      const int idx = (*subdivisionIdx)[u];
      if (idx >= 0) {
        if (flow[idx] <= mu)
          axRight.push_back(u);
        else if (flow[idx] >= avgFlow + 6.0 * l / (double)curSubdivisionCount)
          axLeft.push_back(u);
      }
    }
    // TODO sort again?? check in RST'14
    std::reverse(axRight.begin(), axRight.end());

  }

  if (params.balancedCutStrategy) {
    while (axRight.size() > axLeft.size()) {
      axRight.pop_back();
    }
  } else {
    while ((int)axLeft.size() * 8 > curSubdivisionCount) {
      axLeft.pop_back();
    }
  }

  while (axLeft.size() > axRight.size()) {
    axLeft.pop_back();
  }

  return std::make_pair(axLeft, axRight);
}



std::pair<std::vector<int>, std::vector<int>>
Solver::proposeCut(const std::vector<double> &flow, const Parameters &params) const {
  if (subdivGraph->removedSize() == 0) {
    return KRVCutStep(flow, params);
  } else {
    return RSTCutStep(flow, params);
  }
}

double Solver::ProjectedPotentialConvergenceThreshold() const {
  const int s = subdivGraph->size() - graph->size();
  return 1.0 / (16.0 * s * s * s);
}

void Solver::RemoveCutSide(const std::vector<UnitFlow::Vertex>& cutLeft, const std::vector<UnitFlow::Vertex>& cutRight,
                          std::vector<UnitFlow::Vertex>& axLeft, std::vector<UnitFlow::Vertex>& axRight) {
  std::unordered_set<int> removed;
  if (subdivGraph->globalVolume(cutLeft.begin(), cutLeft.end()) < subdivGraph->globalVolume(cutRight.begin(), cutRight.end())) {
    for (auto u : cutLeft) {
      removed.insert(u);
    }
  } else {
    for (auto u : cutRight) {
      removed.insert(u);
    }
  }

  VLOG(3) << "\tRemoving " << removed.size() << " vertices.";

  auto isRemoved = [&removed](int u) {
    return removed.find(u) != removed.end();
  };
  axLeft.erase(std::remove_if(axLeft.begin(), axLeft.end(), isRemoved),
               axLeft.end());
  axRight.erase(std::remove_if(axRight.begin(), axRight.end(), isRemoved),
                axRight.end());

  for (auto u : removed) {
    if ((*subdivisionIdx)[u] == -1) {
      graph->remove(u);
    }
    subdivGraph->remove(u);
  }

  std::vector<int> zeroDegrees;
  for (auto u : *subdivGraph) {
    if (subdivGraph->degree(u) == 0) {
      zeroDegrees.push_back(u);
      removed.insert(u);
    }
  }

  for (auto u : zeroDegrees) {
    if ((*subdivisionIdx)[u] == -1) {
      graph->remove(u);
    }
    subdivGraph->remove(u);
  }
}

std::pair<size_t, double> Solver::SelectHighestPotentialFlowVector(const std::vector<std::vector<double>>& flows) const {
  size_t highest = -1;
  double highest_potential = std::numeric_limits<double>::lowest();
  for (size_t i = 0; i < flows.size(); ++i) {
    double potential = ProjectedPotential(flows[i]);
    if (potential > highest_potential) {
      highest = i;
      highest_potential = potential;
    }
  }
  VLOG(3) << V(highest_potential) << V(highest);
  return std::make_pair(highest, highest_potential);
}

Result Solver::computeInternal(Parameters params) {
  if (numSplitNodes <= 1) {
    VLOG(3) << "Cut matching exited early with " << numSplitNodes
            << " subdivision vertices.";
    return Result{};
  }

  const int totalVolume = subdivGraph->globalVolume();
  const int lowerVolumeBalance = totalVolume / 2 / 10 / T;

  // TODO minBalance is way too high?? Should revisit.
  const int targetVolumeBalance =
      std::max(lowerVolumeBalance, int(params.minBalance * totalVolume));

  Result result;

  std::vector<std::vector<double>> flow_vectors;
  for (int i = 0; i < params.num_flow_vectors; ++i) {
    flow_vectors.push_back(randomUnitVector());
  }

  int iterations = 0;
  const int iterationsToRun = std::max(params.minIterations, T);
  for (; iterations < iterationsToRun &&
         subdivGraph->globalVolume(subdivGraph->cbeginRemoved(),
                                   subdivGraph->cendRemoved()) <=
             targetVolumeBalance;
       ++iterations) {
    VLOG(2) << "Iteration " << iterations << " out of " << iterationsToRun
            << ".";

    if (params.samplePotential) {
      double p = samplePotential();
      // TODO numSplitNodes should be reducing in each iteration... this condition is wrong

      VLOG(2) << V(p) << " / limit = " << 1.0/(16.0*square(numSplitNodes));
      if (p < 1.0 / (16.0 * square(numSplitNodes))) {
        result.iterationsUntilValidExpansion =
            std::min(result.iterationsUntilValidExpansion, iterations);
      }
    }

    auto [flow_vector_id, potential] = SelectHighestPotentialFlowVector(flow_vectors);

    if (params.use_potential_based_dynamic_stopping_criterion && potential <= ProjectedPotentialConvergenceThreshold()) {
      if (result.iterationsUntilValidExpansion2 == std::numeric_limits<int>::max()) {
        result.iterationsUntilValidExpansion2 = iterations;
      }
      // TODO actually break; after testing how many fewer rounds we need
      // break;
    }

    {
      size_t max = 0;
      for (const auto& flow : flow_vectors) {
        size_t significant_entries = 0;
        for (double x : flow) {
          if (std::abs(x) > 1e-17)
            significant_entries++;
        }
        max = std::max(max, significant_entries);
      }
      VLOG(3) << "#significant entries" << max;
    }

    Timer timer; timer.Start();
    auto [axLeft, axRight] = proposeCut(flow_vectors[flow_vector_id], params);
    Timings::GlobalTimings().AddTiming(Timing::ProposeCut, timer.Restart());

    VLOG(3) << "Number of sources: " << axLeft.size()
            << " sinks: " << axRight.size();

    if (axLeft.empty() || axRight.empty()) {
      break;
    }

    subdivGraph->reset();
    for (const auto u : axLeft)
      subdivGraph->addSource(u, 1);
    for (const auto u : axRight)
      subdivGraph->addSink(u, 1);

    const int h = (int)ceil(1.0 / phi / std::log10(numSplitNodes));

    double excess_fraction = [&]() -> double {
        const size_t max_flow = std::min(axLeft.size(), axRight.size());
        double f = std::log10(numSplitNodes);
        if (f < 1.0) {
            return max_flow;     // we have to finish routing all of the flow
        }
        double fraction = 1.0 - (1. / iterationsToRun);
        // double fraction = 1.0 - (1. / (f*f));
        return fraction * max_flow;
    }();
    subdivGraph->excess_fraction = excess_fraction;

    VLOG(3) << "Computing flow with |S| = " << axLeft.size()
            << " |T| = " << axRight.size() << " and max height " << h << ".";
    const auto hasExcess = subdivGraph->compute(h);

    Timings::GlobalTimings().AddTiming(Timing::FlowMatch, timer.Restart());

    if (hasExcess.empty()) {
      VLOG(3) << "\tAll flow routed.";
    } else {
      VLOG(3) << "\tHas " << hasExcess.size() << " vertices with excess. Computing level cut.";
      const auto [cutLeft, cutRight] = subdivGraph->levelCut(h);
      VLOG(3) << "\tHas level cut with (" << cutLeft.size() << ", " << cutRight.size() << ") vertices.";

      RemoveCutSide(cutLeft, cutRight, axLeft, axRight);
    }

    Timings::GlobalTimings().AddTiming(Timing::Misc, timer.Restart());

    VLOG(3) << "Computing matching with |S| = " << axLeft.size() << " |T| = " << axRight.size() << ".";
    auto matching =
        subdivGraph->matching(axLeft);
    for (auto &p : matching) {
      int u = (*subdivisionIdx)[p.first];
      int v = (*subdivisionIdx)[p.second];

      num_matched_steps[u]++;
      num_matched_steps[v]++;

      for (auto& flow : flow_vectors) {
        const double avg = 0.5 * (flow[u] + flow[v]);
        flow[u] = avg;
        flow[v] = avg;
      }

      if (params.samplePotential) {
        for (int i : *subdivGraph) {
          int w = (*subdivisionIdx)[i];
          if (w >= 0) {
            flowMatrix[u][w] = 0.5 * (flowMatrix[u][w] + flowMatrix[v][w]);
            flowMatrix[v][w] = flowMatrix[u][w];
          }
        }
      }
    }

    Timings::GlobalTimings().AddTiming(Timing::Match, timer.Stop());

    VLOG(3) << "Found matching of size " << matching.size() << ".";
  }

  VLOG(2) << "Iterations until potential reached convergence limit " << V(result.iterationsUntilValidExpansion2)
              << V(result.iterationsUntilValidExpansion) << V(result.iterations);

  result.iterations = iterations;
  result.congestion = 1;
  for (auto u : *subdivGraph)
    for (auto e = subdivGraph->beginEdge(u); e != subdivGraph->endEdge(u); ++e)
      result.congestion = std::max(result.congestion, e->congestion);

  if (graph->size() != 0 && graph->removedSize() != 0 &&
        subdivGraph->globalVolume(subdivGraph->cbeginRemoved(), subdivGraph->cendRemoved())
                                   > lowerVolumeBalance)
    // We have: graph.volume(R) > m / (10 * T)
    result.type = Result::Balanced;
  else if (graph->removedSize() == 0)
    result.type = Result::Expander;
  else if (graph->size() == 0)
    graph->restoreRemoves(), result.type = Result::Expander;
  else
    result.type = Result::NearExpander;

  switch (result.type) {
    case Result::Balanced: {
      VLOG(2) << "Cut matching ran " << iterations
              << " iterations and resulted in balanced cut with size ("
              << graph->size() << ", " << graph->removedSize() << ") and volume ("
              << graph->globalVolume(graph->cbegin(), graph->cend()) << ", "
              << graph->globalVolume(graph->cbeginRemoved(), graph->cendRemoved())
              << ").";
      break;
    }
    case Result::Expander: {
      VLOG(2) << "Cut matching ran " << iterations
              << " iterations and resulted in expander.";
      break;
    }
    case Result::NearExpander: {
      VLOG(2) << "Cut matching ran " << iterations
              << " iterations and resulted in near expander of size "
              << graph->size() << ".";
      break;
    }
  }

  return result;
}


Result Solver::compute(Parameters params) {
  if (!params.tune_num_flow_vectors) { return computeInternal(params); }

  params.num_flow_vectors = 1;
  while (true) {
    VLOG(2) << "Testing convergence with " << V(params.num_flow_vectors) << "projected flow vectors";
    Result result = computeInternal(params);
    if (result.iterationsUntilValidExpansion <= result.iterationsUntilValidExpansion2) {
      result.num_flow_vectors_needed = params.num_flow_vectors;
      VLOG(2) << V(params.num_flow_vectors) << " were needed to converge similarly";
      return result;
    }
    params.num_flow_vectors++;
    if (result.type != Result::Expander) {
      // restoreRemoves() is safe to call repeatedly --> don't worry about the case expander and graph->size() == 0 (before restore)
      graph->restoreRemoves();
      subdivGraph->restoreRemoves();
    }
  }
}

} // namespace CutMatching
