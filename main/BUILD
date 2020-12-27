load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

cc_binary(
  name = "expander-decomp",
  srcs = ["expander_decomp.cpp"],
  deps = [
    "//lib:cluster_util",
    "@com_google_glog//:glog",
  ]
)
