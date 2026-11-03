#!/usr/bin/env bash
# One command to rebuild, test, run every experiment and regenerate every plot.
# Run from WSL/Linux (benchmarks are only valid from the Release build there).
#
#   ./scripts/reproduce.sh            full run (writes results/*.csv, results/env.txt)
#   ./scripts/reproduce.sh --quick    smoke test: small sizes, 2 reps, short cap
#
# For steadier timings: plug the laptop in, close other programs, and optionally
# pin to one core with  TASKSET="taskset -c 2" ./scripts/reproduce.sh
set -euo pipefail
cd "$(dirname "$0")/.."

QUICK=""
if [[ "${1:-}" == "--quick" ]]; then QUICK="--quick"; fi
BUILD=build-wsl
RUN_PREFIX="${TASKSET:-}"

cmake -S . -B "$BUILD" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD" -j
"./$BUILD/test_select"

mkdir -p results
{
  echo "date: $(date -Iseconds)"
  echo "git commit: $(git rev-parse --short HEAD 2>/dev/null || echo unknown)"
  echo "mode: ${QUICK:-full}"
  echo "taskset: ${RUN_PREFIX:-none}"
  echo; uname -a
  echo; lscpu
  echo; g++ --version | head -n 1
  cmake --version | head -n 1
  echo; grep -E "CMAKE_BUILD_TYPE|CMAKE_CXX_COMPILER:|CMAKE_CXX_FLAGS_RELEASE" "$BUILD/CMakeCache.txt" || true
} > results/env.txt 2>&1

# Timing build first (times), then counted build (comparison counts). Quickselect's
# comparison count varies a lot from run to run, so the counted build uses 50 reps
# for sweep/groups/fix (counting is cheap); plots show mean comparisons. The tail
# experiment always uses every pivot seed; dups keeps its fixed 3 reps (Lomuto is slow).
for mode in sweep groups fix tail dups; do
  echo "=== $mode (timing build) ==="
  $RUN_PREFIX "./$BUILD/bench" "$mode" $QUICK --out "results/${mode}_time.csv"
  echo "=== $mode (counted build) ==="
  COUNT_REPS=""
  if [[ -z "$QUICK" && "$mode" =~ ^(sweep|groups|fix)$ ]]; then COUNT_REPS="--reps 50"; fi
  $RUN_PREFIX "./$BUILD/bench_counted" "$mode" $QUICK $COUNT_REPS --out "results/${mode}_count.csv"
done

if [[ -s scripts/plot.py ]]; then
  python3 scripts/plot.py
fi
echo "All done. CSVs and env.txt are in results/."
