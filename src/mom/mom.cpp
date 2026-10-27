// Deterministic median-of-medians (BFPRT) selection.
// Primary author: [Your name]
#include "mom/mom.hpp"

#include <stdexcept>

namespace sel {

Key mom_select(std::vector<Key>& a, std::size_t k, std::size_t group_size, Stats& stats) {
    (void)a; (void)k; (void)group_size; (void)stats;
    // TODO (Wed): split into groups, take each group's median, recursively
    // select the median of medians as pivot, partition, recurse on one side.
    // Think about what the 3n/10 guarantee assumes about duplicate keys.
    throw std::logic_error("mom_select not implemented yet");
}

}  // namespace sel
