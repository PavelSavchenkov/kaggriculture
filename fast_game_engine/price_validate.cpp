#include "sim.hpp"
#include <cstdio>

int main() {
    int item, inventory, expected;
    int checked = 0;
    while (std::scanf("%d%d%d", &item, &inventory, &expected) == 3) {
        int uncached = kag::market_price_uncached(item, inventory);
        int cached = kag::market_price(item, inventory);
        if (uncached != expected || cached != expected) {
            std::fprintf(stderr, "price mismatch item=%d inventory=%d python=%d uncached=%d cached=%d\n",
                         item, inventory, expected, uncached, cached);
            return 1;
        }
        ++checked;
    }
    std::printf("PASS %d official market prices exact\n", checked);
}
