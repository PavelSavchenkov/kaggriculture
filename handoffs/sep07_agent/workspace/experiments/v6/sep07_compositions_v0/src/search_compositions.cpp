#include "../include/evaluation.hpp"
#include "../include/proposals.hpp"
#include "../include/scenario_io.hpp"
#include "../candidates/composition_greedy_v0/source/agent.hpp"
#include "../league/public_router/source/agent.hpp"
#include <filesystem>
#include <iomanip>
#include <unordered_set>

using namespace compositions;
namespace fs=std::filesystem;

void save_proposal(const Proposal& p,const fs::path& directory,const std::string& name) {
    if(fs::exists(directory)) {std::fprintf(stderr,"refusing to overwrite proposal: %s\n",directory.c_str());std::exit(2);}
    fs::create_directories(directory/"source");
    const auto compiler=fs::absolute("experiments/v6/sep07_compositions_v0/candidates/composition_greedy_v0");
    const auto header_path=fs::relative(compiler/"source/agent.hpp",directory/"source").generic_string();
    const auto source_path=fs::relative(compiler/"source/agent.cpp",directory).generic_string();
    std::ofstream json(directory/"PLAN.json");
    json<<"{\"proposal_id\":"<<p.id<<",\"parent_id\":"<<p.parent<<",\"source_program\":"<<p.source_program
        <<",\"edit\":\""<<p.edit<<"\",\"estimated_margin\":"<<p.estimated_margin
        <<",\"exact_margin\":"<<p.exact_margin<<",\"exact_utility\":"<<p.exact_utility
        <<",\"phenotype\":\""<<p.phenotype<<"\",\"lives\":[";
    std::ofstream plan(directory/"source/plan.inc");
    plan<<"inline constexpr Life lives[]={\n";
    for(size_t i=0;i<p.lives.size();++i) {
        const auto& v=p.lives[i];if(i)json<<',';
        json<<'['<<v.item<<','<<v.start<<','<<v.end<<','<<v.x<<','<<v.y<<','<<v.fertilize<<','<<v.water<<','<<v.feed<<','<<v.care<<','<<v.collect<<','<<v.harvest<<']';
        plan<<'{'<<v.item<<','<<v.start<<','<<v.end<<','<<v.x<<','<<v.y<<','<<v.fertilize<<','<<v.water<<','<<v.feed<<','<<v.care<<','<<v.collect<<','<<v.harvest<<"},\n";
    }
    plan<<"};\ninline constexpr Support support={{";
    json<<"],\"hands\":[";
    for(int i=0;i<30;++i) {if(i){plan<<',';json<<',';}plan<<p.support.hands[i];json<<p.support.hands[i];}
    plan<<"},{";json<<"],\"quadrants\":[";
    for(int i=0;i<30;++i) {if(i){plan<<',';json<<',';}plan<<p.support.quadrants[i];json<<p.support.quadrants[i];}
    plan<<"}};\n";json<<"]}\n";
    std::ofstream header(directory/"source/agent.hpp");
    header<<"#pragma once\n#include \""<<header_path<<"\"\nnamespace compositions::"<<name<<" {\n#include \"plan.inc\"\n"
        <<"class Agent:public greedy::AgentCore {\npublic:\nAgent():greedy::AgentCore(std::vector<Life>(lives,lives+sizeof(lives)/sizeof(lives[0])),support,"
        <<(p.recorded_layout?"true":"false")<<','<<(p.recorded_support?"true":"false")<<','<<(p.recorded_service?"true":"false")<<",1,true) {}\n"
        <<"static kag::agent::AgentInfo info(){return {\""<<name<<"\"};}\n};\n}\n";
    std::ofstream cpp(directory/"source/agent.cpp");cpp<<"#include \"agent.hpp\"\n";
    std::ofstream manifest(directory/"agent.json");
    manifest<<"{\"format_version\":1,\"name\":\""<<name<<"\",\"header\":\"source/agent.hpp\",\"type\":\"compositions::"<<name
        <<"::Agent\",\"sources\":[\"source/agent.cpp\",\""<<source_path<<"\"]}\n";
    std::ofstream readme(directory/"README.md");
    readme<<"# "<<name<<"\n\nExperimental composition-search artifact. Proposal "<<p.id<<", parent "<<p.parent<<", source program "<<p.source_program
        <<"; edit: "<<p.edit<<". Source program -1 is a cold rule-built farm; other IDs refer to composition_greedy_v0/PROGRAM_SOURCES.json. "
        <<"Full dated lives and support are in PLAN.json. Compiler and estimates are local implementations.\n\n"
        <<"Eight discovery games versus public_router: utility "<<p.exact_utility<<", mean margin $"<<p.exact_margin
        <<". This is not a promotion result or a claim of superiority over the strong league. Search ancestry and all evaluations are retained in the run directory.\n";
}

