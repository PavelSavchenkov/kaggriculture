#pragma once
#include "bounds.hpp"

namespace labor {
struct RetryOption { int workers; double probability; };
struct RetryChoice { int workers = 0; double utility = 0, predicted_cpu = 0; };

// Optional refinement only: the caller already has an independently verified
// schedule. Each offered query failed at three seconds under the same inputs.
// Probabilities come from the existing frozen cold-query model. This class has
// no solver, schedule, reference outcome or source-workforce access.
class LongRetryPolicy {
public:
    static constexpr double failed_mean_cpu = 30.104672395368077;
    static constexpr double successful_mean_cpu = 7.652708758064516;
    static constexpr double default_minimum_utility = .01;
    int best_workers;

    LongRetryPolicy(int incumbent, const std::vector<RetryOption>& options,
                    double minimum_utility = default_minimum_utility,
                    double failed_cpu = failed_mean_cpu, double successful_cpu = successful_mean_cpu)
        : best_workers(incumbent), threshold(minimum_utility), failed(failed_cpu), successful(successful_cpu) {
        if (incumbent < 1 || incumbent > 40 || !std::isfinite(threshold) || threshold < 0 ||
            !std::isfinite(failed) || !std::isfinite(successful) || failed <= 0 || successful <= 0)
            throw std::runtime_error("invalid long-retry state or timing parameters");
        for (const auto& option : options) {
            if (option.workers < 1 || option.workers >= incumbent || !std::isfinite(option.probability) ||
                option.probability < 0 || option.probability > 1 || offered[option.workers])
                throw std::runtime_error("invalid or duplicate long-retry option");
            offered[option.workers] = true;
            probability[option.workers] = option.probability;
        }
    }

    RetryChoice next() {
        RetryChoice choice;
        for (int workers = 1; workers < best_workers; ++workers) {
            if (!offered[workers] || attempted[workers]) continue;
            const double p = probability[workers];
            const double cpu = (1 - p) * failed + p * successful;
            const double utility = p * (hire_cost(best_workers) - hire_cost(workers)) / cpu;
            if (utility >= threshold && (!choice.workers || utility > choice.utility ||
                                         (utility == choice.utility && workers > choice.workers)))
                choice = {workers, utility, cpu};
        }
        issued = choice.workers;
        return choice;
    }

    void observe(int workers, bool certified) {
        if (!issued || workers != issued) throw std::runtime_error("unexpected long-retry observation");
        attempted[workers] = true;
        if (certified) best_workers = workers;
        issued = 0;
    }

private:
    std::array<bool, 41> offered{}, attempted{};
    std::array<double, 41> probability{};
    double threshold, failed, successful;
    int issued = 0;
};
}
