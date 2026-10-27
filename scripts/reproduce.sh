#!/usr/bin/env bash
# One command to rebuild, test, run every experiment and regenerate every plot.
set -euo pipefail
cd "$(dirname "$0")/.."
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/test_select
# TODO (Thu/Fri): run experiments and plotting
# ./build/bench ... > results/....csv
# python3 scripts/plot.py
