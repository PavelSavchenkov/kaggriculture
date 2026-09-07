#pragma once
#include "../league/top_replay_library/source/agent.hpp"
#include <array>
#include <cstdlib>

namespace compositions {

inline std::array<int,kag::N_ITEMS> species_items(int variant) {
    std::array<int,kag::N_ITEMS> items{};
    for(int i=0;i<kag::N_ITEMS;++i)items[i]=i;
    auto change=[&](int from,int to) {
        items[from]=to;
        items[kag::ANIMALS[from-kag::GOOSE].product]=kag::ANIMALS[to-kag::GOOSE].product;
    };
    switch(variant) {
        case 0:break;
        case 1:change(kag::GOOSE,kag::SHEEP);break;
        case 2:change(kag::GOOSE,kag::COW);break;
        case 3:change(kag::COW,kag::SHEEP);break;
        case 4:change(kag::SHEEP,kag::COW);break;
        case 5:change(kag::GOOSE,kag::SHEEP);change(kag::COW,kag::SHEEP);break;
        default:std::abort();
    }
    return items;
}

// A bounded compiler for whole-species composition substitutions. It preserves
// the source birth dates, tiles and worker service course, changes animal and
// output commodities together, and converts every affected goose structure.
// Full-game validation owns funding, changed yield dates/caps and missed work.
// This template is an execution proposal, not a feasibility certificate.
class SpeciesTemplateAgent {
    top_replay_library::Agent source_;
    std::array<int,kag::N_ITEMS> items_{};
    bool convert_coops_=false,clear_output_=false;
public:
    SpeciesTemplateAgent(int program,int variant,bool clear_output=false)
        :source_(program),clear_output_(clear_output) {
        items_=species_items(variant);convert_coops_=items_[kag::GOOSE]!=kag::GOOSE;
    }
    static kag::agent::AgentInfo info() {return {"species_template"};}
    void reset(const kag::agent::AgentInit& init) {source_.reset(init);}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& budget,kag::Action& action) {
        source_.act(o,budget,action);
        for(int u=0;u<action.n_units;++u) {
            auto& a=action.units[u];
            if(a.op==kag::OP_PLACE || a.op==kag::OP_PICKUP)a.arg=items_[a.arg];
            if(a.op==kag::OP_BUILD_COOP && convert_coops_)a.op=kag::OP_BUILD_PASTURE;
        }
        for(int i=0;i<action.n_orders;++i) {
            auto& order=action.orders[i];
            if(order.op!=kag::M_BUY_ANIMAL && order.op!=kag::M_BUY_PRODUCT && order.op!=kag::M_SELL)continue;
            const int old=order.item;order.item=items_[old];
            if(clear_output_ && order.op==kag::M_SELL) {
                bool changed=items_[old]!=old;
                for(int j=0;j<kag::N_PRODUCTS;++j)changed|=items_[j]!=j && items_[j]==old;
                // Source sale slots remain fixed; sell whatever transformed
                // output actually reaches the shed, including this turn's work.
                if(changed)order.n=100;
            }
        }
        action.finalize();
    }
};
}
