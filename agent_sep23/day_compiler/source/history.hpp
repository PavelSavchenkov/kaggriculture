#pragma once
#include "state.hpp"
#include <array>

namespace kag::day_compiler {
// Bounds concern rival changes to market inventory, not hidden requested orders.
struct FlowObservation {
    int step = -1;
    int lower[N_PRODUCTS]{}, upper[N_PRODUCTS]{};
    bool identifiable[N_PRODUCTS]{};
    int own_net_sales[N_PRODUCTS]{};
    bool own_fill_known[N_PRODUCTS]{};
    int rival_visible_removal[N_PRODUCTS]{};
};
class History {
public:
    bool observe(const Observation& observation, const Configuration& config = {});
    void record(const Observation& observation, const Farm& after_workers, const Action& action);
    const FlowObservation& latest() const { return samples_[last_]; }
    const std::array<FlowObservation, 24>& samples() const { return samples_; }
private:
    std::array<FlowObservation, 24> samples_{};
    int last_ = 0, pending_step_ = -1;
    Observation pending_{};
    Farm after_workers_{};
    Action action_{};
};
}
