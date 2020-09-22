bazel test --config=default --test_output=all --compilation_mode dbg //test:cluster_util_test --sandbox_debug
# bazel test --config=default --test_output=all  //test:cluster_util_test
# lldb ./bazel-bin/test/cluster_util_test -- --gtest_break_on_failure
