load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

git_repository(
  name = "googletest",
  remote = "https://github.com/google/googletest",
  commit = "703bd9caab50b139428cea1aaff9974ebee5742e",
  shallow_since = "1570114335 -0400",
)

git_repository(
  name = "com_google_glog",
  remote = "https://github.com/google/glog",
  commit = "96a2f23dca4cc7180821ca5f32e526314395d26a", shallow_since = "1553223106 +0900",
)

git_repository(
  name = "com_github_gflags_gflags",
  remote = "https://github.com/gflags/gflags.git",
  commit = "a386bd0f204cf99db253b3e84c56795dea8c397f", shallow_since = "1600856796 +0100",
)

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
http_archive(
  name = "com_google_absl",
  urls = ["https://github.com/abseil/abseil-cpp/archive/20200923.2.zip"],
  strip_prefix = "abseil-cpp-20200923.2",
  sha256 = "306639352ec60dcbfc695405e989e1f63d0e55001582a5185b0a8caf2e8ea9ca",
)
