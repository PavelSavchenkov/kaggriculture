#pragma once
#include "../league/top_replay_library/source/agent.hpp"
#include <array>
#include <utility>

namespace catalog_wool_family_context_v1_compositions {
struct ActionAddress {int step=-1,index=-1;};
struct AnimalEdit {
    int original=kag::COW,replacement=kag::SHEEP;
    ActionAddress purchase,pickup,placement,structure;
    int cell=-1;
};

// A single-purchase composition edit with traced input and placement ownership.
// The old service route is reused. Output handling is a selectable compiler
// alternative; complete-game replay must check funding, yield and capacity.
class AnimalTicketPatch {
    AnimalEdit edit_;
    bool change_,handle_output_,active_=false;
    int deposit_repairs_=0,sale_repairs_=0;
public:
    AnimalTicketPatch(AnimalEdit edit,bool change=true,bool output=true)
        :edit_(edit),change_(change),handle_output_(output) {}
    static kag::agent::AgentInfo info() {return {"animal_ticket"};}
    void reset() {
        active_=false;deposit_repairs_=sale_repairs_=0;
    }
    int deposit_repairs() const {return deposit_repairs_;}
    int sale_repairs() const {return sale_repairs_;}
    void set_replacement(int item) {
        if(active_ || !kag::is_animal(item))std::abort();
        edit_.replacement=item;
    }
    void amend(const kag::agent::AgentObservation& o,kag::Action& a) {
        if(o.step==edit_.purchase.step) {
            const int i=edit_.purchase.index;
            if(i<0 || i>=a.n_orders || a.orders[i].op!=kag::M_BUY_ANIMAL ||
               a.orders[i].item!=edit_.original || a.orders[i].n!=1)std::abort();
            active_=true;
            if(change_)a.orders[i].item=edit_.replacement;
        }
        if(change_) {
            for(const auto address:{edit_.pickup,edit_.placement})if(o.step==address.step) {
                // Missing hires are an observed execution failure. Do not issue
                // an action for a nonexistent worker or redirect another worker.
                if(address.index<a.n_units) {
                    auto& unit=a.units[address.index];
                    if(unit.arg!=edit_.original || (unit.op!=kag::OP_PICKUP && unit.op!=kag::OP_PLACE))std::abort();
                    unit.arg=edit_.replacement;
                }
            }
            if(edit_.structure.step>=0 && o.step==edit_.structure.step && edit_.structure.index<a.n_units) {
                auto& unit=a.units[edit_.structure.index];
                if(unit.op!=kag::OP_BUILD_COOP && unit.op!=kag::OP_BUILD_PASTURE)std::abort();
                unit.op=edit_.replacement==kag::GOOSE?kag::OP_BUILD_COOP:kag::OP_BUILD_PASTURE;
            }
        }
        if(active_ && handle_output_ && edit_.original!=edit_.replacement) {
            const int from=kag::ANIMALS[edit_.original-kag::GOOSE].product;
            const int to=kag::ANIMALS[edit_.replacement-kag::GOOSE].product;
            for(int u=0;u<a.n_units;++u) {
                auto& unit=a.units[u];
                if(unit.op==kag::OP_PLACE && unit.arg==from && o.own.inv[u][to]>0) {
                    // A mixed load needs both products deposited. DROP can also
                    // deposit other carried inputs, so its effects are ablated.
                    unit={kag::OP_DROP,0,0};++deposit_repairs_;
                }
            }
            bool sale=false,already=false;
            for(int i=0;i<a.n_orders;++i)if(a.orders[i].op==kag::M_SELL) {
                sale|=a.orders[i].item==from;already|=a.orders[i].item==to;
            }
            if(sale && !already && a.n_orders<10) {
                a.orders[a.n_orders++]={kag::M_SELL,uint8_t(to),100};++sale_repairs_;
            }
        }
        a.finalize();
    }
};
template<class Base> class AnimalTicketOverlay:public AnimalTicketPatch {
    Base source_;
public:
    AnimalTicketOverlay(Base source,AnimalEdit edit,bool change=true,bool output=true)
        :AnimalTicketPatch(edit,change,output),source_(std::move(source)){}
    void reset(const kag::agent::AgentInit& init) {source_.reset(init);AnimalTicketPatch::reset();}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& b,kag::Action& a) {
        source_.act(o,b,a);amend(o,a);
    }
};
class AnimalTicketAgent:public AnimalTicketOverlay<top_replay_library::Agent> {
public:
    AnimalTicketAgent(int program,AnimalEdit edit,bool change=true,bool output=true)
        :AnimalTicketOverlay(top_replay_library::Agent(program),edit,change,output){}
};
}
