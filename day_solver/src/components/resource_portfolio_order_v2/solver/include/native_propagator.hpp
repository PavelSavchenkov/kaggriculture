#pragma once

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <vector>

struct Propagator {
    int n;
    std::vector<int> travel, predecessor, early, late, arrival;
    std::vector<int> owner, successor, indegree, lower, queue, tile_head, tile_next;

    Propagator(int count, const int* x, const int* y, const int* pred,
               const int* first, const int* last, const int* start)
        : n(count), travel(std::size_t(n) * n), predecessor(pred, pred + n), early(first, first + n),
          late(last, last + n), arrival(start, start + n), owner(n), successor(n),
          indegree(n), lower(n), queue(n), tile_head(n, -1), tile_next(n, -1) {
        for (int a = 0; a < n; ++a) {
            for (int b = 0; b < n; ++b)
                travel[a * n + b] = std::abs(x[a] - x[b]) + std::abs(y[a] - y[b]) + 1;
            if (predecessor[a] >= 0) {
                tile_next[a] = tile_head[predecessor[a]];
                tile_head[predecessor[a]] = a;
            }
        }
    }

    int run(const int* flat, int count, const int* offsets, int routes,
            const int* base, int* output, int* penalty) {
        if (count != n || routes < 0 || offsets[0] != 0 || offsets[routes] != n)
            return -1;
        std::fill(owner.begin(), owner.end(), -1);
        std::fill(successor.begin(), successor.end(), -1);
        for (int task = 0; task < n; ++task) {
            indegree[task] = predecessor[task] >= 0;
            lower[task] = base ? std::max(early[task], base[task]) : early[task];
        }
        for (int route = 0; route < routes; ++route) {
            const int begin = offsets[route], end = offsets[route + 1];
            if (begin < 0 || end > n || begin >= end) return -1;
            for (int index = begin; index < end; ++index) {
                const int task = flat[index];
                if (task < 0 || task >= n || owner[task] >= 0) return -1;
                owner[task] = route;
                if (index > begin) {
                    successor[flat[index - 1]] = task;
                    ++indegree[task];
                }
            }
            const int first = flat[begin];
            lower[first] = std::max(lower[first], arrival[first]);
        }
        int head = 0, tail = 0;
        for (int task = 0; task < n; ++task)
            if (indegree[task] == 0) queue[tail++] = task;
        auto relax = [&](int first, int second, int delay) {
            lower[second] = std::max(lower[second], lower[first] + delay);
            if (--indegree[second] == 0) queue[tail++] = second;
        };
        while (head < tail) {
            const int task = queue[head++];
            if (successor[task] >= 0)
                relax(task, successor[task], travel[task * n + successor[task]]);
            for (int next = tile_head[task]; next >= 0; next = tile_next[next])
                relax(task, next, owner[task] == owner[next]);
        }
        if (head != n) {
            *penalty = n - head;
            return 1;
        }
        *penalty = 0;
        for (int task = 0; task < n; ++task) {
            output[task] = lower[task];
            *penalty += std::max(0, lower[task] - late[task]);
        }
        return 0;
    }
};
