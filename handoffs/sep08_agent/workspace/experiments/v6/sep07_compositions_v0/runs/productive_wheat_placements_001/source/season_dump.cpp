#include "season.hpp"
int main(int argc,char**argv){
    if(argc!=3)return 2;Season season(std::stoull(argv[1]));std::ofstream out(argv[2]);
    out<<"cell,crop,plant,end,water,fertilize,harvest,output,exact\n";
    for(const auto&life:season.lives){int output=0;for(int q:life.harvested)output+=q;
        out<<life.cell<<','<<int(life.cohort.item)<<','<<life.cohort.start_day<<','<<life.cohort.end_day<<','<<life.service.water<<','<<life.service.fertilize<<','<<life.service.harvest<<','<<output<<','<<life.exact<<'\n';}
}
