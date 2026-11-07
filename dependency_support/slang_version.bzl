"""Exposes the sv-lang version selected by Bazel and the corresponding version
in the third_party/slang submodule, for //:slang_version_check.
"""

_DIGITS = "0123456789"


def _module_version(module_bazel):
    block = module_bazel[module_bazel.find("module("):]
    start = block.find('version = "')
    if start == -1:
        fail("slang_version: no version in @sv-lang's MODULE.bazel")
    start += len('version = "')
    return block[start:block.find('"', start)]


def _cmake_number(cmake, name):
    marker = "set({} ".format(name)
    start = cmake.find(marker)
    if start == -1:
        fail("slang_version: {} not found in third_party/slang/CMakeLists.txt".format(name))
    start += len(marker)
    for i in range(start, len(cmake)):
        if cmake[i] not in _DIGITS:
            return cmake[start:i]
    return cmake[start:]


def _impl(ctx):
    version = _module_version(ctx.read(ctx.path(Label("@sv-lang//:MODULE.bazel"))))
    expected = ".".join(version.split(".")[:2])

    workspace = ctx.workspace_root
    result = ctx.execute(["git", "-C", str(workspace), "ls-tree", "HEAD", ctx.attr.path])
    if result.return_code != 0:
        fail("slang_version: git ls-tree failed: {}".format(result.stderr))
    revision = result.stdout.strip().split("\t")[0].split(" ")[2]

    result = ctx.execute([
        "git", "-C", str(workspace) + "/" + ctx.attr.path,
        "show", revision + ":CMakeLists.txt",
    ])
    if result.return_code != 0:
        fail("slang_version: could not read CMakeLists.txt at {}; initialize the submodule " +
             "with `git submodule update --init --recursive`: {}".format(revision, result.stderr))
    actual = "{}.{}".format(
        _cmake_number(result.stdout, "SLANG_VERSION_MAJOR"),
        _cmake_number(result.stdout, "SLANG_VERSION_MINOR"),
    )

    ctx.file("expected", expected + "\n")
    ctx.file("actual", actual + "\n")
    ctx.file("BUILD", 'exports_files(["actual", "expected"], visibility = ["//visibility:public"])\n')


slang_version = repository_rule(
    implementation = _impl,
    attrs = {"path": attr.string(mandatory = True)},
    configure = True,  # re-evaluate each invocation so submodule updates are picked up
    local = True,
)
