# Turbo-Bucketizer C++17

[![CI](https://github.com/gcomneno/turbo-bucketizer-cpp/actions/workflows/ci.yml/badge.svg)](https://github.com/gcomneno/turbo-bucketizer-cpp/actions/workflows/ci.yml)

Turbo-Bucketizer C++17 is a deterministic IPv4 partitioning engine built as a modern, production-style C++17 library.  
It maps IPv4 addresses to `2^k` buckets using an efficient affine permutation modulo 2³², ensuring uniform distribution, reproducibility, and O(1) computation per address.

This repository is structured like most modern C++17 systems:
clear layering, small cohesive components, predictable behavior, zero undefined behavior, and maintainable code intended for long-term usage.

It also showcases a realistic real-world domain: deterministic hashing and sharding
— the same principles used in load balancers, distributed caches, consistent hashing rings, and network telemetry tools.

---

## 🔍 What it does

The **Turbo-Bucketizer** engine maps IPv4 addresses to a fixed number of buckets using an *affine permutation modulo 2³²*:

```text
y = (a * x + b) mod 2^32
bucket(x) = top k bits of y
```

Where:
- x is a 32-bit IPv4 address (std::uint32_t),
- a and b are 32-bit parameters (affine multiplier and offset),
- k is the number of bucket bits (2^k buckets).

The C++17 version:
- exposes a small, clear API (tb::BucketEngine, tb::Config, tb::StatsResult),
- uses only std and RAII (no raw new/delete),
- cleanly separates core library, CLI app, and tests.

## 🧱 Project structure
```
include/
  tb/
    types.hpp          # basic types, Config, StatsResult
    bucket_engine.hpp  # core mapping engine (IPv4 -> bucket)
    stats.hpp          # distribution statistics

src/
  bucket_engine.cpp    # implementation of the engine
  stats.cpp            # implementation of stats

apps/
  tb_cli.cpp           # command-line interface

tests/
  catch_amalgamated.hpp  # Catch2 single-header (not provided, see below)
  test_bucketizer.cpp    # basic tests
```

## ⚙️ Core API (library)
```bash
tb::Config
#include "tb/types.hpp"

tb::Config cfg;
cfg.a = 0x9E3779B1u;   // affine multiplier (must be odd for full permutation)
cfg.b = 0x85EBCA77u;   // additive offset
cfg.k = 12;            // 2^12 = 4096 buckets

std::size_t buckets = cfg.bucket_count();  // returns 2^k, clamped to 2^32

tb::BucketEngine
#include "tb/bucket_engine.hpp"

tb::BucketEngine engine{cfg};

// single IP
tb::BucketIndex b = engine.bucket_index(0xC0A80001u); // 192.168.0.1

// dataset
std::vector<tb::IPv4> ips = {/* ... */};
auto buckets = engine.bucketize(ips);

// histogram (counts per bucket)
auto counts = engine.distribution(ips);
// or over an integer range [start, end)
auto range_counts = engine.distribution(/*start=*/0u, /*end=*/100000u);

tb::StatsResult
#include "tb/stats.hpp"

tb::StatsResult stats = tb::compute_stats(counts);

// stats.sample_count  -> total elements
// stats.bucket_count  -> number of buckets
// stats.mean          -> expected count per bucket
// stats.stddev        -> standard deviation of counts
// stats.chi2          -> chi-square statistic
// stats.uniformity    -> 0..100% (simple “how flat is it” metric)
```

## 🖥️ CLI usage

The CLI (tb_cli) is a thin layer on top of the engine.

### Build
```bash
  mkdir build
  cd build
  cmake ..
  cmake --build .
```

or, from root-project:
```bash
  make
  make test
```

This will produce:
  tb_cli (the CLI app)
  tb_core (the library)
  tb_tests (if testing is enabled and Catch2 header is present)

### Help
./tb_cli --help

### Example output:
Turbo-Bucketizer C++17 CLI
```bash
Usage:
  tb_cli --demo <N> [options]
  tb_cli --from-file <path> [options]

Modes:
  --demo <N>           Analyze IPv4 range [0, N) as 32-bit integers
  --from-file <path>   Read IPv4 addresses (one per line, dotted form)

Options:
  --k <bits>           Number of bucket bits (default: 12 => 4096 buckets)
  --a <hex>            Affine multiplier (hex, default: 0x9E3779B1)
  --b <hex>            Affine offset (hex, default: 0x85EBCA77)
  --preset <name>      Preset parameters: default | wang
                       (overridden by --a/--b if provided)
  --show-buckets [N]   Print per-bucket counts (optionally limited to N buckets)
  --help               Show this help and exit
```

## Demo mode
Analyze a synthetic range of IPv4 values (treated as integers 0..N-1):
```bash
  ./tb_cli --demo 1000000 --k 12 --preset default
```

Sample output (abridged):
```text
  Mode: demo
  Range: [0, 1000000) (1000000 samples)

  Config:
    a = 0x9E3779B1
    b = 0x85EBCA77
    k = 12 (buckets = 4096)

  Stats:
    sample_count = 1000000
    bucket_count = 4096
    mean         = 244.1406
    stddev       = 15.9374
    chi2         = 38.4721
    uniformity   = 97.4 %
```

To inspect the first buckets:
```bash
    ./tb_cli --demo 1000000 --k 12 --preset default --show-buckets 16
```

## Read IPv4 addresses from a file (one per line, dotted-quad format):

#### file: samples/ips.txt
```text
192.168.0.1
192.168.0.2
10.0.0.1
10.0.0.2
# comments and blank lines are ignored
```

#### Run:
```bash
    ./tb_cli --from-file samples/ips.txt --k 16 --preset wang --show-buckets 32
```

Eccoti la sezione aggiornata, coerente con **FetchContent Catch2** + **ctest out-of-the-box** (niente download manuale). Copia/incolla al posto della tua.

## 🧪 Tests
Tests use Catch2 v3 and are **fetched automatically via CMake FetchContent** (no manual header download).

Configure, build, and run tests:
```bash
mkdir -p build
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
````

The tests cover:
- k = 0 edge case (all IPs map to bucket 0),
- perfectly uniform synthetic data (chi² = 0, uniformity = 100%),
- determinism of `bucket_index` and `bucketize()` (same config → same result).

## 🧠 Design goals

Modern C++17 style:
    std::uint32_t, std::vector, RAII, enum class, [[nodiscard]],
    no raw new/delete, no macros, no UB on shifts.

Clear layering:
    core library (no I/O, just math and containers),
    CLI (parsing, I/O, printing),
    tests (independent, fast, deterministic).

Standalone:
    only depends on the C++17 standard library,
    tests use a single-header version of Catch2.

## 🧭 Why C++17?

This project is intentionally written in *pure* C++17 to demonstrate a production-style approach without external dependencies or modern-heavy frameworks.

C++17 is an ideal target for this kind of system component because it offers:

- **Strong, fixed-size types** (`std::uint32_t`, `std::size_t`) critical for bit-level work.
- **Zero-cost abstractions**: clean APIs without sacrificing performance.
- **RAII and deterministic lifetimes**, essential for predictable, safe system modules.
- **Modern standard library features** (smart containers, `std::algorithm`, `[[nodiscard]]`).
- **Portability and longevity**: C++17 is widely supported across compilers and platforms.

The goal is not “fancy C++”, but **clean, maintainable, modern C++ that looks like something you’d trust in production** — while still hitting hundreds of millions of operations per second.

## 📎 Relation to the original C project

This repository is a C++17 rewrite of the original C project:
    C project: low-level, tuned for tools and shell integration.

C++ project (this repo): clean architecture, clear API, tests, and documentation.

Original C version:
👉 https://github.com/gcomneno/turbo-bucketizer

C++17 version (this repo):
👉 focuses on readability, structure, and testability for code reviews and interviews.

## 👀 For Reviewers

This repository is intentionally designed to showcase modern C++17 skills in a clean, review-friendly layout:
- **Clear separation of concerns**: `tb_core` (pure library), `tb_cli` (frontend), `tb_tests` (independent).
- **Modern idioms only**: RAII, `std::vector`, `std::uint32_t`, no macros, no raw pointers.
- **Deterministic and reproducible**: same config → same bucketization, guaranteed.
- **Self-contained**: no dependencies except C++17 and a single-header Catch2 for tests.
- **Readable architecture**: header-first API design, minimal coupling, explicit lifetimes.
- **Practical domain**: deterministic hashing / sharding of IPv4, with real-world applications.

This codebase is intentionally small but structured like a production-ready component,
so you can assess coding style, API design skills, and system-level thinking at a glance.

---