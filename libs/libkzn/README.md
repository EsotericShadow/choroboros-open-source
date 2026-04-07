# libkzn

Standalone C++ library for reading, writing, and verifying `.kzn` files — the signed preset and engine format for Kaizen DSP plugins.

## Features

- 128-byte binary header read/write (little-endian, v1)
- Ed25519 signature verification via vendored Monocypher
- Canonical JSON emitter (sorted keys, no whitespace, deterministic bytes)
- Pluggable HTTP signing client (no networking dependency in core)
- Session certificate stubs (Milestone B)

## Building

```bash
mkdir build && cd build
cmake .. -DLIBKZN_BUILD_TESTS=ON
cmake --build .
```

## Running Tests

```bash
cd build
# Generate test vectors first
./tests/generate_test_vectors ../tests/test_vectors

# Run tests
./tests/test_canonical_json
./tests/test_format
./tests/test_verifier
```

## Integration

Add as a git submodule:

```cmake
add_subdirectory(libs/libkzn)
target_link_libraries(YourPlugin PRIVATE libkzn)
```

Then include the single public header:

```cpp
#include <kzn/kzn.h>
```

## Directory Structure

```
libkzn/
├── include/kzn/       Public headers
├── src/               Implementation
├── vendor/monocypher/ Vendored Monocypher (BSD-2-Clause)
├── tests/             Test suite + test vectors
└── docs/              Format specification
```

## License

Proprietary — Kaizen DSP. All rights reserved.
Monocypher is BSD-2-Clause / CC0-1.0 dual licensed.
