// Microbenchmark fixed-size storage against equivalent heap-backed vectors.
#include "sim.hpp"
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace kag;
constexpr int N_TILES = 2 * BOARD * BOARD;
using StaticTiles = std::array<Tile, N_TILES>;

template<class T>
inline void escape(T* value) {
    asm volatile("" : : "g"(value) : "memory");
}

__attribute__((noinline)) uint64_t scan_static(StaticTiles& tiles, int salt) {
    uint64_t sum = 0;
    for (const Tile& tile : tiles)
        sum += tile.kind + tile.yield_units + tile.consecutive_dry + tile.max_lifespan_step;
    tiles[salt % N_TILES].yield_units++;
    return sum;
}

__attribute__((noinline)) uint64_t scan_dynamic(std::vector<Tile>& tiles, int salt) {
    uint64_t sum = 0;
    for (const Tile& tile : tiles)
        sum += tile.kind + tile.yield_units + tile.consecutive_dry + tile.max_lifespan_step;
    tiles[salt % tiles.size()].yield_units++;
    return sum;
}

__attribute__((noinline)) uint64_t construct_static(int salt) {
    StaticTiles tiles{};
    tiles[salt % N_TILES].kind = T_PLANT;
    escape(&tiles);
    return tiles[salt % N_TILES].kind;
}

__attribute__((noinline)) uint64_t construct_dynamic(int salt) {
    std::vector<Tile> tiles(N_TILES);
    tiles[salt % N_TILES].kind = T_PLANT;
    escape(&tiles);
    return tiles[salt % N_TILES].kind;
}

template<class Fn>
double measure(int iterations, Fn&& fn, uint64_t& sink) {
    auto started = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; ++i) sink += fn(i);
    return std::chrono::duration<double, std::nano>(
        std::chrono::steady_clock::now() - started).count() / iterations;
}

int main(int argc, char** argv) {
    const int scans = argc > 1 ? std::atoi(argv[1]) : 2000000;
    const int constructions = argc > 2 ? std::atoi(argv[2]) : 200000;
    StaticTiles fixed{};
    std::vector<Tile> dynamic(N_TILES);
    uint64_t sink = 0;

    const double fixed_scan = measure(scans, [&](int i) { return scan_static(fixed, i); }, sink);
    const double dynamic_scan = measure(scans, [&](int i) { return scan_dynamic(dynamic, i); }, sink);
    const double fixed_construct = measure(constructions, construct_static, sink);
    const double dynamic_construct = measure(constructions, construct_dynamic, sink);

    std::printf("sizes: Tile=%zu Farm=%zu State=%zu Action=%zu bytes\n",
                sizeof(Tile), sizeof(Farm), sizeof(State), sizeof(Action));
    std::printf("reused 200-tile scan: fixed %.2f ns, vector %.2f ns (vector/fixed %.2fx)\n",
                fixed_scan, dynamic_scan, dynamic_scan / fixed_scan);
    std::printf("construct+zero 200 tiles: fixed %.2f ns, vector %.2f ns (vector/fixed %.2fx)\n",
                fixed_construct, dynamic_construct, dynamic_construct / fixed_construct);
    std::printf("sink=%llu\n", static_cast<unsigned long long>(sink));
}
