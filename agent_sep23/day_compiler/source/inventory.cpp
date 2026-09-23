#include "inventory.hpp"
#include <algorithm>
#include <cmath>

namespace kag::day_compiler {
namespace {
constexpr double IMPOSSIBLE=-1e100;
bool valid_problem(const InventoryProblem& p,int stock) {
    if((p.product!=WHEAT && p.product!=FERTILIZER) || p.count<1 || p.count>INVENTORY_EVENTS ||
       p.capacity<0 || p.capacity>100 || stock<0 || stock>p.capacity || p.rival_weight<0) return false;
    double probability=0;
    for(const auto& s:p.residual) {
        if(s.probability<0 || s.requirement<0 || s.requirement>100 || s.usable_receipts<0 || s.usable_receipts>100) return false;
        probability+=s.probability;
    }
    if(!p.terminal && std::abs(probability-1)>1e-8) return false;
    for(int k=0;k<p.count;++k) {
        const auto& e=p.events[k];
        if(std::abs(e.rival)>100 || std::abs(e.rival_after)>100 || e.demand_after<0 ||
           e.minimum_left<0 || e.maximum_left>p.capacity || e.minimum_left>e.maximum_left ||
           std::abs(e.net_receipt_next)>100) return false;
    }
    return true;
}
double residual(const InventoryProblem& problem,int stock,int inventory) {
    if(problem.terminal) return 0;
    double value=0;
    for(const auto& scenario:problem.residual) if(scenario.probability)
        value+=scenario.probability*transact(problem.product,inventory+scenario.inventory_change,
                                            stock+scenario.usable_receipts-scenario.requirement,0).own_cash;
    return value;
}
bool better(double value,int first,double old_value,int old_first,int preferred) {
    return value>old_value+1e-8 || (std::abs(value-old_value)<=1e-8 && std::abs(first-preferred)<std::abs(old_first-preferred));
}
void trade_table(int product,int inventory,int rival,std::array<Transaction,201>& table) {
    table[100]=transact(product,inventory,0,rival);
    const int count=std::abs(rival),direction=(rival>0)-(rival<0);
    for(int own_direction:{-1,1}) {
        Transaction paired{inventory};
        for(int n=1;n<=100;++n) {
            auto& result=table[100+own_direction*n];
            if(n<=count) {
                const int own_price=market_price(product,paired.inventory-(own_direction<0));
                const int rival_price=market_price(product,paired.inventory-(direction<0));
                paired.own_cash+=own_direction*own_price; paired.rival_cash+=direction*rival_price;
                paired.inventory+=own_direction<0?-1:own_price>1;
                paired.inventory+=direction<0?-1:rival_price>1;
                const auto tail=transact(product,paired.inventory,0,rival-direction*n);
                result={tail.inventory,paired.own_cash,paired.rival_cash+tail.rival_cash};
            } else {
                result=table[100+own_direction*(n-1)];
                const int price=market_price(product,result.inventory-(own_direction<0));
                result.own_cash+=own_direction*price;
                result.inventory+=own_direction<0?-1:price>1;
            }
        }
    }
}
}
InventoryChoice InventoryController::solve(const InventoryProblem& p,int stock,int inventory,bool allow_reduction) {
    InventoryChoice invalid;
    if(!valid_problem(p,stock)) return invalid;
    bool simple=allow_reduction;
    int anchor=inventory+stock;
    for(int k=0;k<p.count;++k) {
        const auto& e=p.events[k];
        simple&=!e.rival && !e.rival_after && market_price(p.product,anchor)>1;
        anchor+=e.net_receipt_next-e.demand_after;
    }
    return simple?reduced(p,stock,inventory):general(p,stock,inventory);
}
InventoryChoice InventoryController::reduced(const InventoryProblem& p,int stock,int inventory) {
    InventoryChoice out; out.reduced=true;
    double current[101],next[101]; int first[101]{},next_first[101]{};
    std::fill_n(current,101,IMPOSSIBLE); current[stock]=0;
    int anchor=inventory+stock;
    for(int k=0;k<p.count;++k) {
        const auto& e=p.events[k]; double potential[101]{};
        for(int q=1;q<=p.capacity;++q) potential[q]=potential[q-1]+market_price(p.product,anchor-q);
        double best=IMPOSSIBLE; int source=0;
        for(int q=0;q<=p.capacity;++q) if(current[q]>IMPOSSIBLE/2 &&
            better(current[q]+potential[q],first[q],best,first[source],p.preferred_first_quantity)) { best=current[q]+potential[q]; source=q; }
        if(best<=IMPOSSIBLE/2) return out;
        std::fill_n(next,101,IMPOSSIBLE);
        for(int z=e.minimum_left;z<=e.maximum_left;++z) {
            if(out.transitions++>=p.transition_limit) { out.complete=false; return out; }
            const double cash=e.trade_allowed?best-potential[z]:current[z];
            const int q=z+e.net_receipt_next;
            if(cash<=IMPOSSIBLE/2 || cash+e.cash_available<-1e-8 || q<0 || q>p.capacity) continue;
            next[q]=cash; next_first[q]=k?first[e.trade_allowed?source:z]:stock-z;
        }
        std::copy_n(next,101,current); std::copy_n(next_first,101,first);
        anchor+=e.net_receipt_next-e.demand_after;
    }
    for(int q=0;q<=p.capacity;++q) if(current[q]>IMPOSSIBLE/2) {
        const double value=current[q]+residual(p,q,anchor-q);
        if(better(value,first[q],out.value,out.quantity,p.preferred_first_quantity)) { out.value=value; out.quantity=first[q]; out.feasible=true; }
    }
    return out;
}
InventoryChoice InventoryController::general(const InventoryProblem& p,int stock,int inventory) {
    InventoryChoice out;
    current_[0]={stock,inventory,0,-1,0,0}; int count=1;
    for(int k=0;k<p.count;++k) {
        const auto& e=p.events[k]; heads_.fill(-1); int next_count=0;
        for(int j=0;j<count;++j) {
            const auto& before=current_[j];
            std::array<Transaction,201> trades;
            trade_table(p.product,before.inventory,e.rival,trades);
            for(int z=e.minimum_left;z<=e.maximum_left;++z) {
                if(!e.trade_allowed && z!=before.stock) continue;
                if(out.transitions++>=p.transition_limit) { out.complete=false; return out; }
                const int q=z+e.net_receipt_next;
                if(q<0 || q>p.capacity) continue;
                const auto& trade=trades[100+before.stock-z];
                const double cash=before.cash+trade.own_cash;
                if(cash+e.cash_available<-1e-8) continue;
                const auto later=transact(p.product,trade.inventory,0,e.rival_after);
                const double value=before.value+trade.own_cash-p.rival_weight*(trade.rival_cash+later.rival_cash);
                const int market=later.inventory-e.demand_after,first=k?before.first:stock-z;
                const uint64_t key=(uint64_t(q)<<32)|uint32_t(market);
                int slot=(key*11400714819323198485ull)>>51;
                while(heads_[slot]>=0 && (next_[heads_[slot]].stock!=q || next_[heads_[slot]].inventory!=market)) slot=(slot+1)&(HASH-1);
                bool dominated=false;
                for(int at=heads_[slot];at>=0;at=next_[at].next) {
                    const auto& old=next_[at];
                    if(old.cash>=cash-1e-8 && old.value>=value-1e-8 &&
                       (old.cash>cash+1e-8 || old.value>value+1e-8 || std::abs(old.first-p.preferred_first_quantity)<=std::abs(first-p.preferred_first_quantity))) { dominated=true; break; }
                }
                if(dominated) continue;
                int reuse=-1;
                for(int at=heads_[slot];at>=0;at=next_[at].next) {
                    auto& old=next_[at];
                    if(cash>=old.cash-1e-8 && value>=old.value-1e-8 &&
                       (cash>old.cash+1e-8 || value>old.value+1e-8 || std::abs(first-p.preferred_first_quantity)<=std::abs(old.first-p.preferred_first_quantity))) {
                        if(reuse<0) reuse=at; else old.value=IMPOSSIBLE;
                    }
                }
                if(reuse<0) {
                    if(next_count==LABELS) { out.complete=false; return out; }
                    reuse=next_count++; next_[reuse].next=heads_[slot]; heads_[slot]=reuse;
                }
                const int link=next_[reuse].next;
                next_[reuse]={q,market,first,link,cash,value};
            }
        }
        count=0;
        for(int j=0;j<next_count;++j) if(next_[j].value>IMPOSSIBLE/2) current_[count++]=next_[j];
    }
    for(int j=0;j<count;++j) {
        const auto& label=current_[j]; const double value=label.value+residual(p,label.stock,label.inventory);
        if(better(value,label.first,out.value,out.quantity,p.preferred_first_quantity)) { out.value=value; out.quantity=label.first; out.feasible=true; }
    }
    return out;
}
InventoryCurve InventoryController::curve(const InventoryProblem& problem,int stock,int inventory) {
    InventoryCurve out;
    if(!valid_problem(problem,stock)) return out;
    // The market/stock conservation identity also holds during simultaneous
    // trades above the floor. Their payoff is not separable, so retain exact
    // transitions and a Pareto frontier of required cash and continuation value.
    bool conserved=true; int anchor=inventory+stock;
    for(int k=0;k<problem.count;++k) {
        const auto& e=problem.events[k];
        conserved&=market_price(problem.product,anchor+std::max(0,e.rival)+std::max(0,e.rival_after))>1;
        anchor+=e.net_receipt_next+e.rival+e.rival_after-e.demand_after;
    }
    if(conserved) return conserved_curve(problem,stock,inventory);
    for(int z=problem.events[0].minimum_left;z<=problem.events[0].maximum_left;++z) {
        if(!problem.events[0].trade_allowed && z!=stock) continue;
        auto p=problem; p.events[0].minimum_left=p.events[0].maximum_left=z;
        p.transition_limit-=std::min(p.transition_limit,out.transitions);
        const auto choice=solve(p,stock,inventory);
        out.transitions+=choice.transitions;
        if(!choice.complete) { out.complete=false; return out; }
        if(choice.feasible) out.value[z]=choice.value;
    }
    return out;
}
InventoryCurve InventoryController::conserved_curve(const InventoryProblem& p,int stock,int inventory) {
    InventoryCurve out;
    int anchors[INVENTORY_EVENTS+1]; anchors[0]=inventory+stock;
    for(int k=0;k<p.count;++k) {
        const auto& e=p.events[k];
        anchors[k+1]=anchors[k]+e.rival+e.rival_after-e.demand_after+e.net_receipt_next;
    }
    double potentials[INVENTORY_EVENTS+1][101]{},wealth_floor[INVENTORY_EVENTS+1]{};
    for(int k=0;k<=p.count;++k) for(int q=1;q<=p.capacity;++q)
        potentials[k][q]=potentials[k][q-1]+market_price(p.product,anchors[k]-q);
    wealth_floor[0]=potentials[0][stock];
    for(int k=0;k<p.count;++k) {
        const auto& e=p.events[k]; double minimum_change=1e100; int quote_impact=0;
        for(int z=e.minimum_left;z<=e.maximum_left;++z) {
            const int after=z+e.net_receipt_next;
            if(after>=0 && after<=p.capacity)
                minimum_change=std::min(minimum_change,potentials[k+1][after]-potentials[k][z]);
        }
        if(minimum_change>1e99) return out;
        for(int q=0;q<=p.capacity;++q)
            quote_impact=std::max(quote_impact,market_price(p.product,anchors[k]-q+std::min(0,e.rival))-
                                             market_price(p.product,anchors[k]-q+std::max(0,e.rival)));
        wealth_floor[k+1]=wealth_floor[k]+minimum_change-p.capacity*quote_impact;
    }
    int counts[101],next_counts[101]{};
    for(int q=0;q<=p.capacity;++q) {
        counts[q]=1; cash_current_[q][0]={IMPOSSIBLE,residual(p,q,anchors[p.count]-q)};
    }
    for(int k=p.count-1;k>=0;--k) {
        const auto& e=p.events[k]; std::fill_n(next_counts,101,0);
        const auto* potential=potentials[k]; double later_cash[101]{};
        for(int z=e.minimum_left;z<=e.maximum_left;++z)
            later_cash[z]=transact(p.product,anchors[k]-z+e.rival,0,e.rival_after).rival_cash;
        for(int q=k?0:stock;q<=(k?p.capacity:stock);++q) {
            std::array<Transaction,201> trades;
            if(e.rival) trade_table(p.product,anchors[k]-q,e.rival,trades);
            for(int z=e.minimum_left;z<=e.maximum_left;++z) {
                if(!e.trade_allowed && z!=q) continue;
                const int after=z+e.net_receipt_next;
                if(after<0 || after>p.capacity) continue;
                const double own=e.rival?trades[100+q-z].own_cash:potential[q]-potential[z];
                const double rival=(e.rival?trades[100+q-z].rival_cash:0)+later_cash[z];
                for(int n=0;n<counts[after];++n) {
                    if(out.transitions++>=p.transition_limit) { out.complete=false; return out; }
                    const auto tail=cash_current_[after][n];
                    double need=std::max(-e.cash_available,tail.required)-own;
                    const double value=own-p.rival_weight*rival+tail.value;
                    if(!k) {
                        if(need<=1e-8) out.value[z]=std::max(out.value[z],value);
                        continue;
                    }
                    // Every reachable incoming path has at least this cash.
                    // Requirements below that bound are the same constraint.
                    need=std::max({need,wealth_floor[k]-potential[q],-p.events[k-1].cash_available});
                    auto* labels=cash_next_[q]; auto& count=next_counts[q]; bool dominated=false;
                    for(int j=0;j<count;++j) if(labels[j].required<=need+1e-8 && labels[j].value>=value-1e-8) {
                        dominated=true; break;
                    }
                    if(dominated) continue;
                    for(int j=0;j<count;) {
                        if(need<=labels[j].required+1e-8 && value>=labels[j].value-1e-8) labels[j]=labels[--count];
                        else ++j;
                    }
                    if(count==CASH_LABELS) { out.complete=false; return out; }
                    labels[count++]={need,value};
                }
            }
        }
        if(k) for(int q=0;q<=p.capacity;++q) {
            counts[q]=next_counts[q]; std::copy_n(cash_next_[q],counts[q],cash_current_[q]);
        }
    }
    return out;
}
}
