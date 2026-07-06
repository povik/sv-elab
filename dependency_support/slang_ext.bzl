load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

# Version of the slang library tracked in third_party/slang for the
# regular build. In BAZEL, we don't have to use the submodule but
# can refer to the hash here directly.
# Bump when the submodule is updated. SLANG_VERSION_PATCH/HASH are
# normally derived from `git describe`
SLANG_VERSION_MAJOR = 11
SLANG_VERSION_MINOR = 0
SLANG_VERSION_PATCH = 0
SLANG_VERSION_HASH = "6175750969f17289695f94be5105f6006c82eb61"

def _vendored_slang_extension_impl(ctx):
    git_repository(
        name = "vendored-slang",
        remote = "https://github.com/MikePopoloski/slang.git",
        commit = SLANG_VERSION_HASH,
        build_file = Label("//:dependency_support/slang.BUILD"),
    )

vendored_slang_extension = module_extension(
    implementation = _vendored_slang_extension_impl,
)
