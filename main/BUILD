load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

cc_binary(
  name = "edc",
  srcs = ["edc.cpp"],
  deps = [
    "//lib:cluster_util",
    "@com_google_glog//:glog",
  ]
)
