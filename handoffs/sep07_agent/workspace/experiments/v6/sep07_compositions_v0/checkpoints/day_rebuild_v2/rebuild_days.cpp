#include "../include/evaluation.hpp"
#include "../league/top_replay_library/source/agent.hpp"
#include "../league/public_router/source/agent.hpp"
#include "day_solver/scheduler.hpp"
#include "day_solver/io.hpp"
#include <filesystem>
#include <iostream>

using namespace compositions;
using namespace kag;
using namespace day_solver;

ManagedTileState managed(const Tile& tile,int day) {
    ManagedTileState result;
    constexpr ManagedTileKind kinds[]={ManagedTileKind::EMPTY,ManagedTileKind::LOCKED,ManagedTileKind::WEED,
        ManagedTileKind::COOP,ManagedTileKind::PASTURE,ManagedTileKind::CROP};
    result.kind=kinds[tile.kind];
    if(tile.kind==T_PLANT || tile.has_animal) {
        result.age_days=day-tile.planted_day;
        result.stored_units=tile.yield_units;result.consecutive_dry_days=tile.consecutive_dry;
    }
    if(tile.kind==T_PLANT) {
        result.crop=tile.what;result.watered_today=tile.watered_today;
        result.fertilizer_days_remaining=std::max(0,tile.fertilized_until_day-day+1);
    }
    if(tile.has_animal) {
        result.animal=tile.what;result.fed_today=tile.fed_today;result.cared_today=tile.cared_today;
        result.pending_care_bonus=tile.pending_care_bonus;result.fertilizer_available=tile.fertilizer_available;
    }
    return result;
}

// Offline source-contract extraction only. Prefix probes repeat the same
// pre-turn state; no copied opponent state is passed to an executable policy.
Sim prefix_phase(const Sim& before,const Action (&actions)[2],int orders,int workers=-1) {
    Sim phase=before;
    if(phase.st.hour==23)phase.st.hour=0;
    Action pair[2]={actions[0],actions[1]};
    for(auto& action:pair) {
        action.n_orders=std::min(action.n_orders,orders);
        if(workers>=0)for(int u=workers;u<action.n_units;++u)action.units[u]={};
        action.finalize();
    }
    phase.step(pair[0],pair[1]);
    return phase;
}

struct RecordedDay {
    Sim start;
    std::array<Action,24> own,rival;
    DayProblem problem;
    int discarded=0;
    explicit RecordedDay(const Sim& sim):start(sim) {
        const auto& farm=sim.st.farms[0];
        for(int y=0;y<10;++y)for(int x=0;x<10;++x)
            problem.start.managed_tiles.push_back({int8_t(x),int8_t(y),managed(farm.tiles[y][x],sim.st.day)});
        std::copy_n(farm.shed,N_ITEMS,problem.start.shed.begin());
        std::copy_n(farm.seeds,N_CROPS,problem.start.seeds.begin());
    }
};

void save_actions(const std::array<Action,24>& actions,const std::filesystem::path& path) {
    std::ofstream file(path);
    for(const auto& action:actions) {
        file<<action.n_units<<' '<<action.n_orders;
        for(int u=0;u<action.n_units;++u)file<<' '<<+action.units[u].op<<' '<<+action.units[u].arg<<' '<<action.units[u].n;
        for(int i=0;i<action.n_orders;++i)file<<' '<<+action.orders[i].op<<' '<<+action.orders[i].item<<' '<<action.orders[i].n;
        file<<'\n';
    }
}

