// Benchmark harness for PA2: randomized quickselect vs median-of-medians.
// Written by Claude (AI) as infrastructure; reviewed by Ethan Fernandez.
// Disclosed in ai-log.md. The algorithms themselves are in src/.
//
// Usage:  bench <mode> [options]
//   modes:   sweep   size sweep, both algorithms + std::nth_element, 3 input families
//            groups  median-of-medians with g = 3, 5, 7 on random input
//            fix     Lomuto vs three-way on random input (the cost of the fix)
//            dups    Lomuto vs three-way on duplicate-heavy input, with a time cap
//            tail    quickselect over many pivot seeds on ONE fixed input
//   options: --out FILE     CSV destination (default: stdout)
//            --quick        small sizes and few reps (smoke test)
//            --reps N       override repetitions for every size
//            --max-n N      skip sizes above N
//            --seed S       base seed (default 20261103)
//            --cap SEC      dups: stop a series once a run exceeds SEC (default 10)
//            --seeds N      tail: number of quickselect seeds (default 2000)
//            --tail-n N     tail: input size (default 1000000)
//
// Built twice by CMake: `bench` (timing build, comparisons not counted) and
// `bench_counted` (SEL_COUNT_COMPARISONS defined). Use times from `bench` and
// comparison counts from `bench_counted`; the `build` column says which is which.
//
// Fairness rules implemented here:
//   * every (family, n, rep) input is generated once from a recorded seed and each
//     algorithm gets its own fresh copy of it;
//   * input generation, copying and result checking are outside the timed region;
//   * one discarded warm-up/calibration run per series and size, on its OWN input
//     (never one that is then timed), skipped once runs get slow;
//   * small n is timed in batches of DISTINCT inputs (never repeated copies of one
//     input: repeating identical data lets the branch predictor learn the exact
//     branch pattern of a deterministic algorithm and makes it look ~3x faster);
//   * algorithm order is rotated per rep so no algorithm always runs first;
//   * every result is checked against std::nth_element; a mismatch is written as
//     status=wrong and makes the program exit non-zero.

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <string>
#include <vector>

#include "mom/mom.hpp"
#include "quickselect/quickselect.hpp"

using sel::Key;
using sel::Partition;

