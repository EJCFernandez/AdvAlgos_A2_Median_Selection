# Randomized quickselect vs median-of-medians

Programming Assignment 2 for Advanced Algorithms (solo project), Theme 2: Deterministic vs Randomized. The project compares two algorithms for selection, which means finding the k-th smallest key (here always the median) without sorting the whole array. Randomized quickselect relies on a cheap random pivot, while median-of-medians (BFPRT) pays up front for a pivot that is guaranteed to be good. `std::nth_element` from libstdc++ is timed alongside them as a reference baseline.

| Module | Algorithm | Primary author |
|---|---|---|
| `src/quickselect/` | Randomized quickselect (Hoare, 1961) | Ethan Fernandez |
| `src/mom/` | Median-of-medians, BFPRT (Blum et al., 1973) | Ethan Fernandez |

## Deliverables

- **Report:** [`report/AdvAlgos_A2_Report_Final.pdf`](report/AdvAlgos_A2_Report_Final.pdf), with the Word source alongside it
- **Reflections:** [`report/Reflection_Quickselect.pdf`](report/Reflection_Quickselect.pdf) and [`report/Reflection_MedianOfMedians.pdf`](report/Reflection_MedianOfMedians.pdf)
- **Video (unlisted):** https://youtu.be/YML36Mb1mdU
- **AI use log:** [`ai-log.md`](ai-log.md)

## Repository layout

```
src/common/select_common.hpp     shared types and less(), the only way keys are compared
src/quickselect/                 randomized quickselect (Lomuto and three-way partitioning)
src/mom/                         median-of-medians (Lomuto and three-way, group size g)
tests/test_select.cpp            differential tests against sort-then-index
bench/bench.cpp                  benchmark harness (five experiment modes)
scripts/reproduce.sh             one command to build, test, run every experiment and plot
scripts/plot.py                  figures and summary tables from the CSVs
results/                         the final results used in the report
results_v1_batching_artifact/    an earlier, flawed run kept as evidence (see below)
report/                          report and reflections (PDF and Word)
ai-log.md                        record of AI use
```

Both algorithm modules are self-contained and hold identical copies of the two partition functions, so any difference between them comes from how the pivot is chosen.

## Requirements

- **Linux or WSL2 for anything timed.** MSVC and libstdc++ implement `std::uniform_int_distribution` differently, so the same seed gives quickselect different pivots on each platform. All results in this repository come from WSL2 Ubuntu.
- **CMake 3.16 or newer and a C++17 compiler.** The results were produced with GCC 15.2.0 and CMake 4.2.3.
- **Python 3 with matplotlib and numpy** for the plots (`pip install matplotlib numpy`).

