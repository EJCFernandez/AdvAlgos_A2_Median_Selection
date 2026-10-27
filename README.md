# PA2: Randomized quickselect vs median-of-medians selection

Theme 2 (Deterministic vs Randomized), median finding. Solo project.

| Module | Algorithm | Primary author |
|---|---|---|
| `src/quickselect/` | Randomized quickselect | [Your name] |
| `src/mom/` | Median-of-medians (BFPRT) selection | [Your name] |

## Build (Linux / WSL)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/test_select
```

Two builds of the library are produced: `selection` (timing, no comparison
counting) and `selection_counted` (every key comparison counted). Tests and
comparison-count experiments use the counted build.

## Reproduce everything

```bash
./scripts/reproduce.sh
```

## Layout

- `src/common/` shared types and the comparison counter
- `src/quickselect/`, `src/mom/` the two algorithms
- `tests/` differential tests against sort-then-index
- `bench/` benchmark harness
- `scripts/` reproduction and plotting
- `results/` raw CSVs and figures
- `report/` report drafts
