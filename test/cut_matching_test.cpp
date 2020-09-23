#include "gtest/gtest.h"

#include "lib/cut_matching.hpp"

#include <vector>

TEST(CutMatching, ProjectFlowNoRounds) {
  const std::vector<double> xs = {0.1, 0.2, 0.3, 0.4};
  std::unordered_map<int, int> id;
  for (int i = 0; i < (int)xs.size(); ++i)
    id[i] = i;
  const auto ys = CutMatching::projectFlow({}, id, xs);
  for (int i = 0; i < (int)xs.size(); ++i)
    EXPECT_DOUBLE_EQ(xs[i], ys[i]);
}

TEST(CutMatching, ProjectFlowSingleRoundSingleMatch) {
  const std::vector<double> xs = {0.0, 0.25, 0.5, 0.25};
  const std::vector<std::pair<int, int>> round = {{0, 3}};
  std::unordered_map<int, int> id;
  for (int i = 0; i < (int)xs.size(); ++i)
    id[i] = i;
  const auto ys = CutMatching::projectFlow({round}, id, xs);
  EXPECT_DOUBLE_EQ(ys[0], 0.125);
  EXPECT_DOUBLE_EQ(ys[1], 0.25);
  EXPECT_DOUBLE_EQ(ys[2], 0.5);
  EXPECT_DOUBLE_EQ(ys[3], 0.125);
}

TEST(CutMatching, ProjectFlowTwoRoundsSingleMatches) {
  const std::vector<double> xs = {0.0, 0.25, 0.5, 0.25};
  const std::vector<std::pair<int, int>> round1 = {{0, 3}}, round2 = {{0, 2}};
  std::unordered_map<int, int> id;
  for (int i = 0; i < (int)xs.size(); ++i)
    id[i] = i;
  const auto ys = CutMatching::projectFlow({round1, round2}, id, xs);
  EXPECT_DOUBLE_EQ(ys[0], 0.3125);
  EXPECT_DOUBLE_EQ(ys[1], 0.25);
  EXPECT_DOUBLE_EQ(ys[2], 0.3125);
  EXPECT_DOUBLE_EQ(ys[3], 0.125);
}
