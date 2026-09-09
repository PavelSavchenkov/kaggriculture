#pragma once
#include <array>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <string>

// Offline fixture configuration. Only currently revealed entries are copied
// into observations; neither agent receives this complete sequence.
inline std::array<uint8_t, 8> warm_fixture_shops() {
    const char* value = std::getenv("LABOR_WARM_SHOPS");
    if (!value || std::string(value).size() != 8)
        throw std::runtime_error("LABOR_WARM_SHOPS requires exactly eight shop digits");
    std::array<uint8_t, 8> shops{};
    for (int i = 0; i < 8; ++i) {
        if (value[i] < '0' || value[i] > '7') throw std::runtime_error("shop outside 0..7");
        shops[i] = value[i] - '0';
    }
    return shops;
}
