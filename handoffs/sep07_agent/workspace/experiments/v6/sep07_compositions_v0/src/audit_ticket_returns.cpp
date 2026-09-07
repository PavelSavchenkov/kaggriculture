#include "../include/ticket_trace.hpp"
#include "../candidates/justin_recall_v0/source/agent.hpp"
#include "../candidates/shop_herd_guarded_001_best/source/agent.hpp"
#include "../runs/animal_investments_001/proposals/animal_adaptive_r1_c0_b0/source/agent.hpp"
#include "../league/public_router/source/agent.hpp"
#include <filesystem>
#include <iostream>

using namespace compositions;
template<class Source> void check(const char* name,const std::filesystem::path& folder) {
    Source source;public_router::Agent rival;std::ofstream out(folder/(std::string(name)+".csv"));
    out<<"seed,seat,failure,cash,rival_cash,biology_equal,tickets\n";int passed=0;
    for(int seed=1000;seed<1008;++seed)for(int seat=0;seat<2;++seat) {
        const auto trace=trace_tickets_from(source,rival,seed,seat);std::array<int,kag::N_PRODUCTS> produced{};
        for(const auto& t:trace.tickets)if(t.start_day>=0) {
            const auto life=biology({uint8_t(t.edit.original),1,t.start_day,t.end_day},Service{0,t.feed,t.care,t.collect,t.harvest_requested,0});
            for(const auto& day:life.days)for(int p=kag::EGG;p<=kag::WOOL;++p)produced[p]+=day.output[p];
        }
        bool equal=trace.failure.empty();for(int p=kag::EGG;p<=kag::WOOL;++p)equal&=produced[p]==trace.produced[p];passed+=equal;
        out<<seed<<','<<seat<<','<<trace.failure<<','<<trace.cash<<','<<trace.rival_cash<<','<<equal<<',';
        for(const auto& t:trace.tickets)out<<t.edit.original<<':'<<t.edit.purchase.step<<':'<<t.edit.purchase.index<<':'<<t.edit.pickup.step<<':'<<t.edit.pickup.index<<':'<<t.edit.placement.step<<':'<<t.edit.placement.index<<':'<<t.edit.cell<<':'<<t.start_day<<':'<<t.end_day<<':'<<t.feed<<':'<<t.care<<':'<<t.collect<<':'<<t.harvest<<':'<<t.harvest_requested<<':'<<t.eligible<<':'<<t.returned<<';';out<<'\n';
    }
    std::cout<<name<<' '<<passed<<"/16 complete biology-matching traces\n";
}
int main(int argc,char** argv) {
    if(argc!=2 || std::filesystem::exists(argv[1]))return 2;std::filesystem::create_directories(argv[1]);
    check<justin_recall_v0::Agent>("terminal",argv[1]);check<shop_herd_guarded_001_best::Agent>("shop_guarded",argv[1]);check<animal_adaptive_r1_c0_b0::Agent>("animal_adaptive",argv[1]);
}