void append_contract(RecordedDay& day,const Sim& before,const Sim& after,const Action (&actions)[2]) {
    const int hour=before.st.hour;
    day.own[hour]=actions[0];day.rival[hour]=actions[1];
    auto& problem=day.problem;
    Action no_market=actions[0];no_market.n_orders=0;
    const auto accepted=before.sanitize_solo_action(0,no_market);
    std::copy_n(accepted.units,accepted.n_units,day.own[hour].units);
    day.own[hour].finalize();
    Action physical[2]={accepted,actions[1]};
    physical[1].n_orders=0;
    auto previous=prefix_phase(before,physical,0,0);
    for(int u=0;u<before.st.farms[0].n_units;++u) {
        auto current=prefix_phase(before,physical,0,u+1);
        const auto action=accepted.units[u];
        const int x=before.st.farms[0].pos_x[u],y=before.st.farms[0].pos_y[u],cell=y*10+x;
        const auto& tile=previous.st.farms[0].tiles[y][x];
        const bool place=action.op==OP_PLACE && is_animal(action.arg) && !tile.has_animal &&
            tile.kind==(action.arg==GOOSE?T_COOP:T_PASTURE);
        const bool field=place || (action.op>=OP_PLANT && action.op<=OP_CARE);
        if(field) {
            auto found=std::find_if(problem.tile_work.begin(),problem.tile_work.end(),[&](const TileWork& work){return work.tile==cell;});
            if(found==problem.tile_work.end()) {problem.tile_work.push_back({int16_t(cell),{}});found=problem.tile_work.end()-1;}
            TileWorkAction work;work.op=action.op;
            if(action.op==OP_PLANT || place)work.arg=action.arg;
            if(action.op==OP_COLLECT_FERTILIZER) {work.output_item=FERTILIZER;work.output_quantity=1;}
            if(action.op==OP_HARVEST) {
                const auto& a=previous.st.farms[0];const auto& b=current.st.farms[0];
                int item=-1,quantity=0;
                for(int i=0;i<N_PRODUCTS;++i)if(b.produced[i]>a.produced[i]) {item=i;quantity+=b.produced[i]-a.produced[i];}
                if(item<0 || quantity<=0)std::abort();
                work.arg=work.output_item=item;work.output_quantity=quantity;
            }
            found->actions.push_back(work);
        }
        previous=std::move(current);
    }
    // Exact accepted orders with their original slots, including simultaneous
    // rival quotes. Public v3 withdrawals require sales before that hour's buys;
    // the original schedule replay below detects unsupported same-hour reuse.
    auto prior=prefix_phase(before,actions,0);
    std::array<int64_t,N_ITEMS> sold{};
    for(int i=0;i<actions[0].n_orders;++i) {
        auto current=prefix_phase(before,actions,i+1);
        const auto& a=prior.st.farms[0];const auto& b=current.st.farms[0];
        const auto order=actions[0].orders[i];
        MarketEvent event;event.hour=hour;event.order_index=i;event.market_op=order.op;
        int quantity=0;
        if(order.op==M_HIRE) {quantity=b.n_units-a.n_units;event.item=-1;problem.worker_count+=quantity;}
        if(order.op==M_BUY_LAND) {quantity=b.n_quadrants-a.n_quadrants;event.item=a.n_quadrants;}
        if(order.op==M_BUY_SEED) {quantity=b.seeds[order.item]-a.seeds[order.item];event.item=order.item;}
        if(order.op==M_BUY_PRODUCT || order.op==M_BUY_ANIMAL) {quantity=b.shed[order.item]-a.shed[order.item];event.item=order.item;}
        if(order.op==M_SELL)sold[order.item]+=b.sold_units[order.item]-a.sold_units[order.item];
        if(quantity>0) {event.quantity=quantity;problem.market_plan.push_back(event);}
        // The semantic contract contains accepted quantities. Reusing a larger
        // failed request could buy/sell more after worker routes are changed.
        const int committed=order.op==M_SELL?b.sold_units[order.item]-a.sold_units[order.item]:quantity;
        day.own[hour].orders[i]=committed>0?Order{order.op,order.item,committed}:Order{};
        prior=std::move(current);
    }
    // Workers cannot act between market slots. Cancel same-hour product buys
    // and sales only in the physical scheduler contract, whose withdrawal
    // phase precedes buys. Preserve every accepted trade in day.own for exact
    // cash/price replay. This keeps opening wheat round trips out of field-work
    // requirements without erasing their economic effects.
    for(auto& event:problem.market_plan)
        if(event.hour==hour && event.market_op==M_BUY_PRODUCT) {
            const int64_t cancel=std::min<int64_t>(event.quantity,sold[event.item]);
            event.quantity-=cancel;sold[event.item]-=cancel;
        }
    std::erase_if(problem.market_plan,[](const MarketEvent& event){return event.quantity==0;});
    for(int item=0;item<N_ITEMS;++item) {
        problem.shed_availability[hour][item]=(hour?problem.shed_availability[hour-1][item]:0)+sold[item];
        day.discarded+=after.st.farms[0].discarded[item]-before.st.farms[0].discarded[item];
    }
    if(hour==23) {
        const auto& farm=after.st.farms[0];
        std::copy_n(farm.shed,N_ITEMS,problem.end_shed.begin());
        std::copy_n(farm.seeds,N_CROPS,problem.end_seeds.begin());
        for(int cell=0;cell<100;++cell) {
            auto tile=farm.tiles[cell/10][cell%10];
            // Remove only stochastic weeds born on previously empty squares.
            if(tile.kind==T_WEED && prior.st.farms[0].tiles[cell/10][cell%10].kind==T_EMPTY)tile={};
            EndTileRequirement end;end.tile=cell;end.exact_state=managed(tile,after.st.day);
            problem.required_end_tiles.push_back(end);
        }
    }
}

