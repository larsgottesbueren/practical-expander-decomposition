load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

cc_binary(
  name = "cluster",
  srcs = ["cluster.cpp"],
  deps = [
    "//lib:cluster_util",
    "@com_google_glog//:glog",
  ]
)

cc_binary(
  name = "expander-decomp",
  srcs = ["expander_decomp.cpp"],
  deps = [
    "//lib:cluster_util",
    "@com_google_glog//:glog",
  ]
)

cc_binary(
  name = "sparse-cut",
  srcs = ["cut.cpp"],
  deps = [
    "//lib:cluster_util",
    "@com_google_glog//:glog",
  ]
)
