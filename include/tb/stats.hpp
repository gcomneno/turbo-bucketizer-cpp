#pragma once

#include "types.hpp"
#include <vector>

namespace tb {
    // compute standard deviation, chi², and uniformity %
    StatsResult compute_stats(const std::vector<std::size_t>& counts);
}
