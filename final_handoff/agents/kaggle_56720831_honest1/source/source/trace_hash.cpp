#include <cstddef>
#include <cstdint>
extern "C" uint64_t hash_words(const int64_t* words, size_t count) {
    uint64_t result = 14695981039346656037ull;
    for (size_t i = 0; i < count; ++i) {
        const auto word = uint64_t(words[i]);
        for (int shift = 0; shift < 64; shift += 8) {
            result ^= (word >> shift) & 255;
            result *= 1099511628211ull;
        }
    }
    return result;
}
