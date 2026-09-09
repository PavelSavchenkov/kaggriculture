#pragma once
#include "warm_shops.hpp"
// Source-contract extractor copied from improve_fertilization.cpp; offline only.
#include "baselines/warm/include/day_compile_helpers.hpp"
#include "baselines/warm/include/biology.hpp"
#include "agents/external/early_structure_cow/source/agent.hpp"
#include "agents/external/nanare_four_quadrant_course/source/agent.hpp"
using namespace compositions;
using namespace compositions::day_contract;
using Source=kag::agents::early_structure_cow::Agent;
inline kag::agent::DecisionBudget decision_budget(){
    kag::agent::DecisionBudget b;b.max_expansions=100000;return b;
}

struct CropLife {
    int cell;
    Cohort cohort;
    Service service{0,0,0,0,0,0};
    std::array<int,30> harvested{};
    bool exact=false;
};

struct AnimalLife {
    int cell;
    Cohort cohort;
    Service service{0,0,0,0,0,0};
    std::array<int,30> harvested{},fertilizer{};
    bool exact=false;
};

using ContextRival=kag::agents::nanare_four_quadrant_course::Agent;
struct Season {
    std::vector<RecordedDay> days;
    std::array<Farm,2> final_farms;
    uint64_t hashes[2]={14695981039346656037ULL,14695981039346656037ULL};
    std::array<Farm,30> end_farms;
    std::vector<CropLife> lives;
    std::vector<AnimalLife> animals;
    std::array<uint8_t,8> shops;
    std::array<std::array<Action,24>,30> raw;
    explicit Season(uint64_t seed) {
        Config config;config.seed=seed;Sim sim(config);Source own;ContextRival rival;
        own.reset(agent::runtime::make_agent_init(sim,0));rival.reset(agent::runtime::make_agent_init(sim,1));
        // Offline diagnostic world from episode106867299. Revealed to agents only on schedule.
        shops=warm_fixture_shops();
        days.reserve(30);
        while(!sim.st.done) {
            std::copy_n(shops.begin(),sim.st.n_shops,sim.st.shops);
            if(sim.st.hour==0)days.emplace_back(sim);
            Action pair[2];own.act(agent::runtime::make_observation(sim,0),decision_budget(),pair[0]);
            rival.act(agent::runtime::make_observation(sim,1),decision_budget(),pair[1]);
            for(int p=0;p<2;++p){
                validate_action(pair[p],agent::runtime::make_observation(sim,p));
                hash_action(hashes[p],pair[p]);
            }
            raw[sim.st.day][sim.st.hour]=pair[0];
            const auto before=sim;sim.step(pair[0],pair[1]);append_contract(days.back(),before,sim,pair);
            end_farms[before.st.day]=sim.st.farms[0];
        }
        if(sim.st.step!=719)std::abort();
        final_farms={sim.st.farms[0],sim.st.farms[1]};
        // Pad only the offline day contract with a PASS hour. Actual games
        // still end after hour 22 on day 29; no padded output is scored.
        sim.st.done=false; sim.cfg.episode_steps=744;
        Action padding[2];
        for(int p=0;p<2;++p){padding[p].n_units=sim.st.farms[p].n_units;padding[p].finalize();}
        const auto before_padding=sim;
        sim.step(padding[0],padding[1]);
        append_contract(days.back(),before_padding,sim,padding);
        end_farms[29]=sim.st.farms[0];
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
        std::array<int,100> active_animal;active_animal.fill(-1);
        for(int day=0;day<30;++day){
            for(const auto& work:days[day].problem.tile_work)for(const auto& op:work.actions){
                auto& id=active_animal[work.tile];
                if(op.op==OP_PLACE && is_animal(op.arg)){
                    if(id>=0)std::abort();
                    id=animals.size();animals.push_back({work.tile,{uint8_t(op.arg),1,day,30}});
                }
                if(id<0)continue;
                auto& life=animals[id];const uint32_t bit=1u<<day;
                if(op.op==OP_FEED)life.service.feed|=bit;
                if(op.op==OP_CARE)life.service.care|=bit;
                if(op.op==OP_COLLECT_FERTILIZER){life.service.collect_fertilizer|=bit;life.fertilizer[day]+=op.output_quantity;}
                if(op.op==OP_HARVEST){life.service.harvest|=bit;life.harvested[day]+=op.output_quantity;}
            }
            if(day<29)for(int cell=0;cell<100;++cell)if(active_animal[cell]>=0){
                const auto& tile=end_farms[day].tiles[cell/10][cell%10];
                if(!tile.has_animal){animals[active_animal[cell]].cohort.end_day=day+1;active_animal[cell]=-1;}
            }
        }
        for(auto& life:animals){
            const auto predicted=biology(life.cohort,life.service);life.exact=true;
            const int product=ANIMALS[life.cohort.item-GOOSE].product;
            for(int day=0;day<30;++day)life.exact&=predicted.days[day].output[product]==life.harvested[day] &&
                predicted.days[day].output[FERTILIZER]==life.fertilizer[day];
        }
        for(auto& life:lives) {
            const auto predicted=biology(life.cohort,life.service);life.exact=true;
            for(int day=0;day<30;++day)life.exact&=predicted.days[day].output[life.cohort.item]==life.harvested[day];
        }
    }
};
