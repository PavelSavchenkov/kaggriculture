#include "experiments/v6/sep07_compositions_v0/runs/animal_repair_sep08_001/proposals/animal_repair_q24_premium_m2/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/observed_sale_lead_003/parent_source/candidates/investment_context_guarded_001_best/source/agent.hpp"
#include <iomanip>
#include <iostream>
namespace source_tapes {
#include "experiments/v6/sep07_compositions_v0/runs/observed_sale_lead_003/parent_source/league/top_replay_library/source/tapes.inc"
}
template<class T>void array(const T&values){std::cout<<'[';bool first=true;for(auto v:values){if(!first)std::cout<<',';first=false;std::cout<<+v;}std::cout<<']';}
void action(const kag::Action&a){std::cout<<"[[";for(int u=0;u<a.n_units;++u){if(u)std::cout<<',';std::cout<<'['<<+a.units[u].op<<','<<+a.units[u].arg<<','<<a.units[u].n<<']';}std::cout<<"],[";for(int i=0;i<a.n_orders;++i){if(i)std::cout<<',';std::cout<<'['<<+a.orders[i].op<<','<<+a.orders[i].item<<','<<a.orders[i].n<<']';}std::cout<<"]]";}
template<class T>void day(const T&d){
    std::cout<<"{\"day\":"<<d.plan.day<<",\"quadrants\":"<<d.quadrants<<",\"shed\":";array(d.shed);std::cout<<",\"seeds\":";array(d.seeds);std::cout<<",\"tiles\":[";bool first=true;
    for(int cell=0;cell<100;++cell)if(d.check[cell]){if(!first)std::cout<<',';first=false;std::cout<<'['<<cell<<',';array(d.tiles[cell]);std::cout<<']';}
    std::cout<<"],\"actions\":[";for(int h=0;h<24;++h){if(h)std::cout<<',';action(d.plan.actions[h]);}std::cout<<"]}";
}
template<class T>void days(const T&entries){std::cout<<'[';bool first=true;for(const auto&e:entries){if(!first)std::cout<<',';first=false;day(e);}std::cout<<']';}
template<class T>void flows(const T&values){std::cout<<'[';for(int d=0;d<30;++d){if(d)std::cout<<',';array(values[d]);}std::cout<<']';}
int main(){
    using namespace compositions_sale_copy;std::cout<<std::setprecision(17)<<"{\"tape\":[";
    for(int step=0;step<719;++step){if(step)std::cout<<',';int p=source_tapes::offsets[150][step];kag::Action a;a.clear();a.n_units=source_tapes::values[p++];a.n_orders=source_tapes::values[p++];
        for(int u=0;u<a.n_units;++u){a.units[u]={uint8_t(source_tapes::values[p]),uint8_t(source_tapes::values[p+1]),source_tapes::values[p+2]};p+=3;}
        for(int i=0;i<a.n_orders;++i){a.orders[i]={uint8_t(source_tapes::values[p]),uint8_t(source_tapes::values[p+1]),source_tapes::values[p+2]};p+=3;}action(a);}
    std::cout<<"],\"days\":[";bool first=true;for(const auto&e:day_library::entries()){if(!first)std::cout<<',';first=false;day(e.day);}std::cout<<"]";
    std::cout<<",\"shop_days\":";days(shop_herd_days::select({8,9,11,13,14,15,16,18,19,20,23,24,25,26,27}));
    std::cout<<",\"context_days\":";days(investment_context_days::select({13,14,15,16,18,19,20,23,24,25,27,113,114,115,116,118,119,120,122,123,124,125,127}));
    const auto models=animal_entry_bank_001::models();std::cout<<",\"models\":[";
    for(size_t i=0;i<models.size();++i){if(i)std::cout<<',';const auto&m=models[i];std::cout<<"{\"sales\":";flows(m.flows.sales);std::cout<<",\"buys\":";flows(m.flows.buys);std::cout<<",\"service\":["<<m.service.water<<','<<m.service.feed<<','<<m.service.care<<','<<m.service.collect_fertilizer<<','<<m.service.harvest<<','<<m.service.fertilize<<"],\"force_entry_service\":"<<m.force_entry_service<<'}';}
    std::cout<<"],\"entries\":[";first=true;for(const auto&e:animal_entry_bank_001::entries()){
        if(!first)std::cout<<',';first=false;const auto&m=models[e.model];const auto biology=investment_biology(e.item,e.day.plan.day,m.service,m.force_entry_service);
        std::cout<<"{\"item\":"<<e.item<<",\"model\":"<<e.model<<",\"plan\":";day(e.day);std::cout<<",\"animal_cost\":"<<biology.animal_cost<<",\"biology\":[";
        for(int d=0;d<30;++d){if(d)std::cout<<',';std::cout<<'[';array(biology.days[d].output);std::cout<<','<<biology.days[d].wheat<<','<<biology.days[d].operations<<']';}std::cout<<"]}";
    }
    std::cout<<"],\"market\":[";for(int p=0;p<9;++p){if(p)std::cout<<',';const auto&m=kag::MARKET[p];std::cout<<'['<<m.base<<','<<m.I0<<','<<m.T<<','<<+m.below_f<<','<<m.below_t<<','<<+m.above_f<<','<<m.above_t<<']';}
    std::cout<<"],\"animals\":[";for(int i=0;i<3;++i){if(i)std::cout<<',';const auto&a=kag::ANIMALS[i];std::cout<<'['<<a.first_yield_day<<','<<a.interval<<','<<a.max_held<<','<<+a.product<<']';}
    std::cout<<"],\"shop_mask\":";array(kag::SHOP_MASK);std::cout<<",\"shop_mult\":";array(kag::SHOP_MULT);
    std::cout<<",\"fert_days\":";days(fertilization_004_0_closure::days());
    std::cout<<",\"rotation\":[";days(crop_rotation_t2_berry::off_days());std::cout<<',';days(crop_rotation_t2_berry::on_days());std::cout<<']';
    std::cout<<",\"wheat\":[";days(wheat_one_fert::off_days());std::cout<<',';days(wheat_one_fert::on_days());std::cout<<']';
    std::cout<<",\"portfolio\":[";first=true;
    for(const auto& c:late_portfolio::calendars()){
        if(!first)std::cout<<',';first=false;std::cout<<'[';
        for(int leaf=0;leaf<2;++leaf){if(leaf)std::cout<<',';std::cout<<"{\"days\":";days(c.days[leaf]);std::cout<<",\"sales\":";flows(c.flows[leaf].sales);std::cout<<",\"buys\":";flows(c.flows[leaf].buys);std::cout<<",\"fixed\":"<<c.fixed_cost[leaf]<<'}';}std::cout<<']';
    }std::cout<<']';
    std::cout<<",\"wool_target\":{"<<"\"shed\":";array(v52_family::target_shed);std::cout<<",\"seeds\":";array(v52_family::target_seeds);std::cout<<",\"tiles\":[";
    first=true;for(int cell=0;cell<100;++cell)if(v52_family::target_tiles[cell][0]!=kag::T_LOCKED){if(!first)std::cout<<',';first=false;std::cout<<'['<<cell<<',';array(v52_family::target_tiles[cell]);std::cout<<']';}std::cout<<"]}";
    std::cout<<",\"wool_tape\":[";for(int step=0;step<719;++step){if(step)std::cout<<',';kag::agent::AgentObservation o{};o.step=step;int p=v52_family::data::offsets[1][step];o.farms[0].n_units=v52_family::data::values[p];kag::Action a;v52_family::tape(1,o,a);action(a);}std::cout<<']';
    std::cout<<",\"wool_days\":";days(v52_family::improved_days());
    std::cout<<",\"wool_v2_days\":";days(v52_family_v2::improved_days());
    std::cout<<",\"groups\":[";first=true;
    for(const auto& f:compositions::animal_groups_policy::library()){
        if(!first)std::cout<<',';first=false;std::cout<<"{\"first\":"<<f.first<<",\"choices\":[";
        for(int choice=0;choice<f.count;++choice){if(choice)std::cout<<',';std::cout<<'[';
            for(int leaf=0;leaf<2;++leaf){if(leaf)std::cout<<',';const auto& c=f.choices[choice][leaf];std::cout<<"{\"days\":";days(c.days);std::cout<<",\"sales\":";flows(c.flow.sales);std::cout<<",\"buys\":";flows(c.flow.buys);std::cout<<",\"fixed\":";array(c.fixed);std::cout<<'}';}std::cout<<']';
        }std::cout<<"]}";
    }std::cout<<']';
    std::cout<<",\"repair\":";day(compositions::animal_repair::repair_day());
    std::cout<<"}\n";

}
