#pragma once

#include "tb/types.hpp"
#include <string>

namespace tb {
    /// Parse a dotted-quad IPv4 string (e.g. "192.168.0.1") into a 32-bit value. Throws std::runtime_error on invalid input.
    IPv4 parse_ipv4(const std::string& s);
}
