#include "tb/bucket_engine.hpp"
#include "tb/stats.hpp"
#include "tb/utils.hpp"

#include <algorithm>
#include <cstdint>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>
#include <cctype>

namespace {
    // ---------- Helper per stampa usage ----------
    void print_usage(std::ostream& os) {
        os << "Turbo-Bucketizer C++17 CLI\n"
        << "Usage:\n"
        << "  tb_cli --demo <N> [options]\n"
        << "  tb_cli --from-file <path> [options]\n"
        << "\n"
        << "Modes:\n"
        << "  --demo <N>           Analyze IPv4 range [0, N) as 32-bit integers\n"
        << "  --from-file <path>   Read IPv4 addresses (one per line, dotted form)\n"
        << "\n"
        << "Options:\n"
        << "  --k <bits>           Number of bucket bits (default: 12 => 4096 buckets)\n"
        << "  --a <hex>            Affine multiplier (hex, default: 0x9E3779B1)\n"
        << "  --b <hex>            Affine offset (hex, default: 0x85EBCA77)\n"
        << "  --preset <name>      Preset parameters: default | wang\n"
        << "                       (overridden by --a/--b if provided)\n"
        << "  --show-buckets [N]   Print per-bucket counts (optionally limited to N buckets)\n"
        << "  --help               Show this help and exit\n"
        << "\n"
        << "Examples:\n"
        << "  tb_cli --demo 1000000 --k 12 --preset default\n"
        << "  tb_cli --from-file data/ips.txt --k 16 --preset wang --show-buckets 32\n";
    }

    // ---------- Parse helpers ----------
    std::uint64_t parse_u64(const std::string& s, const std::string& what) {
        std::size_t pos = 0;
        std::uint64_t value = 0;
        try {
            value = std::stoull(s, &pos, 10);
        } catch (const std::exception&) {
            throw std::runtime_error("Invalid " + what + " value: '" + s + "'");
        }
        if (pos != s.size()) {
            throw std::runtime_error("Invalid " + what + " value (trailing chars): '" + s + "'");
        }
        return value;
    }

    unsigned int parse_uint(const std::string& s, const std::string& what) {
        const std::uint64_t v = parse_u64(s, what);
        if (v > std::numeric_limits<unsigned int>::max()) {
            throw std::runtime_error(what + " out of range: " + s);
        }
        return static_cast<unsigned int>(v);
    }

    std::uint32_t parse_hex32(const std::string& s, const std::string& what) {
        std::size_t pos = 0;
        std::string txt = s;
        if (txt.size() > 2 && (txt[0] == '0') && (txt[1] == 'x' || txt[1] == 'X')) {
            txt = txt.substr(2);
        }
        std::uint64_t value = 0;
        try {
            value = std::stoull(txt, &pos, 16);
        } catch (const std::exception&) {
            throw std::runtime_error("Invalid hex " + what + ": '" + s + "'");
        }
        if (pos != txt.size()) {
            throw std::runtime_error("Invalid hex " + what + " (trailing chars): '" + s + "'");
        }
        if (value > 0xFFFFFFFFu) {
            throw std::runtime_error(what + " out of 32-bit range: '" + s + "'");
        }
        return static_cast<std::uint32_t>(value);
    }

    // ---------- CLI options ----------
    enum class Mode {
        None,
        Demo,
        FromFile
    };

    struct Options {
        Mode mode = Mode::None;
        std::uint64_t demo_count = 0;
        std::string file_path;

        tb::Config cfg{};
        bool preset_used = false;

        bool show_buckets = false;
        std::size_t show_buckets_limit = 0; // 0 = no limit
    };

    void apply_preset(tb::Config& cfg, const std::string& name) {
        if (name == "default") {
            cfg.a = 0x9E3779B1u;
            cfg.b = 0x85EBCA77u;
        } else if (name == "wang") {
            cfg.a = 0x27D4EB2Du;
            cfg.b = 0x165667B1u;
        } else {
            throw std::runtime_error("Unknown preset: '" + name + "'");
        }
    }

