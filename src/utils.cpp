#include "tb/utils.hpp"

#include <cstdint>
#include <sstream>
#include <stdexcept>

namespace tb {
    IPv4 parse_ipv4(const std::string& s) {
        std::istringstream iss{s};
        std::string token;
        std::uint32_t parts[4] = {0, 0, 0, 0};
        int idx = 0;

        while (std::getline(iss, token, '.')) {
            if (idx >= 4) {
                throw std::runtime_error("Invalid IPv4 (too many octets): '" + s + "'");
            }
            if (token.empty()) {
                throw std::runtime_error("Invalid IPv4 (empty octet): '" + s + "'");
            }
            std::uint64_t v = 0;
            try {
                v = std::stoull(token);
            } catch (const std::exception&) {
                throw std::runtime_error("Invalid IPv4 octet: '" + token + "' in '" + s + "'");
            }
            if (v > 255u) {
                throw std::runtime_error("IPv4 octet out of range [0,255]: '" + token + "' in '" + s + "'");
            }
            parts[idx++] = static_cast<std::uint32_t>(v);
        }
        if (idx != 4) {
            throw std::runtime_error("Invalid IPv4 (expected 4 octets): '" + s + "'");
        }

        return (parts[0] << 24) |
            (parts[1] << 16) |
            (parts[2] << 8)  |
            (parts[3]);
    }
}
