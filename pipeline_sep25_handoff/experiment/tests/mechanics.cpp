// Mechanics checks for the DayIntent rules and the converter.
#include "source/convert.hpp"
#include <cstdio>

using namespace dc10;

namespace {
int failures = 0;
void check(bool ok, const char* what) {
    if (!ok) {
        std::printf("FAIL %s\n", what);
        ++failures;
    }
}
}

int main() {
    CropGroup wheat;
    wheat.crop = WHEAT;
    wheat.age = 2;
    wheat.yield = 1;
    check(!option_fixed(wheat, 10, 4 | 2 | 1), "wheat age 2: fertilize, water, harvest is useful");
    check(option_fixed(wheat, 10, 2 | 1), "fertilize + harvest without water is fixed");
    check(!option_fixed(wheat, 10, CLEAR), "existing crops may be cleared");
    check(option_fixed(wheat, 29, CLEAR), "clear is fixed on day 29");
    check(option_fixed(wheat, 29, 4), "water without harvest is fixed on day 29");
    wheat.age = 1;
    check(option_fixed(wheat, 10, 1), "harvest is fixed while too young");
    CropGroup fresh;
    fresh.crop = MELON;
    fresh.fresh = true;
    fresh.yield = 1;
    check(option_fixed(fresh, 5, 0), "new crops must be watered");
    check(option_fixed(fresh, 5, 4 | 2), "fertilizer cannot help a new melon");
    fresh.crop = WHEAT;
    check(!option_fixed(fresh, 5, 4 | 2), "fertilizer on new wheat reaches age 2");
    AnimalGroup cow;
    cow.species = 1;
    cow.age = 0;
    check(!care_fixed(cow, 3), "new cow care reaches its first production");
    check(care_fixed(cow, 28), "care on day 28 cannot reach a later production");
    std::printf("%s\n", failures ? "mechanics FAILED" : "mechanics ok");
    return failures ? 1 : 0;
}
