#include "funding.hpp"
#include <algorithm>
#include <vector>

namespace kag::day_compiler {
FundingProposal::FundingProposal() = default;
namespace {
int count_events(const worker::DayInput& input,int flag) {
    int count=0;
    for(int events:input.events) count+=bool(events&flag);
    for(int n=0;n<input.establish_count;++n) count+=bool(input.establish[n].events&flag);
    return count;
}
int hire_limit(double cash,int multiplier,int existing=0) {
    int count=existing;
    while(count+1<MAX_UNITS && cash>=multiplier*double(fib(count))) {
        cash-=multiplier*double(fib(count)); ++count;
    }
    return count;
}
}
bool propose_funding(const Observation& dawn,const detail::BoundIntent& intent,const Workload& work,
                     FundingProposal& out,int funding_hour,const Configuration& config) {
    out=FundingProposal(); out.input=work.input; out.constraints=work.constraints;
    auto& input=out.input; auto& constraints=out.constraints;
    const int start=input.start_hour,existing=input.worker_count-1;
    if(funding_hour<start || funding_hour>=input.hours ||
       (funding_hour==input.hours-1 && (!start || funding_hour!=start))) return false;
    std::fill_n(&input.buy_seeds[0][0],24*N_CROPS,0); std::fill_n(&input.buy_animals[0][0],24*3,0);
    std::fill_n(input.buy_wheat,24,0); std::fill_n(input.buy_fertilizer,24,0); input.land_hour=-1;
    int carried_wheat=0,carried_fertilizer=0;
    for(int u=0;u<input.worker_count;++u) {
        carried_wheat+=input.workers[u].inventory[WHEAT]; carried_fertilizer+=input.workers[u].inventory[FERTILIZER];
    }
    out.funding_hour=funding_hour;
    out.dawn_reserve[WHEAT]=std::min(input.shed[WHEAT],std::max(0,count_events(input,worker::Feed)-carried_wheat));
    out.dawn_reserve[FERTILIZER]=std::min(input.shed[FERTILIZER],std::max(0,count_events(input,worker::Fertilize)-carried_fertilizer));
    auto farm=own_farm(dawn); OrderLedger capital(dawn,farm,config);
    // Safe surplus is available funding, not a mandate to liquidate it. The
    // complete candidate's seller chooses the actual sequence and quantities.
    for(int p=0;p<N_PRODUCTS;++p) {
        const int quantity=std::max(0,capital.stock[p]-out.dawn_reserve[p]);
        if(quantity && !capital.append({M_SELL,uint8_t(p),quantity})) return false;
    }
    out.dawn_liquid_cash=capital.cash;
    int seed[N_CROPS],animal[3],wheat=work.wheat_need,fertilizer=work.fertilizer_need;
    std::copy_n(work.seed_need,N_CROPS,seed); std::copy_n(work.animal_need,3,animal);
    double total=0;
    for(int p=0;p<N_CROPS;++p) total+=seed[p]*CROPS[p].seed;
    for(int a=0;a<3;++a) total+=animal[a]*ANIMALS[a].cost;
    total-=transact(WHEAT,capital.inventory[WHEAT],-wheat,0).own_cash;
    total-=transact(FERTILIZER,capital.inventory[FERTILIZER],-fertilizer,0).own_cash;
    if(intent.buy_land) total+=LAND_PRICES[dawn.self().n_quadrants-1];
    out.mandatory_bill=total;
    double available=capital.cash;
    if(funding_hour==start) {
        if(total>available) return false;
        constraints.max_hires=hire_limit(available-total,config.hire_mult,existing);
        for(int p=0;p<N_CROPS;++p) input.buy_seeds[start][p]=seed[p];
        for(int a=0;a<3;++a) input.buy_animals[start][a]=animal[a];
        input.buy_wheat[start]=wheat; input.buy_fertilizer[start]=fertilizer;
        if(intent.buy_land) input.land_hour=start;
    } else {
        // Keep the first workers funded from dawn capital. Purchases that cannot
        // fit wait for the selected financing event; its returns are explicit.
        const int first_hires=std::max(existing,std::min(3,hire_limit(available,config.hire_mult,existing)));
        for(int i=existing;i<first_hires;++i) available-=config.hire_mult*double(fib(i));
        auto fixed=[&](int quantity,int price,int& early,int& late) {
            early=std::min(quantity,int(available/price)); available-=early*price; late=quantity-early;
        };
        // Retained service inputs precede optional expansion purchases.
        auto commodity=[&](int p,int need,int& early,int& late) {
            int inventory=capital.inventory[p];
            while(early<need) {
                const int price=market_price(p,inventory-1); if(available<price) break;
                available-=price; --inventory; ++early;
            }
            late=need-early;
        };
        commodity(WHEAT,wheat,input.buy_wheat[start],input.buy_wheat[funding_hour]);
        commodity(FERTILIZER,fertilizer,input.buy_fertilizer[start],input.buy_fertilizer[funding_hour]);
        for(int p=0;p<N_CROPS;++p) fixed(seed[p],CROPS[p].seed,input.buy_seeds[start][p],input.buy_seeds[funding_hour][p]);
        for(int a=0;a<3;++a) fixed(animal[a],ANIMALS[a].cost,input.buy_animals[start][a],input.buy_animals[funding_hour][a]);
        if(intent.buy_land) input.land_hour=available>=LAND_PRICES[dawn.self().n_quadrants-1]?start:funding_hour;
        int inventory[N_PRODUCTS]; std::copy_n(capital.inventory,N_PRODUCTS,inventory);
        double future_cash=capital.cash;
        for(int p=0;p<N_PRODUCTS;++p) {
            const int reserved=p==WHEAT?work.field_wheat_used:p==FERTILIZER?work.field_fertilizer_used:0;
            const int quantity=std::max(0,work.field_output[p]-reserved);
            for(int h=start;h<funding_hour;++h) inventory[p]-=demand(dawn,config,p,dawn.step+h-start);
            future_cash+=transact(p,inventory[p],quantity,0).own_cash;
        }
        if(future_cash<total) return false;
        constraints.max_hires=hire_limit(future_cash-total,config.hire_mult,existing);
        for(int u=first_hires+1;u<MAX_UNITS;++u) constraints.hire_not_before[u]=funding_hour;
    }
    // Normal nights deposit remaining cargo automatically. Do not turn every
    // selected harvest into a mandatory trip. Financing and terminal liquidation
    // still need explicit receipts; optional returns are separate candidates.
    const int due=funding_hour>start?funding_hour:input.hours-1;
    if(funding_hour>start || dawn.day==29) for(int p=0;p<N_PRODUCTS;++p) {
        const int consumed=p==WHEAT?work.field_wheat_used:p==FERTILIZER?work.field_fertilizer_used:0;
        const int quantity=std::max(0,work.field_output[p]-consumed);
        for(int h=due;h<24;++h) input.returns[h][p]=std::max(input.returns[h][p],quantity);
    }
    constraints.reserved_order_slots=1;
    constraints.max_attempts=128;
    return true;
}

bool propose_cashflow(const Observation& dawn,const detail::BoundIntent& intent,const Workload& work,
                      FundingProposal& out,int hires,int receipt_slack,const Configuration& config,const FundingScenario& scenario) {
    const int start=work.input.start_hour,existing=work.input.worker_count-1;
    if(hires<existing || hires>=MAX_UNITS || receipt_slack<0) return false;
    out=FundingProposal(); out.input=work.input; out.constraints=work.constraints;
    auto& input=out.input; auto& constraints=out.constraints;
    std::fill_n(&input.buy_seeds[0][0],24*N_CROPS,0); std::fill_n(&input.buy_animals[0][0],24*3,0);
    std::fill_n(input.buy_wheat,24,0); std::fill_n(input.buy_fertilizer,24,0); input.land_hour=-1;
    int proposed_returns[24][N_PRODUCTS]{},carried[N_PRODUCTS]{};
    for(int u=0;u<input.worker_count;++u)for(int p=0;p<N_PRODUCTS;++p)carried[p]+=input.workers[u].inventory[p];
    constraints.max_hires=constraints.first_hires=hires;
    constraints.hire_not_before.fill(23); constraints.reserved_order_slots=2;
    constraints.max_attempts=32;
    out.dawn_reserve[WHEAT]=std::min(input.shed[WHEAT],std::max(0,count_events(input,worker::Feed)-carried[WHEAT]));
    out.dawn_reserve[FERTILIZER]=std::min(input.shed[FERTILIZER],std::max(0,count_events(input,worker::Fertilize)-carried[FERTILIZER]));
    auto quote=dawn;
    for(int p=0;p<N_PRODUCTS;++p) {
        if(scenario.rival_sales[start][p]<0) return false;
        quote.market.inventory[p]=transact(p,quote.market.inventory[p],0,scenario.rival_sales[start][p]).inventory;
    }
    OrderLedger capital(quote,own_farm(dawn),config);
    for(int p=0;p<N_PRODUCTS;++p) {
        const int spare=std::max(0,capital.stock[p]-out.dawn_reserve[p]);
        if(spare && !capital.append({M_SELL,uint8_t(p),spare})) return false;
    }
    out.dawn_liquid_cash=capital.cash;
    int seeds[N_CROPS],animals[3],wheat=work.wheat_need,fertilizer=work.fertilizer_need;
    std::copy_n(work.seed_need,N_CROPS,seeds); std::copy_n(work.animal_need,3,animals);
    bool land=intent.buy_land;
    double cash=capital.cash; int inventory[N_PRODUCTS]; std::copy_n(capital.inventory,N_PRODUCTS,inventory);
    int hired=existing;
    struct Source { int cell,product,quantity,earliest,work,event; bool used=false; };
    std::vector<Source> sources;
    const int farmer=dawn.self().pos_y[0]*BOARD+dawn.self().pos_x[0];
    for(int u=0;u<input.worker_count;++u) {
        const int d=shed_distance(input.workers[u].tile),earliest=start+d+(d?receipt_slack:0);
        if(earliest>=input.hours-1)continue;
        for(int p=0;p<N_PRODUCTS;++p) if(input.workers[u].inventory[p])
            sources.push_back({100+u,p,input.workers[u].inventory[p],earliest,d+1,0});
    }
    for(int c=0;c<100;++c) {
        const auto& tile=dawn.self().tiles[c/10][c%10];
        const int distance_to_shed=shed_distance(c);
        int approach=hires>existing?1+distance_to_shed:24;
        for(int u=0;u<input.worker_count;++u)approach=std::min(approach,distance(input.workers[u].tile,c));
        // Keep the established optimistic dawn proposal exactly; suffixes use
        // the current workforce and its real starting positions.
        const int arrival=start+(start?approach:std::min(distance(farmer,c),1+distance_to_shed));
        const int slack=distance_to_shed?receipt_slack:0;
        if(tile.has_animal && tile.fertilizer_available && arrival+1+distance_to_shed+slack<input.hours-2)
            sources.push_back({c,FERTILIZER,1,arrival+1+distance_to_shed+slack,2*distance_to_shed+2,worker::CollectFertilizer});
        if(!tile.yield_units || (!tile.has_animal && tile.kind!=T_PLANT)) continue;
        if(!tile.has_animal && (dawn.day-tile.planted_day<CROPS[tile.what].first_yield_day ||
            (!CROPS[tile.what].ongoing && !(input.events[c]&worker::Harvest) && intent.crops[c].mode!=CropMode::Retire))) continue;
        const int p=tile.has_animal?int(ANIMALS[tile.what-GOOSE].product):int(tile.what);
        int quantity=tile.yield_units,actions=1;
        if(!tile.has_animal && !CROPS[tile.what].ongoing) {
            const int events=input.events[c];
            actions+=bool(events&worker::Water)+bool(events&worker::Fertilize);
            if((events&worker::Water) && dawn.day-tile.planted_day<=CROPS[tile.what].max_yield_day)
                quantity=std::min(CROPS[tile.what].max_yield,quantity+1+int(tile.fertilized_until_day>=dawn.day || (events&worker::Fertilize)));
        }
        int earliest=arrival+actions+distance_to_shed;
        if(distance_to_shed) earliest+=receipt_slack;
        if(earliest>=input.hours-2) continue;
        // Decay concerns the harvest action, not the later shed deposit.
        const int harvest_hour=arrival+actions-1;
        if(tile.max_lifespan_step>=0 && tile.max_lifespan_step<dawn.step+harvest_hour-start) continue;
        sources.push_back({c,p,quantity,earliest,2*distance_to_shed+actions+1,worker::Harvest});
    }
    // Only one source can use the farmer's earlier departure. Other first
    // returns start with a hired worker on the following turn. Giving every
    // producer the farmer's arrival time creates impossible financing prefixes.
    int first_source=-1; double first_value=-1;
    for(int j=0;j<int(sources.size());++j) {
        const auto& source=sources[j];
        if(start || source.cell>=100)continue;
        if(distance(farmer,source.cell)>=1+shed_distance(source.cell)) continue;
        const double value=transact(source.product,inventory[source.product],source.quantity,0).own_cash/(source.earliest+1);
        if(value>first_value) { first_source=j; first_value=value; }
    }
    for(int j=0;j<int(sources.size());++j) if(!start && j!=first_source && sources[j].cell<100) {
        auto& source=sources[j];
        source.earliest+=std::max(0,1+shed_distance(source.cell)-distance(farmer,source.cell));
    }
    // Do not insist on a mature-farm workforce when the day's realizable output
    // only supports a few workers. The exact cash-flow pass still has to fund it.
    double bill=0;
    for(int p=0;p<N_CROPS;++p) bill+=seeds[p]*CROPS[p].seed;
    for(int a=0;a<3;++a) bill+=animals[a]*ANIMALS[a].cost;
    bill-=transact(WHEAT,inventory[WHEAT],-wheat,0).own_cash;
    bill-=transact(FERTILIZER,inventory[FERTILIZER],-fertilizer,0).own_cash;
    if(land) bill+=LAND_PRICES[dawn.self().n_quadrants-1];
    out.mandatory_bill=bill;
    // Field inputs cannot also finance purchases as sale stock. Redistribution
    // is an input dependency, not an immediate sale: leave the first workers
    // free to make the financing returns before redistributing service inputs.
    int committed[N_PRODUCTS]{};
    committed[WHEAT]=work.field_wheat_used; committed[FERTILIZER]=work.field_fertilizer_used;
    for(auto& source:sources) {
        const int kept=std::min(committed[source.product],source.quantity);
        source.quantity-=kept; committed[source.product]-=kept;
        // A total field balance is not a carrier assignment. Returning these
        // inputs makes them available to whichever workers receive feed/service
        // jobs. They are reserved stock, never virtual sale proceeds.
        if(kept && source.cell<100) {
            out.input_returns[source.product]+=kept;
            const int due=std::max(source.earliest,input.hours/2);
            for(int h=due;h<24;++h) {
                proposed_returns[h][source.product]+=kept;
                input.returns[h][source.product]=std::max(input.returns[h][source.product],proposed_returns[h][source.product]);
            }
        }
    }
    if(committed[WHEAT] || committed[FERTILIZER]) return false;
    const bool first_is_finished=first_source>=0 && sources[first_source].quantity>0 &&
        sources[first_source].product>=CARROT && sources[first_source].product<=WOOL;
    const int first_finished=first_is_finished?sources[first_source].earliest:24;
    std::erase_if(sources,[](const Source& source){return !source.quantity;});
    // A cheap early fertilizer collection must not occupy the farmer needed
    // for the first valuable financing trip. Keep it as later sale capital.
    // Only the tight first-return proposal changes; relaxed proposals remain.
    if(!start && !receipt_slack && first_finished<24) {
        for(auto& source:sources)
            if(source.product==FERTILIZER)source.earliest=std::max(source.earliest,first_finished);
    }
    int available[N_PRODUCTS]{};
    for(const auto& source:sources) available[source.product]+=source.quantity;
    double potential=cash-bill;
    for(int p=0;p<N_PRODUCTS;++p) potential+=transact(p,inventory[p],available[p],0).own_cash;
    if(potential<0) return false;
    hires=std::min(hires,hire_limit(potential,config.hire_mult,existing));
    constraints.max_hires=constraints.first_hires=hires;
    double wages=0; for(int u=existing;u<hires;++u) wages+=config.hire_mult*double(fib(u));
    const double harvest_capital=potential-transact(FERTILIZER,inventory[FERTILIZER],available[FERTILIZER],0).own_cash;
    if(harvest_capital>=wages)
        std::erase_if(sources,[](const Source& source){return source.product==FERTILIZER;});
    int next_source[100+MAX_UNITS]{};
    auto pending=[&] {
        int count=wheat+fertilizer+int(land)+hires-hired;
        for(int n:seeds) count+=n;
        for(int n:animals) count+=n;
        return count;
    };
    // Cash and market inventory are an optimistic financing screen. Actual
    // worker timing, order slots, storage and every purchase are verified later.
    for(int h=start;h<input.hours-1 && pending();++h) {
        // Quote after the proposed rival sales at this hour. That ordering is
        // conservative within this scenario; complete joint execution follows.
        if(h>start) for(int p=0;p<N_PRODUCTS;++p) {
            if(scenario.rival_sales[h][p]<0) return false;
            inventory[p]=transact(p,inventory[p],0,scenario.rival_sales[h][p]).inventory;
        }
        int slots=h>start?0:capital.orders.n_orders;
        auto buy=[&] {
            bool changed=false;
            // Three cheap workers precede inputs; remaining labor precedes
            // expansion, so a farm is not left waiting for its first cash crop.
            auto hire=[&](int target) {
                while(hired<target && slots<8 && cash>=config.hire_mult*double(fib(hired))) {
                    cash-=config.hire_mult*double(fib(hired)); constraints.hire_not_before[++hired]=h;
                    ++slots; changed=true;
                }
            };
            hire(std::min(3,hires));
            auto commodity=[&](int product,int& remaining,int& bought) {
                if(!remaining || slots>=8) return;
                int quantity=0;
                while(quantity<remaining && cash>=market_price(product,inventory[product]-1)) {
                    cash-=market_price(product,inventory[product]-1); --inventory[product]; ++quantity;
                }
                if(quantity) { bought+=quantity; remaining-=quantity; ++slots; changed=true; }
            };
            commodity(WHEAT,wheat,input.buy_wheat[h]); commodity(FERTILIZER,fertilizer,input.buy_fertilizer[h]);
            // Release the financing workforce before spending its working
            // capital on seeds that can wait for those workers' first returns.
            hire(hires);
            for(int p=0;p<N_CROPS && slots<8;++p) if(seeds[p] && cash>=CROPS[p].seed) {
                const int quantity=std::min(seeds[p],int(cash/CROPS[p].seed));
                cash-=quantity*CROPS[p].seed; seeds[p]-=quantity; input.buy_seeds[h][p]+=quantity; ++slots; changed=true;
            }
            if(land && slots<8 && cash>=LAND_PRICES[dawn.self().n_quadrants-1]) {
                cash-=LAND_PRICES[dawn.self().n_quadrants-1]; input.land_hour=h; land=false; ++slots; changed=true;
            }
            for(int a=0;a<3 && slots<8;++a) if(animals[a] && cash>=ANIMALS[a].cost) {
                const int quantity=std::min(animals[a],int(cash/ANIMALS[a].cost));
                cash-=quantity*ANIMALS[a].cost; animals[a]-=quantity; input.buy_animals[h][a]+=quantity; ++slots; changed=true;
            }
            return changed;
        };
        buy();
        while(pending() && slots<8) {
            int selected=-1; double best=-1;
            for(int j=0;j<int(sources.size());++j) {
                const auto& source=sources[j]; if(source.used || source.earliest>h || next_source[source.cell]>h) continue;
                const double value=transact(source.product,inventory[source.product],source.quantity,0).own_cash/source.work;
                if(value>best) { selected=j; best=value; }
            }
            if(selected<0) break;
            auto& source=sources[selected]; source.used=true;
            // One nearby farmer cannot HARVEST and COLLECT on the same turn,
            // then finance both by the same DROP. Keep successive batches apart.
            next_source[source.cell]=h+(source.cell<100?2:0);
            const auto sale=transact(source.product,inventory[source.product],source.quantity,0);
            cash+=sale.own_cash; inventory[source.product]=sale.inventory;
            out.financing_returns[source.product]+=source.quantity;
            if(source.cell<100) input.events[source.cell]|=source.event;
            for(int future=h;future<24;++future) {
                proposed_returns[future][source.product]+=source.quantity;
                input.returns[future][source.product]=std::max(input.returns[future][source.product],proposed_returns[future][source.product]);
            }
            ++slots; buy();
        }
        for(int p=0;p<N_PRODUCTS;++p) inventory[p]-=demand(dawn,config,p,dawn.step+h-start);
    }
    return !pending();
}
}
