#pragma once
#include "animal_ticket.hpp"
#include "biology.hpp"
#include "evaluation.hpp"
#include "market_tape.hpp"
#include <deque>
#include <string>

namespace compositions {
struct AnimalTicket {
    AnimalEdit edit;
    int purchased=0,picked=0,start_day=-1,end_day=30;
    uint32_t feed=0,care=0,collect=0,harvest=0,harvest_requested=0;
    std::array<int,30> harvest_step;
    bool eligible=false,returned=false;
    AnimalTicket() {harvest_step.fill(-1);}
};
struct TicketTrace {
    std::vector<AnimalTicket> tickets;
    std::array<std::array<int,kag::N_PRODUCTS>,719> prices{};
    std::array<uint8_t,8> shops{};
    std::array<int,kag::N_PRODUCTS> produced{};
    MarketTape market;
    std::array<std::array<int,kag::N_PRODUCTS>,719> sale_slots;
    int seat=0;
    double cash=0,rival_cash=0;
    std::string failure;
};

// Offline compiler instrumentation. Sim and sampled seeds never enter the
// deployable policy. Exact observed animal stock conserves purchase identities;
// unsupported returns are reported rather than assigned a guessed lineage.
template<class Source,class Opponent>
TicketTrace trace_tickets_from(Source& source,Opponent& rival,uint64_t seed,int seat) {
    using namespace kag;
    TicketTrace result;result.seat=seat;
    for(auto& slots:result.sale_slots)slots.fill(-1);
    Config config;config.seed=seed;Sim sim(config);
    source.reset(agent::runtime::make_agent_init(sim,seat));
    rival.reset(agent::runtime::make_agent_init(sim,seat^1));
    agent::DecisionBudget budget;budget.max_expansions=100000;
    uint64_t shop_rng=seed^0xa37108e62d045fb9ULL;
    for(auto& shop:result.shops)shop=random_word(shop_rng)%N_SHOPS;
    std::array<std::deque<int>,3> shed;
    std::array<std::array<std::deque<int>,3>,MAX_UNITS> carried;
    std::array<int,100> active;active.fill(-1);
    std::array<ActionAddress,100> structures;
    auto fail=[&](const char* reason){result.failure=std::to_string(sim.st.step)+":"+reason;};
    while(!sim.st.done) {
        const int step=sim.st.step,day=sim.st.day;
        const uint32_t bit=uint32_t{1}<<day;
        std::copy_n(result.shops.begin(),sim.st.n_shops,sim.st.shops);
        for(int item=0;item<N_PRODUCTS;++item)result.prices[step][item]=sim.st.market.prices[item];
        const auto own=agent::runtime::make_observation(sim,seat),other=agent::runtime::make_observation(sim,seat^1);
        Action acts[2];source.act(own,budget,acts[seat]);rival.act(other,budget,acts[seat^1]);
        validate_action(acts[seat],own);validate_action(acts[seat^1],other);
        result.market[step]=accepted_market(sim,acts);
        for(int i=0;i<acts[seat].n_orders;++i) {
            const auto& order=acts[seat].orders[i];
            if(order.op==M_SELL)result.sale_slots[step][order.item]=i;
        }
        auto unit_only=acts[seat];unit_only.n_orders=0;
        const auto accepted=sim.sanitize_solo_action(seat,unit_only);
        for(int u=0;u<own.self().n_units;++u) {
            const int cell=own.self().pos_y[u]*10+own.self().pos_x[u];
            int id=active[cell];
            if(id>=0 && acts[seat].units[u].op==OP_HARVEST) {
                auto& t=result.tickets[id];t.harvest_requested|=bit;
                if(t.harvest_step[day]<0)t.harvest_step[day]=step;
            }
            const auto& a=accepted.units[u];
            if(a.op==OP_BUILD_COOP || a.op==OP_BUILD_PASTURE)structures[cell]={step,u};
            if(a.op==OP_PICKUP && is_animal(a.arg)) {
                const int item=a.arg-GOOSE,take=std::min<int>(a.n,shed[item].size());
                if(take<1){fail("accepted pickup lacks traced stock");return result;}
                for(int k=0;k<take;++k) {
                    const int ticket=shed[item].front();shed[item].pop_front();
                    auto& t=result.tickets[ticket];
                    t.edit.pickup={step,u};t.picked=take;carried[u][item].push_back(ticket);
                }
            }
            if(a.op==OP_PLACE && is_animal(a.arg)) {
                const int item=a.arg-GOOSE;
                if(carried[u][item].empty()){fail("accepted animal place lacks traced stock");return result;}
                const auto& tile=own.self().tiles[cell/10][cell%10];
                const bool built=structures[cell].step==step;
                if(id>=0 || (!built && tile.kind!=(a.arg==GOOSE?T_COOP:T_PASTURE))) {
                    fail("animal return to shed unsupported");return result;
                }
                id=carried[u][item].front();carried[u][item].pop_front();active[cell]=id;
                auto& t=result.tickets[id];t.edit.placement={step,u};t.edit.structure=structures[cell];
                t.edit.cell=cell;t.start_day=day;
            }
            if(a.op==OP_DROP)for(int item=0;item<3;++item)if(!carried[u][item].empty()) {
                fail("animal DROP return unsupported");return result;
            }
            if(id>=0) {
                auto& t=result.tickets[id];
                if(a.op==OP_FEED)t.feed|=bit;
                if(a.op==OP_CARE)t.care|=bit;
                if(a.op==OP_COLLECT_FERTILIZER)t.collect|=bit;
                if(a.op==OP_HARVEST)t.harvest|=bit;
            }
        }
        for(int i=0;i<acts[seat].n_orders;++i) {
            const auto& order=acts[seat].orders[i];
            if(order.op!=M_BUY_ANIMAL)continue;
            int count[2]{};
            for(int after=0;after<2;++after) {
                auto phase=sim;phase.st.hour=0;
                Action prefix[2]={acts[0],acts[1]};
                for(auto& a:prefix)a.n_orders=std::min(a.n_orders,i+after);
                phase.step(prefix[0],prefix[1]);count[after]=phase.st.farms[seat].shed[order.item];
            }
            const int bought=count[1]-count[0];
            if(bought<0 || bought>order.n){fail("purchase accounting mismatch");return result;}
            for(int k=0;k<bought;++k) {
                AnimalTicket t;t.edit.original=order.item;t.edit.purchase={step,i};t.purchased=order.n;
                shed[order.item-GOOSE].push_back(result.tickets.size());result.tickets.push_back(t);
            }
        }
        const bool night=sim.st.hour==23;
        if(night) {
            auto phase=sim;phase.st.hour=0;phase.step(acts[0],acts[1]);
            const auto& farm=phase.st.farms[seat];
            int total=farm.shed_total;
            // Match the engine's worker and inventory insertion order. Returned
            // tickets keep their identity but leave the single-transfer search.
            for(int u=0;u<farm.n_units;++u)for(int k=0;k<farm.inv_nkeys[u];++k) {
                const int item=farm.inv_keys[u][k],count=farm.inv[u][item];
                const int kept=std::min(count,std::max(0,100-total));total+=kept;
                if(!is_animal(item))continue;
                auto& stock=carried[u][item-GOOSE];
                if(int(stock.size())!=count){fail("night animal identity mismatch");return result;}
                for(int i=0;i<count;++i) {
                    const int id=stock.front();stock.pop_front();result.tickets[id].returned=true;
                    if(i<kept)shed[item-GOOSE].push_back(id);
                }
            }
        }
        sim.step(acts[0],acts[1]);
        const auto& farm=sim.st.farms[seat];
        for(int item=0;item<3;++item)if(int(shed[item].size())!=farm.shed[GOOSE+item]) {
            fail("shed animal identity conservation failed");return result;
        }
        for(int u=0;u<farm.n_units;++u)for(int item=0;item<3;++item)
            if(int(carried[u][item].size())!=farm.inv[u][GOOSE+item]) {
                fail("carried animal identity conservation failed");return result;
            }
        for(int cell=0;cell<100;++cell)if(active[cell]>=0) {
            auto& t=result.tickets[active[cell]];const auto& tile=farm.tiles[cell/10][cell%10];
            if(!tile.has_animal || tile.what!=t.edit.original || tile.planted_day!=t.start_day) {
                t.end_day=day+1;active[cell]=-1;
            }
        }
    }
    for(auto& t:result.tickets)t.eligible=t.purchased==1 && t.picked==1 && t.start_day>=0 && !t.returned;
    const auto& farm=sim.st.farms[seat];
    result.cash=farm.money;result.rival_cash=sim.st.farms[seat^1].money;
    std::copy_n(farm.produced,N_PRODUCTS,result.produced.begin());
    return result;
}

template<class Opponent>
TicketTrace trace_tickets(int program,Opponent& rival,uint64_t seed,int seat) {
    top_replay_library::Agent source(program);
    return trace_tickets_from(source,rival,seed,seat);
}
}