int main(int argc,char** argv) {
    if(argc!=4) {std::cerr<<"usage: rebuild_days PROGRAM OUTPUT_DIRECTORY SECONDS_PER_DAY\n";return 2;}
    const int program=std::stoi(argv[1]);const double seconds=std::stod(argv[3]);
    const std::filesystem::path directory=argv[2];std::filesystem::create_directories(directory);
    top_replay_library::Agent agent(program);public_router::Agent rival;
    Config config;config.seed=1000;Sim sim(config);
    agent.reset(agent::runtime::make_agent_init(sim,0));rival.reset(agent::runtime::make_agent_init(sim,1));
    uint64_t random=1000^0xa37108e62d045fb9ULL;
    std::array<uint8_t,8> shops;for(auto& shop:shops)shop=random_word(random)%N_SHOPS;
    std::ofstream report(directory/"days.csv");
    report<<"day,workers,tile_operations,discarded,source_contract_valid,solved,seconds,full_game_equal,note\n";
    for(int day=0;day<29;++day) {
        RecordedDay recorded(sim);
        for(int hour=0;hour<24;++hour) {
            std::copy_n(shops.begin(),sim.st.n_shops,sim.st.shops);
            Action actions[2];
            agent.act(agent::runtime::make_observation(sim,0),{},actions[0]);
            rival.act(agent::runtime::make_observation(sim,1),{},actions[1]);
            const auto before=sim;sim.step(actions[0],actions[1]);
            append_contract(recorded,before,sim,actions);
        }
        int operations=0;for(const auto& work:recorded.problem.tile_work)operations+=work.actions.size();
        report<<day<<','<<recorded.problem.worker_count<<','<<operations<<','<<recorded.discarded<<',';
        if(recorded.discarded) {report<<"0,0,0,0,source_overflow_outside_solver_contract\n";continue;}
        try {
            day_scheduler::prepare_problem(recorded.problem);
            const auto prefix=directory/("day_"+std::to_string(day));
            save_problem_json(recorded.problem,prefix.string()+"_problem.json");
            save_actions(recorded.own,prefix.string()+"_source.txt");
            auto source=recorded.own;
            for(auto& action:source) {action.n_orders=0;std::fill(std::begin(action.orders),std::end(action.orders),Order{});action.finalize();}
            for(const auto& event:recorded.problem.market_plan) {
                auto& action=source[event.hour];action.n_orders=std::max(action.n_orders,int(event.order_index)+1);
                action.orders[event.order_index]={event.market_op,uint8_t(std::max(0,int(event.item))),event.quantity};
            }
            for(auto& action:source)action.finalize();
            const auto reference=replay_schedule(recorded.problem,source);
            if(!reference.requirements_satisfied || !reference.invariants_satisfied) {
                report<<"0,0,0,0,source_contract_replay_failed\n";
                std::ofstream errors(directory/("day_"+std::to_string(day)+"_errors.txt"));
                for(const auto& error:reference.errors)errors<<error<<'\n';
                continue;
            }
            day_scheduler::Options options;options.seconds=seconds;options.fallback_workers=1;
            auto result=day_scheduler::solve(recorded.problem,options);
            bool equal=false;
            if(result.schedule) {
                save_actions(*result.schedule,prefix.string()+"_schedule.txt");
                auto rebuilt=recorded.start;
                for(int hour=0;hour<24;++hour) {
                    std::copy_n(shops.begin(),rebuilt.st.n_shops,rebuilt.st.shops);
                    auto action=(*result.schedule)[hour];
                    // Restore the exact original market course. This validation
                    // is conditional on the recorded rival course, not adaptive
                    // full-game deployment or a cash-insensitive solver claim.
                    action.n_orders=recorded.own[hour].n_orders;
                    std::copy_n(recorded.own[hour].orders,action.n_orders,action.orders);action.finalize();
                    rebuilt.step(action,recorded.rival[hour]);
                }
                const auto& a=sim.st.farms[0];const auto& b=rebuilt.st.farms[0];
                equal=a.money==b.money && a.n_quadrants==b.n_quadrants;
                for(int item=0;item<N_ITEMS;++item)equal&=a.shed[item]==b.shed[item] && a.produced[item]==b.produced[item];
                for(int item=0;item<N_CROPS;++item)equal&=a.seeds[item]==b.seeds[item];
                for(int y=0;y<10;++y)for(int x=0;x<10;++x)equal&=managed(a.tiles[y][x],day+1)==managed(b.tiles[y][x],day+1);
                if(!equal) {
                    std::ofstream difference(prefix.string()+"_full_difference.txt");
                    difference<<"cash "<<a.money<<' '<<b.money<<" land "<<a.n_quadrants<<' '<<b.n_quadrants<<'\n';
                    for(int item=0;item<N_ITEMS;++item)
                        if(a.shed[item]!=b.shed[item] || a.produced[item]!=b.produced[item] || a.discarded[item]!=b.discarded[item])
                            difference<<"item "<<item<<" shed "<<a.shed[item]<<' '<<b.shed[item]<<" produced "<<a.produced[item]<<' '<<b.produced[item]
                                <<" discarded "<<a.discarded[item]<<' '<<b.discarded[item]<<'\n';
                    for(int y=0;y<10;++y)for(int x=0;x<10;++x)
                        if(managed(a.tiles[y][x],day+1)!=managed(b.tiles[y][x],day+1))difference<<"tile "<<x<<' '<<y<<'\n';
                }
            }
            report<<"1,"<<bool(result.schedule)<<','<<result.seconds<<','<<equal<<",conditional_rebuild\n";
            std::cout<<"day="<<day<<" solved="<<bool(result.schedule)<<" seconds="<<result.seconds<<" full_equal="<<equal<<std::endl;
        } catch(const std::exception& error) {
            report<<"0,0,0,0,invalid_contract\n";
            std::ofstream errors(directory/("day_"+std::to_string(day)+"_errors.txt"));errors<<error.what()<<'\n';
        }
        report.flush();
    }
}
