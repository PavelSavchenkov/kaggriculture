#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

using Quantity = std::int64_t;

struct Resources {
    int patterns, items;
    std::vector<int> offsets, input, output;
    std::vector<Quantity> quantity, free, cargo, unsupplied;

    Resources(int count, int item_count, const int* boundaries, const int* inputs,
              const int* outputs, const Quantity* quantities, const Quantity* stock)
        : patterns(count), items(item_count), offsets(boundaries, boundaries + count + 1),
          input(inputs, inputs + boundaries[count]), output(outputs, outputs + boundaries[count]),
          quantity(quantities, quantities + boundaries[count]), free(stock, stock + item_count),
          cargo(item_count), unsupplied(item_count) {}

    Quantity run(const int* route_patterns, int count) {
        std::fill(cargo.begin(), cargo.end(), 0);
        std::fill(unsupplied.begin(), unsupplied.end(), 0);
        for (int index = 0; index < count; ++index) {
            const int pattern = route_patterns[index];
            if (pattern == -1) {
                std::fill(cargo.begin(), cargo.end(), 0);
                continue;
            }
            if (pattern < 0 || pattern >= patterns) return -1;
            for (int task = offsets[pattern]; task < offsets[pattern + 1]; ++task) {
                if (output[task] >= 0) cargo[output[task]] += quantity[task];
                if (input[task] < 0) continue;
                if (cargo[input[task]]) --cargo[input[task]];
                else ++unsupplied[input[task]];
            }
        }
        Quantity result = 0;
        for (int item = 0; item < items; ++item)
            if (unsupplied[item]) result += std::max(Quantity(0), unsupplied[item] - free[item]);
        return result;
    }
};
