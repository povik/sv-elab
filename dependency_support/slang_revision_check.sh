#!/usr/bin/env bash
# Fails unless SLANG_VERSION_HASH (from slang_ext.bzl, passed via $EXPECTED_SLANG_REVISION)
# matches the third_party/slang submodule gitlink.
set -euo pipefail
actual="$(tr -d '[:space:]' < "$(find "$TEST_SRCDIR" -name revision -path '*slang_revision*' -print -quit)")"
echo "expected (slang_ext.bzl): $EXPECTED_SLANG_REVISION"
echo "actual   (submodule):     $actual"
if [[ "$EXPECTED_SLANG_REVISION" != "$actual" ]]; then
    echo "ERROR: SLANG_VERSION_HASH in dependency_support/slang_ext.bzl does not match" \
        "the third_party/slang submodule revision. Bump SLANG_VERSION_HASH when" \
        "updating the submodule." >&2
    exit 1
fi
