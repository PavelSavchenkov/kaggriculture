#include "agent.cpp"
#include <cstdio>
int main(){
    int comparisons=0;using namespace compositions::atakan_integrated;
    for(int revealed=0;revealed<=8;++revealed)for(int hour:{0,10,23}){
        kag::agent::AgentObservation o;o.day=std::min(29,3*revealed);o.hour=hour;o.step=o.day*24+hour;o.n_shops=revealed;
        for(int i=0;i<revealed;++i)o.shops[i]=(i*3+1)%8;
        const auto expected=mean_demand(o,1.);
        for(int count:{8,32,64}){Days total{};for(int sample=0;sample<count;++sample){const auto demand=sample_demand(o,sample);for(int day=o.day;day<30;++day)for(int p=0;p<9;++p)total[day][p]+=demand[day][p];}
            for(int day=o.day;day<30;++day)for(int p=0;p<9;++p){if(std::abs(total[day][p]/count-expected[day][p])>1e-10)std::abort();++comparisons;}
        }
    }
    std::printf("{\"stratified_mean_demand_equalities\":%d}\n",comparisons);
}
