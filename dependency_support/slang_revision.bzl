"""Records the submodule gitlink revision (git ls-tree HEAD <path>) as a file."""

def _impl(ctx):
    workspace = ctx.path(Label("//:MODULE.bazel")).dirname
    result = ctx.execute(["git", "-C", str(workspace), "ls-tree", "HEAD", ctx.attr.path])
    if result.return_code != 0:
        fail("slang_revision: git ls-tree failed: {}".format(result.stderr))
    revision = result.stdout.strip().split("\t")[0].split(" ")[2]
    ctx.file("revision", revision + "\n")
    ctx.file("BUILD", 'exports_files(["revision"], visibility = ["//visibility:public"])\n')

slang_revision = repository_rule(
    implementation = _impl,
    attrs = {"path": attr.string(mandatory = True)},
    configure = True,  # re-evaluate each invocation, so submodule updates are picked up
    local = True,
)
