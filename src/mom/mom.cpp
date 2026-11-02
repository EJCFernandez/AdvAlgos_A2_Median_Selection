// Deterministic median-of-medians (BFPRT) selection.
// Primary author: Ethan Fernandez

#include "mom/mom.hpp"

#include <stdexcept>
#include <algorithm>
#include <utility>

// Algorithm version was selected via personal research specifically helpful was the site https://www.geeksforgeeks.org/dsa/kth-smallest-largest-element-in-unsorted-array-worst-case-linear-time/
// The Algorithm and the specifics of how this implementation changes given the skeleton were then further explained to me by Claude.

namespace sel {

namespace {

    // Declared early because median_of_medians and select_range call each other
    std::size_t select_range(std::vector<Key>& a, std::size_t lo, std::size_t hi, std::size_t k, std::size_t g, Stats& stats, Partition scheme);

    // This partition is intentionally identical to the one in quickselect.cpp. I personally prefered to keep the
    // algorithms self contained however by having identical partitions performance differences become more obvious
    // as pivot selection should now be the only contributing factor.
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

    void insertion_sort(std::vector<Key>& a, std::size_t lo, std::size_t hi, Stats& stats) {
        for (std::size_t i = lo + 1; i <= hi; ++i) {
            const Key x = a[i];
            std::size_t j = i;

            while (j > lo && less(x, a[j - 1], stats)) {
                a[j] = a[j - 1];
                --j;
            }

            a[j] = x;
        }
    }

    std::size_t median_of_medians(std::vector<Key>& a, std::size_t lo, std::size_t hi, std::size_t g, Stats& stats, Partition scheme) {
        
        std::size_t count = 0;

        for (std::size_t i = lo; i <= hi; i += g) {
            const std::size_t end = std::min(i + g - 1, hi);

            insertion_sort(a, i, end, stats);

            const std::size_t med = i + (end - i) / 2;

            std::swap(a[med], a[lo + count]);
            ++count;
        }

        const std::size_t mid = lo + (count - 1) / 2;
        return select_range(a, lo, lo + count - 1, mid, g, stats, scheme);
    }

    std::size_t select_range(std::vector<Key>& a, std::size_t lo, std::size_t hi, std::size_t k, std::size_t g, Stats& stats, Partition scheme) {
        while (hi - lo + 1 > g) {
            const std::size_t m = median_of_medians(a, lo, hi, g, stats, scheme);

            if (scheme == Partition::Lomuto) {
                std::swap(a[m], a[hi]);
                const std::size_t p = lomuto_partition(a, lo, hi, stats);

                if (k == p) {
                    return p;
                } else if (k < p) {
                    hi = p - 1;
                }
                else {
                    lo = p + 1;
                }
            } else {
                const Bounds b = three_way_partition(a, lo, hi, a[m], stats);

                if (k < b.lt) {
                    hi = b.lt - 1;
                } else if (k > b.gt) {
                    lo = b.gt + 1;
                } else {
                    return k;
                }
            }
        }

        insertion_sort(a, lo, hi, stats);
        return k;
    }
}

    Key mom_select(std::vector<Key>& a, std::size_t k, std::size_t group_size, Stats& stats, Partition scheme) {
        return a[select_range(a, 0, a.size() - 1, k, group_size, stats, scheme)];
    }

}  // namespace sel
