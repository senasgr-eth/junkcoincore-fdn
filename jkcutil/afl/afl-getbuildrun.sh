#!/usr/bin/env bash
# Builds AFL and an instrumented junkcoin, then begins fuzzing.
# This script must be run from within the top level directory of a junkcoin clone.
# Pass it the name of a directory in ./src/fuzzing.
# Additional arguments are passed-through to AFL.

set -eu -o pipefail

FUZZ_CASE="$1"
shift 1

export AFL_INSTALL_DIR=$(realpath "./afl-temp")

if [ ! -d "$AFL_INSTALL_DIR" ]; then
    mkdir "$AFL_INSTALL_DIR"
    ./zcutil/afl/afl-get.sh "$AFL_INSTALL_DIR"
fi

./jkcutil/afl/afl-build.sh "$AFL_INSTALL_DIR" "$FUZZ_CASE" -j$(nproc)
./jkcutil/afl/afl-run.sh "$AFL_INSTALL_DIR" "$FUZZ_CASE" "$@"
