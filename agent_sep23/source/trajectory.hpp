#pragma once
#include "features.hpp"
#include <cstdlib>

namespace bc {
// Only observed dawn summaries. No projected plans or teacher action labels.
struct Trajectory {
    std::array<std::array<float,49>,30> dawns{};
    std::array<bool,30> present{};
    static std::array<float,49> summary(const Features& f) {
        std::array<float,49> result{};
        for(int side=0;side<2;++side) {
            const float x=f.global[4+side];
            result[side]=std::copysign(std::expm1(std::abs(x)*10.f),x)/10000.f;
            result[2+side]=f.global[6+side];
        }
        int index=4;
        for(int p=0;p<9;++p)for(int column:{0,6,7,15,16})result[index++]=f.products[p][column];
        return result;
    }
    void record(const Features& f) {
        if(f.day<0||f.day>=30)std::abort();
        dawns[f.day]=summary(f);present[f.day]=true;
    }
    void observe(const dc::Observation& o,const dc::Configuration& config) {
        if(o.hour==0)record(encode(o,{},dc::IntentSchema{},config));
    }
    void fill(Features& f) const {
        if(f.day<0||f.day>=30)std::abort();
        const auto current=summary(f);std::fill_n(f.trajectory,150,0.f);
        for(int day=0;day<30;++day) {
            std::fill_n(f.memory[day],50,0.f);
            if(day==f.day||(day<f.day&&present[day])) {
                f.memory[day][0]=1;
                const auto& observed=day==f.day?current:dawns[day];
                std::copy_n(observed.data(),49,f.memory[day]+1);
            }
        }
        f.memory_valid=true;
        int index=0;
        for(int horizon:{1,3,7}) {
            const int day=f.day-horizon;
            if(day>=0&&day<30&&present[day]) {
                f.trajectory[index]=1;
                for(int k=0;k<49;++k)f.trajectory[index+1+k]=current[k]-dawns[day][k];
            }
            index+=50;
        }
        f.trajectory_valid=true;
    }
};
}
