#include "../../include/biology.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>

using namespace compositions;

int main(int argc,char** argv) {
    if(argc!=3)return 2;
    std::ifstream in(argv[1]);std::ofstream out(argv[2]);
    int cases;in>>cases;out<<std::setprecision(12)<<'[';
    for(int c=0;c<cases;++c) {
        std::string name;int planned,actual;in>>name>>planned>>actual;
        int totals[3][kag::N_PRODUCTS]{};
        for(int i=0;i<planned;++i) {
            int item,born,end;in>>item>>born>>end;
            Cohort cohort{uint8_t(item),1,born,end};auto b=biology(cohort,productive_service(cohort));
            for(const auto& day:b.days)for(int p=0;p<kag::N_PRODUCTS;++p)totals[0][p]+=day.output[p];
        }
        for(int i=0;i<actual;++i) {
            int item,born,end;uint32_t feed,care,collect;in>>item>>born>>end>>feed>>care>>collect;
            Cohort cohort{uint8_t(item),1,born,end};auto service=productive_service(cohort);
            auto ideal=biology(cohort,service);
            service.feed=feed;service.care=care;service.collect_fertilizer=collect;
            auto observed=biology(cohort,service);
            for(int d=0;d<30;++d)for(int p=0;p<kag::N_PRODUCTS;++p) {
                totals[1][p]+=ideal.days[d].output[p];totals[2][p]+=observed.days[d].output[p];
            }
        }
        if(c)out<<',';out<<"{\"case\":\""<<name<<"\",\"outputs\":[";
        for(int mode=0;mode<3;++mode){if(mode)out<<',';out<<'[';for(int p=0;p<kag::N_PRODUCTS;++p){if(p)out<<',';out<<totals[mode][p];}out<<']';}
        out<<"]}";
    }
    if(!in)return 3;out<<"]\n";
}
