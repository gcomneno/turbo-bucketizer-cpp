#pragma once

#include <cstdint>
#include <cstddef>
#include <cmath>

namespace tb {

    using IPv4 = std::uint32_t;
    using BucketIndex = std::uint32_t;

    struct Config {
        std::uint32_t a = 0x9E3779B1u;   // affine multiplier (must be odd)
        std::uint32_t b = 0x85EBCA77u;   // additive offset
        unsigned int  k = 12;            // number of bucket bits (2^k buckets)

        [[nodiscard]] std::size_t bucket_count() const noexcept {
            if (k >= 32) return static_cast<std::size_t>(1ULL << 32);
            return static_cast<std::size_t>(1ULL << k);
        }
    };

    struct StatsResult {
        std::size_t sample_count = 0;
        std::size_t bucket_count = 0;
        double mean = 0.0;
        double stddev = 0.0;
        double chi2 = 0.0;
        double uniformity = 0.0; // 0–100%
    };

}
