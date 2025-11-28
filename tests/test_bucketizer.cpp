#include "catch_amalgamated.hpp"

#include "tb/bucket_engine.hpp"
#include "tb/stats.hpp"

#include <random>
#include <limits>
#include <vector>

// Catch2 v3: Approx is in namespace Catch
using Catch::Approx;

TEST_CASE("k=0 maps everything to bucket 0", "[bucket_engine]") {
    tb::Config cfg;
    cfg.k = 0;
    cfg.a = 0x9E3779B1u;
    cfg.b = 0x85EBCA77u;

    tb::BucketEngine engine{cfg};

    std::vector<tb::IPv4> ips = {
        0u,
        1u,
        42u,
        0x7F000001u,      // 127.0.0.1
        0xC0A80001u,      // 192.168.0.1
        0xFFFFFFFFu
    };

    for (auto ip : ips) {
        auto b = engine.bucket_index(ip);
        REQUIRE(b == 0u);
    }

    auto counts = engine.distribution(ips);
    REQUIRE(counts.size() == cfg.bucket_count());
    REQUIRE(counts.size() == 1);

    tb::StatsResult stats = tb::compute_stats(counts);
    REQUIRE(stats.sample_count == ips.size());
    REQUIRE(stats.bucket_count == 1);
}

TEST_CASE("compute_stats on perfectly uniform synthetic data", "[stats]") {
    const std::size_t buckets = 16;
    const std::size_t M = 8;                // elements per bucket
    const std::size_t N = buckets * M;      // total samples

    std::vector<std::size_t> counts(buckets, M);

    tb::StatsResult stats = tb::compute_stats(counts);

    REQUIRE(stats.sample_count == N);
    REQUIRE(stats.bucket_count == buckets);
    REQUIRE(stats.mean == Approx(static_cast<double>(M)));
    REQUIRE(stats.stddev == Approx(0.0).margin(1e-9));
    REQUIRE(stats.chi2 == Approx(0.0).margin(1e-9));
    REQUIRE(stats.uniformity == Approx(100.0).margin(1e-6));
}

TEST_CASE("Determinism: same IPs, same config -> same buckets", "[bucket_engine]") {
    tb::Config cfg;
    cfg.k = 12;
    cfg.a = 0x9E3779B1u;
    cfg.b = 0x85EBCA77u;

    tb::BucketEngine engine{cfg};

    std::mt19937_64 rng{123456789ULL};
    std::uniform_int_distribution<std::uint32_t> dist(
        0u, std::numeric_limits<std::uint32_t>::max()
    );

    std::vector<tb::IPv4> ips;
    ips.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
        ips.push_back(dist(rng));
    }

    std::vector<tb::BucketIndex> b1;
    b1.reserve(ips.size());
    for (auto ip : ips) {
        b1.push_back(engine.bucket_index(ip));
    }

    auto b2 = engine.bucketize(ips);

    REQUIRE(b1.size() == b2.size());
    for (std::size_t i = 0; i < b1.size(); ++i) {
        REQUIRE(b1[i] == b2[i]);
    }

    auto counts1 = engine.distribution(ips);
    auto counts2 = engine.distribution(ips);

    REQUIRE(counts1.size() == counts2.size());
    for (std::size_t i = 0; i < counts1.size(); ++i) {
        REQUIRE(counts1[i] == counts2[i]);
    }

    tb::StatsResult stats = tb::compute_stats(counts1);
    REQUIRE(stats.sample_count == ips.size());
    REQUIRE(stats.bucket_count == cfg.bucket_count());
}
