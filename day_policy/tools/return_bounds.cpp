#include "case.hpp"
#include <cstring>
#include <fstream>
#include <stdexcept>
using namespace kag;
using namespace kag::agents::day_policy_contract;

// Optimistic field-return diagnostic, not a feasibility proof for the whole API.
// Each producer gets its own ideal worker; service and cargo conflicts are free.
int main(int argc,char** argv) {
    if(argc!=3)throw std::runtime_error("return_bounds cases.bin output.csv");
    std::ifstream file(argv[1],std::ios::binary);std::ofstream out(argv[2]);
    out<<"game,seat,rank,day,hour,product,required,optimistic_field_returns\n";
    contract_benchmark::Case c;
    while(file.read(reinterpret_cast<char*>(&c),sizeof(c))) {
        if(std::strcmp(c.reason,"eligible"))continue;
        int available[24][N_PRODUCTS]{};
        for(int tile=0;tile<100;++tile) {
            const auto e=c.input.events[tile];const auto& t=c.input.grid[tile];
            if(!(e&(Harvest|CollectFertilizer)))continue;
            const int d=shed_distance(tile);
            const int earliest=std::min(distance(44,tile),1+d)+d+1;
            Job job;job.tile=tile;
            auto add=[&](int op){job.steps[job.count++]={uint8_t(op),0,1};};
            if(e&Fertilize)add(OP_FERTILIZE);
            if(e&Water)add(OP_WATER);
            if(e&Harvest)add(OP_HARVEST);
            if(e&CollectFertilizer)add(OP_COLLECT_FERTILIZER);
            for(int it=0;it<N_PRODUCTS;++it) {
                const int quantity=harvest_output(job,0,t,0,it);
                for(int h=earliest;h<24;++h)available[h][it]+=quantity;
            }
        }
        int first=-1,product=-1;
        // Wheat can also come from scheduled shed purchases; the field-only
        // bound is not a valid impossibility certificate for that product.
        for(int h=0;h<24 && first<0;++h)for(int it=1;it<N_PRODUCTS;++it)
            if(available[h][it]<c.input.returns[h][it]){first=h;product=it;break;}
        out<<c.game<<','<<c.seat<<','<<c.rank<<','<<c.day<<','<<first<<','<<product<<','
           <<(first<0?0:c.input.returns[first][product])<<','<<(first<0?0:available[first][product])<<'\n';
    }
}
