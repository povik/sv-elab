#!/usr/bin/env bash
# Fails unless the sv-lang version selected by Bazel matches the version in
# third_party/slang/CMakeLists.txt.
set -euo pipefail

repo="$(dirname "$(find "$TEST_SRCDIR" -name expected -print -quit)")"
expected="$(< "$repo/expected")"
actual="$(< "$repo/actual")"

echo "expected (Bazel sv-lang): $expected"
echo "actual   (submodule):     $actual"

if [[ "$expected" != "$actual" ]]; then
    echo "ERROR: the sv-lang version selected by Bazel does not match" \
        "third_party/slang. Update the override and the submodule together." >&2
    exit 1
fi
