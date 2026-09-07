#include "../include/crop_service.hpp"
#include <iostream>

using namespace kag;
using namespace compositions;

int main() {
    int cases=0,removed_water=0,removed_fert=0;
    for(int item=0;item<N_CROPS;++item)for(bool fertilize:{false,true})for(bool early:{false,true}) {
        int age=early?earliest_full_crop_age(item,fertilize):crop_last_age(item);
        if(item==MELON && early && age!=10)std::abort();
        for(bool prune:{false,true}) {
            Cohort c{uint8_t(item),1,0,age+1};auto service=productive_service(c,fertilize);
            if(!CROPS[item].ongoing)service.harvest=uint32_t{1}<<age;
            const auto original=service;if(prune)service=prune_crop_service(c,service);
            removed_water+=std::popcount(original.water)-std::popcount(service.water);
            removed_fert+=std::popcount(original.fertilize)-std::popcount(service.fertilize);
            const auto prediction=biology(c,service);
            Config cfg;cfg.weed_chance=0;Sim sim(cfg);auto& farm=sim.st.farms[0];farm.seeds[item]=1;
            int daily_work=0,total=0;
            while(!sim.st.done) {
                const int day=sim.st.day,hour=sim.st.hour;
                if(hour==0)farm.inv_add(0,FERTILIZER,100-farm.inv[0][FERTILIZER]);
                Action a,b;a.clear();b.clear();
                if(day==0 && hour==0)a.units[0]={OP_PLANT,uint8_t(item),1};
                else if(day<=age && hour==1 && on(service.fertilize,day))a.units[0]={OP_FERTILIZE,0,1};
                else if(day<=age && hour==2 && on(service.water,day))a.units[0]={OP_WATER,0,1};
                else if(day<=age && hour==3 && on(service.harvest,day))a.units[0]={OP_HARVEST,0,1};
                a.finalize();b.finalize();
                daily_work+=sim.diagnose_joint_actions(a,b).players[0].successful_unit_actions;
                sim.step(a,b);
                if(hour==23 || (day==29 && hour==22)) {
                    total+=prediction.days[day].output[item];
                    if(farm.produced[item]!=total || daily_work!=prediction.days[day].operations) {
                        std::cerr<<"mismatch item="<<item<<" fertilize="<<fertilize<<" early="<<early<<" prune="<<prune<<" day="<<day<<" work="<<daily_work<<" expected="<<prediction.days[day].operations<<'\n';return 1;
                    }
                    daily_work=0;
                }
            }
            ++cases;
        }
    }
    std::cout<<"exact_cases="<<cases<<" deleted_water="<<removed_water<<" deleted_fertilize="<<removed_fert<<'\n';
}