namespace {

#ifdef SEL_COUNT_COMPARISONS
const char* const kBuild = "counted";
#else
const char* const kBuild = "timing";
#endif

// ---------- options ----------
struct Options {
    std::string mode;
    std::string out;
    bool quick = false;
    int reps = 0;  // 0 = per-size default
    std::size_t max_n = 0;  // 0 = no limit
    std::uint64_t seed = 20261103;
    double cap_sec = 10.0;
    int tail_seeds = 2000;
    std::size_t tail_n = 1000000;
};

// ---------- seeds ----------
std::uint64_t splitmix(std::uint64_t x) {
    x += 0x9E3779B97F4A7C15ULL;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
    return x ^ (x >> 31);
}
// Inputs depend only on (base seed, family, n, rep), so sweep/groups/fix use the
// SAME instances wherever they overlap, and the timing and counted builds see
// identical inputs and identical quickselect pivot sequences.
std::uint64_t input_seed(std::uint64_t base, int family, std::size_t n, int rep) {
    return splitmix(splitmix(splitmix(base ^ (std::uint64_t)family) ^ n) ^ (std::uint64_t)rep);
}
std::uint64_t pivot_seed_for(std::uint64_t in_seed) { return splitmix(in_seed ^ 0xA5A5A5A5ULL); }

// ---------- input families (same definitions as tests/test_select.cpp) ----------
enum Family { RANDOM = 0, FEW_DISTINCT = 1, ALL_EQUAL = 2, SORTED = 3, REVERSE = 4 };
const char* family_name(int f) {
    static const char* names[] = {"random", "few_distinct", "all_equal", "sorted", "reverse"};
    return names[f];
}
std::vector<Key> make_input(int family, std::size_t n, std::uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::vector<Key> v(n);
    switch (family) {
        case RANDOM: {
            std::uniform_int_distribution<Key> d(0, (Key)n * 10);
            for (auto& x : v) x = d(rng);
            break;
        }
        case FEW_DISTINCT: {
            std::uniform_int_distribution<Key> d(0, 3);
            for (auto& x : v) x = d(rng);
            break;
        }
        case ALL_EQUAL: std::fill(v.begin(), v.end(), 7); break;
        case SORTED: for (std::size_t i = 0; i < n; ++i) v[i] = (Key)i; break;
        case REVERSE: for (std::size_t i = 0; i < n; ++i) v[i] = (Key)(n - i); break;
    }
    return v;
}

// ---------- algorithms ----------
enum class Kind { QS, MOM, NTH };
struct Algo {
    Kind kind;
    Partition scheme;
    std::size_t g;  // only used by MOM
};
const char* algo_name(const Algo& a) {
    return a.kind == Kind::QS ? "quickselect" : a.kind == Kind::MOM ? "mom" : "nth_element";
}
const char* scheme_name(const Algo& a) {
    if (a.kind == Kind::NTH) return "na";
    return a.scheme == Partition::Lomuto ? "lomuto" : "threeway";
}
std::string series_label(const Algo& a) {
    std::string s = std::string(algo_name(a)) + "/" + scheme_name(a);
    if (a.kind == Kind::MOM) s += "/g" + std::to_string(a.g);
    return s;
}

Key run_algo(const Algo& a, std::vector<Key>& w, std::size_t k, std::mt19937_64& rng, sel::Stats& st) {
    switch (a.kind) {
        case Kind::QS: return sel::quickselect(w, k, rng, st, a.scheme);
        case Kind::MOM: return sel::mom_select(w, k, a.g, st, a.scheme);
        case Kind::NTH:
            // Comparator routed through less() so nth_element is counted the same way.
            std::nth_element(w.begin(), w.begin() + (std::ptrdiff_t)k, w.end(),
                             [&st](Key x, Key y) { return sel::less(x, y, st); });
            return w[k];
    }
    return 0;
}

Key reference_value(const std::vector<Key>& input, std::size_t k) {
    std::vector<Key> c = input;
    std::nth_element(c.begin(), c.begin() + (std::ptrdiff_t)k, c.end());
    return c[k];
}

// Fast runs (small n) finish in well under what the clock resolves reliably, so
// they are timed in batches: one timer around `batch` runs, time divided by batch.
// Each run in a batch gets a DIFFERENT input from a per-rep pool (see run_grid);
// every algorithm uses the same pool, taking its first `batch` inputs. The batch
// size is chosen per series and size from one single-run calibration on a separate
// input (which also serves as the warm-up), aiming for about 5 ms per measurement,
// capped so a pool stays under ~200k keys. Slow runs (e.g. Lomuto on duplicates)
// therefore always get batch = 1.
std::size_t max_batch_for(std::size_t n) { return n < 10000 ? (200000 + n - 1) / n : 1; }

struct Measurement {
    double ns_per_run;
    double comps_per_run;
    bool ok;
};

// Times `batch` runs, run b on a fresh copy of pool[b]. pool.size() >= batch.
Measurement measure(const Algo& a, const std::vector<std::vector<Key>>& pool,
                    const std::vector<Key>& expected, std::size_t k, std::size_t batch,
                    std::uint64_t pivot_seed) {
    std::vector<std::vector<Key>> work(pool.begin(), pool.begin() + (std::ptrdiff_t)batch);  // copies OUTSIDE the timer
    std::vector<Key> results(batch);
    std::mt19937_64 rng(pivot_seed);
    sel::Stats st;

    const auto t0 = std::chrono::steady_clock::now();
    for (std::size_t b = 0; b < batch; ++b) results[b] = run_algo(a, work[b], k, rng, st);
    const auto t1 = std::chrono::steady_clock::now();

    bool ok = true;
    for (std::size_t b = 0; b < batch; ++b) ok = ok && (results[b] == expected[b]);  // checked OUTSIDE the timer
    const double ns = (double)std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
    return {ns / (double)batch, (double)st.comparisons / (double)batch, ok};
}

// ---------- CSV ----------
FILE* g_out = stdout;
bool g_any_wrong = false;

void csv_header() {
    std::fprintf(g_out,
                 "experiment,build,algorithm,scheme,g,family,n,k,rep,input_seed,pivot_seed,batch,"
                 "time_ns,comparisons,status\n");
}
void csv_row(const char* exp, const Algo& a, int family, std::size_t n, std::size_t k, int rep,
             std::uint64_t in_seed, std::uint64_t pv_seed, std::size_t batch, const Measurement* m,
             const char* status) {
    std::fprintf(g_out, "%s,%s,%s,%s,", exp, kBuild, algo_name(a), scheme_name(a));
    if (a.kind == Kind::MOM) std::fprintf(g_out, "%zu", a.g);
    std::fprintf(g_out, ",%s,%zu,%zu,%d,%llu,", family_name(family), n, k, rep, (unsigned long long)in_seed);
    if (a.kind == Kind::QS) std::fprintf(g_out, "%llu", (unsigned long long)pv_seed);
    std::fprintf(g_out, ",%zu,", batch);
    if (m) {
        std::fprintf(g_out, "%.1f,", m->ns_per_run);
#ifdef SEL_COUNT_COMPARISONS
        std::fprintf(g_out, "%.1f", m->comps_per_run);
#endif
    } else {
        std::fprintf(g_out, ",");
    }
    std::fprintf(g_out, ",%s\n", status);
    std::fflush(g_out);
}

int default_reps(std::size_t n) {
    if (n <= 100000) return 15;
    if (n <= 1000000) return 9;
    return 5;
}

double median(std::vector<double> v) {
    if (v.empty()) return 0;
    std::sort(v.begin(), v.end());
    const std::size_t m = v.size() / 2;
    return v.size() % 2 ? v[m] : 0.5 * (v[m - 1] + v[m]);
}

// ---------- generic grid: families x sizes x reps x algorithms ----------
struct SeriesState {
    bool capped = false;
    double last_median_ns = -1;  // median time at the previous size
    double prev_median_ns = -1;  // median time two sizes back
};

void run_grid(const char* exp, const Options& opt, const std::vector<int>& families,
              const std::vector<std::size_t>& sizes, const std::vector<Algo>& algos, int fixed_reps,
              bool use_cap) {
    const double cap_ns = opt.cap_sec * 1e9;
    for (int fam : families) {
        std::vector<SeriesState> state(algos.size());
        for (std::size_t n : sizes) {
            if (opt.max_n && n > opt.max_n) continue;
            const std::size_t k = n / 2;
            const int reps = opt.reps ? opt.reps : (fixed_reps ? fixed_reps : default_reps(n));
            std::vector<std::size_t> batch(algos.size(), 1);  // fixed per series for all reps of this n
            std::fprintf(stderr, "[%s/%s] %s n=%zu reps=%d\n", exp, kBuild, family_name(fam), n, reps);

            // Calibration / warm-up input: its own seed, never timed.
            const std::vector<std::vector<Key>> cal_pool = {make_input(fam, n, input_seed(opt.seed, fam, n, 1000000))};
            const std::vector<Key> cal_expected = {reference_value(cal_pool[0], k)};

            std::vector<std::vector<double>> times(algos.size());
            for (int rep = 0; rep < reps; ++rep) {
                const std::uint64_t in_seed = input_seed(opt.seed, fam, n, rep);
                const std::uint64_t pv_seed = pivot_seed_for(in_seed);
                // Pool of distinct inputs for this rep: input 0 uses in_seed (the seed
                // recorded in the CSV); input j > 0 uses splitmix(in_seed + j). The pool
                // is sized for the largest batch any series needs at this n.
                std::size_t pool_size = 1;
                if (rep > 0)
                    for (std::size_t b : batch) pool_size = std::max(pool_size, b);
                else
                    pool_size = max_batch_for(n);  // batch sizes are not known until calibration
                std::vector<std::vector<Key>> pool;
                std::vector<Key> expected;
                pool.reserve(pool_size);
                for (std::size_t j = 0; j < pool_size; ++j) {
                    pool.push_back(make_input(fam, n, j == 0 ? in_seed : splitmix(in_seed + j)));
                    expected.push_back(reference_value(pool.back(), k));
                }

                for (std::size_t j = 0; j < algos.size(); ++j) {
                    const std::size_t ai = (j + (std::size_t)rep) % algos.size();  // rotate order
                    const Algo& a = algos[ai];
                    SeriesState& s = state[ai];
                    if (s.capped) {
                        if (rep == 0) csv_row(exp, a, fam, n, k, rep, in_seed, pv_seed, 1, nullptr, "capped");
                        continue;
                    }
                    // Rep 0: one single-run calibration, discarded (it is also the warm-up),
                    // used to choose this series' batch size. Skipped once the series is slow.
                    if (rep == 0 && (s.last_median_ns < 0 || s.last_median_ns < 2e8)) {
                        const Measurement cal = measure(a, cal_pool, cal_expected, k, 1, pv_seed);
                        const double target_ns = 5e6;
                        if (cal.ns_per_run < target_ns) {
                            const double want = std::ceil(target_ns / std::max(cal.ns_per_run, 1.0));
                            batch[ai] = std::max<std::size_t>(
                                1, std::min<std::size_t>(max_batch_for(n), (std::size_t)want));
                        }
                    }

                    const Measurement m = measure(a, pool, expected, k, batch[ai], pv_seed);
                    if (!m.ok) {
                        g_any_wrong = true;
                        std::fprintf(stderr, "WRONG RESULT: %s %s n=%zu rep=%d\n", series_label(a).c_str(),
                                     family_name(fam), n, rep);
                    }
                    csv_row(exp, a, fam, n, k, rep, in_seed, pv_seed, batch[ai], &m, m.ok ? "ok" : "wrong");
                    times[ai].push_back(m.ns_per_run);
                }
            }

            // Time cap: a running call can't be interrupted, so instead stop a series
            // BEFORE a size it is predicted to blow past the cap at.
            for (std::size_t ai = 0; ai < algos.size(); ++ai) {
                SeriesState& s = state[ai];
                if (s.capped || times[ai].empty()) continue;
                const double med = median(times[ai]);
                s.prev_median_ns = s.last_median_ns;
                s.last_median_ns = med;
                if (!use_cap) continue;
                const double growth = s.prev_median_ns > 0 ? std::max(2.0, med / s.prev_median_ns) : 4.0;
                if (med > cap_ns || med * growth > cap_ns) {
                    s.capped = true;
                    std::fprintf(stderr, "  capping %s on %s after n=%zu (median %.2f s)\n",
                                 series_label(algos[ai]).c_str(), family_name(fam), n, med / 1e9);
                }
            }
        }
    }
}

// ---------- tail: many pivot seeds on one fixed input ----------
void run_tail(const Options& opt) {
    const std::size_t n = opt.quick ? 100000 : opt.tail_n;
    const int qs_runs = opt.quick ? 50 : opt.tail_seeds;
    const int baseline_runs = opt.quick ? 20 : 200;
    const std::size_t k = n / 2;
    const std::uint64_t in_seed = input_seed(opt.seed, RANDOM, n, 0);
    // By design every run uses the SAME input (only the pivot seed varies), so the
    // spread isolates quickselect's randomness. n is large enough that repeating
    // it cannot be learned by the branch predictor (its times match the sweep's).
    const std::vector<std::vector<Key>> pool = {make_input(RANDOM, n, in_seed)};
    const std::vector<Key> expected = {reference_value(pool[0], k)};
    const std::size_t batch = 1;

    struct Job { Algo a; int runs; };
    const std::vector<Job> jobs = {
        {{Kind::QS, Partition::ThreeWay, 0}, qs_runs},              // the distribution of interest
        {{Kind::MOM, Partition::ThreeWay, 5}, baseline_runs},       // deterministic: machine-noise baseline
        {{Kind::NTH, Partition::ThreeWay, 0}, baseline_runs},       // fixed pivots too: second baseline
    };
    for (const Job& job : jobs) {
        std::fprintf(stderr, "[tail/%s] %s n=%zu runs=%d\n", kBuild, series_label(job.a).c_str(), n, job.runs);
        for (int w = 0; w < 3; ++w) (void)measure(job.a, pool, expected, k, batch, splitmix(w));  // warm-up
        for (int i = 0; i < job.runs; ++i) {
            const std::uint64_t pv_seed = splitmix(opt.seed ^ (0x7A11ULL + (std::uint64_t)i));
            const Measurement m = measure(job.a, pool, expected, k, batch, pv_seed);
            if (!m.ok) g_any_wrong = true;
            csv_row("tail", job.a, RANDOM, n, k, i, in_seed, pv_seed, batch, &m, m.ok ? "ok" : "wrong");
        }
    }
}

std::vector<std::size_t> doubling(std::size_t from, std::size_t to) {
    std::vector<std::size_t> v;
    for (std::size_t n = from; n <= to; n *= 2) v.push_back(n);
    return v;
}

void usage() {
    std::fprintf(stderr,
                 "usage: bench <sweep|groups|fix|dups|tail> [--out FILE] [--quick] [--reps N] "
                 "[--max-n N] [--seed S] [--cap SEC] [--seeds N] [--tail-n N]\n");
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) { usage(); return 2; }
    Options opt;
    opt.mode = argv[1];
    for (int i = 2; i < argc; ++i) {
        const std::string a = argv[i];
        auto next = [&]() -> const char* {
            if (i + 1 >= argc) { usage(); std::exit(2); }
            return argv[++i];
        };
        if (a == "--out") opt.out = next();
        else if (a == "--quick") opt.quick = true;
        else if (a == "--reps") opt.reps = std::atoi(next());
        else if (a == "--max-n") opt.max_n = (std::size_t)std::strtoull(next(), nullptr, 10);
        else if (a == "--seed") opt.seed = std::strtoull(next(), nullptr, 10);
        else if (a == "--cap") opt.cap_sec = std::atof(next());
        else if (a == "--seeds") opt.tail_seeds = std::atoi(next());
        else if (a == "--tail-n") opt.tail_n = (std::size_t)std::strtoull(next(), nullptr, 10);
        else { usage(); return 2; }
    }
    if (opt.quick) {
        if (!opt.max_n) opt.max_n = 100000;
        if (!opt.reps) opt.reps = 2;
        opt.cap_sec = std::min(opt.cap_sec, 1.0);
    }
    if (!opt.out.empty()) {
        g_out = std::fopen(opt.out.c_str(), "w");
        if (!g_out) { std::perror(opt.out.c_str()); return 1; }
    }
    csv_header();