void exact_evaluate(Proposal& p,const fs::path& directory,const std::string& prefix) {
    Options options;options.a=prefix+"_p"+std::to_string(p.id);options.b="public_router";
    options.output=(directory/(options.a+".json")).string();options.validate=true;
    greedy::AgentCore agent(p.lives,p.support,p.recorded_layout,p.recorded_support,p.recorded_service,1,true);
    public_router::Agent opponent;std::vector<Outcome> games;
    auto start=std::chrono::steady_clock::now();
    double wins=0,margin=0,cash=0;uint64_t hash=14695981039346656037ULL;
    for(uint64_t seed=1000;seed<1004;++seed)for(int seat=0;seat<2;++seat) {
        auto game=run_game(agent,opponent,seed,seat,options);
        wins+=game.cash[seat]>game.cash[seat^1]?1:game.cash[seat]==game.cash[seat^1]?0.5:0;
        margin+=game.cash[seat]-game.cash[seat^1];cash+=game.cash[seat];
        hash^=game.hash[seat];hash*=1099511628211ULL;
        games.push_back(std::move(game));
    }
    p.exact_utility=wins/8;p.exact_margin=margin/8;p.exact_cash=cash/8;p.phenotype=hash;
    write_results(options,games,std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count());
}

int main(int argc,char** argv) {
    if(argc!=6) {std::fprintf(stderr,"usage: search_compositions SCENARIOS RUN_DIRECTORY GENERATIONS MUTATIONS FINALISTS\n");return 2;}
    auto scenarios=read_scenarios(argv[1]);fs::path directory=argv[2];
    int generations=std::stoi(argv[3]),mutations=std::stoi(argv[4]),finalists=std::stoi(argv[5]);
    if(generations<1 || mutations<1 || finalists<1 || finalists>100)return 2;
    const std::string prefix=directory.filename().string();
    if(prefix.empty() || prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_")!=std::string::npos) {
        std::fprintf(stderr,"run directory basename must use lowercase letters, digits and underscores\n");return 2;
    }
    if(fs::exists(directory) && !fs::is_empty(directory)) {
        std::fprintf(stderr,"refusing to overwrite nonempty run directory: %s\n",directory.c_str());return 2;
    }
    fs::create_directories(directory/"exact");fs::create_directories(directory/"proposals");
    std::vector<Proposal> seeds,archive;
    int next_id=0;
    for(int i=0;i<72;++i) {
        auto source=greedy::recorded_program(i);Proposal p;
        p.id=next_id++;p.source_program=i;p.lives.assign(source.begin(),source.end());p.support=greedy::recorded_support(i);
        seeds.push_back(std::move(p));
    }
    uint64_t random_state=0x6c6f6f705f7630ULL;
    auto random=[&]{return random_word(random_state);};
    std::ofstream log(directory/"iterations.csv");
    log<<"generation,proposal_id,parent_id,source_program,edit,lives,estimate,estimated_cash,estimated_margin,min_cash,work_gap,exact_utility,exact_cash,exact_margin,phenotype\n";
    std::unordered_set<uint64_t> seen;
    int evaluated=0,estimated=0;double estimation_seconds=0;
    for(int generation=0;generation<generations;++generation) {
        std::vector<Proposal> proposals;
        if(generation==0)proposals=seeds;
        for(int i=0;i<mutations;++i) {
            Proposal p;
            if(i%8==0) p=cold_proposal(random);
            else {
                const auto& parents=archive.empty() || i%4==0?seeds:archive;
                p=parents[random()%parents.size()];p.parent=p.id;
                if(!mutate(p,random))continue;
            }
            p.id=next_id++;proposals.push_back(std::move(p));
        }
        auto start=std::chrono::steady_clock::now();
        for(auto& p:proposals) {estimate_proposal(p,scenarios);++estimated;}
        estimation_seconds+=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
        std::stable_sort(proposals.begin(),proposals.end(),[](const Proposal& a,const Proposal& b){return a.estimate>b.estimate;});
        std::vector<size_t> selected;
        for(size_t i=0;i<proposals.size() && selected.size()<size_t(finalists);++i)
            if(!proposals[i].layout_failures)selected.push_back(i);
        // Preserve exact exploration of an independent cold construction even
        // when the uncalibrated labor estimate ranks every cold farm poorly.
        for(size_t i=0;i<proposals.size();++i)if(proposals[i].edit=="cold_rule_build" && !proposals[i].layout_failures) {
            if(std::find(selected.begin(),selected.end(),i)==selected.end())selected.push_back(i);
            break;
        }
        for(size_t i:selected) {
            auto& p=proposals[i];exact_evaluate(p,directory/"exact",prefix);++evaluated;
            // Full typed proposals are retained even when behavior duplicates.
            const std::string name=prefix+"_p"+std::to_string(p.id);
            save_proposal(p,directory/"proposals"/name,name);
            if(seen.insert(p.phenotype).second)archive.push_back(p);
        }
        for(const auto& p:proposals)
            log<<generation<<','<<p.id<<','<<p.parent<<','<<p.source_program<<','<<p.edit<<','<<p.lives.size()<<','<<p.estimate<<','<<p.estimated_cash
                <<','<<p.estimated_margin<<','<<p.min_cash<<','<<p.work_gap<<','<<p.exact_utility<<','<<p.exact_cash<<','<<p.exact_margin<<','<<p.phenotype<<'\n';
        std::sort(archive.begin(),archive.end(),[](const Proposal& a,const Proposal& b){return a.exact_utility!=b.exact_utility?a.exact_utility>b.exact_utility:a.exact_margin>b.exact_margin;});
        if(archive.empty())return 1;
        if(archive.size()>12)archive.resize(12);
        const auto& best=archive.front();
        std::printf("generation=%d best_id=%d source=%d utility=%.4f margin=%.1f archive=%zu\n",generation,best.id,best.source_program,best.exact_utility,best.exact_margin,archive.size());
    }
    const auto& best=archive.front();
    // A directly runnable local package, separate from the strong incumbent.
    const std::string name=prefix+"_p"+std::to_string(best.id);
    save_proposal(best,fs::path("experiments/v6/sep07_compositions_v0/candidates")/name,name);
    std::ofstream summary(directory/"summary.json");
    summary<<"{\"estimated\":"<<estimated<<",\"exact_candidates\":"<<evaluated<<",\"exact_games\":"<<8*evaluated
        <<",\"estimation_seconds\":"<<estimation_seconds<<",\"best_id\":"<<best.id<<",\"best_source_program\":"<<best.source_program
        <<",\"best_utility\":"<<best.exact_utility<<",\"best_margin\":"<<best.exact_margin<<",\"best_cash\":"<<best.exact_cash
        <<",\"scope\":\"Fixed public_router flow forecast; live public_router exact discovery; no promotion\"}\n";
}
