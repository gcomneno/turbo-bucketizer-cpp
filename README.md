# Turbo-Bucketizer C++17

A modern C++17 reimplementation of the original C project [**Turbo-Bucketizer**](https://github.com/gcomneno/turbo-bucketizer).

This edition is designed as a **clean, reusable library + CLI** that can be used as:
- a portfolio piece for C++17 (system / performance-oriented),
- a small building block for deterministic IPv4 sharding / load balancing,
- a sandbox for uniformity / hashing experiments.

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

## 🖥️ CLI usage

The CLI (tb_cli) is a thin layer on top of the engine.

Build
mkdir build
cd build
cmake ..
cmake --build .


This will produce:
tb_cli (the CLI app)
tb_core (the library)
tb_tests (if testing is enabled and Catch2 header is present)

Help
./tb_cli --help

Example output:

Turbo-Bucketizer C++17 CLI
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

## Demo mode

Analyze a synthetic range of IPv4 values (treated as integers 0..N-1):
./tb_cli --demo 1000000 --k 12 --preset default


Sample output (abridged):

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


To inspect the first buckets:
    ./tb_cli --demo 1000000 --k 12 --preset default --show-buckets 16

## Read IPv4 addresses from a file (one per line, dotted-quad format):

# file: samples/ips.txt
192.168.0.1
192.168.0.2
10.0.0.1
10.0.0.2
# comments and blank lines are ignored

Run:
    ./tb_cli --from-file samples/ips.txt --k 16 --preset wang --show-buckets 32

## 🧪 Tests

Tests are implemented with Catch2 as a single header.

Download catch_amalgamated.hpp from the Catch2 project.

Place it under:
    tests/catch_amalgamated.hpp

Configure & build with testing enabled (default):

mkdir build
cd build
cmake -DBUILD_TESTING=ON ..
cmake --build .
ctest

The tests cover:
- k = 0 edge case (all IPs map to bucket 0),
- a small “identity-like” configuration with perfect uniformity,
- determinism of bucket_index and bucketize() (same config → same result).

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

## 📎 Relation to the original C project

This repository is a C++17 rewrite of the original C project:
    C project: low-level, tuned for tools and shell integration.

C++ project (this repo): clean architecture, clear API, tests, and documentation.

Original C version:
👉 https://github.com/gcomneno/turbo-bucketizer

C++17 version (this repo):
👉 focuses on readability, structure, and testability for code reviews and interviews.

---