    const Algo qs3{Kind::QS, Partition::ThreeWay, 0}, qsL{Kind::QS, Partition::Lomuto, 0};
    const Algo mom3{Kind::MOM, Partition::ThreeWay, 5}, momL{Kind::MOM, Partition::Lomuto, 5};
    const Algo nth{Kind::NTH, Partition::ThreeWay, 0};
    const std::vector<std::size_t> decades = {1000, 10000, 100000, 1000000, 10000000};

    if (opt.mode == "sweep") {
        const std::vector<std::size_t> sizes = {10,     30,     100,     300,     1000,    3000,    10000,
                                                30000,  100000, 300000,  1000000, 3000000, 10000000};
        run_grid("sweep", opt, {RANDOM, FEW_DISTINCT, SORTED}, sizes, {qs3, mom3, nth}, 0, false);
    } else if (opt.mode == "groups") {
        run_grid("groups", opt, {RANDOM}, decades,
                 {{Kind::MOM, Partition::ThreeWay, 3}, mom3, {Kind::MOM, Partition::ThreeWay, 7}}, 0, false);
    } else if (opt.mode == "fix") {
        run_grid("fix", opt, {RANDOM}, decades, {qs3, qsL, mom3, momL}, 0, false);
    } else if (opt.mode == "dups") {
        run_grid("dups", opt, {FEW_DISTINCT, ALL_EQUAL}, doubling(250, 1024000), {qsL, qs3, momL, mom3}, 3, true);
    } else if (opt.mode == "tail") {
        run_tail(opt);
    } else {
        usage();
        return 2;
    }

    if (g_out != stdout) std::fclose(g_out);
    if (g_any_wrong) {
        std::fprintf(stderr, "ERROR: at least one result disagreed with std::nth_element\n");
        return 1;
    }
    return 0;
}
