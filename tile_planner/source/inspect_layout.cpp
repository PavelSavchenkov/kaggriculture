#include "course.hpp"
#include "certify.hpp"
#include <iostream>

int main(int argc, char** argv) {
    using namespace placement;
    try {
        if (argc != 3) throw std::runtime_error("usage: inspect_layout course layout");
        const Course course(argv[1]); const auto layout = read_layout(argv[2]);
        for (int d = 0; d < 30; ++d) {
            const auto prediction = fast_day_solver_estimator::estimate_day(course.problem(d, layout), course.days[d].menu);
            if (!prediction.analytically_rejected) continue;
            const auto& f = prediction.features;
            std::cout << "day " << d << " lower " << prediction.lower << " deadline_missing " << f[labor::deadline_missing_quantity]
                      << " supply_missing " << f[labor::planning_supply_base + 1] << " seed_missing " << f[labor::planning_release_base + 1]
                      << " land_missing " << f[labor::planning_release_base + 2] << '\n';
        }
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
