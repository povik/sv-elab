"""Macro for running a yosys script as an sh_test."""

load("@rules_shell//shell:sh_test.bzl", "sh_test")

def yosys_test(name, script, data = []):
    """Runs `yosys -m slang.so <script>` from the script's directory.

    Pass/fail is the yosys exit code, matching tests/CMakeLists.txt.
    """
    sh_test(
        name = name,
        srcs = ["//tests:run_test.sh"],
        args = [
            "$(location @yosys//:yosys)",
            "$(location //src/yosys_plugin:slang.so)",
            "$(location %s)" % script,
        ],
        data = [
            "@yosys//:yosys",
            "//src/yosys_plugin:slang.so",
            script,
        ] + data,
    )

