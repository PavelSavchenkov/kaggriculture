#pragma once
#include "../day_service_bank_sep08_001/proposals/service_bank_p362_m2/source/agent.hpp"
#include "../cold_renewal_sep08_001/proposals/cold_renewal_p98/source/agent.hpp"

namespace compositions::shop_branch {
template<int Mode> class Agent {
    service_bank_p362_m2::Agent baseline_;
    cold_renewal_p98::Agent cows_;
    int choice_ = -1;
public:
    void reset(const kag::agent::AgentInit& init) {
        baseline_.reset(init);
        cows_.reset(init);
        choice_ = -1;
    }
    void act(const kag::agent::AgentObservation& o,
             const kag::agent::DecisionBudget& budget, kag::Action& action) {
        action.clear();
        action.n_units = 1 + o.own_hand_count();
        action.finalize();
        if constexpr (Mode == 0) {
            baseline_.act(o, budget, action);
            return;
        }
        if constexpr (Mode == 1) {
            cows_.act(o, budget, action);
            return;
        }
        if (o.step < 144) {
            kag::Action unused;
            baseline_.act(o, budget, action);
            cows_.act(o, budget, unused);
            return;
        }
        if (choice_ < 0) {
            choice_ = 0;
            if (o.step == 144 && o.n_shops == 2) {
                bool egg_or_yarn = false;
                for (int i = 0; i < 2; ++i)
                    egg_or_yarn |= o.shops[i] == 0 || o.shops[i] == 1 || o.shops[i] == 7;
                choice_ = !egg_or_yarn;
            }
        }
        if (choice_) cows_.act(o, budget, action);
        else baseline_.act(o, budget, action);
    }
};
}
