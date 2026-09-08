#pragma once
#include <algorithm>
#include "../../include/composition.hpp"
namespace compositions::compiler_labor_data {
inline std::vector<Life> cold_farm(int cows,int sheep,int geese,int wheat,int melon,int strawberry) {
    std::vector<Life> lives;
    auto add=[&](int item,int count,int day,int end) {
        for(int i=0;i<count;++i)lives.push_back({item,day*24,std::min(719,end*24),0,0});
    };
    add(kag::COW,cows,0,30);add(kag::SHEEP,sheep,0,30);add(kag::GOOSE,geese,3,30);
    add(kag::MELON,melon,0,13);
    add(kag::STRAWBERRY,strawberry,5,22);
    for(int day=0;day<=24;day+=5)add(kag::WHEAT,wheat,day,day+5);
    std::stable_sort(lives.begin(),lives.end(),[](const Life& a,const Life& b){return a.start<b.start;});
    return lives;
}

}
