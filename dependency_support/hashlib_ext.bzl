load("@bazel_tools//tools/build_defs/repo:local.bzl", "new_local_repository")

def _vendored_hashlib_extension_impl(ctx):
    new_local_repository(
        name = "vendored-hashlib",
        path = "third_party/hashlib",
        build_file = "//:dependency_support/hashlib.BUILD.bazel",
    )

vendored_hashlib = module_extension(
    implementation = _vendored_hashlib_extension_impl,
)
