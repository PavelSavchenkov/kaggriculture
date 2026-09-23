#pragma once
#include "workload.hpp"

namespace kag::day_compiler {
// Tracks the selected day's own completed work. Semantic groups remain bound
// at dawn; current market fills are read from the next legal observation.
class RemainingWork {
public:
    bool begin(const Observation& observation,const worker::DayInput& input,const worker::SolveOptions& constraints={});
    bool record(const Observation& before,const Action& action,const Configuration& config={});
    bool build(const Observation& current,Workload& out,bool direct_field_inputs=false) const;
    bool needs_land(const Observation& current) const { return current.self().n_quadrants<target_quadrants_; }
private:
    worker::DayInput original_{};
    worker::SolveOptions constraints_{};
    uint8_t events_[100]{};
    bool established_[100]{};
    int receipts_[N_PRODUCTS]{};
    int day_=-1, next_hour_=0, target_quadrants_=0;
};
}
