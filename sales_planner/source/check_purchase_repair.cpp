#include "purchase_repair.hpp"
#include "case.hpp"

using namespace sales_planner;
bool same(const Orders& a,const Orders& b) {
    if(a.count!=b.count)return false;
    for(int k=0;k<a.count;++k)
        if(a.values[k].op!=b.values[k].op || a.values[k].item!=b.values[k].item || a.values[k].n!=b.values[k].n)return false;
    return true;
}
int main(int argc,char** argv) {
    const MarketRules rules;
    PlannerObservation obs;obs.own.cash=100;obs.inventory.fill(10000);
    std::vector<CalendarTurn> plan(3);
    plan[1].before_market.push_back({Flow::withdraw,kag::WHEAT,0,2});
    PurchaseRepairStats stats;
    Orders empty;
    auto repaired=repair_purchases(obs,plan,empty,rules,stats);
    require(repaired.count==1 && repaired.values[0].op==kag::M_BUY_PRODUCT && repaired.values[0].n==2,"missed next-turn wheat repair");
    Orders buying_one;buying_one.add(kag::M_BUY_PRODUCT,kag::WHEAT,1);
    repaired=repair_purchases(obs,plan,buying_one,rules,stats);
    require(repaired.count==2 && repaired.values[1].n==1,"current full-fill bound overbought wheat");
    Orders buying_two;buying_two.add(kag::M_BUY_PRODUCT,kag::WHEAT,2);
    require(same(repair_purchases(obs,plan,buying_two,rules,stats),buying_two),"duplicated planned input");
    obs.own.cash=10;
    require(same(repair_purchases(obs,plan,empty,rules,stats),empty),"accepted an unfunded partial repair");
    obs.own.cash=100;obs.resources.add(0,kag::WHEAT,2);
    plan[0].after_market.push_back({Flow::drop_all,0,0,0});
    require(same(repair_purchases(obs,plan,empty,rules,stats),empty),"ignored pre-deadline deposit");
    int cases=0,turns=0;
    for(int i=1;i<argc;++i) {
        auto c=read_case(argv[i]);certify_commitments(c);
        auto state=c.initial.financial;auto resources=c.initial.resources;
        const auto rule=rules_for(c.config);
        std::array<std::vector<CalendarTurn>,2> calendars;
        for(const auto& t:c.turns)for(int p=0;p<2;++p)calendars[p].push_back(t.calendar[p]);
        for(int t=0;t<int(c.turns.size());++t) {
            const auto& step=c.turns[t];
            state.shops=step.shops;state.n_shops=step.n_shops;
            for(int p=0;p<2;++p)apply(state.accounts[p],resources[p],step.calendar[p].before_market,rule.capacity);
            for(int p=0;p<2;++p) {
                PlannerObservation o{t,state.accounts[p],resources[p],state.inventory,state.shops,state.n_shops,state.accounts[p^1].cash};
                PurchaseRepairStats s;
                const auto next=repair_purchases(o,calendars[p],step.original_orders[p],rule,s);
                require(same(next,step.original_orders[p]),"repair changed a source-feasible calendar");
            }
            trade(state,step.original_orders,rule);consume(state,rule);
            for(int p=0;p<2;++p)apply(state.accounts[p],resources[p],step.calendar[p].after_market,rule.capacity);
            advance(state,rule);++turns;
        }
        ++cases;
    }
    std::printf("{\"synthetic_checks\":5,\"paired_cases\":%d,\"turns\":%d,\"source_order_changes\":0}\n",cases,turns);
}
