#pragma once
#include "library.hpp"
#include "../../animal_repair_sep08_001/source/repair.hpp"
#include "../../empty_sale_slots_sep08_001/proposals/empty_sale_slots_m2/source/agent.hpp"
namespace catalog_cow_service_retained_q24_premium_m2_compositions::cow_service_retained {
using animal_repair::repair_day;
struct Diagnostics {
    int family=-1,choice=0,entry_step=-1,advanced_orders=0,advanced_units=0;
    uint32_t matched_days=0,missed_days=0,repaired_days=0;
    std::array<double,5> predicted_own{},predicted_margin{},deviation{};
};
class Policy {
    empty_sale_slots_m2::Agent base_;
    bool repriced_;
    int mode_,forced_family_,forced_choice_,locked_leaf_,family_=-1,choice_=0,leaf_=0,due_=-1;
    const GuardedDay* day_=nullptr;
    std::array<int,kag::N_ITEMS> suppression_{};
    Diagnostics diagnostics_{};
    static int berry(const kag::agent::AgentObservation& o){
        int demand=0;for(int s=0;s<o.n_shops;++s)
            if(kag::SHOP_MASK[o.shops[s]]&(1u<<kag::STRAWBERRY))demand+=kag::SHOP_MULT[o.shops[s]];
        return demand>=4;
    }
    static int distance(const GuardedDay& g,const kag::agent::AgentObservation& o){
        int n=0;const auto& f=o.self();
        n+=1000*(f.n_units!=1 || f.n_quadrants!=g.quadrants || f.pos_x[0]!=4 || f.pos_y[0]!=4);
        for(int i=0;i<kag::N_ITEMS;++i)n+=(o.own.shed[i]!=g.shed[i])+(o.own.inv[0][i]!=0);
        for(int i=0;i<kag::N_CROPS;++i)n+=o.own.seeds[i]!=g.seeds[i];
        for(int c=0;c<100;++c)n+=10*(g.check[c] && tile_key(f.tiles[c/10][c%10],o.day)!=g.tiles[c]);
        return n;
    }
    void choose(const kag::agent::AgentObservation& o){
        if(!mode_ || family_>=0 || (o.day!=15 && o.day!=20))return;
        const int f=o.day==15?0:1;
        if(forced_family_>=0 && forced_family_!=f)return;
        const auto& family=(repriced_?library():animal_groups_policy::library())[f];const int visible_leaf=locked_leaf_>=0?locked_leaf_:f?berry(o):0;
        std::array<bool,5> eligible{};
        for(int c=1;c<family.count;++c){
            const auto& course=family.choices[c][visible_leaf];
            double cash=course.fixed[o.day]+1000;
            for(int p=0;p<kag::N_PRODUCTS;++p)cash+=course.flow.buys[o.day][p]*o.market.prices[p];
            eligible[c]=course.days.front().matches(o) && o.self().money>=cash;
        }
        int selected=0;double best=0;
        diagnostics_.predicted_own.fill(0);diagnostics_.predicted_margin.fill(0);diagnostics_.deviation.fill(0);
        std::array<double,5> squared{};const Biology empty;
        for(int sample=0;sample<32;++sample){
            const auto future=late_portfolio::scenario(o,sample);
            // Day-20 berry demand is already visible. Future weed work in the
            // cow family is priced as an equal mixture of the two solved cases.
            const int leaf=f?visible_leaf:sample%2;
            const auto& baseline=family.choices[0][leaf];
            const auto before=value_animal_investment(o,baseline.flow,empty,0,1,&future.demand);
            for(int c=1;c<family.count;++c)if(eligible[c]){
                const auto& course=family.choices[c][leaf];
                const auto value=value_animal_investment(o,course.flow,empty,0,1,&future.demand);
                double fixed=0;for(int d=o.day;d<30;++d)fixed+=course.fixed[d]-baseline.fixed[d];
                const double own=value.own-before.own-fixed,margin=value.margin()-before.margin()-fixed;
                diagnostics_.predicted_own[c]+=own;diagnostics_.predicted_margin[c]+=margin;squared[c]+=margin*margin;
            }
        }
        for(int c=1;c<family.count;++c)if(eligible[c]){
            diagnostics_.predicted_own[c]/=32;diagnostics_.predicted_margin[c]/=32;
            diagnostics_.deviation[c]=std::sqrt(std::max(0.,squared[c]/32-diagnostics_.predicted_margin[c]*diagnostics_.predicted_margin[c]));
            const double score=diagnostics_.predicted_margin[c]-.5*diagnostics_.deviation[c];
            if(score>best){best=score;selected=c;}
        }
        if(forced_choice_>=0)selected=forced_choice_<family.count && eligible[forced_choice_]?forced_choice_:0;
        if(selected){family_=f;choice_=selected;leaf_=visible_leaf;diagnostics_.family=f;diagnostics_.choice=selected;diagnostics_.entry_step=o.step;}
    }
    void live_sales(const kag::agent::AgentObservation& o,kag::Action& a){
        using namespace kag;
        if(due_==o.step)for(int s=0;s<a.n_orders;++s){auto& m=a.orders[s];if(m.op==M_SELL){
            const int n=std::min(std::max(0,int(m.n)),suppression_[m.item]);m.n-=n;suppression_[m.item]-=n;}}
        due_=-1;suppression_.fill(0);
        if(o.step<718 && o.hour<23 && o.step%4!=0){
            // Ahmed V23 / accepted sale-lead projection, applied to this
            // committed day's next orders. No future observation is read.
            std::array<int,N_ITEMS> available{},forecast{};bool already[N_ITEMS]{};
            for(int i=0;i<N_ITEMS;++i)available[i]=o.own.shed[i];int total=o.own.shed_total;
            for(int u=0;u<a.n_units;++u){
                const int x=o.self().pos_x[u],y=o.self().pos_y[u];if((x!=4 && x!=5)||(y!=4 && y!=5))continue;
                const auto& v=a.units[u];
                if(v.op==OP_PICKUP){const int n=std::min(available[v.arg],std::max(0,int(v.n)));available[v.arg]-=n;total-=n;}
                else if(v.op==OP_DROP){for(int k=0;k<o.own.inv_nkeys[u];++k){const int i=o.own.inv_keys[u][k];const int n=std::min(int(o.own.inv[u][i]),std::max(0,100-total));available[i]+=n;total+=n;}}
                else if(v.op==OP_PLACE && v.arg<GOOSE){const int n=std::min({std::max(0,int(v.n)),int(o.own.inv[u][v.arg]),std::max(0,100-total)});available[v.arg]+=n;total+=n;}
            }
            for(int s=0;s<a.n_orders;++s)if(a.orders[s].op==M_SELL)already[a.orders[s].item]=true;
            const auto& next=day_->plan.actions[o.hour+1];
            for(int s=0;s<next.n_orders;++s)if(next.orders[s].op==M_SELL)forecast[next.orders[s].item]+=std::max(0,int(next.orders[s].n));
            for(int i=1;i<FERTILIZER && a.n_orders<10;++i)if(!already[i] && o.market.prices[i]>=2){
                const int n=std::min(available[i],forecast[i]);if(n<=0)continue;
                a.orders[a.n_orders++]={M_SELL,uint8_t(i),n};suppression_[i]=n;due_=o.step+1;
                ++diagnostics_.advanced_orders;diagnostics_.advanced_units+=n;
            }
        }
        int kept=0;for(int s=0;s<a.n_orders;++s){const auto& m=a.orders[s];
            if(m.op==M_SELL && m.item>=CARROT && m.item<=WOOL && m.n==0)continue;a.orders[kept++]=m;}
        a.n_orders=kept;
    }
public:
    explicit Policy(int mode=1,int forced_family=-1,int forced_choice=-1,int locked_leaf=-1,bool repriced=true):repriced_(repriced),mode_(mode),forced_family_(forced_family),forced_choice_(forced_choice),locked_leaf_(locked_leaf){
        if(mode<0 || mode>2 || forced_family>1 || forced_choice>4 || locked_leaf>1)std::abort();(void)library();
    }
    static kag::agent::AgentInfo info(){return {"cow_service_retained"};}
    void reset(const kag::agent::AgentInit& init){
        base_.reset(init);family_=-1;choice_=leaf_=0;due_=-1;day_=nullptr;suppression_.fill(0);diagnostics_={};
    }
    const Diagnostics& diagnostics()const{return diagnostics_;}
    bool active()const{return family_>=0;}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& budget,kag::Action& a){
        base_.act(o,budget,a);
        if(o.hour==0){
            choose(o);day_=nullptr;
            if(active()){
                const auto& family=library()[family_];const int index=o.day-family.first;
                int selected=leaf_,best=1000000;
                for(int leaf=0;leaf<2;++leaf){
                    if(locked_leaf_>=0 && leaf!=locked_leaf_)continue;
                    if(family_==1 && leaf!=leaf_)continue;
                    const auto& g=family.choices[choice_][leaf].days[index];const int score=distance(g,o);
                    if(score<best){best=score;selected=leaf;}
                }
                leaf_=selected;day_=&family.choices[choice_][leaf_].days[index];
                // Only this cow course has an independently certified repair.
                if(!day_->matches(o) && family_==0 && choice_==2 && o.day==23){
                    const auto& repair=repair_day();
                    if(repair.matches(o)){day_=&repair;diagnostics_.repaired_days|=uint32_t{1}<<o.day;}
                }
                if(day_->matches(o))diagnostics_.matched_days|=uint32_t{1}<<o.day;
                else diagnostics_.missed_days|=uint32_t{1}<<o.day;
                // A missed guard is diagnosed; it never abandons new animals
                // for the old farm's policy. Continue the closest complete
                // course and measure failures before accepting this policy.
            }
        }
        if(!day_)return;
        a=day_->plan.actions[o.hour];
        for(int u=a.n_units;u<o.self().n_units;++u)a.units[u]={};a.n_units=o.self().n_units;
        if(mode_==2)live_sales(o,a);
        a.finalize();
    }
};
}
