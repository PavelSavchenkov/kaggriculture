#pragma once
#include "../league/top_replay_library/source/agent.hpp"

namespace catalog_rival_wool_context_v3_compositions {

template<class Base> class MarketOverlayAgent {
    Base base_;
    top_replay_library::Agent market_;
    int first_,last_;
public:
    MarketOverlayAgent(int market,int first,int last):market_(market),first_(first),last_(last) {}
    static kag::agent::AgentInfo info() {return {"market_overlay"};}
    void reset(const kag::agent::AgentInit& init) {base_.reset(init);market_.reset(init);}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& budget,kag::Action& action) {
        base_.act(o,budget,action);
        if(o.step>=first_ && o.step<=last_) {
            kag::Action source;market_.act(o,budget,source);
            action.n_orders=source.n_orders;std::copy_n(source.orders,source.n_orders,action.orders);action.finalize();
        }
    }
};

// A causal compiler diagnostic: retain one full worker course and replace a
// bounded market component. Full exact replay checks all resulting dependencies;
// this transform does not assume that the source physical contracts survive.
class MarketCourseAgent {
    top_replay_library::Agent workers_,markets_;
    int first_,last_;
public:
    MarketCourseAgent(int workers,int markets,int first,int last)
        :workers_(workers),markets_(markets),first_(first),last_(last) {}
    static kag::agent::AgentInfo info() {return {"market_course"};}
    void reset(const kag::agent::AgentInit& init) {workers_.reset(init);markets_.reset(init);}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& budget,kag::Action& action) {
        workers_.act(o,budget,action);
        if(o.step>=first_ && o.step<=last_) {
            kag::Action source;markets_.act(o,budget,source);
            action.n_orders=source.n_orders;std::copy_n(source.orders,source.n_orders,action.orders);
            action.finalize();
        }
    }
};

}
