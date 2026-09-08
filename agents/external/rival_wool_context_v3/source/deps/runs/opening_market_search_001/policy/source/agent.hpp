#pragma once
#include "../../../crop_mix_001/proposals/crop_mix_t2_wheat/source/agent.hpp"

namespace catalog_rival_wool_context_v3_compositions::opening_market_search {
class Agent {
    crop_mix_t2_wheat::Agent base_;
    int quantity_, buffer_, mode_, shift_ = 0;
public:
    Agent(int quantity = 81, int buffer = 13, int mode = 0) : quantity_(quantity), buffer_(buffer), mode_(mode) {
        if (quantity < 0 || quantity > 120 || buffer < 5 || buffer > 120 || mode < 0 || mode > 2) std::abort();
    }
    static kag::agent::AgentInfo info() { return {"opening_market_search"}; }
    void reset(const kag::agent::AgentInit& init) { base_.reset(init); shift_ = 0; }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& budget, kag::Action& a) {
        base_.act(o, budget, a);
        using namespace kag;
        if (o.step == 0) {
            const auto original = a;
            if (a.n_orders > 8) return;
            a.n_orders = 0;
            if (quantity_) {
                a.orders[a.n_orders++] = {M_BUY_PRODUCT, WHEAT, quantity_};
                a.orders[a.n_orders++] = {M_SELL, WHEAT, quantity_};
            }
            if (mode_ == 0) a.orders[a.n_orders++] = {M_BUY_PRODUCT, WHEAT, buffer_};
            else for (int i = 0; i < original.n_orders; ++i) {
                auto order = original.orders[i];
                if (mode_ == 2 && order.op == M_BUY_PRODUCT && order.item == WHEAT) {
                    shift_ = order.n - buffer_;
                    order.n = buffer_;
                }
                a.orders[a.n_orders++] = order;
            }
        } else if (o.step == 1) {
            if (mode_ == 0) {
                a.n_orders = 8;
                a.orders[0] = {M_SELL, WHEAT, buffer_ - 5};
                for (int i = 1; i <= 5; ++i) a.orders[i] = {M_HIRE, 0, 0};
                a.orders[6] = {M_BUY_ANIMAL, COW, 2};
                a.orders[7] = {M_BUY_ANIMAL, SHEEP, 2};
            } else if (mode_ == 2) {
                for (int i = 0; i < a.n_orders; ++i) if (a.orders[i].op == M_SELL && a.orders[i].item == WHEAT) {
                    a.orders[i].n = std::max(0, a.orders[i].n - shift_);
                    break;
                }
            }
        }
        a.finalize();
    }
};
}
