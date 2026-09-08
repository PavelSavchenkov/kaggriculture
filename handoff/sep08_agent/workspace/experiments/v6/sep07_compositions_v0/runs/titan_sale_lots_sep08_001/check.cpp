#include "lots.hpp"
#include <fstream>
#include <iostream>
using namespace compositions::titan_lots;

int main(int argc,char** argv) {
    if(argc!=3)return 2;std::ifstream input(argv[1]);std::ofstream output(argv[2]);
    int cases=0;input>>cases;
    for(int k=0;k<cases;++k) {
        Context c;int capacity_limit=0;
        input>>c.item>>c.quantity>>c.inventory>>c.now>>c.last>>c.rival_quantity>>c.minimum_now>>c.n_dates;
        for(int i=0;i<c.n_dates;++i)input>>c.dates[i];
        input>>c.n_shops;for(int i=0;i<c.n_shops;++i)input>>c.shops[i];
        input>>c.reference.count;for(int i=0;i<c.reference.count;++i)input>>c.reference.sales[i].step>>c.reference.sales[i].quantity;
        input>>capacity_limit;if(!input)return 2;
        auto result=optimize(c,[&](const Plan& p){return p.at(c.now)>=capacity_limit;});
        output<<result.worst_gain<<' '<<result.sum_gain<<' '<<result.plans<<' '<<result.plan.count;
        for(int i=0;i<result.plan.count;++i)output<<' '<<result.plan.sales[i].step<<' '<<result.plan.sales[i].quantity;
        output<<' '<<result.n_scenarios;
        for(int s=0;s<result.n_scenarios;++s)output<<' '<<result.scores[s].relative<<' '<<result.scores[s].own<<' '<<result.scores[s].rival<<' '<<result.scores[s].carry;
        output<<'\n';
    }
}
