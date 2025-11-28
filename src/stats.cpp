#include "tb/stats.hpp"

#include <algorithm>
#include <numeric>
#include <cmath>

namespace tb {

    StatsResult compute_stats(const std::vector<std::size_t>& counts) {
        StatsResult r{};
        const std::size_t m = counts.size();
        r.bucket_count = m;

        const std::size_t n = std::accumulate(counts.begin(), counts.end(), std::size_t{0});
        r.sample_count = n;

        if (m == 0 || n == 0) {
            // tutto a 0 (nessun dato o 0 bucket)
            return r;
        }

        const double mean = static_cast<double>(n) / static_cast<double>(m);
        r.mean = mean;

        // varianza campionaria (quando n>1); altrimenti 0
        // var(counts) = (1/m) * sum( (ci - mean)^2 )
        // poi stddev = sqrt(var)
        {
            long double acc = 0.0L;
            for (const auto c : counts) {
                const long double d = static_cast<long double>(c) - static_cast<long double>(mean);
                acc += d * d;
            }
            const long double var = acc / static_cast<long double>(m);
            r.stddev = std::sqrt(static_cast<double>(var));
        }

        // Chi-quadro Pearson: sum( (ci - E)^2 / E ) con E=mean
        {
            long double chi = 0.0L;
            for (const auto c : counts) {
                const long double diff = static_cast<long double>(c) - static_cast<long double>(mean);
                chi += (diff * diff) / static_cast<long double>(mean);
            }
            r.chi2 = static_cast<double>(chi);
        }

        // Uniformity % (intuitiva):
        // 100% quando max deviation == 0
        // 0% quando max deviation >= mean (clamp a [0,100])
        // È una metrica semplice, ben leggibile a colpo d'occhio.
        {
            const auto [min_it, max_it] = std::minmax_element(counts.begin(), counts.end());
            const double max_dev = std::max(std::abs(static_cast<double>(*max_it) - mean),
                                            std::abs(static_cast<double>(*min_it) - mean));
            double u = 1.0;
            if (mean > 0.0) {
                u = 1.0 - (max_dev / mean);
            }
            // clamp [0,1] e scala %
            if (u < 0.0) u = 0.0;
            if (u > 1.0) u = 1.0;
            r.uniformity = u * 100.0;
        }

        return r;
    }

}
