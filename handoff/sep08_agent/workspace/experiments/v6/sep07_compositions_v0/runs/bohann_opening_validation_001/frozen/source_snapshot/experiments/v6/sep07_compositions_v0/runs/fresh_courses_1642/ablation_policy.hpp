#pragma once
#include "../../include/terminal_layer.hpp"
#include "../crop_mix_001/proposals/crop_mix_t2_wheat/source/agent.hpp"
#include "library/source/agent.hpp"
#include "entry_guards.hpp"

namespace compositions::fresh_bohann {
class Policy {
    crop_mix_t2_wheat::Agent base_;
    fresh_courses_1642::Agent donor_{25};
    GuardedDay guard_;
    bool opening_, selected_ = false;
    int first_day_, capacity_ = 100;
public:
    Policy(bool opening, int first_day) : guard_(entry_guard(first_day)), opening_(opening), first_day_(first_day) {}
    static kag::agent::AgentInfo info() { return {"fresh_bohann_component"}; }
    void reset(const kag::agent::AgentInit& init) {
        base_.reset(init); donor_.reset(init); selected_ = false; capacity_ = init.config.shed_capacity;
    }
    bool selected() const { return selected_; }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& budget, kag::Action& action) {
        base_.act(o, budget, action);
        if (o.hour == 0 && o.day == first_day_) selected_ = guard_.matches(o);
        if ((opening_ && o.step <= 1) || selected_) {
            kag::Action alternative; donor_.act(o, budget, alternative);
            if (selected_) {
                action = alternative;
                terminal_layer(o, action, 3, capacity_);
            } else {
                action.n_orders = alternative.n_orders;
                std::copy_n(alternative.orders, alternative.n_orders, action.orders);
                action.finalize();
            }
        }
    }
};
}
