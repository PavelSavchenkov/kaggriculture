#include "../include/proposals.hpp"
#include "../include/scenario_io.hpp"
#include "../include/species_template.hpp"
#include "../candidates/composition_greedy_v1/source/agent.hpp"
#include <chrono>
#include <filesystem>
#include <iostream>

using namespace compositions;

int main(int argc,char** argv) {
    if(argc!=3){std::cerr<<"usage: estimate_species_templates SCENARIOS OUTPUT.csv\n";return 2;}
    auto scenarios=read_scenarios(argv[1]);
    if(std::filesystem::exists(argv[2]))return 2;
    std::ofstream out(argv[2]);
    out<<"program,variant,scenarios,estimated_cash,estimated_margin,min_cash,work_gap,layout_failures,animal_cost,wheat,egg,milk,wool,fertilizer,microseconds\n";
    for(int program:{0,4,55,78,85,89,116,131})for(int variant=0;variant<6;++variant) {
        const auto start=std::chrono::steady_clock::now();
        const auto source=greedy_v1::recorded_program(program);const auto items=species_items(variant);
        Proposal proposal;proposal.lives.assign(source.begin(),source.end());proposal.support=greedy_v1::recorded_support(program);
        for(auto& life:proposal.lives)life.item=items[life.item];
        estimate_proposal(proposal,scenarios);
        EstimateOptions options;options.recorded_support=true;options.service=ServiceModel::Recorded;
        const auto plan=estimate_plan(proposal.lives,proposal.support,options);
        const double us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();
        out<<program<<','<<variant<<','<<scenarios.size()<<','<<proposal.estimated_cash<<','<<proposal.estimated_margin<<','
            <<proposal.min_cash<<','<<proposal.work_gap<<','<<proposal.layout_failures<<','<<plan.animal_cost;
        for(int item:{kag::WHEAT,kag::EGG,kag::MILK,kag::WOOL,kag::FERTILIZER})out<<','<<plan.produced[item];
        out<<','<<us<<'\n';
    }
}
