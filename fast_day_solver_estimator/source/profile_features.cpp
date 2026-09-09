#include "bounds.hpp"
#include <day_solver/io.hpp>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 3) return 2;
    std::ifstream input(argv[1]); std::ofstream output(argv[2]);
    output << "id,base_us,pressure_us,bound_us,combined_us\n";
    std::string id, path; double checksum = 0;
    constexpr int repeats = 100;
    auto measure = [&](auto function) {
        const auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < repeats; ++i) { checksum += function(); asm volatile("" : : "g"(checksum) : "memory"); }
        return std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start).count() / repeats;
    };
    int count = 0;
    while (input >> id >> path) {
        const auto p = day_solver::load_problem_json(path); const auto menu = labor::earliest_menu(p);
        const auto full = labor::extract(p);
        const double base = measure([&] { return labor::extract_base(p)[labor::rooted_mst]; });
        const double pressure = measure([&] { labor::Features f{}; labor::add_pressure_features(p, f); return f[labor::route_pack_open]; });
        const double bound = measure([&] { return labor::workforce_lower_bound(p, full, menu); });
        const double combined = measure([&] { const auto f = labor::extract(p); return f[labor::route_pack_open] + labor::workforce_lower_bound(p, f, menu); });
        output << std::setprecision(9) << id << ',' << base << ',' << pressure << ',' << bound << ',' << combined << '\n';
        ++count;
    }
    if (!input.eof()) return 2;
    std::cout << count << " cases x" << repeats << " repeats, checksum=" << checksum << '\n';
}
