// Randomized quickselect.
// Primary author: [Your name]
#include "quickselect/quickselect.hpp"

#include <stdexcept>

namespace sel {

Key quickselect(std::vector<Key>& a, std::size_t k, std::mt19937_64& rng, Stats& stats) {
    (void)a; (void)k; (void)rng; (void)stats;
    // TODO (Wed): choose a random pivot, partition, recurse/iterate on one side.
    // Decide and document how keys EQUAL to the pivot are handled.
    throw std::logic_error("quickselect not implemented yet");
}

}  // namespace sel
