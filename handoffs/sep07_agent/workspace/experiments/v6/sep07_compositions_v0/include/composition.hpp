#pragma once
#include <array>
#include <cstdint>
#include <span>

namespace compositions {
struct Life {
    int item,start,end,x,y;
    uint32_t fertilize=0,water=0,feed=0,care=0,collect=0,harvest=0;
};

struct Support {
    std::array<int,30> hands{},quadrants{};
};
}
