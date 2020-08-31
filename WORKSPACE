load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
git_repository(
  name = "com_github_nelhage_rules_boost",
  commit = "1e3a69bf2d5cd10c34b74f066054cd335d033d71",
  remote = "https://github.com/nelhage/rules_boost",
  shallow_since = "1591047380 -0700",
)

load("@com_github_nelhage_rules_boost//:boost/boost.bzl", "boost_deps")
boost_deps()

git_repository(
  name = "googletest",
  remote = "https://github.com/google/googletest",
  commit = "703bd9caab50b139428cea1aaff9974ebee5742e",
  shallow_since = "1570114335 -0400",
)

load("@bazel_tools//tools/build_defs/repo:git.bzl", "new_git_repository")
new_git_repository(
  name = "eigen",
  remote = "https://gitlab.com/libeigen/eigen",
  commit = "25424d91f60a9f858e7dc1c7936021cc1dd72019",
  build_file = "//:eigen.BUILD",
  shallow_since = "1598437940 +0200",
)
