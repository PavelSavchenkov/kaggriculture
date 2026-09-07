#include "../include/ticket_estimate.hpp"
#include "../include/terminal_layer.hpp"
#include "../league/public_router/source/agent.hpp"
#include "../candidates/opening_router_v4/source/agent.hpp"
#include "../candidates/justin_recall_v0/source/agent.hpp"
#include "../candidates/justin_guarded_hires_001_best/source/agent.hpp"
#include "../league/king_rc4/source/agent.hpp"
#include <filesystem>
#include <iostream>
#include <numeric>

using namespace compositions;
namespace fs=std::filesystem;

struct Choice {
    int ticket,item;double estimate=0,margin=0,shadow=0,minimum=0;
    int unmatched=0,unsold=0,matched=0,output[3]{};
};

void validate_trace(const TicketTrace& trace) {
    const auto market=value_market(trace.market,trace.shops);const int p=trace.seat;
    if(market.cash[p]!=trace.cash || market.cash[p^1]!=trace.rival_cash)std::abort();
    std::array<int,kag::N_PRODUCTS> produced{};
    for(const auto& ticket:trace.tickets)if(ticket.start_day>=0) {
        const auto bio=ticket_biology(ticket,ticket.edit.original,false);
        for(const auto& day:bio.days)for(int item=kag::EGG;item<=kag::FERTILIZER;++item)produced[item]+=day.output[item];
    }
    for(int item=kag::EGG;item<=kag::FERTILIZER;++item)if(produced[item]!=trace.produced[item])std::abort();
}

bool same_ticket(const AnimalEdit& a,const AnimalEdit& b) {
    if(a.original!=b.original || a.cell!=b.cell)return false;
    const ActionAddress aa[]={a.purchase,a.pickup,a.placement},bb[]={b.purchase,b.pickup,b.placement};
    for(int i=0;i<3;++i)if(aa[i].step!=bb[i].step || aa[i].index!=bb[i].index)return false;
    return true;
}

void emit_package(const fs::path& directory,int program,const Choice& choice,const AnimalTicket& ticket,bool terminal) {
    const std::string name=std::string(terminal?"ticket_terminal_p":"ticket_p")+std::to_string(program)+"_t"+std::to_string(choice.ticket)+"_i"+std::to_string(choice.item);
    const auto path=directory/name;fs::create_directories(path/"source");
    const auto include=fs::relative(fs::absolute("experiments/v6/sep07_compositions_v0/include/animal_ticket.hpp"),fs::absolute(path/"source"));
    std::ofstream header(path/"source/agent.hpp");
    header<<"#pragma once\n#include \""<<include.string()<<"\"\n";
    if(terminal)header<<"#include \""<<fs::relative(fs::absolute("experiments/v6/sep07_compositions_v0/include/terminal_layer.hpp"),fs::absolute(path/"source")).string()<<"\"\n";
    header<<"namespace compositions::"<<name<<" {\n";
    if(terminal)header<<"using Source=TerminalAgent<top_replay_library::Agent,3>;using Parent=AnimalTicketOverlay<Source>;\n"
        <<"class Agent:public Parent {public:Agent():Parent(Source(top_replay_library::Agent("<<program<<")),AnimalEdit{";
    else header<<"class Agent:public AnimalTicketAgent {public:Agent():AnimalTicketAgent("<<program<<",AnimalEdit{";
    header<<ticket.edit.original<<','<<choice.item;
    for(const auto address:{ticket.edit.purchase,ticket.edit.pickup,ticket.edit.placement,ticket.edit.structure})
        header<<",{"<<address.step<<','<<address.index<<'}';
    header<<','<<ticket.edit.cell<<"},true,true){}\nstatic kag::agent::AgentInfo info(){return {\""<<name<<"\"};}};\n}\n";
    std::ofstream(path/"source/agent.cpp")<<"#include \"agent.hpp\"\n";
    const auto source=fs::relative(fs::absolute("experiments/v6/sep07_compositions_v0/league/top_replay_library/source/agent.cpp"),fs::absolute(path));
    std::ofstream(path/"agent.json")<<"{\"format_version\":1,\"name\":\""<<name<<"\",\"header\":\"source/agent.hpp\",\"type\":\"compositions::"<<name<<"::Agent\",\"sources\":[\"source/agent.cpp\",\""<<source.string()<<"\"]}\n";
    std::ofstream(path/"README.md")<<"# "<<name<<"\n\nExperimental single-animal composition substitution. Purchase, pickup and placement\nare linked by exact offline animal-stock tracing. Source program "<<program<<"; exact\nreplay identity/hash is in league/top_replay_library/IMPORT.json. Uses the local\nanimal_ticket.hpp output-deposit/sale alternative and the old service schedule.\nNo feasibility certificate or promotion; all local test results remain in this run.\n";
    std::ofstream(path/"IMPORT.json")<<"{\"source_program\":"<<program<<",\"source_metadata\":\"league/top_replay_library/IMPORT.json\",\"ticket\":"<<choice.ticket<<",\"original\":"<<ticket.edit.original<<",\"replacement\":"<<choice.item<<",\"estimated_cash_delta\":"<<choice.estimate<<",\"compiler\":\"Local exact animal identity trace plus optional output repair; old dated worker service reused\",\"status\":\"experimental, complete deployment checks pending\"}\n";
    if(terminal) {
        std::ofstream(path/"PARENT.json")<<"{\"program\":"<<program<<",\"terminal_mode\":3,\"terminal_lineage\":\"candidates/justin_recall_v0/IMPORT.json\",\"ordering\":\"Terminal parent before animal overlay. Recompile affected workforce days after any composition change; no guarded day plans silently reused.\"}\n";
        std::ofstream(path/"README.md",std::ios::app)<<"\nIncludes the attributed mode3 terminal parent; see PARENT.json.\n";
    }
}

