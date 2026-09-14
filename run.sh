#!/usr/bin/env bash
# run.sh [test|bench|all]   default: test
set -euo pipefail
cd "$(dirname "$0")"
JOBS=$(nproc)

case "${1:-test}" in
  dev)
    cmake --preset dev
    cmake --build build-dev -j"$JOBS"
    ctest --test-dir build-dev --output-on-failure
    ;;
  test)
    cmake --preset asan
    cmake --build build-asan -j"$JOBS"
    ctest --test-dir build-asan --output-on-failure
    ;;
  tsan)
    cmake --preset tsan
    cmake --build build-tsan -j"$JOBS"
    ctest --test-dir build-tsan --output-on-failure
    ;;
  bench)
    cmake --preset release
    cmake --build build-release -j"$JOBS"
    # pin to an isolated core; never benchmark from inside the IDE
    taskset -c 2 ./build-release/bench
    ;;
  all)
    "$0" test && "$0" bench
    ;;
  *)
    echo "usage: run.sh [dev|test|tsan|bench|all]"; exit 1
    ;;
esac
