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


load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
  name = "eigen",
  build_file = "//:eigen.BUILD",
  url = "https://gitlab.com/libeigen/eigen/-/archive/3.3.7/eigen-3.3.7.tar",
  sha256 = "92d7a913120cb64bf2d00b55c9a4275a1c6a61f848bad6a212b6eba7d172deec",
  strip_prefix = "eigen-3.3.7",
)
