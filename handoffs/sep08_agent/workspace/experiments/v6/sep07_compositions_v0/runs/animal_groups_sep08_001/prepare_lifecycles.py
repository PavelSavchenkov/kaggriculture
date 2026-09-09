"""Correct opportunity costs for animals already planned on released crop tiles."""
from pathlib import Path
import hashlib
import json

RUN=Path(__file__).resolve().parent
assert not (RUN/'source/season_v2.hpp').exists()
season=(RUN/'source/season.hpp').read_text()
season=season.replace('struct Season {','''struct AnimalLife {
    int cell;
    Cohort cohort;
    Service service{0,0,0,0,0,0};
    std::array<int,30> harvested{},fertilizer{};
    bool exact=false;
};

struct Season {''')
season=season.replace('std::vector<CropLife> lives;','std::vector<CropLife> lives;\n    std::vector<AnimalLife> animals;')
point='        for(auto& life:lives) {'
assert season.count(point)==1
season=season.replace(point,'''        std::array<int,100> active_animal;active_animal.fill(-1);
        for(int day=0;day<30;++day){
            for(const auto& work:days[day].problem.tile_work)for(const auto& op:work.actions){
                auto& id=active_animal[work.tile];
                if(op.op==OP_PLACE && is_animal(op.arg)){
                    if(id>=0)std::abort();
                    id=animals.size();animals.push_back({work.tile,{uint8_t(op.arg),1,day,30}});
                }
                if(id<0)continue;
                auto& life=animals[id];const uint32_t bit=1u<<day;
                if(op.op==OP_FEED)life.service.feed|=bit;
                if(op.op==OP_CARE)life.service.care|=bit;
                if(op.op==OP_COLLECT_FERTILIZER){life.service.collect_fertilizer|=bit;life.fertilizer[day]+=op.output_quantity;}
                if(op.op==OP_HARVEST){life.service.harvest|=bit;life.harvested[day]+=op.output_quantity;}
            }
            if(day<29)for(int cell=0;cell<100;++cell)if(active_animal[cell]>=0){
                const auto& tile=end_farms[day].tiles[cell/10][cell%10];
                if(!tile.has_animal){animals[active_animal[cell]].cohort.end_day=day+1;active_animal[cell]=-1;}
            }
        }
        for(auto& life:animals){
            const auto predicted=biology(life.cohort,life.service);life.exact=true;
            const int product=ANIMALS[life.cohort.item-GOOSE].product;
            for(int day=0;day<30;++day)life.exact&=predicted.days[day].output[product]==life.harvested[day] &&
                predicted.days[day].output[FERTILIZER]==life.fertilizer[day];
        }
''' + point)
(RUN/'source/season_v2.hpp').write_text(season)

estimate=(RUN/'source/estimate.cpp').read_text().replace('#include "season.hpp"','#include "season_v2.hpp"')
estimate=estimate.replace('Biology delta;\n};','Biology delta;\n    std::array<int,N_ANIMALS> removed{};\n};')
estimate=estimate.replace('animal_cost,seeds_saved,','animal_cost,removed_animals,seeds_saved,')
estimate=estimate.replace('        Source original;', '''        for(const auto& life:source.animals)if(!life.exact){std::cerr<<"inexact animal life "<<life.cell<<'\\n';return 3;}
        Source original;''')
estimate=estimate.replace('<<",\\"cash\\":"<<control.cash[0]', '<<",\\"animal_lives_exact\\":"<<source.animals.size()<<",\\"cash\\":"<<control.cash[0]')
estimate=estimate.replace('        FarmFlowPlan plan;', '        FarmFlowPlan plan;\n        std::array<std::array<int,N_ANIMALS>,30> animal_buys{};')
estimate=estimate.replace('                    if(order.op==M_SELL)', '                    if(order.op==M_BUY_ANIMAL)animal_buys[day][order.item-GOOSE]+=std::max(0,order.n);\n                    if(order.op==M_SELL)')
point='                    for(int d=day;d<30;++d)p.visits+='
assert estimate.count(point)==1
estimate=estimate.replace(point,'''                    for(const auto& life:source.animals)if(life.cell==cell && life.cohort.start_day>=day){
                        const auto removed=biology(life.cohort,life.service);++p.removed[life.cohort.item-GOOSE];
                        for(int d=day;d<30;++d){
                            for(int product=0;product<N_PRODUCTS;++product)p.delta.days[d].output[product]-=removed.days[d].output[product];
                            p.delta.days[d].wheat-=removed.days[d].wheat;
                            p.delta.days[d].operations-=removed.days[d].operations;
                        }
                    }
''' + point)
estimate=estimate.replace('Biology delta;int saved=0,deficit=0,operations=0;std::string encoded;',
    'Biology delta;int saved=0,deficit=0,operations=0,removed_count=0;std::array<int,N_ANIMALS> removed{};std::string encoded;')
