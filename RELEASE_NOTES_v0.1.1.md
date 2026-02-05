## v0.1.1 — CI + tests out-of-the-box

This release makes Turbo-Bucketizer a “clone → build → test” portfolio-grade repository.

### Highlights
- CI on GitHub Actions (gcc + clang, Ubuntu).
- Catch2 fetched automatically via CMake FetchContent (no manual header download).
- Clear test instructions in README.

### Build & test
```bash
mkdir -p build
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```
