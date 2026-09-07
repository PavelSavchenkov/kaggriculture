#include "pyrandom.hpp"
#include <bit>
#include <cstdio>
#include <cstring>

int main() {
    char operation[8];
    kag::PyRandom* rng = nullptr;
    int checked = 0;
    while (std::scanf("%7s", operation) == 1) {
        if (std::strcmp(operation, "KEY") == 0) {
            unsigned long long key;
            if (std::scanf("%llu", &key) != 1) return 2;
            delete rng;
            rng = new kag::PyRandom(key);
        } else if (std::strcmp(operation, "R") == 0) {
            unsigned long long expected;
            if (!rng || std::scanf("%llu", &expected) != 1) return 2;
            auto actual = std::bit_cast<uint64_t>(rng->random());
            if (actual != expected) {
                std::fprintf(stderr, "random mismatch expected=%llu actual=%llu\n", expected,
                             static_cast<unsigned long long>(actual));
                delete rng;
                return 1;
            }
            ++checked;
        } else if (std::strcmp(operation, "B") == 0) {
            unsigned bound, expected;
            if (!rng || std::scanf("%u%u", &bound, &expected) != 2) return 2;
            unsigned actual = rng->randbelow(bound);
            if (actual != expected) {
                std::fprintf(stderr, "randbelow mismatch n=%u expected=%u actual=%u\n",
                             bound, expected, actual);
                delete rng;
                return 1;
            }
            ++checked;
        }
    }
    delete rng;
    std::printf("PASS %d sequential CPython RNG outputs exact\n", checked);
}
