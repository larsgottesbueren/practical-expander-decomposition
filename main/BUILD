load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")


cc_library(
  name = "input_util",
  hdrs = ["util.hpp"],
  deps = [
    "//lib:cluster_util",
  ],
)

cc_binary(
  name = "edc",
  srcs = ["edc.cpp"],
  deps = [
    "input_util",
    "//lib:cluster_util",
    "@com_google_glog//:glog",
  ]
)

cc_binary(
  name = "edc-cut",
  srcs = ["edc_cut.cpp"],
  deps = [
    "input_util",
    "//lib:cluster_util",
    "@com_google_glog//:glog",
  ]
)
