// Offline day-contract repair; no simulator state enters a runtime policy.
#include "season.hpp"
#include "policy.hpp"
#include "experiments/v6/sep07_compositions_v0/league/king_rc4/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/animal_groups_sep08_001/source/fixed_repair.hpp"
using Policy=compositions::animal_groups_policy::Policy;
using Rival=compositions::king_rc4::Agent;
Sim canonical(Sim sim){std::swap(sim.st.farms[0],sim.st.farms[1]);return sim;}
bool endpoint(const Farm& farm,const DayProblem& problem,int day){
    for(int i=0;i<N_ITEMS;++i)if(farm.shed[i]!=problem.end_shed[i])return false;
    for(int i=0;i<N_CROPS;++i)if(farm.seeds[i]!=problem.end_seeds[i])return false;
    for(int c=0;c<100;++c){auto actual=managed(farm.tiles[c/10][c%10],day);const auto expected=*problem.required_end_tiles[c].exact_state;
        if(actual.kind==ManagedTileKind::WEED && expected.kind==ManagedTileKind::EMPTY)actual={};
        if(actual!=expected)return false;
    }return true;
}
int main(int argc,char** argv){
    if(argc!=2 || fs::exists(argv[1]))return 2;const fs::path out=argv[1];fs::create_directories(out);
    Config config;config.seed=1014;Sim sim(config),saved(config);Policy own(1),saved_own(1);Rival rival,saved_rival;
    own.reset(agent::runtime::make_agent_init(sim,1));rival.reset(agent::runtime::make_agent_init(sim,0));
    std::array<uint8_t,8> shops;uint64_t rng=config.seed^0xa37108e62d045fb9ULL;for(auto& s:shops)s=random_word(rng)%N_SHOPS;
    std::optional<RecordedDay> original;uint64_t hashes[2]={14695981039346656037ULL,14695981039346656037ULL};
    while(!sim.st.done){
        std::copy_n(shops.begin(),sim.st.n_shops,sim.st.shops);
        if(sim.st.day==23 && sim.st.hour==0){saved=sim;saved_own=own;saved_rival=rival;original.emplace(canonical(sim));}
        Action pair[2];own.act(agent::runtime::make_observation(sim,1),decision_budget(),pair[1]);
        rival.act(agent::runtime::make_observation(sim,0),decision_budget(),pair[0]);
        for(int p=0;p<2;++p){validate_action(pair[p],agent::runtime::make_observation(sim,p));hash_action(hashes[p],pair[p]);}
        if(sim.st.day==23){original->own[sim.st.hour]=pair[1];original->rival[sim.st.hour]=pair[0];}
        sim.step(pair[0],pair[1]);
    }
    Policy control(1);Rival control_rival;Options options;options.validate=true;
    const auto base=run_game(control,control_rival,1014,1,options);
    for(int p=0;p<2;++p)if(base.hash[p]!=hashes[p] || base.cash[p]!=sim.st.farms[p].money)std::abort();
    const auto baseline_farm=sim.st.farms[1];
    auto problem=load_problem_json("experiments/v6/sep07_compositions_v0/runs/animal_group_policy_sep08_001/contexts_audit/cow_triple_alternate/days/23/problem.json");
    original->problem=problem;problem.start=RecordedDay(canonical(saved)).problem.start;
    bool added=false;
    for(auto& work:problem.tile_work)if(work.tile==38){
        if(work.actions.empty() || work.actions.front().op!=OP_PLANT)std::abort();
        TileWorkAction dig;dig.op=OP_DIG;dig.arg=-1;work.actions.insert(work.actions.begin(),dig);added=true;
    }
    if(!added || saved.st.farms[1].tiles[3][8].kind!=T_WEED)std::abort();
    for(int c=0;c<100;++c){
        bool worked=false;for(const auto& w:problem.tile_work)worked|=w.tile==c;
        auto& end=*problem.required_end_tiles[c].exact_state;const auto& tile=saved.st.farms[1].tiles[c/10][c%10];
        if(!worked && (tile.kind==T_EMPTY || tile.kind==T_WEED) && (end.kind==ManagedTileKind::EMPTY || end.kind==ManagedTileKind::WEED))end=managed(tile,24);
    }
    std::ofstream results(out/"RESULTS.csv");results<<"extra_hires,physical,endpoint,turns,cash,rival_cash,own_gain,margin_gain,wheat_gain,matched_days,missed_days\n";
    for(int extra=0;extra<=2;++extra){
        auto trial=problem;auto orders=original->own;
        for(int n=0;n<extra;++n){
            bool placed=false;
            for(int h=0;h<3 && !placed;++h)if(orders[h].n_orders<10){
                const int slot=orders[h].n_orders++;orders[h].orders[slot]={M_HIRE,0,1};orders[h].finalize();
                trial.market_plan.push_back({int8_t(h),int8_t(slot),M_HIRE,-1,1});++trial.worker_count;placed=true;
            }if(!placed)std::abort();
        }
        day_scheduler::prepare_problem(trial);const auto folder=out/("h"+std::to_string(extra));fs::create_directories(folder);
        save_problem_json(trial,folder/"problem.json");
        auto solved=fixed_repair(*original,trial,30);
        if(!solved.schedule){day_scheduler::Options budget;budget.seconds=30;budget.fallback_workers=1;solved=day_scheduler::solve(trial,budget);}
        if(!solved.schedule){results<<extra<<",0,0,0,0,0,0,0,0,0,0\n";results.flush();continue;}
        auto plan=*solved.schedule;const auto certificate=replay_schedule(trial,plan);
        if(!certificate.requirements_satisfied || !certificate.invariants_satisfied || !certificate.errors.empty())std::abort();
        for(int h=0;h<24;++h){plan[h].n_orders=orders[h].n_orders;std::copy_n(orders[h].orders,orders[h].n_orders,plan[h].orders);plan[h].finalize();}
        save_actions(plan,folder/"actions.txt");sim=saved;own=saved_own;rival=saved_rival;bool exact=false;
        while(!sim.st.done){
            std::copy_n(shops.begin(),sim.st.n_shops,sim.st.shops);Action pair[2];
            own.act(agent::runtime::make_observation(sim,1),decision_budget(),pair[1]);rival.act(agent::runtime::make_observation(sim,0),decision_budget(),pair[0]);
            if(sim.st.day==23)pair[1]=plan[sim.st.hour];
            for(int p=0;p<2;++p)validate_action(pair[p],agent::runtime::make_observation(sim,p));
            sim.step(pair[0],pair[1]);if(sim.st.day==24 && sim.st.hour==0)exact=endpoint(sim.st.farms[1],trial,24);
        }
        const auto& d=own.diagnostics();
        results<<extra<<",1,"<<exact<<','<<sim.st.step<<','<<sim.st.farms[1].money<<','<<sim.st.farms[0].money
            <<','<<sim.st.farms[1].money-base.cash[1]<<','<<sim.st.farms[1].money-sim.st.farms[0].money-base.cash[1]+base.cash[0]
            <<','<<sim.st.farms[1].produced[WHEAT]-baseline_farm.produced[WHEAT]<<','<<d.matched_days<<','<<d.missed_days<<'\n';results.flush();
        if(exact){
            const auto g=guarded(RecordedDay(canonical(saved)),trial,plan);
            export_schedule("animal_weed38_h"+std::to_string(extra),plan,folder);
            std::ofstream guard(folder/"guard.txt");guard<<23<<' '<<g.quadrants<<'\n';
            for(int c=0;c<100;++c){guard<<g.check[c];for(int v:g.tiles[c])guard<<' '<<v;guard<<'\n';}
            for(int v:g.shed)guard<<v<<' ';guard<<'\n';for(int v:g.seeds)guard<<v<<' ';guard<<'\n';
        }
    }
}
