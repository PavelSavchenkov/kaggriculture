// Replays a Local-LB game recorded by the bridge (scripts/lb_play.py with LB_DUMP=1)
// through the same bridge, so a failing dawn can be debugged without the Python
// opponent. Prints each dawn's report; with a day argument, sets DC10_DEBUG for that
// day's compile only.
// usage: lb_replay <dump.bin> [debug_day]
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

extern "C" {
void* opus_new(const double* config, int count, int seat);
int opus_act(void* pointer, const double* fields, int count, int32_t* out, int capacity);
int opus_last_day(void* pointer, char* out, int capacity);
void opus_delete(void* pointer);
}

namespace {
bool read_buffer(std::FILE* f, std::vector<double>& values) {
    int count = 0;
    if (std::fread(&count, sizeof count, 1, f) != 1) return false;
    values.resize(size_t(count));
    if (std::fread(values.data(), sizeof(double), values.size(), f) != values.size()) {
        std::fprintf(stderr, "truncated dump\n");
        std::exit(1);
    }
    return true;
}
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: lb_replay <dump.bin> [debug_day]\n");
        return 2;
    }
    const int debug_day = argc > 2 ? std::atoi(argv[2]) : -1;
    std::FILE* f = std::fopen(argv[1], "rb");
    if (!f) return 1;
    int seat = 0;
    std::vector<double> values;
    if (std::fread(&seat, sizeof seat, 1, f) != 1 || !read_buffer(f, values)) return 1;
    void* agent = opus_new(values.data(), int(values.size()), seat);
    std::vector<int32_t> out(4096);
    char text[4096];
    // FNV-1a hash of every action the bridge emits during a day (printed after the day's report).
    uint64_t hash = 14695981039346656037ull;
    bool open_day = false;
    auto flush = [&] { if (open_day) std::printf("actions %016llx\n", (unsigned long long)hash); hash = 14695981039346656037ull; };
    while (read_buffer(f, values)) {
        // Fields: player, step, day, hour, ...
        const int day = int(values[2]), hour = int(values[3]);
        const bool debug = hour == 0 && day == debug_day;
        if (debug) setenv("DC10_DEBUG", "1", 1);
        if (hour == 0) flush();
        const int n = opus_act(agent, values.data(), int(values.size()), out.data(), int(out.size()));
        if (debug) unsetenv("DC10_DEBUG");
        for (int k = 0; k < n; ++k) hash = (hash ^ uint32_t(out[k])) * 1099511628211ull;
        open_day = true;
        if (hour == 0 && opus_last_day(agent, text, sizeof text)) std::printf("%s\n", text);
    }
    flush();
    opus_delete(agent);
    return 0;
}
