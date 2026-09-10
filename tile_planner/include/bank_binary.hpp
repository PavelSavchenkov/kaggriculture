#pragma once
#include <bit>
#include <cstdint>
#include <istream>
#include <ostream>
#include <stdexcept>
#include <string>

namespace placement::bank_binary {
inline void u32(std::ostream& output, uint32_t value) {
    for (int b = 0; b < 4; ++b) output.put(char((value >> (8 * b)) & 255));
}
inline uint32_t u32(std::istream& input) {
    uint32_t result = 0;
    for (int b = 0; b < 4; ++b) {
        const int byte = input.get(); if (byte < 0) throw std::runtime_error("truncated packed schedule bank");
        result |= uint32_t(byte) << (8 * b);
    }
    return result;
}
inline void i64(std::ostream& output, int64_t value) {
    const auto bits = std::bit_cast<uint64_t>(value); u32(output, uint32_t(bits)); u32(output, uint32_t(bits >> 32));
}
inline int64_t i64(std::istream& input) {
    const uint64_t low = u32(input), high = u32(input); return std::bit_cast<int64_t>(low | (high << 32));
}
inline void string(std::ostream& output, const std::string& value) {
    u32(output, value.size()); output.write(value.data(), value.size());
}
inline std::string string(std::istream& input, uint32_t maximum) {
    const auto count = u32(input); if (count > maximum) throw std::runtime_error("oversized packed schedule-bank field");
    std::string value(count, '\0'); input.read(value.data(), count);
    if (!input) throw std::runtime_error("truncated packed schedule-bank field");
    return value;
}
}
