// Randomized quickselect.
// Primary author: Ethan Fernandez
#include "quickselect/quickselect.hpp"

#include <stdexcept>
#include <utility>

// Algorithm version was selected via personal research specifically helpful was the site https://www.geeksforgeeks.org/dsa/quickselect-algorithm/
// The Algorithm and the specifics behind this particular implemenation version and lomuto were all then explained to me by Claude.
// Both partitions below are intentionally identical to the ones in mom.cpp, so that performance differences come from pivot selection only.

namespace sel {
namespace {

    std::size_t lomuto_partition(std::vector<Key>& a, std::size_t lo, std::size_t hi, Stats& stats) {
        const Key pivot = a[hi];

        std::size_t currentLow = lo;

        for (std::size_t i = lo; i < hi; ++i) {
            // Does a[i] <= pivot but as part of the skeleton that Claude made less() does a comparison
            // and counts it towards final running stats so that I can easily compare running time and comparison count
            // between the two algorithms.
            if (!less(pivot, a[i], stats)) {
                std::swap(a[currentLow], a[i]);
                ++currentLow;
            }
        }

        std::swap(a[currentLow], a[hi]);
        return currentLow;
    }

    struct Bounds { std::size_t lt, gt; };

    Bounds three_way_partition(std::vector<Key>& a, std::size_t lo, std::size_t hi, Key pivot, Stats& stats) {
        std::size_t lt = lo;  // a[lo..lt-1] < pivot
        std::size_t i = lo;   // a[lt..i-1] == pivot, a[i..gt] not yet examined
        std::size_t gt = hi;  // a[gt+1..hi] > pivot

        while (i <= gt) {
            if (less(a[i], pivot, stats)) {
                std::swap(a[lt], a[i]);
                ++lt;
                ++i;
            } else if (less(pivot, a[i], stats)) {
                std::swap(a[i], a[gt]);
                --gt;
            } else {
                ++i;
            }
        }

        return {lt, gt};
    
    }

}

    Key quickselect(std::vector<Key>& a, std::size_t k, std::mt19937_64& rng, Stats& stats, Partition scheme) {

        std::size_t lo = 0;
        std::size_t hi = a.size() - 1;

        while (lo < hi) {
            std::uniform_int_distribution<std::size_t> pick(lo, hi);
            const std::size_t r = pick(rng);

            if (scheme == Partition::Lomuto) {
                std::swap(a[r], a[hi]);
                const std::size_t p = lomuto_partition(a, lo, hi, stats);
                if (k == p) {
                    return a[p];
                } else if (k < p) {
                    // Note: k >= lo therefore p > lo >= 0
                    hi = p - 1;
                } else {
                    lo = p + 1;
                }
            } else {
                const Bounds b = three_way_partition(a, lo, hi, a[r], stats);
                if (k < b.lt) {
                    //Note: k >= lo therefore b.lt > lo >= 0
                    hi = b.lt - 1;
                } else if (k > b.gt) {
                    lo = b.gt + 1;
                } else {
                    return a[k];
                }
            }
        }

        return a[lo];
    }

}  // namespace sel
