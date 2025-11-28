#include "tb/bucket_engine.hpp"

#include <algorithm>
#include <numeric>
#include <limits>

namespace tb {

    namespace {
        // calcola l’indice di bucket dai 32 bit alti di y
        // y = a*x + b (overflow 32 bit voluto); shift robusto
        inline BucketIndex bucket_from_y(std::uint32_t y, unsigned int k) noexcept {
            if (k == 0)  return 0u;
            if (k >= 32) return y; // nessuno shift: restituisce tutti i 32 bit
            // shift effettuato su 64 bit per evitare UB quando (32 - k) == 32
            const unsigned s = 32u - k;
            return static_cast<BucketIndex>( static_cast<std::uint64_t>(y) >> s );
        }
    }

    BucketEngine::BucketEngine(const Config& cfg)
    : cfg_{cfg} {
        // Nulla da fare: l’engine è immutabile dopo la config.
        // Nota: assumiamo cfg_.a dispari per la permutazione completa su 2^32.
    }

    BucketIndex BucketEngine::bucket_index(IPv4 ip) const noexcept {
        // y = a*ip + b (overflow su 32 bit: comportamento definito per unsigned)
        const std::uint32_t y = static_cast<std::uint32_t>(
            static_cast<std::uint64_t>(cfg_.a) * static_cast<std::uint64_t>(ip)
            + static_cast<std::uint64_t>(cfg_.b)
        );
        return bucket_from_y(y, cfg_.k);
    }

    std::vector<BucketIndex> BucketEngine::bucketize(const std::vector<IPv4>& ips) const {
        std::vector<BucketIndex> out;
        out.reserve(ips.size());
        for (IPv4 ip : ips) {
            out.push_back(bucket_index(ip));
        }
        return out;
    }

    std::vector<std::size_t> BucketEngine::distribution(const std::vector<IPv4>& ips) const {
        const std::size_t m = cfg_.bucket_count();
        std::vector<std::size_t> counts(m, 0);
        if (m == 0) return counts;

        for (IPv4 ip : ips) {
            const auto b = bucket_index(ip);
            // Difensivo: clamp se k>=32 e BucketIndex estende 32 bit pieni
            if (b < counts.size()) {
                counts[b] += 1;
            }
        }
        return counts;
    }

    std::vector<std::size_t> BucketEngine::distribution(IPv4 start, IPv4 end) const {
        const std::size_t m = cfg_.bucket_count();
        std::vector<std::size_t> counts(m, 0);
        if (m == 0) return counts;

        // Iteriamo su [start, end) senza wrap-around; se end <= start, l’intervallo è vuoto.
        // (Se in futuro servirà supportare wrap mod 2^32, potremo aggiungere un flag)
        if (end <= start) return counts;

        // Iterazione semplice e prevedibile dal compilatore
        for (std::uint64_t v = static_cast<std::uint64_t>(start);
            v < static_cast<std::uint64_t>(end);
            ++v) {
            const auto b = bucket_index(static_cast<IPv4>(v));
            if (b < counts.size()) {
                counts[b] += 1;
            }
        }
        return counts;
    }

}
