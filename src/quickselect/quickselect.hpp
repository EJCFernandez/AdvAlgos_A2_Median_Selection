#pragma once
// Randomized quickselect.
// Primary author: [Your name]
#include "common/select_common.hpp"

namespace sel {

// Returns the k-th smallest element (0-based) of a, reordering a in place.
// Precondition: k < a.size().
Key quickselect(std::vector<Key>& a, std::size_t k, std::mt19937_64& rng, Stats& stats);

}  // namespace sel