template<class SourceFactory,class Factory>
int search_from(int program,const fs::path& directory,Options config,int baseline_games,SourceFactory source_factory,Factory factory,bool terminal) {
    const auto trace_start=std::chrono::steady_clock::now();
    std::vector<TicketTrace> traces;traces.reserve(2*baseline_games);
    std::ofstream cases(directory/"baseline_cases.csv");cases<<"seed,seat,trace_failure,cash,rival_cash\n";
    for(int game=0;game<baseline_games;++game)for(int seat=0;seat<2;++seat) {
        auto source=source_factory();auto rival=factory();auto trace=trace_tickets_from(source,rival,1000+game,seat);
        cases<<1000+game<<','<<seat<<','<<trace.failure<<','<<trace.cash<<','<<trace.rival_cash<<'\n';cases.flush();
        if(!trace.failure.empty()) {
            if(game==0 && seat==0){std::cerr<<trace.failure<<'\n';return 3;}
            continue;
        }
        validate_trace(trace);traces.push_back(std::move(trace));
    }
    cases.close();
    const auto& trace=traces.front();
    const double trace_seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-trace_start).count();
    const auto estimate_start=std::chrono::steady_clock::now();
    std::ofstream tickets(directory/"tickets.csv");
    tickets<<"id,item,purchased,picked,purchase_step,purchase_order,pickup_step,pickup_worker,place_step,place_worker,cell,start,end,eligible,feed,care,collect,harvest,harvest_requests\n";
    std::array<int,kag::N_PRODUCTS> produced{};
    std::vector<Choice> choices;
    for(int id=0;id<int(trace.tickets.size());++id) {
        const auto& t=trace.tickets[id];
        tickets<<id<<','<<t.edit.original<<','<<t.purchased<<','<<t.picked;
        for(const auto address:{t.edit.purchase,t.edit.pickup,t.edit.placement})tickets<<','<<address.step<<','<<address.index;
        tickets<<','<<t.edit.cell<<','<<t.start_day<<','<<t.end_day<<','<<t.eligible<<','<<t.feed<<','<<t.care<<','<<t.collect<<','<<t.harvest<<','<<t.harvest_requested<<'\n';
        if(t.start_day<0)continue;
        const auto original=ticket_biology(t,t.edit.original,false);
        for(const auto& day:original.days)for(int item=0;item<kag::N_PRODUCTS;++item)produced[item]+=day.output[item];
        if(!t.eligible)continue;
        for(int item:{int(kag::GOOSE),int(kag::COW),int(kag::SHEEP)})if(item!=t.edit.original) {
            const bool structure_change=(item==kag::GOOSE)!=(t.edit.original==kag::GOOSE);
            if(structure_change) {
                if(t.edit.structure.step<0)continue;
                int occupants=0;for(const auto& other:trace.tickets)occupants+=other.edit.cell==t.edit.cell;
                if(occupants!=1)continue;
            }
            const auto changed=ticket_biology(t,item,true);
            Choice choice{id,item};choice.shadow=original.animal_cost-changed.animal_cost;
            for(int day=0;day<30;++day)for(int product=kag::EGG;product<=kag::WOOL;++product) {
                const int delta=changed.days[day].output[product]-original.days[day].output[product];
                // Baseline observed end-of-day quotes are a heuristic shadow
                // value. Routes, sale times, own impact and rival response are
                // not certified by this first marginal estimator.
                choice.shadow+=delta*trace.prices[std::min(718,day*24+22)][product];
                choice.output[product-kag::EGG]+=delta;
            }
            choice.minimum=1e100;
            for(const auto& scenario:traces)for(const auto& other:scenario.tickets)
                if(other.eligible && same_ticket(t.edit,other.edit)) {
                    if(structure_change) {
                        int occupants=0;for(const auto& life:scenario.tickets)occupants+=life.edit.cell==other.edit.cell;
                        if(occupants!=1)continue;
                    }
                    const auto forecast=estimate_ticket(scenario,other,item);const int p=scenario.seat;
                    const double delta=forecast.value.cash[p]-scenario.cash;
                    choice.estimate+=delta;choice.margin+=delta-forecast.value.cash[p^1]+scenario.rival_cash;
                    choice.minimum=std::min(choice.minimum,forecast.value.minimum[p]);
                    choice.unmatched+=forecast.removed_unmatched;choice.unsold+=forecast.added_unsold;++choice.matched;
                    break;
                }
            if(choice.matched<1)std::abort();
            choice.estimate/=choice.matched;choice.margin/=choice.matched;
            choices.push_back(choice);
        }
    }
    tickets.close();
    bool exact=true;
    std::ofstream biological(directory/"biology.csv");biological<<"product,estimated,realized\n";
    for(int item=kag::EGG;item<=kag::FERTILIZER;++item) {
        biological<<item<<','<<produced[item]<<','<<trace.produced[item]<<'\n';exact&=produced[item]==trace.produced[item];
    }
    biological.close();
    if(!exact){std::cerr<<"baseline animal biology mismatch\n";return 4;}
    const double estimate_us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-estimate_start).count();
    std::ofstream(directory/"timing.json")<<"{\"trace_seconds\":"<<trace_seconds<<",\"estimate_batch_us\":"<<estimate_us<<",\"choices\":"<<choices.size()<<",\"requested_baselines\":"<<2*baseline_games<<",\"valid_baselines\":"<<traces.size()<<",\"baseline_market_and_biology_exact\":true}\n";
    std::stable_sort(choices.begin(),choices.end(),[](const Choice& a,const Choice& b){return a.margin>b.margin;});
    std::ofstream estimates(directory/"estimates.csv");estimates<<"rank,ticket,original,replacement,estimated_cash_delta,estimated_margin_delta,fixed_quote_cash_delta,min_cash,unmatched_removed,unsold_added,matched_baselines,available_baselines,egg_delta,milk_delta,wool_delta\n";
    for(int i=0;i<int(choices.size());++i) {
        const auto& c=choices[i];estimates<<i<<','<<c.ticket<<','<<trace.tickets[c.ticket].edit.original<<','<<c.item<<','<<c.estimate<<','<<c.margin<<','<<c.shadow<<','<<c.minimum<<','<<c.unmatched<<','<<c.unsold<<','<<c.matched<<','<<traces.size();
        for(int value:c.output)estimates<<','<<value;estimates<<'\n';
    }
    estimates.close();
    config.a="source_"+std::to_string(program);config.output=(directory/"source.json").string();config.validate=true;
    run_batch(config,source_factory,factory);
    int equal=0;
    if(choices.empty())return 0;
    const auto identity=trace.tickets[choices.front().ticket].edit;
    for(int seat=0;seat<2;++seat)for(uint64_t seed=1000;seed<1008;++seed) {
        auto source=source_factory();AnimalTicketOverlay same(source_factory(),identity,false,false);auto other=factory();
        const auto a=run_game(source,other,seed,seat,config),b=run_game(same,other,seed,seat,config);
        for(int p=0;p<2;++p)if(a.cash[p]!=b.cash[p] || a.hash[p]!=b.hash[p])std::abort();++equal;
    }
    std::ofstream(directory/"parity.json")<<"{\"identity_source_equal_games\":"<<equal<<"}\n";
    for(int rank=0;rank<int(choices.size());++rank) {
        // Eight highest estimates plus two low-ranked calibration alternatives.
        if(rank>=8 && rank<int(choices.size())-2)continue;
        const auto& choice=choices[rank];auto edit=trace.tickets[choice.ticket].edit;edit.replacement=choice.item;
        emit_package(directory/"proposals",program,choice,trace.tickets[choice.ticket],terminal);
        for(bool change:{false,true}) {
            config.a="ticket_"+std::to_string(choice.ticket)+"_to_"+std::to_string(choice.item)+(change?"_changed":"_output_control");
            config.output=(directory/(config.a+".json")).string();
            run_batch(config,[=]{return AnimalTicketOverlay(source_factory(),edit,change,true);},factory);
        }
    }
    std::cout<<"tickets="<<trace.tickets.size()<<" choices="<<choices.size()<<" baseline_biology_exact=1\n";
    return 0;
}

