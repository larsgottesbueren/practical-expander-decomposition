load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
  name = "cluster_util",
  srcs = glob(["**/*.cpp"]),
  hdrs = glob(["**/*.hpp"]),
  deps = [
    "@boost//:graph"
  ],
  visibility = [
    "//main:__pkg__",
    "//test:__pkg__"
  ],
)
