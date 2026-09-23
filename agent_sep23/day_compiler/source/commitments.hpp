#pragma once
#include "intent.hpp"

namespace kag::day_compiler {
struct CommitmentReport {
    int retained_animals = 0, escapes = 0, feeds = 0, care = 0, animal_production = 0;
    int retained_crops = 0, crop_yield = 0, crop_production = 0;
    int new_crop_counts = 0, establishment_water = 0, animal_targets = 0, land = 0;
    int total() const {
        return retained_animals + escapes + feeds + care + animal_production + retained_crops + crop_yield + crop_production +
               new_crop_counts + establishment_water + animal_targets + land;
    }
};
class CommitmentAudit {
public:
    void begin(const Observation& dawn, const detail::BoundIntent& intent);
    void record(const Observation& before, const Action& action, const Farm& after_workers);
    CommitmentReport finish(const Observation& final);
private:
    Observation dawn_{};
    detail::BoundIntent intent_{};
    Farm latest_{};
    int harvest_[100]{};
    int planted_[N_CROPS]{};
    bool fed_[100]{}, cared_[100]{}, established_[100]{};
    Biology biology_;
};
}
