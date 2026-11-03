#pragma once
// Randomized quickselect.
// Primary author: Ethan Fernandez
#include "common/select_common.hpp"

namespace sel {

// Returns the k-th smallest element (0-based) of a, reordering a in place.
// Precondition: k < a.size().
// scheme selects the partition step (three-way by default and Lomuto kept for comparison).
Key quickselect(std::vector<Key>& a, std::size_t k, std::mt19937_64& rng, Stats& stats, Partition scheme = Partition::ThreeWay);

}  // namespace sel
