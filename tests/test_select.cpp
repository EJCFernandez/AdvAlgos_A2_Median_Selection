// Differential tests: both algorithms must agree with sort-then-index.
#include <algorithm>
#include <cstdio>
#include <exception>
#include <functional>
#include <random>
#include <string>
#include <vector>

#include "mom/mom.hpp"
#include "quickselect/quickselect.hpp"

using sel::Key;

namespace {

int failures = 0;
int checks = 0;

// include_lomuto: also test the Lomuto variants. Only done for small n, because
// Lomuto is quadratic (quickselect) or worse (median-of-medians) on inputs with
// many equal keys, which would make the test suite take minutes.
void check_all(const std::vector<Key>& input, std::size_t k, const std::string& family,
               std::mt19937_64& rng, bool include_lomuto) {
    std::vector<Key> sorted = input;
    std::sort(sorted.begin(), sorted.end());
    const Key expected = sorted[k];

    auto run = [&](const char* name,
                   const std::function<Key(std::vector<Key>&, sel::Stats&)>& f) {
        std::vector<Key> work = input;
        ++checks;
        try {
            sel::Stats s;
            Key got = f(work, s);
            if (input.size() >= 2 && s.comparisons < input.size() - 1) {
                if (failures < 20)
                    std::printf("FAIL %-14s family=%s n=%zu k=%zu only %llu comparisons counted "
                                "(need >= n-1): is something bypassing less()?\n",
                                name, family.c_str(), input.size(), k,
                                (unsigned long long)s.comparisons);
                ++failures;
                return;
            }
            std::vector<Key> after = work;
            std::sort(after.begin(), after.end());
            if (got != expected || after != sorted) {  // value right AND no elements lost
                if (failures < 20)
                    std::printf("FAIL %-14s family=%s n=%zu k=%zu expected=%lld got=%lld\n", name,
                                family.c_str(), input.size(), k, (long long)expected, (long long)got);
                ++failures;
            }
        } catch (const std::exception& e) {
            if (failures < 20)
                std::printf("FAIL %-14s family=%s n=%zu k=%zu threw: %s\n", name, family.c_str(),
                            input.size(), k, e.what());
            ++failures;
        }
    };

    using sel::Partition;
    run("qs-3way", [&](std::vector<Key>& w, sel::Stats& s) {
        return sel::quickselect(w, k, rng, s, Partition::ThreeWay);
    });
    for (std::size_t g : {3u, 5u, 7u}) {
        std::string name = "mom3way(g=" + std::to_string(g) + ")";
        run(name.c_str(), [&](std::vector<Key>& w, sel::Stats& s) {
            return sel::mom_select(w, k, g, s, Partition::ThreeWay);
        });
    }
    if (include_lomuto) {
        run("qs-lomuto", [&](std::vector<Key>& w, sel::Stats& s) {
            return sel::quickselect(w, k, rng, s, Partition::Lomuto);
        });
        for (std::size_t g : {3u, 5u, 7u}) {
            std::string name = "momLom(g=" + std::to_string(g) + ")";
            run(name.c_str(), [&](std::vector<Key>& w, sel::Stats& s) {
                return sel::mom_select(w, k, g, s, Partition::Lomuto);
            });
        }
    }
}

std::vector<Key> make(const std::string& family, std::size_t n, std::mt19937_64& rng) {
    std::vector<Key> v(n);
    if (family == "random") {
        std::uniform_int_distribution<Key> d(0, (Key)n * 10);
        for (auto& x : v) x = d(rng);
    } else if (family == "few_distinct") {
        std::uniform_int_distribution<Key> d(0, 3);
        for (auto& x : v) x = d(rng);
    } else if (family == "all_equal") {
        std::fill(v.begin(), v.end(), 7);
    } else if (family == "sorted") {
        for (std::size_t i = 0; i < n; ++i) v[i] = (Key)i;
    } else if (family == "reverse") {
        for (std::size_t i = 0; i < n; ++i) v[i] = (Key)(n - i);
    }
    return v;
}

}  // namespace

int main() {
    std::mt19937_64 rng(12345);
    const std::vector<std::string> families = {"random", "few_distinct", "all_equal", "sorted",
                                               "reverse"};

    // Small sizes: every k.
    for (const auto& fam : families)
        for (std::size_t n = 1; n <= 40; ++n)
            for (int rep = 0; rep < 3; ++rep) {
                auto v = make(fam, n, rng);
                for (std::size_t k = 0; k < n; ++k) check_all(v, k, fam, rng, true);
            }

    // Larger sizes: random k plus the extremes.
    std::uniform_int_distribution<std::size_t> nd(41, 1000);
    for (const auto& fam : families)
        for (int rep = 0; rep < 200; ++rep) {
            std::size_t n = nd(rng);
            auto v = make(fam, n, rng);
            std::uniform_int_distribution<std::size_t> kd(0, n - 1);
            for (std::size_t k : {std::size_t{0}, n - 1, n / 2, kd(rng)}) check_all(v, k, fam, rng, false);
        }

    std::printf("%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
