#pragma once
#include <array>
#include <cstdint>
#include <span>

namespace catalog_early_melon_b98_m1_compositions {
struct Life {
    int item,start,end,x,y;
    uint32_t fertilize=0,water=0,feed=0,care=0,collect=0,harvest=0;
};

struct Support {
    std::array<int,30> hands{},quadrants{};
};
}
