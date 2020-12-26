# Expander decomposition

This is an implementation of [Expander decomposition and pruning: Faster,
stronger, and simpler.](https://arxiv.org/pdf/1812.08958.pdf) by Thatchaphol
Saranurak and Di Wang.

## Building

The project is built using the [Bazel](https://bazel.build) build system. Using
the 'build' command a binary will be built and moved to
'bazel-bin/main/expander-decomp':

``` shell
# Build optimized configuration
bazel build -c opt //main:expander-decomp

# Build debug configuration
bazel build -c dbg //main:expander-decomp

# Build with better debug information and sanitizers
bazel build --config debug --compilation_mode dbg --sandbox_debug //main:expander-decomp
```

## Testing

Tests are run using Bazel and [Google Test](https://github.com/google/googletest):

``` shell
bazel test --test_output=all --compilation_mode dbg //test:cluster_util_test --sandbox_debug
```