## Build and test

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/test_select
```

The tests should print `114400 checks, 0 failures`. They compare both algorithms against sort-then-index on five input families (random, few distinct values, all equal, sorted and reverse sorted), covering every k for n ≤ 40 and 200 random sizes from 41 to 1,000 per family. The three-way variants are tested at every size and the slow Lomuto variants only up to n = 40. A further check fails if fewer than n − 1 comparisons are counted, which catches any comparison that bypasses `less()`.

CMake builds the library twice. The `selection` library is for timing, and in it `less()` compiles to a plain `a < b`. The `selection_counted` library defines `SEL_COUNT_COMPARISONS` so that every key comparison is counted. The tests and the `bench_counted` binary use the counted build, while `bench` uses the timing build.

On Windows with Visual Studio, the generator is multi-config, so build with `cmake --build build --config Release` and run `build\Release\test_select.exe`. That is fine for the tests, but run the benchmarks on Linux for the reason above.

## Reproducing the experiments

```bash
./scripts/reproduce.sh --quick    # smoke test: small sizes and 2 repetitions
./scripts/reproduce.sh            # full run, about 15 minutes on my laptop
```

> **Warning:** `reproduce.sh` writes into `results/`, which replaces the committed results, and the `--quick` run does this too. To get the committed results back, run `git checkout -- results/`. For a clean check, clone the repository into a separate folder first.

The script builds Release into `build-wsl/`, runs the tests and records the environment (date, commit, CPU, compiler and flags) in `results/env.txt`. It then runs all five experiments in the timing build and again in the counted build, and finishes by running `scripts/plot.py`. For steadier timings, plug the laptop in and close other programs. To pin the runs to one core, run `TASKSET="taskset -c 2" ./scripts/reproduce.sh`, although under WSL2 this pins a virtual CPU rather than a physical core.

Absolute times will differ on other hardware, but the shapes of the curves and the ratios between the algorithms are what the report relies on. The committed results come from an Intel Core Ultra 7 258V (four performance and four efficiency cores, 12 MiB L3) under WSL2 Ubuntu (kernel 6.6.87.2), built with `-O3 -DNDEBUG` at commit 7eac51c.

### Running one experiment

After building as above, each mode can also be run on its own (this also writes into `results/`), for example:

```bash
./build/bench sweep --out results/sweep_time.csv
./build/bench_counted sweep --reps 50 --out results/sweep_count.csv
python3 scripts/plot.py
```

| Mode | What it runs |
|---|---|
| `sweep` | Quickselect, median-of-medians (g = 5) and `nth_element` on random, few-distinct and sorted input, n = 10 to 10⁷ |
| `groups` | Median-of-medians with g = 3, 5 and 7 on random input, n = 10³ to 10⁷ |
| `fix` | Both algorithms under Lomuto and three-way partitioning on random input, n = 10³ to 10⁷ |
| `dups` | Both algorithms under both schemes on few-distinct and all-equal input, n = 250 doubling to 1,024,000, with a time cap |
| `tail` | Quickselect over 2,000 pivot seeds on one fixed random input of 10⁶ keys, plus 200 runs each of median-of-medians and `nth_element` |

| Option | Effect |
|---|---|
| `--out FILE` | CSV destination (default: standard output) |
| `--quick` | Small sizes (n ≤ 10⁵), 2 repetitions and a 1 s cap |
| `--reps N` | Repetitions for every size |
| `--max-n N` | Skip sizes above N |
| `--seed S` | Base seed (default 20261103) |
| `--cap SEC` | `dups` time cap in seconds (default 10) |
| `--seeds N` | `tail` number of quickselect pivot seeds (default 2000) |
| `--tail-n N` | `tail` input size (default 1000000) |

### How the comparison is kept fair

- Every input is generated from a seed built from the base seed, the input family, n and the repetition number, so every algorithm and both builds see exactly the same instances. Quickselect's pivot seeds are recorded too, so any run can be replayed.
- Input generation, copying and result checking all sit outside the timed region, and every result is checked against `std::nth_element`. A wrong answer is logged as `wrong` and makes the program exit with an error.
- The algorithm order rotates each repetition, and one warm-up run on its own input is discarded before timing.
- Small inputs are timed in batches of **distinct** inputs, never repeated copies of one input (see the note on `results_v1_batching_artifact/` below).
- In the `dups` mode a series stops before the first size predicted to take more than the cap, and those sizes are logged as `capped`.

## Results

| File | Contents |
|---|---|
| `results/<mode>_time.csv` | Raw rows from the timing build. Use these for times |
| `results/<mode>_count.csv` | Raw rows from the counted build. Use these for comparison counts |
| `results/summary.csv` | One row per configuration, with median time, quartiles and comparisons |
| `results/tail_summary.txt` | Percentiles, ratios and skewness for the tail experiment |
| `results/dups_slopes.csv` | Local log-log slopes for the duplicates experiment |
| `results/env.txt` | The machine, compiler, flags and commit for the run |
| `results/run.log` | Console output of the final full run |
| `results/figures/` | The seven figures below |

Every raw CSV has the columns `experiment, build, algorithm, scheme, g, family, n, k, rep, input_seed, pivot_seed, batch, time_ns, comparisons, status`. Times are reported as the median over repetitions with 25th to 75th percentile bars. Comparison counts are reported as the mean with standard-error bars, because the formulas in Section 2 of the report are expected values and quickselect's count varies widely from run to run.

| Figure file | In the report |
|---|---|
| `fig1_sweep_time.png` | Figure 1 |
| `fig2_sweep_comparisons.png` | Table 4.1 gives its key values |
| `fig3_time_ratio.png` | Figure 2 |
| `fig4_tail.png` | Figure 3 |
| `fig5_group_size.png` | Table 4.2 gives its key values |
| `fig6_cost_of_fix.png` | Figure 5 |
| `fig7_duplicates.png` | Figure 4 |

### About `results_v1_batching_artifact/`

This folder holds the raw CSVs from my first full run, which was flawed. That version of the harness timed small inputs in batches of identical copies of one input, so the branch predictor learned the exact branch pattern of the deterministic algorithms and made median-of-medians and `nth_element` look three to five times faster below n = 10⁴. Quickselect was unaffected because its random pivots change the pattern every run. The harness now batches distinct inputs, and every result in `results/` comes from the corrected run. I kept the flawed run as evidence, and it is discussed in Sections 3.4 and 8 of the report.

## AI use

I typed all of the algorithm code in `src/quickselect/` and `src/mom/` myself and I am its primary author. Claude (Anthropic) explained both algorithms and how to adapt the GeeksforGeeks versions to this project, including the three-way partition. It also wrote the project skeleton (CMake, the shared header, the tests and stubs), the n − 1 comparison test, `bench/bench.cpp`, `scripts/reproduce.sh` and `scripts/plot.py`, all of which I reviewed and ran, and it drafted parts of the report and the video script and slides, which I edited. `ai-log.md` and Section 8 of the report give the full record, including the cases where it was wrong.

## Sources

The two GeeksforGeeks pages were my starting points for the implementations and the Stack Overflow threads helped me check my understanding. The papers and the textbook are the sources for the theory.

- Blum, M., Floyd, R. W., Pratt, V. R., Rivest, R. L., & Tarjan, R. E. (1973). Time bounds for selection. *Journal of Computer and System Sciences, 7*(4), 448–461. https://doi.org/10.1016/S0022-0000(73)80033-9
- Cormen, T. H., Leiserson, C. E., Rivest, R. L., & Stein, C. (2022). *Introduction to algorithms* (4th ed.). MIT Press.
- Edge. (2012, June 2). *Quickselect algorithm simplified explanation* [Online forum post]. Stack Overflow. https://stackoverflow.com/questions/10861388/quickselect-algorithm-simplified-explanation
- GeeksforGeeks. (2024, August 14). *Quickselect algorithm*. https://www.geeksforgeeks.org/dsa/quickselect-algorithm/
- GeeksforGeeks. (2025, July 23). *K’th smallest/largest element in unsorted array: Worst case linear time*. https://www.geeksforgeeks.org/dsa/kth-smallest-largest-element-in-unsorted-array-worst-case-linear-time/
- Hoare, C. A. R. (1961). Algorithm 65: Find. *Communications of the ACM, 4*(7), 321–322. https://doi.org/10.1145/366622.366647
- Knuth, D. E. (1972). Mathematical analysis of algorithms. In *Information processing 71: Proceedings of IFIP Congress 1971* (pp. 19–27). North-Holland.
- Musser, D. R. (1997). Introspective sorting and selection algorithms. *Software: Practice and Experience, 27*(8), 983–993. https://doi.org/10.1002/(SICI)1097-024X(199708)27:8<983::AID-SPE117>3.0.CO;2-#
- Sayle, R., & Carlini, P. (2006, August 28). *[Patch] Change std::nth_element to O(N log N) worst case* [Online forum post]. libstdc++ mailing list archive. https://gcc.gnu.org/ml/libstdc++/2006-08/msg00028.html
- simon. (2012, February 28). *Understanding “median of medians” algorithm* [Online forum post]. Stack Overflow. https://stackoverflow.com/questions/9489061/understanding-median-of-medians-algorithm