template<class Factory>
int search(int program,const fs::path& directory,Options config,int baseline_games,Factory factory,bool terminal) {
    if(terminal)return search_from(program,directory,config,baseline_games,
        [=]{return TerminalAgent<top_replay_library::Agent,3>(top_replay_library::Agent(program));},factory,true);
    return search_from(program,directory,config,baseline_games,[=]{return top_replay_library::Agent(program);},factory,false);
}

int main(int argc,char** argv) {
    if(argc<3){std::cerr<<"usage: search_animal_tickets PROGRAM DIRECTORY [arena options]\n";return 2;}
    const int program=std::stoi(argv[1]);const fs::path directory=argv[2];
    if(fs::exists(directory)&&!fs::is_empty(directory))return 2;fs::create_directories(directory);
    int baseline_games=8;bool terminal=false;std::vector<char*> arguments{argv[2]};
    for(int i=3;i<argc;++i) {
        if(std::string(argv[i])=="--baseline-games") {if(++i>=argc)return 2;baseline_games=std::stoi(argv[i]);}
        else if(std::string(argv[i])=="--terminal")terminal=true;
        else arguments.push_back(argv[i]);
    }
    if(baseline_games<1 || baseline_games>128)return 2;
    auto config=options(arguments.size(),arguments.data());
    if(config.b=="public_router")return search(program,directory,config,baseline_games,[]{return public_router::Agent{};},terminal);
    if(config.b=="opening_router_v4")return search(program,directory,config,baseline_games,[]{return opening_router_v4::Agent{};},terminal);
    if(config.b=="junghoon_78")return search(program,directory,config,baseline_games,[]{return top_replay_library::Agent(78);},terminal);
    if(config.b=="binghua_116")return search(program,directory,config,baseline_games,[]{return top_replay_library::Agent(116);},terminal);
    if(config.b=="justin_150")return search(program,directory,config,baseline_games,[]{return top_replay_library::Agent(150);},terminal);
    if(config.b=="justin_recall_v0")return search(program,directory,config,baseline_games,[]{return justin_recall_v0::Agent{};},terminal);
    if(config.b=="justin_guarded_hires_001_best")return search(program,directory,config,baseline_games,[]{return justin_guarded_hires_001_best::Agent{};},terminal);
    if(config.b=="king_rc4")return search(program,directory,config,baseline_games,[]{return king_rc4::Agent{};},terminal);
    std::cerr<<"unsupported opponent\n";return 2;
}
