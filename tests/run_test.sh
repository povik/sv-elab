#!/usr/bin/env bash
# Args: $1 = yosys binary, $2 = slang.so plugin, $3 = test script (all runfiles-relative)
set -euo pipefail

yosys_bin="$(readlink -f "$1")"
plugin="$(readlink -f "$2")"
script="$(readlink -f "$3")"

# Tests read sidecar files (.hex/.bin/.sv) by bare relative path and some
# glob *.sv, so run from the script's own directory — same as CMake's
# WORKING_DIRECTORY ${test_dir}.
cd "$(dirname "$script")"
exec "$yosys_bin" -m "$plugin" "$(basename "$script")"

