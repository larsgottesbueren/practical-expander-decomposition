load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
  name = "cluster_util",
  srcs = glob(["**/*.cpp"]),
  hdrs = glob(["**/*.hpp"]),
  deps = [
    "@com_google_glog//:glog",
    "@com_google_absl//absl/container:flat_hash_set",
    "@com_google_absl//absl/container:flat_hash_map",
  ],
  visibility = [
    "//main:__pkg__",
    "//test:__pkg__"
  ],
)
