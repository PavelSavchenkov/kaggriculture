#include "policy.hpp"
#include "verify.hpp"
#include "placement.hpp"
#include "day_jobs.hpp"
#include "routes.hpp"
#include "agents/common/runtime/observation_builder.hpp"
#include <iostream>
#include <memory>
#include <cstdlib>
using namespace kag;
using namespace kag::agents::day_policy_contract;
void require(bool value,const char* message) { if(!value){std::cerr<<message<<'\n';std::exit(1);} }
DayInput empty() {
    DayInput in;
    for(int c=0;c<100;++c){in.grid[c].kind=quadrant_of(c%10,c/10,10)==0?T_EMPTY:T_LOCKED;in.grid[c].max_lifespan_step=INT_MAX;}
    return in;
}
void crop(DayInput& in,int c,int item,int age,int yield) {
    auto& t=in.grid[c]; t.kind=T_PLANT;t.what=item;t.planted_day=-age;t.yield_units=yield;
}
SolveResult replay(const DayInput& in, std::initializer_list<UnitAction> actions) {
    SolveResult r; r.status=SolveStatus::Success;
    auto sim=detail::initial_state(in);int h=0, receipts[N_PRODUCTS]{};
    for(auto a:actions)r.schedule[h++].units[0]=a;
    for(h=0;h<24;++h) {
        auto& a=r.schedule[h];a.n_units=1;a.finalize();
        const auto unit=a.units[0];
        for(int it=0;it<N_PRODUCTS;++it) {
            if(unit.op==OP_DROP)receipts[it]+=sim.st.farms[0].inv[0][it];
            r.receipts[h][it]=receipts[it];
        }
        detail::advance(sim,a,h);
    }
    r.state=detail::export_state(sim);
    std::copy_n(sim.st.farms[0].produced,N_PRODUCTS,r.production);return r;
}
int main() {
    auto solver=std::make_unique<Solver>();
    auto run=[&](DayInput in,const char* name) {
        auto r=solver->solve(in); std::cout<<name<<" status="<<int(r.status)<<" hires="<<r.hires<<" us="<<r.microseconds<<'\n';
        require(r.status==SolveStatus::Success,name);require(verify(in,r).valid,"independent verification");return r;
    };
    auto in=empty(); auto r=run(in,"empty");require(r.hires==0,"empty hires");
    in=empty(); crop(in,43,WHEAT,2,2);in.events[43]=Water|Harvest; run(in,"water harvest no returns");
    for(int h=6;h<24;++h)in.returns[h][WHEAT]=2;
    r=run(in,"partial wheat return"); require(r.receipts[23][WHEAT]==2,"exact return");
    in=empty();in.buy_seeds[WHEAT]=3;in.establish_count=3;
    for(int j=0;j<3;++j)in.establish[j]={WHEAT,Water};run(in,"new crops");
    in=empty();in.buy_animals[0]=1;in.buy_wheat[0]=1;in.establish_count=1;in.establish[0]={GOOSE,Feed|Care};run(in,"new animal");
    in=empty();crop(in,43,WHEAT,2,2);in.events[43]=Fertilize|Water|Harvest;
    auto& a=in.grid[34];a.kind=T_PASTURE;a.has_animal=true;a.what=COW;a.fertilizer_available=true;in.events[34]=CollectFertilizer;
    run(in,"field fertilizer");in.events[34]=0;require(solver->solve(in).status==SolveStatus::InvalidInput,"missing fertilizer source");
    in=empty();crop(in,0,WHEAT,2,2);in.events[0]=Harvest;for(int h=0;h<24;++h)in.returns[h][WHEAT]=1;
    require(solver->solve(in).status==SolveStatus::NoScheduleFound,"impossible deadline");
    in=empty();in.grid[24].kind=T_PASTURE;in.grid[24].has_animal=true;in.grid[24].what=COW;
    in.grid[24].yield_units=2;in.events[24]=Harvest;
    for(int h=2;h<24;++h)in.returns[h][MILK]=2;
    r=solver->solve(in);require(r.status==SolveStatus::NoScheduleFound && r.attempts==0,"reject unreachable field return before route search");
    in=empty();crop(in,43,WHEAT,2,2);in.events[43]=Fertilize;
    in.grid[44].kind=T_PASTURE;in.grid[44].has_animal=true;in.grid[44].what=COW;
    in.grid[44].fertilizer_available=true;in.events[44]=CollectFertilizer;in.returns[23][FERTILIZER]=1;
    r=solver->solve(in);require(r.status==SolveStatus::NoScheduleFound && r.attempts==0,"fertilizer cannot be both applied and returned");
    in=empty();in.buy_wheat[7]=3;in.land_hour=9;run(in,"timed purchases");
    in=empty();
    for(int c:{44,43}){auto& t=in.grid[c];t.kind=T_PASTURE;t.has_animal=true;t.what=COW;t.fertilizer_available=true;in.events[c]=CollectFertilizer;}
    crop(in,24,WHEAT,1,1);in.events[24]=Water;
    for(int h=1;h<24;++h)in.returns[h][FERTILIZER]=h<5?1:2;
    r=run(in,"early repeated returns then work");require(r.hires==0,"farmer alone can execute repeated returns");
    auto second=std::make_unique<Solver>();auto again=second->solve(in);
    for(int h=0;h<24;++h){require(r.schedule[h].n_units==again.schedule[h].n_units,"independent deterministic units");for(int u=0;u<r.schedule[h].n_units;++u){auto a=r.schedule[h].units[u],b=again.schedule[h].units[u];require(a.op==b.op && a.arg==b.arg && a.n==b.n,"independent deterministic action");}}
    auto wrong=r;wrong.receipts[23][FERTILIZER]++;require(!verify(in,wrong).valid,"reject falsified receipts");
    wrong=r;wrong.production[FERTILIZER]++;require(!verify(in,wrong).valid,"reject falsified production");
    wrong=r;wrong.hires=14;require(!verify(in,wrong).valid,"reject hire mismatch");
    in=empty();crop(in,44,WHEAT,2,2);in.events[44]=Harvest;for(int h=1;h<24;++h)in.returns[h][WHEAT]=1;
    r=run(in,"bounded receipt with excess cargo");wrong=r;
    for(auto& a:wrong.schedule)for(int u=0;u<a.n_units;++u)if(a.units[u].op==OP_PLACE)a.units[u].n=2;
    require(!verify(in,wrong).valid,"reject excess deposits");
    for(auto& a:wrong.schedule)for(int u=0;u<a.n_units;++u)if(a.units[u].op==OP_PLACE)a.units[u]={OP_DROP,0,1};
    require(!verify(in,wrong).valid,"reject excess cargo in DROP");
    in=empty();in.grid[44].kind=T_PASTURE;in.grid[44].has_animal=true;in.grid[44].what=COW;
    in.grid[44].yield_units=2;in.grid[44].fertilizer_available=true;in.events[44]=Harvest|CollectFertilizer;
    for(int h=3;h<24;++h){in.returns[h][MILK]=2;in.returns[h][FERTILIZER]=1;}
    r=replay(in,{{OP_HARVEST,0,1},{OP_COLLECT_FERTILIZER,0,1},{OP_DROP,0,1}});
    require(verify(in,r).valid,"combined bounded DROP");
    in.shed[WHEAT]=1;
    r=replay(in,{{OP_PICKUP,WHEAT,1},{OP_HARVEST,0,1},{OP_COLLECT_FERTILIZER,0,1},{OP_DROP,0,1}});
    require(!verify(in,r).valid,"reject unrequested wheat in combined DROP");
    in=empty();in.returns[1][WHEAT]=1;require(solver->solve(in).status==SolveStatus::InvalidInput,"reject noncumulative returns");
    in=empty();for(int c=0;c<100;++c)if(in.grid[c].kind!=T_LOCKED)crop(in,c,WHEAT,2,2);
    in.events[0]=Harvest;in.buy_seeds[WHEAT]=1;in.establish_count=1;in.establish[0]={WHEAT,Water};
    for(int h=20;h<24;++h)in.returns[h][WHEAT]=2;
    SolveOptions bounded;bounded.max_hires=0;bounded.variants=16;
    r=solver->solve(in,bounded);require(r.status==SolveStatus::Success && verify(in,r).valid,"distant harvest, reuse and return with farmer only");
    in=empty();for(int c=0;c<100;++c)if(in.grid[c].kind!=T_LOCKED)crop(in,c,WHEAT,2,2);
    in.land_hour=22;in.buy_seeds[WHEAT]=1;in.establish_count=1;in.establish[0]={WHEAT,0};
    r=solver->solve(in,bounded);require(r.status==SolveStatus::Success && verify(in,r).valid,"travel before late land unlock");
    in=empty();
    for(int c=0;c<100;++c)if(quadrant_of(c%10,c/10,10)<2)in.grid[c].kind=T_EMPTY;
    for(int c:{35,43}){auto& t=in.grid[c];t.kind=T_PASTURE;t.has_animal=true;t.what=SHEEP;t.yield_units=c==35?6:4;in.events[c]=Harvest;}
    for(int h=3;h<24;++h)in.returns[h][WOOL]=4;
    auto source_sim=detail::initial_state(in);auto source_observation=agent::runtime::make_observation(source_sim,0);
    DayPlan sources;sources.day=detail::CALENDAR;sources.hires=1;sources.first_wave=1;sources.count=2;
    for(int j=0;j<2;++j){auto& job=sources.jobs[j];job.tile=job.key=j?43:35;job.count=1;job.steps[0]={OP_HARVEST,0,1};}
    DayReturns quota;std::copy_n(&in.returns[0][0],24*N_PRODUCTS,&quota.target[0][0]);
    auto nearest=sources;add_deliveries(source_observation,nearest,&quota,0,false);
    require(nearest.jobs[2].predecessor==0,"reference nearest-source tie");
    add_deliveries(source_observation,sources,&quota);
    require(sources.jobs[2].predecessor==1,"hour-three return uses reachable source");
    r=run(in,"early return source with farmer birth advantage");
    require(r.receipts[3][WOOL]==4,"native hour-three wool return");
    in=empty();
    for(int c=0;c<100;++c)if(quadrant_of(c%10,c/10,10)<2)crop(in,c,WHEAT,2,2);
    for(int c:{42,43,44}){in.grid[c]=Tile{};in.grid[c].max_lifespan_step=INT_MAX;}
    in.land_hour=19;in.buy_seeds[WHEAT]=3;in.establish_count=3;
    for(int j=0;j<3;++j)in.establish[j]={WHEAT,Water};
    auto land_sim=detail::initial_state(in);auto land_observation=agent::runtime::make_observation(land_sim,0);
    DayPlan land;land.relocate_new=true;land.buy_land=1;land.land_hour=19;land.count=3;
    for(int j=0;j<3;++j){auto& job=land.jobs[j];job.new_site=true;job.count=2;job.steps[0]={OP_PLANT,WHEAT,1};job.steps[1]={OP_WATER,0,1};}
    place_day(land_observation,land);
    for(int j=0;j<3;++j)require(in.grid[land.jobs[j].tile].kind!=T_LOCKED,"prefer available land for today's planting");
    run(in,"late land purchase without delaying available planting");
    in=empty();crop(in,34,MELON,10,5);in.events[34]=Water|Harvest;
    for(int h=4;h<24;++h)in.returns[h][MELON]=6;
    auto cooperative=detail::initial_state(in);SolveResult simultaneous;simultaneous.status=SolveStatus::Success;simultaneous.hires=1;
    simultaneous.schedule[0].n_orders=1;simultaneous.schedule[0].orders[0]={M_HIRE,0,1};
    simultaneous.schedule[0].units[0]={OP_NORTH,0,1};
    simultaneous.schedule[1].units[1]={OP_NORTH,0,1};
    simultaneous.schedule[2].units[0]={OP_WATER,0,1};simultaneous.schedule[2].units[1]={OP_HARVEST,0,1};
    simultaneous.schedule[3].units[1]={OP_SOUTH,0,1};simultaneous.schedule[4].units[1]={OP_PLACE,MELON,6};
    for(int h=0;h<24;++h){auto& action=simultaneous.schedule[h];action.n_units=cooperative.st.farms[0].n_units;action.finalize();
        if(h>=4)simultaneous.receipts[h][MELON]=6;detail::advance(cooperative,action,h);}
    simultaneous.state=detail::export_state(cooperative);std::copy_n(cooperative.st.farms[0].produced,N_PRODUCTS,simultaneous.production);
    require(verify(in,simultaneous).valid,"two workers water and harvest in the same hour");
    auto depot_sim=detail::initial_state(empty());auto& carrier=depot_sim.st.farms[0];
    carrier.inv[0][FERTILIZER]=2;carrier.inv[0][MILK]=3;
    carrier.inv_nkeys[0]=2;carrier.inv_keys[0][0]=FERTILIZER;carrier.inv_keys[0][1]=MILK;
    DayPlan depot_plan;depot_plan.day=detail::CALENDAR;depot_plan.hires=0;depot_plan.trade=false;depot_plan.count=1;
    auto& request=depot_plan.jobs[0];request.tile=44;request.depot=true;request.count=2;
    request.steps[0]={OP_PLACE,FERTILIZER,1};request.steps[1]={OP_PLACE,MILK,3};
    auto dispatcher=std::make_unique<Agent>();dispatcher->reset(agent::runtime::make_agent_init(depot_sim,0));dispatcher->set_plan(depot_plan);
    DayReturns depot_returns;for(int h=0;h<24;++h){depot_returns.target[h][MILK]=3;if(h>=1)depot_returns.target[h][FERTILIZER]=1;}
    dispatcher->set_returns(depot_returns);Options dispatch;dispatch.route_rounds=-1;dispatch.deadline_deposits=true;dispatcher->set_options(dispatch);
    Action drop;dispatcher->act(agent::runtime::make_observation(depot_sim,0),{},drop);
    require(drop.units[0].op==OP_PLACE && drop.units[0].arg==MILK && drop.units[0].n==3,"partial deposit serves earliest requested deadline");
    detail::advance(depot_sim,drop,0);dispatcher->act(agent::runtime::make_observation(depot_sim,0),{},drop);
    require(drop.units[0].op==OP_PLACE && drop.units[0].arg==FERTILIZER && drop.units[0].n==1,"retain surplus fertilizer after bounded deposit");
    bounded.max_hires=14;
    require(solver->solve(empty(),bounded).status==SolveStatus::InvalidInput,"reject worker cap above fixed search buffers");
    auto crowded=detail::initial_state(empty());
    Action hire;hire.n_units=1;hire.n_orders=10;
    for(int k=0;k<10;++k)hire.orders[k]={M_HIRE,0,1};hire.finalize();crowded.step(hire,Action{});
    hire.clear();hire.n_units=11;hire.n_orders=4;
    for(int k=0;k<4;++k)hire.orders[k]={M_HIRE,0,1};hire.finalize();crowded.step(hire,Action{});
    auto executor=std::make_unique<Agent>();executor->reset(agent::runtime::make_agent_init(crowded,0));
    Action fallback;executor->act(agent::runtime::make_observation(crowded,0),{},fallback);
    require(fallback.n_units==15 && fallback.metadata_ready,"safe fallback beyond worker contract");
    for(int u=0;u<15;++u)require(fallback.units[u].op==OP_PASS,"out-of-contract worker fallback");
    for(auto effort:{SearchEffort::Fast,SearchEffort::Compact,SearchEffort::Balanced,SearchEffort::Full,SearchEffort::Classic}) {
        SolveOptions profile;profile.effort=effort;
        auto sample=empty();sample.buy_animals[0]=1;sample.buy_wheat[0]=1;sample.establish_count=1;sample.establish[0]={GOOSE,Feed|Care};
        auto answer=solver->solve(sample,profile);
        require(answer.status==SolveStatus::Success && verify(sample,answer).valid,"every search profile respects event and purchase contract");
    }
    auto throughput_profile=day_policy_80p();
    auto throughput_sample=empty();throughput_sample.buy_animals[0]=1;throughput_sample.buy_wheat[0]=1;
    throughput_sample.establish_count=1;throughput_sample.establish[0]={GOOSE,Feed|Care};
    auto throughput_answer=solver->solve(throughput_sample,throughput_profile);
    require(throughput_answer.status==SolveStatus::Success && throughput_answer.hires==11 &&
        verify(throughput_sample,throughput_answer).valid,"day_policy_80p uses its fixed workforce and returns a verified schedule");
    throughput_answer=solver->solve(empty(),throughput_profile);
    require(throughput_answer.status==SolveStatus::Success && throughput_answer.hires==11,
        "day_policy_80p keeps its fixed workforce on an empty day");
    throughput_profile.max_hires=10;
    require(solver->solve(empty(),throughput_profile).status==SolveStatus::InvalidInput,"day_policy_80p requires exactly 11 hires");
    throughput_profile=day_policy_80p();throughput_profile.minimize_hires=true;
    require(solver->solve(empty(),throughput_profile).status==SolveStatus::InvalidInput,"day_policy_80p disables hire minimization");
    SolveOptions invalid_profile;invalid_profile.effort=static_cast<SearchEffort>(99);
    require(solver->solve(empty(),invalid_profile).status==SolveStatus::InvalidInput,"reject unknown search profile");
    invalid_profile=SolveOptions{};invalid_profile.placement=static_cast<PlacementStyle>(99);
    require(solver->solve(empty(),invalid_profile).status==SolveStatus::InvalidInput,"reject unknown placement style");
    in=empty();in.buy_seeds[MELON]=12;in.buy_seeds[WHEAT]=7;
    in.buy_animals[COW-GOOSE]=2;in.buy_animals[SHEEP-GOOSE]=2;in.buy_wheat[0]=4;
    for(int k=0;k<7;++k)in.establish[in.establish_count++]={WHEAT,Water};
    for(int k=0;k<12;++k)in.establish[in.establish_count++]={MELON,Water};
    for(int item:{COW,COW,SHEEP,SHEEP})in.establish[in.establish_count++]={uint8_t(item),Feed|Care};
    r=run(in,"mixed opening with room for future animals");
    int melon_distance=0,wheat_distance=0,near_free=0;
    for(int c=0;c<100;++c) {
        const auto& t=r.state.grid[c];
        if(t.kind==T_PLANT && t.what==MELON)melon_distance+=shed_distance(c);
        if(t.kind==T_PLANT && t.what==WHEAT)wheat_distance+=shed_distance(c);
        near_free+=t.kind==T_EMPTY && shed_distance(c)<=2;
    }
    require(melon_distance*7<wheat_distance*12,"opening wheat leaves melon sites closer to shed");
    require(near_free>=2,"opening leaves short trips for two later animals");
    in=empty();in.buy_seeds[WHEAT]=25;in.establish_count=25;
    for(auto& product:in.establish)product={WHEAT,Water};
    run(in,"soft reserve allows a full crop-only quadrant");
    in=empty();in.grid[44].kind=T_PASTURE;in.grid[44].has_animal=true;in.grid[44].what=COW;
    in.land_hour=10;in.buy_animals[SHEEP-GOOSE]=1;in.buy_wheat[0]=1;
    in.establish_count=1;in.establish[0]={SHEEP,Feed|Care};
    r=run(in,"permanent animal waits for central new land");
    require(r.state.grid[45].has_animal && r.state.grid[45].what==SHEEP,"animal uses new shed-access site");
    std::cout<<"All contract regressions passed\n";
}