    Options parse_args(int argc, char** argv) {
        Options opt;
        tb::Config cfg; // start from defaults

        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];

            if (arg == "--help" || arg == "-h") {
                print_usage(std::cout);
                std::exit(0);
            } else if (arg == "--demo") {
                if (i + 1 >= argc) {
                    throw std::runtime_error("--demo requires an argument <N>");
                }
                opt.mode = Mode::Demo;
                opt.demo_count = parse_u64(argv[++i], "demo count");
            } else if (arg == "--from-file") {
                if (i + 1 >= argc) {
                    throw std::runtime_error("--from-file requires a path");
                }
                opt.mode = Mode::FromFile;
                opt.file_path = argv[++i];
            } else if (arg == "--k") {
                if (i + 1 >= argc) {
                    throw std::runtime_error("--k requires an integer argument");
                }
                cfg.k = parse_uint(argv[++i], "k");
            } else if (arg == "--a") {
                if (i + 1 >= argc) {
                    throw std::runtime_error("--a requires a hex 32-bit argument");
                }
                cfg.a = parse_hex32(argv[++i], "a");
            } else if (arg == "--b") {
                if (i + 1 >= argc) {
                    throw std::runtime_error("--b requires a hex 32-bit argument");
                }
                cfg.b = parse_hex32(argv[++i], "b");
            } else if (arg == "--preset") {
                if (i + 1 >= argc) {
                    throw std::runtime_error("--preset requires a name");
                }
                const std::string name = argv[++i];
                apply_preset(cfg, name);
                opt.preset_used = true;
            } else if (arg == "--show-buckets") {
                opt.show_buckets = true;
                // optional argument: number of buckets
                if (i + 1 < argc) {
                    const std::string next = argv[i + 1];
                    if (!next.empty() && next[0] != '-') {
                        opt.show_buckets_limit = parse_u64(next, "show-buckets limit");
                        ++i;
                    }
                }
            } else {
                throw std::runtime_error("Unknown argument: " + arg);
            }
        }

        if (opt.mode == Mode::None) {
            throw std::runtime_error("No mode specified. Use --demo or --from-file.");
        }

        opt.cfg = cfg;
        return opt;
    }

    // ---------- Run modes ----------
    void run_demo(const Options& opt) {
        const std::uint64_t N = opt.demo_count;
        if (N == 0) {
            throw std::runtime_error("Demo count N must be > 0");
        }

        tb::BucketEngine engine{opt.cfg};
        const std::uint64_t max_u32 = static_cast<std::uint64_t>(
            std::numeric_limits<tb::IPv4>::max()
        ) + 1ULL;
        const std::uint64_t clamped = std::min(N, max_u32);
        const auto start = static_cast<tb::IPv4>(0u);
        const auto end   = static_cast<tb::IPv4>(clamped);

        const auto counts = engine.distribution(start, end);
        const tb::StatsResult stats = tb::compute_stats(counts);

        std::cout << "Mode: demo\n"
                << "Range: [0, " << clamped << ") ("
                << stats.sample_count << " samples)\n\n";

        std::cout << "Config:\n"
                << "  a = 0x" << std::hex << std::uppercase << opt.cfg.a << std::dec << "\n"
                << "  b = 0x" << std::hex << std::uppercase << opt.cfg.b << std::dec << "\n"
                << "  k = " << opt.cfg.k << " (buckets = " << opt.cfg.bucket_count() << ")\n\n";

        std::cout << "Stats:\n"
                << std::fixed << std::setprecision(4)
                << "  sample_count = " << stats.sample_count << "\n"
                << "  bucket_count = " << stats.bucket_count << "\n"
                << "  mean         = " << stats.mean << "\n"
                << "  stddev       = " << stats.stddev << "\n"
                << "  chi2         = " << stats.chi2 << "\n"
                << "  uniformity   = " << stats.uniformity << " %\n";

        if (opt.show_buckets) {
            const std::size_t limit = (opt.show_buckets_limit == 0)
                ? counts.size()
                : std::min<std::size_t>(opt.show_buckets_limit, counts.size());

            std::cout << "\nBucket counts (first " << limit << "):\n";
            for (std::size_t i = 0; i < limit; ++i) {
                std::cout << "  [" << i << "] = " << counts[i] << "\n";
            }
        }
    }

    void run_from_file(const Options& opt) {
        std::ifstream in{opt.file_path};
        if (!in) {
            throw std::runtime_error("Cannot open input file: " + opt.file_path);
        }

        std::vector<tb::IPv4> ips;
        ips.reserve(1024);

        std::string line;
        std::size_t line_no = 0;
        while (std::getline(in, line)) {
            ++line_no;
            // trim minimo
            auto is_space = [](unsigned char c){ return std::isspace(c) != 0; };
            line.erase(line.begin(),
                    std::find_if(line.begin(), line.end(),
                                    [&](char c){ return !is_space(static_cast<unsigned char>(c)); }));
            line.erase(std::find_if(line.rbegin(), line.rend(),
                                    [&](char c){ return !is_space(static_cast<unsigned char>(c)); }).base(),
                    line.end());

            if (line.empty()) continue;
            if (!line.empty() && line[0] == '#') continue;

            try {
                tb::IPv4 ip = tb::parse_ipv4(line);
                ips.push_back(ip);
            } catch (const std::exception& e) {
                std::ostringstream oss;
                oss << "Error parsing IPv4 at line " << line_no << ": " << e.what();
                throw std::runtime_error(oss.str());
            }
        }

        if (ips.empty()) {
            throw std::runtime_error("No valid IPv4 addresses found in file: " + opt.file_path);
        }

        tb::BucketEngine engine{opt.cfg};
        const auto counts = engine.distribution(ips);
        const tb::StatsResult stats = tb::compute_stats(counts);

        std::cout << "Mode: from-file\n"
                << "File: " << opt.file_path << "\n\n";

        std::cout << "Config:\n"
                << "  a = 0x" << std::hex << std::uppercase << opt.cfg.a << std::dec << "\n"
                << "  b = 0x" << std::hex << std::uppercase << opt.cfg.b << std::dec << "\n"
                << "  k = " << opt.cfg.k << " (buckets = " << opt.cfg.bucket_count() << ")\n\n";

        std::cout << "Stats:\n"
                << std::fixed << std::setprecision(4)
                << "  sample_count = " << stats.sample_count << "\n"
                << "  bucket_count = " << stats.bucket_count << "\n"
                << "  mean         = " << stats.mean << "\n"
                << "  stddev       = " << stats.stddev << "\n"
                << "  chi2         = " << stats.chi2 << "\n"
                << "  uniformity   = " << stats.uniformity << " %\n";

        if (opt.show_buckets) {
            const std::size_t limit = (opt.show_buckets_limit == 0)
                ? counts.size()
                : std::min<std::size_t>(opt.show_buckets_limit, counts.size());

            std::cout << "\nBucket counts (first " << limit << "):\n";
            for (std::size_t i = 0; i < limit; ++i) {
                std::cout << "  [" << i << "] = " << counts[i] << "\n";
            }
        }
    }

}

// ---------- main ----------
int main(int argc, char** argv) {
    try {
        if (argc <= 1) {
            print_usage(std::cout);
            return 1;
        }

        Options opt = parse_args(argc, argv);

        switch (opt.mode) {
            case Mode::Demo:
                run_demo(opt);
                break;
            case Mode::FromFile:
                run_from_file(opt);
                break;
            case Mode::None:
            default:
                throw std::runtime_error("Internal error: no mode selected");
        }

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n\n";
        print_usage(std::cerr);
        return 1;
    }
}
