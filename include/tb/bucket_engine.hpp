#pragma once

#include "types.hpp"
#include <vector>

namespace tb {

    class BucketEngine {
    public:
        explicit BucketEngine(const Config& cfg);

        [[nodiscard]] BucketIndex bucket_index(IPv4 ip) const noexcept;

        // bucketize arbitrary dataset
        std::vector<BucketIndex> bucketize(const std::vector<IPv4>& ips) const;

        // histogram on arbitrary dataset
        std::vector<std::size_t> distribution(const std::vector<IPv4>& ips) const;

        // histogram on range [start, end)
        std::vector<std::size_t> distribution(IPv4 start, IPv4 end) const;

        const Config& config() const noexcept { return cfg_; }

    private:
        Config cfg_;
    };

}
