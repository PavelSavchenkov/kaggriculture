#include "fast_game_engine/sim.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

struct Case {
    int opponent=0,seat=0,tomato=0,wheat=0;
    uint64_t seed=0;
    long long own=0,rival=0,changed=0,changed_rival=0;
};
double utility(long long margin){return margin>0?1:margin==0?.5:0;}

int main(int argc,char** argv) {
    if(argc!=3)return 2;
    std::ifstream input(argv[1]);if(!input)return 2;
    int count=0;input>>count;if(count<1 || count>32)return 2;
    std::vector<std::string> names(count);for(auto& name:names)input>>name;
    std::vector<Case> cases;
    while(true) {
        Case c;if(!(input>>c.opponent))break;
        input>>c.seed>>c.seat;
        for(int i=0,shop;i<4;++i) {
            input>>shop;if(shop<0 || shop>=kag::N_SHOPS)return 2;
            c.tomato+=bool(kag::SHOP_MASK[shop]&(1u<<kag::TOMATO));
            c.wheat+=bool(kag::SHOP_MASK[shop]&(1u<<kag::WHEAT));
        }
        input>>c.own>>c.rival>>c.changed>>c.changed_rival;
        if(!input || c.opponent<0 || c.opponent>=count)return 2;
        cases.push_back(c);
    }
    if(cases.empty())return 2;
    struct Result {
        int tomatoes=0,wheat=0,selected=0;
        double utility_gain=0,margin_gain=0,worst_utility=0,worst_margin=0;
        std::vector<double> utilities,margins;
    };
    std::vector<Result> rows;
    for(int tomatoes=0;tomatoes<=4;++tomatoes)for(int wheat=0;wheat<=4;++wheat) {
        Result r;r.tomatoes=tomatoes;r.wheat=wheat;
        r.utilities.resize(count);r.margins.resize(count);std::vector<int> n(count);
        for(const auto& c:cases) {
            const bool choose=c.tomato>=tomatoes && c.wheat<=wheat;
            const long long before=c.own-c.rival,after=choose?c.changed-c.changed_rival:before;
            ++n[c.opponent];r.selected+=choose;
            r.utilities[c.opponent]+=utility(after)-utility(before);
            r.margins[c.opponent]+=after-before;
        }
        for(int i=0;i<count;++i) {
            if(!n[i])return 2;
            r.utilities[i]/=n[i];r.margins[i]/=n[i];
            r.utility_gain+=r.utilities[i]/count;r.margin_gain+=r.margins[i]/count;
        }
        r.worst_utility=*std::min_element(r.utilities.begin(),r.utilities.end());
        r.worst_margin=*std::min_element(r.margins.begin(),r.margins.end());rows.push_back(r);
    }
    std::stable_sort(rows.begin(),rows.end(),[](const auto& a,const auto& b) {
        if(a.utility_gain!=b.utility_gain)return a.utility_gain>b.utility_gain;
        return a.margin_gain>b.margin_gain;
    });
    std::ofstream out(argv[2]);out<<std::setprecision(12);
    out<<"minimum_tomato_shops,maximum_wheat_shops,selected_games,utility_gain,margin_gain,worst_opponent_utility_gain,worst_opponent_margin_gain";
    for(const auto& name:names)out<<','<<name<<"_utility_gain,"<<name<<"_margin_gain";out<<'\n';
    for(const auto& r:rows) {
        out<<r.tomatoes<<','<<r.wheat<<','<<r.selected<<','<<r.utility_gain<<','<<r.margin_gain<<','<<r.worst_utility<<','<<r.worst_margin;
        for(int i=0;i<count;++i)out<<','<<r.utilities[i]<<','<<r.margins[i];out<<'\n';
    }
    std::cout<<"cases="<<cases.size()<<" gates="<<rows.size()<<" best_min_tomatoes="<<rows[0].tomatoes<<" best_max_wheat="<<rows[0].wheat<<" discovery_utility_gain="<<rows[0].utility_gain<<" discovery_margin_gain="<<rows[0].margin_gain<<'\n';
}
