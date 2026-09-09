#pragma once
// Source-contract extractor copied from improve_fertilization.cpp; offline only.
#include "day_compile_helpers.hpp"
#include "biology.hpp"
#include "guarded_sequence.hpp"
#include "../candidates/investment_context_guarded_001_best/source/agent.hpp"

using namespace compositions;
using namespace compositions::day_contract;
using Source=investment_context_guarded_001_best::Agent;


struct CropLife {
    int cell;
    Cohort cohort;
    Service service{0,0,0,0,0,0};
    std::array<int,30> harvested{};
    bool exact=false;
};

struct Season {
    std::vector<RecordedDay> days;
    std::vector<CropLife> lives;
    std::array<uint8_t,8> shops;
    std::array<std::array<Action,24>,30> raw;
    explicit Season(uint64_t seed) {
        Config config;config.seed=seed;Sim sim(config);Source own;public_router::Agent rival;
        own.reset(agent::runtime::make_agent_init(sim,0));rival.reset(agent::runtime::make_agent_init(sim,1));
        uint64_t random=seed^0xa37108e62d045fb9ULL;
        for(auto& shop:shops)shop=random_word(random)%N_SHOPS;
        days.reserve(30);
        while(!sim.st.done) {
            std::copy_n(shops.begin(),sim.st.n_shops,sim.st.shops);
            if(sim.st.hour==0)days.emplace_back(sim);
            Action pair[2];own.act(agent::runtime::make_observation(sim,0),{},pair[0]);
            rival.act(agent::runtime::make_observation(sim,1),{},pair[1]);
            raw[sim.st.day][sim.st.hour]=pair[0];
            const auto before=sim;sim.step(pair[0],pair[1]);append_contract(days.back(),before,sim,pair);
        }
        std::array<int,100> active;active.fill(-1);
        for(int day=0;day<30;++day) {
            for(const auto& work:days[day].problem.tile_work)for(const auto& op:work.actions) {
                auto& id=active[work.tile];
                if(op.op==OP_PLANT) {
                    if(id>=0)std::abort();
                    id=lives.size();lives.push_back({work.tile,{uint8_t(op.arg),1,day,30}});
                }
                if(id<0)continue;
                auto& life=lives[id];const uint32_t bit=uint32_t(1)<<day;
                if(op.op==OP_WATER)life.service.water|=bit;
                if(op.op==OP_FERTILIZE)life.service.fertilize|=bit;
                if(op.op==OP_HARVEST) {
                    life.service.harvest|=bit;life.harvested[day]+=op.output_quantity;
                    if(!CROPS[life.cohort.item].ongoing){life.cohort.end_day=day+1;id=-1;}
                }
                if(op.op==OP_DIG){life.cohort.end_day=day+1;id=-1;}
            }
            if(day<29)for(int cell=0;cell<100;++cell)if(active[cell]>=0) {
                const auto& tile=days[day+1].start.st.farms[0].tiles[cell/10][cell%10];
                auto& life=lives[active[cell]];
                if(tile.kind!=T_PLANT || tile.planted_day!=life.cohort.start_day) {
                    life.cohort.end_day=day+1;active[cell]=-1;
                }
            }
        }
        for(auto& life:lives) {
            const auto predicted=biology(life.cohort,life.service);life.exact=true;
            for(int day=0;day<30;++day)life.exact&=predicted.days[day].output[life.cohort.item]==life.harvested[day];
        }
    }
};

