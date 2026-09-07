#include "../include/biology.hpp"
#include <iostream>

using namespace kag;
using namespace compositions;

bool verify(int item, int mode) {
    Cohort cohort{uint8_t(item),1,0,30};
    auto service = productive_service(cohort, mode&1);
    if (mode&2) { service.care &= ~0x15555555u; service.water &= ~0x15555554u; }
    if (mode&4) { service.feed &= ~0x7000u; service.water &= ~0x7000u; }
    if (mode&8) service.harvest &= ~0x11111111u;
    const auto prediction = biology(cohort,service);
    Config cfg; cfg.weed_chance=0;
    Sim sim(cfg);
    auto& f = sim.st.farms[0];
    if (is_crop(item)) f.seeds[item]=1;
    else f.inv_add(0,item,1);
    f.inv_add(0,WHEAT,100); f.inv_add(0,FERTILIZER,100);
    int predicted[N_PRODUCTS]{};
    int daily_work=0, daily_wheat=0, daily_fertilizer=0;
    while (!sim.st.done) {
        const int day=sim.st.day, hour=sim.st.hour;
        if (hour==0) {
            // Isolate biology: make every assumed input available. Logistics
            // and the end-of-day shed transfer are outside this test's scope.
            f.inv_add(0,WHEAT,100-f.inv[0][WHEAT]);
            f.inv_add(0,FERTILIZER,100-f.inv[0][FERTILIZER]);
        }
        Action a,b; a.clear(); b.clear();
        UnitAction op;
        if (day==0 && hour==0) op={uint8_t(is_crop(item) ? OP_PLANT : item==GOOSE ? OP_BUILD_COOP : OP_BUILD_PASTURE),uint8_t(item),1};
        else if (day==0 && hour==1 && is_animal(item)) op={OP_PLACE,uint8_t(item),1};
        else if (hour==1 && is_crop(item) && on(service.fertilize,day)) op={OP_FERTILIZE,0,1};
        else if (hour==2 && is_crop(item) && on(service.water,day)) op={OP_WATER,0,1};
        else if (hour==2 && is_animal(item) && on(service.feed,day)) op={OP_FEED,0,1};
        else if (hour==3 && is_animal(item) && on(service.care,day)) op={OP_CARE,0,1};
        else if (hour==4 && is_animal(item) && on(service.collect_fertilizer,day)) op={OP_COLLECT_FERTILIZER,0,1};
        else if (hour==5 && on(service.harvest,day)) op={OP_HARVEST,0,1};
        a.units[0]=op; a.finalize(); b.finalize();
        const auto diagnostic=sim.diagnose_joint_actions(a,b);
        const int success=diagnostic.players[0].successful_unit_actions;
        daily_work += success;
        if (op.op==OP_FEED) daily_wheat+=success;
        if (op.op==OP_FERTILIZE) daily_fertilizer+=success;
        sim.step(a,b);
        if (hour==23 || (day==29 && hour==22)) {
            const auto& expected=prediction.days[day];
            if (daily_work!=expected.operations || daily_wheat!=expected.wheat || daily_fertilizer!=expected.fertilizer) {
                std::cerr << "service mismatch item=" << item << " mode=" << mode << " day=" << day
                          << " exact=" << daily_work << ',' << daily_wheat << ',' << daily_fertilizer
                          << " estimated=" << expected.operations << ',' << expected.wheat << ',' << expected.fertilizer << '\n';
                return false;
            }
            daily_work=daily_wheat=daily_fertilizer=0;
            for (int i=0; i<N_PRODUCTS; ++i) {
                predicted[i] += prediction.days[day].output[i];
                if (f.produced[i] != predicted[i]) {
                    std::cerr << "item=" << item << " mode=" << mode << " day=" << day
                              << " output=" << i << " exact=" << f.produced[i] << " estimated=" << predicted[i] << '\n';
                    return false;
                }
            }
        }
    }
    return true;
}

int main() {
    int cases=0;
    for (int item=0; item<N_ITEMS; ++item) if (is_crop(item) || is_animal(item))
        for (int mode=0; mode<16; ++mode) { if (!verify(item,mode)) return 1; ++cases; }
    std::cout << "biology_exact_cases=" << cases << " daily_output_checks=" << cases*30*N_PRODUCTS << '\n';
}
