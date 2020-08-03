load("@rules_cc//cc:defs.bzl", "cc_test")

cc_test(
  name = "cluster_util_test",
  srcs = glob(["**/*.cpp"]),
  deps = [
    "//lib:cluster_util",
    "@googletest//:gtest_main"
  ],
)
