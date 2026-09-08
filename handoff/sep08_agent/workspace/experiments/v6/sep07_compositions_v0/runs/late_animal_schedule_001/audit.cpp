#include "../late_animal_rotation_001/source/season.hpp"
#include "../late_animal_rotation_001/source/sequence.hpp"
#include <map>

std::array<Action,24> read_actions(const fs::path& path) {
    std::ifstream input(path);if(!input)std::abort();std::array<Action,24> actions;
    for(auto& a:actions) {
        input>>a.n_units>>a.n_orders;
        for(int u=0;u<a.n_units;++u){int op,arg;input>>op>>arg>>a.units[u].n;a.units[u].op=op;a.units[u].arg=arg;}
        for(int s=0;s<a.n_orders;++s){int op,item;input>>op>>item>>a.orders[s].n;a.orders[s].op=op;a.orders[s].item=item;}
        a.finalize();
    }
    if(!input)std::abort();return actions;
}

std::vector<GuardedDay> read_course(const fs::path& folder,const std::map<int,fs::path>& changes) {
    std::vector<GuardedDay> result;
    for(int day=13;day<30;++day) {
        const auto path=folder/"days"/std::to_string(day);
        GuardedDay guard;std::ifstream input(path/"guard.txt");if(!input)std::abort();
        input>>guard.plan.day>>guard.quadrants;if(guard.plan.day!=day)std::abort();
        for(int cell=0;cell<100;++cell){int check;input>>check;guard.check[cell]=check;for(int& v:guard.tiles[cell])input>>v;}
        for(int& n:guard.shed)input>>n;for(int& n:guard.seeds)input>>n;if(!input)std::abort();
        auto found=changes.find(day);
        guard.plan.actions=read_actions(found==changes.end()?path/"actions.txt":found->second);
        result.push_back(std::move(guard));
    }
    return result;
}

int main(int argc,char** argv) {
    if(argc!=4 || fs::exists(argv[3]))return 2;
    const fs::path source(argv[1]),out(argv[3]);fs::create_directories(out);
    std::ifstream input(argv[2]);if(!input)return 2;
    std::map<int,fs::path> changes;int day;std::string path;
    while(input>>day>>path){if(day<13||day>29||changes.contains(day))return 2;changes.emplace(day,path);}
    const auto original=read_course(source,{}),candidate=read_course(source,changes);
    Options options;options.a="goose_source";options.b="public_router";options.games=32;options.seed_start=1000;
    options.threads=4;options.profile=true;options.validate=true;
    for(int i=0;i<options.games;++i)options.seeds.push_back(options.seed_start+i);
    options.output=(out/"original.json").string();
    run_batch(options,[&]{return LateSequence<Source>(original);},[]{return public_router::Agent{};});
    options.a="goose_retimed";options.output=(out/"candidate.json").string();
    run_batch(options,[&]{return LateSequence<Source>(candidate);},[]{return public_router::Agent{};});
}