estimate=estimate.replace('if(!encoded.empty())encoded+=\';\';', 'for(int i=0;i<N_ANIMALS;++i)removed[i]+=p.removed[i];\n                    if(!encoded.empty())encoded+=\';\';')
estimate=estimate.replace('                for(const auto& d:delta.days)operations+=d.operations;', '''                for(int i=0;i<N_ANIMALS;++i){
                    int future_buys=0;for(int d=day;d<30;++d)future_buys+=animal_buys[d][i];
                    delta.animal_cost-=std::min(removed[i],future_buys)*ANIMALS[i].cost;
                    removed_count+=removed[i];
                }
                for(const auto& d:delta.days)operations+=d.operations;''')
estimate=estimate.replace("<<delta.animal_cost<<','<<saved", "<<delta.animal_cost<<','<<removed_count<<','<<saved")
(RUN/'source/estimate_v2.cpp').write_text(estimate)

compiler=(RUN/'source/compile_v3.cpp').read_text().replace('#include "season.hpp"','#include "season_v2.hpp"')
compiler=compiler.replace('if(job.op==OP_PLANT)break;',
    'if(job.op==OP_PLANT || job.op==OP_BUILD_COOP || job.op==OP_BUILD_PASTURE || (job.op==OP_PLACE && is_animal(job.arg)))break;')
point='        // Preserve final useful stock.'
assert compiler.count(point)==1
compiler=compiler.replace(point,'''        // Cancel animal purchases displaced by this dated continuation before
        // searching a funded hour for the replacement. Already bought animals
        // are sunk stock: retain them for future placements instead of selling.
        std::array<int,N_ITEMS> canceled_animals{};
        for(int i=GOOSE;i<=SHEEP;++i){
            int surplus=actual.problem.start.shed[i]-source.problem.start.shed[i]+flow_delta[i];
            for(int h=23;h>=0 && surplus>0;--h)for(int s=markets[h].n_orders-1;s>=0 && surplus>0;--s){
                auto& order=markets[h].orders[s];
                if(order.op!=M_BUY_ANIMAL || order.item!=i)continue;
                const int n=std::min(surplus,order.n);order.n-=n;surplus-=n;canceled_animals[i]+=n;
                if(!order.n)order={};markets[h].finalize();
            }
        }
''' + point)
compiler=compiler.replace('int delta=actual.problem.start.shed[i]-source.problem.start.shed[i]+flow_delta[i];',
    'int delta=actual.problem.start.shed[i]-source.problem.start.shed[i]+flow_delta[i]-canceled_animals[i];')
compiler=compiler.replace('            if(delta>0) {\n                const int hour=',
    '            if(delta>0 && is_animal(i)){problem.end_shed[i]+=delta;delta=0;}\n            if(delta>0) {\n                const int hour=')
(RUN/'source/compile_v4.cpp').write_text(compiler)
cmake=RUN/'CMakeLists.txt'
text=cmake.read_text().replace('source_control_v2 hinted_solve)','source_control_v2 hinted_solve estimate_v2 compile_v4)')
cmake.write_text(text)
(RUN/'LIFECYCLE_CORRECTION.json').write_text(json.dumps({
    'previous_estimate_verdict':'Not a valid opportunity-cost screen for candidates whose source tile later receives an animal. Original timings and source controls remain facts; those economic scores are invalid.',
    'witness':'Source1001day10tile23 already places a goose; tile32 builds a coop for its following animal. Original compiler appended another build/place sequence.',
    'corrections':['Extract exact animal lifetimes and productive service from the current source.',
        'Subtract displaced animal output, feed, fertilizer and operations from the proposed continuation.',
        'Refund at most future animal purchases; already funded animals are sunk inventory.',
        'Stop old tile work before the existing structure/animal continuation, not only before the next crop plant.',
        'Cancel displaced animal purchases and retain unplaced sunk animals as inventory for future use.'],
    'new_files':{f:hashlib.sha256((RUN/'source'/f).read_bytes()).hexdigest() for f in ['season_v2.hpp','estimate_v2.cpp','compile_v4.cpp']}},indent=2)+'\n')
