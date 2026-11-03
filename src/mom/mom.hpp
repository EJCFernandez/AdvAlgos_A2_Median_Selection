#pragma once
// Deterministic median-of-medians (BFPRT) selection.
// Primary author: Ethan Fernandez
#include "common/select_common.hpp"

namespace sel {

// Returns the k-th smallest element (0-based) of a, reordering a in place.
// group_size is the size of the groups whose medians are taken (classically 5).
// Preconditions: k < a.size(), group_size >= 3 and odd.
// scheme selects the partition step (three-way by default and Lomuto kept for comparison).
Key mom_select(std::vector<Key>& a, std::size_t k, std::size_t group_size, Stats& stats, Partition scheme = Partition::ThreeWay);

}  // namespace sel
