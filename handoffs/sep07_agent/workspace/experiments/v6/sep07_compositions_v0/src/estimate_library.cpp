#include "../include/estimate.hpp"
#include "../candidates/composition_greedy_v0/source/agent.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <iomanip>

int main(int argc,char** argv) {
    if(argc!=3)return 2;
    std::ifstream input(argv[1]);int count;std::string format;input>>format;
    bool two_sided=format=="two_sided_v1";
    if(two_sided)input>>count;else count=std::stoi(format);
    if(!input || count<1 || count>10000)return 2;
    std::vector<compositions::EconomicScenario> scenarios(count);
    for(auto& scenario:scenarios) {
        uint64_t seed;int seat,nstock,nfixed,nrival;double cash;
        input>>seed>>seat>>cash>>nstock>>nfixed>>nrival;
        if(two_sided)input>>scenario.rival_fixed_cost;
        for(auto& shop:scenario.shops) {int value;input>>value;shop=value;}
        for(int i=0;i<nstock;++i) {int a,b,c,d;input>>a>>b>>c>>d;}
        for(int i=0;i<nfixed;++i) {int a,b,c;input>>a>>b>>c;}
        for(int i=0;i<nrival;++i) {
            int step,item,bought,sold;input>>step>>item>>bought>>sold;
            if(step<0 || step>=719 || item<0 || item>=kag::N_PRODUCTS)return 2;
            scenario.rival_buys[step][item]+=bought;scenario.rival_sells[step][item]+=sold;
        }
    }
    if(!input)return 2;
    std::ofstream out(argv[2]);
    out<<"program,service,recorded_support,recorded_layout,scenarios,conditional_cash,min_cash,funding_cases,input_gap_cases,missing_inputs,discards,excess_orders,tile_conflicts,unplaced_lives,uncovered_work,field_operations,work_turns,hires,hire_cost,land_cost,capital,wheat_input,fertilizer_input";
    out<<",conditional_rival_cash,conditional_margin";
    for(int i=0;i<9;++i)out<<",produced_"<<i;
    out<<'\n'<<std::fixed<<std::setprecision(3);
    const auto start=std::chrono::steady_clock::now();
    int profiles=0,evaluations=0;
    for(int program=0;program<72;++program)for(int mode=0;mode<3;++mode)for(int source_support=0;source_support<2;++source_support)for(int layout=0;layout<2;++layout) {
        compositions::EstimateOptions options;
        options.service=compositions::ServiceModel(mode);options.recorded_support=source_support;options.recorded_layout=layout;
        const auto plan=compositions::estimate_plan(compositions::greedy::recorded_program(program),compositions::greedy::recorded_support(program),options);
        double cash=0,rival_cash=0,min_cash=0,missing=0,discards=0,orders=0;int funding=0,gaps=0;
        for(const auto& scenario:scenarios) {
            auto value=compositions::economics(plan.financial,scenario);
            cash+=value.cash;rival_cash+=value.rival_cash;min_cash+=value.min_cash;funding+=value.first_funding_step>=0;gaps+=value.first_input_gap_step>=0;
            missing+=std::accumulate(value.missing.begin(),value.missing.end(),0);
            discards+=std::accumulate(value.discarded.begin(),value.discarded.end(),0);
            orders+=value.excess_order_slots;++evaluations;
        }
        auto total=[](const auto& values){return std::accumulate(values.begin(),values.end(),0);};
        out<<program<<','<<mode<<','<<source_support<<','<<layout<<','<<count<<','<<cash/count<<','<<min_cash/count
            <<','<<funding<<','<<gaps<<','<<missing/count<<','<<discards/count<<','<<orders/count
            <<','<<plan.tile_conflicts<<','<<plan.unplaced_lives<<','<<plan.uncovered_work<<','<<total(plan.field_operations)
            <<','<<total(plan.work_turns)<<','<<total(plan.hands)<<','<<plan.hire_cost<<','<<plan.land_cost
            <<','<<plan.seed_cost+plan.animal_cost<<','<<plan.input_wheat<<','<<plan.input_fertilizer
            <<','<<rival_cash/count<<','<<(cash-rival_cash)/count;
        for(int v:plan.produced)out<<','<<v;out<<'\n';++profiles;
    }
    double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    std::cout<<"profiles="<<profiles<<" scenario_evaluations="<<evaluations<<" seconds="<<seconds
        <<" microseconds_per_profile_scenario="<<seconds*1e6/evaluations<<'\n';
}
