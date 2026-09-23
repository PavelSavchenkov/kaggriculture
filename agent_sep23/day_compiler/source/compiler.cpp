#include "compiler.hpp"
#include "forecast.hpp"
#include "rival_path.hpp"
#include "settlement.hpp"
#include "collections.hpp"
#include "agents/common/runtime/observation_builder.hpp"
#include <algorithm>
#include <bit>
#include <chrono>
#include <cstring>
#include <cstdio>

namespace kag::day_compiler {
namespace {
bool pending_other_purchases(const DaySchedule& schedule,int hour) {
    for(int h=hour;h<schedule.resources.hours;++h)for(const Action* program:{schedule.resources.workers,schedule.input_program})
        for(int k=0;k<program[h].n_orders;++k) {
            const auto order=program[h].orders[k];
            if(order.op==M_HIRE || order.op==M_BUY_LAND || order.op==M_BUY_SEED || order.op==M_BUY_ANIMAL ||
               (order.op==M_BUY_PRODUCT && order.item!=WHEAT))return true;
        }
    return false;
}
bool same_problem(const FundingProposal& a,const FundingProposal& b) {
    const auto& x=a.constraints; const auto& y=b.constraints;
    return std::memcmp(&a.input,&b.input,sizeof(a.input))==0 && x.reserved_order_slots==y.reserved_order_slots &&
        x.hire_not_before==y.hire_not_before && x.harvest_deadline==y.harvest_deadline && x.effort==y.effort &&
        x.max_hires==y.max_hires && x.first_hires==y.first_hires && x.variants==y.variants && x.route_rounds==y.route_rounds &&
        x.minimize_hires==y.minimize_hires && x.opportunistic_hire_reduction==y.opportunistic_hire_reduction &&
        x.minimize_variants==y.minimize_variants && x.animal_reserve==y.animal_reserve && x.placement==y.placement;
}
int funding_workforce(const Workload& work) {
    int jobs=2*work.input.establish_count;
    for(int events:work.input.events) jobs+=std::popcount(unsigned(events));
    // Use the existing worker solver's work-based warm start, with one extra
    // worker for financing trips. Do not spend all potential sale capital on
    // an arbitrary mature-farm workforce before buying the required inputs.
    return std::clamp((jobs+7)/8,3,13);
}
struct ReturnQuota {
    int lower=0,upper=100,next=25,trials=0,failed=0;
    bool bracketed=false;
    void record(bool feasible) {
        if(feasible) {
            lower=std::max(lower,next); failed=0;
            if(!bracketed) upper=100;
        } else {
            upper=std::min(upper,next-1);
            // A heuristic failure is not a monotone feasibility boundary.
            // Retain the next larger probe before narrowing after two failures.
            bracketed|=++failed>=2;
        }
        constexpr int fractions[]={25,50,75,90,100};
        ++trials; next=bracketed?(lower+upper+1)/2:fractions[std::min(trials,4)];
    }
};
void terminal_returns(FundingProposal& funding,const Observation& dawn,int percent,const Workload& required,
                      const Workload& available_work,const int* anchor=nullptr) {
    int available[N_PRODUCTS]{},selected[N_PRODUCTS]{},total=0;
    for(int p=0;p<N_PRODUCTS;++p) {
        available[p]=funding.input.returns[22][p]; selected[p]=anchor?std::min(available[p],anchor[p]):0;
        total+=available[p]-selected[p];
    }
    const int target=(total*percent+99)/100;
    // A return quota is an economic choice. Keep the most valuable marginal
    // units; the existing worker solver chooses which declared producers supply it.
    for(int n=0;n<target;++n) {
        int best=-1,value=-1;
        for(int p=0;p<N_PRODUCTS;++p) if(selected[p]<available[p]) {
            const int price=market_price(p,dawn.market.inventory[p]+selected[p]);
            if(price>value) { best=p; value=price; }
        }
        if(best<0) break;
        ++selected[best];
    }
    std::fill_n(&funding.input.returns[0][0],24*N_PRODUCTS,0);
    for(int p=0;p<N_PRODUCTS;++p) funding.input.returns[22][p]=funding.input.returns[23][p]=selected[p];
    // A smaller return choice must also remove unneeded optional field work.
    // Otherwise discarded terminal cargo still consumes visits and workers.
    int remaining[N_PRODUCTS]; std::copy_n(selected,N_PRODUCTS,remaining);
    int sources[100],count=0;
    for(int cell=0;cell<100;++cell) if(funding.input.events[cell]&worker::Harvest) {
        const auto& tile=funding.input.grid[cell];
        const int p=tile.has_animal?int(ANIMALS[tile.what-GOOSE].product):int(tile.what);
        if(required.input.events[cell]&worker::Harvest) remaining[p]-=required.harvest_amount[cell];
        else { sources[count++]=cell; funding.input.events[cell]&=~worker::Harvest; }
    }
    std::sort(sources,sources+count,[&](int a,int b) {
        auto cost=[&](int c) {
            const int visit=required.input.events[c]?1:1+2*shed_distance(c);
            return double(visit)/std::max(1,available_work.harvest_amount[c]);
        };
        return cost(a)!=cost(b)?cost(a)<cost(b):a<b;
    });
    for(int n=0;n<count;++n) {
        const int cell=sources[n]; const auto& tile=funding.input.grid[cell];
        const int p=tile.has_animal?int(ANIMALS[tile.what-GOOSE].product):int(tile.what);
        if(remaining[p]<=0) continue;
        funding.input.events[cell]|=worker::Harvest; remaining[p]-=available_work.harvest_amount[cell];
    }
}
bool evaluate_schedule(const Observation& dawn,const History& history,DaySchedule& plan,DayExecutor& executor,
                       const RivalPath& rivals,const Configuration& config,std::chrono::steady_clock::time_point deadline,
                       double& cash,double& margin,Sim* ending=nullptr) {
    auto sim=observed_state(dawn,config); auto projected_history=history;
    CommitmentAudit audit; audit.begin(dawn,plan.intent);
    for(int h=0;h<plan.resources.hours;++h) {
        if(std::chrono::steady_clock::now()>=deadline) return false;
        const auto o=agent::runtime::make_observation(sim,dawn.player); if(h) projected_history.observe(o,config);
        Action action;
        if(executor.act(o,projected_history,plan,action,config)!=ExecutionError::None) return false;
        const auto& input=executor.input_diagnostics();
        if(input.attempted) {
            plan.diagnostics.ordinary_wheat_applied+=input.applied;
            plan.diagnostics.ordinary_wheat_fallbacks+=!input.applied;
            plan.diagnostics.ordinary_wheat_incomplete+=input.incomplete;
        }
        const auto rival=rivals.prepare(sim,dawn.player,h,config,&action);
        const auto checked=(dawn.player==0?sim.diagnose_joint_actions(action,rival):sim.diagnose_joint_actions(rival,action)).players[dawn.player];
        if(checked.requested_unit_actions!=checked.successful_unit_actions || checked.requested_order_units!=checked.successful_order_units) return false;
        const auto workers=worker_phase(o,action,config);
        audit.record(o,action,workers); projected_history.record(o,workers,action); plan.actions[h]=action;
        if(dawn.player==0) sim.step(action,rival); else sim.step(rival,action);
    }
    if(audit.finish(agent::runtime::make_observation(sim,dawn.player)).total()) return false;
    cash=sim.st.farms[dawn.player].money;
    margin=cash-(sim.st.farms[dawn.player^1].money-dawn.opponent().money);
    if(ending)*ending=sim;
    return true;
}
}
DaySchedule::DaySchedule() = default;
ExecutionError DayExecutor::act(const Observation& o,const History& history,const DaySchedule& schedule,Action& action,
                               const Configuration& config) {
    input_diagnostics_={};
    action.clear(); action.n_units=o.self().n_units; std::fill_n(action.units,action.n_units,UnitAction{}); action.finalize();
    if(!schedule.verified || o.hour>=schedule.resources.hours || o.self().n_units!=schedule.resources.required_workers[o.hour])
        return ExecutionError::Workforce;
    const auto& fixed=schedule.resources.workers[o.hour];
    const auto after=worker_phase(o,fixed,config);
    const bool ordinary=o.day<29 && (schedule.ordinary_wheat || schedule.ordinary_sales);
    const bool funding_pending=ordinary && pending_other_purchases(schedule,o.hour);
    input_diagnostics_.funding_deferred=funding_pending;
    const bool rolling=ordinary && schedule.ordinary_sales && !funding_pending;
    auto error=market_.orders(o,history,after,schedule.resources,
                              rolling?MarketMode::SellerRollingH6:schedule.market,action,config);
    if(rolling) {
        input_diagnostics_.sales_applied=error==ExecutionError::None;
        if(error!=ExecutionError::None) {
            input_diagnostics_.sales_fallback=true;
            error=market_.orders(o,history,after,schedule.resources,schedule.market,action,config);
        }
    }
    if(error!=ExecutionError::None) return error;
    if(schedule.sales_first && !finished_sales_first(o,after,action,config)) return ExecutionError::Funding;
    if(o.day<29 && schedule.sales_first && !fertilizer_sales_after_products(o,after,action,config))
        return ExecutionError::Funding;
    if(schedule.ordinary_wheat && o.day<29) {
        // Preserve the verified financing prefix exactly. Conditional input
        // profit must not change the cash supporting unresolved asset orders.
        if(funding_pending) {
            input_diagnostics_.funding_deferred=true; return ExecutionError::None;
        }
        Action program[24]; std::copy_n(schedule.input_program,24,program); program[o.hour]=action;
        const auto continuation=wheat_continuation(o,history,schedule.resources,config);
        const InputMarketOptions options{.forecast=1,.continuation=&continuation,.protect_purchase_cash=true,
            .prefer_required_purchase=true,.clip_unavoidable_returns=true,.buy_after_fixed_orders=true};
        Action candidate;
        input_diagnostics_.attempted=true;
        input_diagnostics_.error=input_market_.orders(o,history,schedule.resources,program,candidate,options,config);
        input_diagnostics_.incomplete=!input_market_.diagnostics.complete;
        input_diagnostics_.transitions=input_market_.diagnostics.transitions;
        if(input_diagnostics_.error==ExecutionError::None && !input_diagnostics_.incomplete) {
            RivalPath rivals(o,history,schedule.market,o.hour+1,config,true,true,true);
            auto sim=observed_state(o,config);
            for(int k=0;k<candidate.n_orders;++k) {
                const auto order=candidate.orders[k];
                if(order.item!=WHEAT || (order.op!=M_SELL && order.op!=M_BUY_PRODUCT))continue;
                sim.st.market.inventory[WHEAT]=wheat_inventory_bound(o.market.inventory[WHEAT],
                    order.op==M_SELL?order.n:-order.n,config.shed_capacity);
                sim.st.market.prices[WHEAT]=market_price(WHEAT,sim.st.market.inventory[WHEAT]);
                rivals.flow.rival_sales[o.hour][WHEAT]=0;
            }
            const auto rival=rivals.prepare(sim,o.player,o.hour,config,&candidate);
            const auto checked=(o.player==0?sim.diagnose_joint_actions(candidate,rival):sim.diagnose_joint_actions(rival,candidate)).players[o.player];
            if(checked.requested_order_units==checked.successful_order_units) {
                action=candidate; input_diagnostics_.applied=true;
            } else input_diagnostics_.error=ExecutionError::Funding;
        }
    }
    return ExecutionError::None;
}
CompileStatus DayCompiler::compile(const Observation& dawn,const History& history,const DayIntent& intent,
                                  DaySchedule& out,const Configuration& config,const CompileOptions& options) {
    if((!options.refine_ordinary_wheat && !options.refine_ordinary_sales && !options.refine_ordinary_collections) || dawn.day==29)
        return compile_base(dawn,history,intent,out,config,options);
    const auto started=std::chrono::steady_clock::now();
    // Keep a failed primary workload from consuming the caller's whole budget.
    // A successful base still gets the full remaining budget for collections.
    auto base_options=options;
    if(options.refine_ordinary_collections && options.max_worker_attempts>64)
        base_options.max_worker_attempts=options.max_worker_attempts/2;
    const auto status=compile_base(dawn,history,intent,out,config,base_options);
    const auto deadline=started+std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::duration<double,std::milli>(std::max(0.0,options.milliseconds)));
    if(out.verified && options.refine_ordinary_collections)refine_collections(dawn,history,out,config,options,deadline);
    if(out.verified && (options.refine_ordinary_wheat || options.refine_ordinary_sales))refine_inputs(dawn,history,out,config,options,deadline);
    out.diagnostics.milliseconds=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count();
    return status;
}
CompileStatus DayCompiler::compile_base(const Observation& dawn,const History& history,const DayIntent& intent,
                                       DaySchedule& out,const Configuration& config,const CompileOptions& options) {
    const auto started=std::chrono::steady_clock::now();
    if(options.cover_visible_funding && options.cover_visible_fertilizer && options.fallback_to_model && dawn.day!=29) {
        auto baseline=options; baseline.cover_visible_fertilizer=false;
        const auto status=compile_base(dawn,history,intent,out,config,baseline);
        const auto deadline=started+std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            std::chrono::duration<double,std::milli>(std::max(0.0,options.milliseconds)));
        if(status!=CompileStatus::InvalidIntent) refine_funding(dawn,history,intent,out,config,options,deadline);
        out.diagnostics.milliseconds=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count();
        return out.verified?CompileStatus::Success:status;
    }
    const bool refine=dawn.day==29 && options.refine_terminal_wheat && controls_wheat(options.market);
    auto initial=options; if(refine) initial.market=MarketMode::SellerTerminalJoint;
    const auto status=compile_once(dawn,history,intent,out,config,initial);
    if(refine && out.verified) {
        const auto deadline=started+std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            std::chrono::duration<double,std::milli>(std::max(0.0,options.milliseconds)));
        refine_wheat(dawn,history,out,config,options,deadline);
        out.diagnostics.milliseconds=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count();
        return status;
    }
    if(!options.cover_visible_funding || !options.fallback_to_model || dawn.day==29 ||
       status==CompileStatus::Success || status==CompileStatus::InvalidIntent) return status;
    const auto attempted=out.diagnostics;
    auto ordinary=options; ordinary.cover_visible_funding=false;
    ordinary.max_candidates-=attempted.candidates;
    ordinary.max_worker_attempts-=std::min(ordinary.max_worker_attempts,uint64_t(attempted.worker_attempts));
    ordinary.milliseconds-=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count();
    if(ordinary.max_candidates<=0 || !ordinary.max_worker_attempts || ordinary.milliseconds<=0) return status;
    const auto fallback=compile_once(dawn,history,intent,out,config,ordinary);
    out.diagnostics.forecast_fallback=true;
    out.diagnostics.candidates+=attempted.candidates;
    out.diagnostics.worker_attempts+=attempted.worker_attempts;
    out.diagnostics.worker_failures+=attempted.worker_failures;
    out.diagnostics.funding_rejections+=attempted.funding_rejections;
    out.diagnostics.duplicate_proposals+=attempted.duplicate_proposals;
    out.diagnostics.milliseconds=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count();
    return fallback;
}
void DayCompiler::refine_collections(const Observation& dawn,const History& history,DaySchedule& out,const Configuration& config,
                                   const CompileOptions& options,std::chrono::steady_clock::time_point deadline) {
    // Protect production first, then consider manure, ongoing crops and animal
    // harvests. All stages preserve the intent and share the search budget.
    const int hire_limit=out.worker_constraints.max_hires;
    for(int stage=0;stage<4;++stage) {
    const bool fertilizer=stage==1;
    int cells[100],count=0,loss[100]{};
    for(int c=0;c<100;++c) {
        const auto& t=dawn.self().tiles[c/10][c%10]; const auto events=out.worker_input.events[c];
        if(stage==0 && t.kind==T_PLANT && CROPS[t.what].ongoing) {
            const int farmer=dawn.self().pos_y[0]*BOARD+dawn.self().pos_x[0];
            const int next_arrival=std::min(distance(farmer,c),1+shed_distance(c));
            const bool decay=t.max_lifespan_step>=0 && t.max_lifespan_step<dawn.step+out.resources.hours+next_arrival;
            const bool dies=t.consecutive_dry && !(events&worker::Water);
            if(t.yield_units && !(events&worker::Harvest) && (decay || dies || (events&worker::Clear))) {
                loss[c]=t.yield_units;cells[count++]=c;
            }
            continue;
        }
        if(stage==2) {
            if(t.kind==T_PLANT && CROPS[t.what].ongoing && t.yield_units &&
               dawn.day-t.planted_day>=CROPS[t.what].first_yield_day && !(events&worker::Harvest)) {
                loss[c]=t.yield_units;cells[count++]=c;
            }
            continue;
        }
        if(fertilizer) {
            if(t.has_animal && t.fertilizer_available && !(events&worker::CollectFertilizer)) {
                loss[c]=1;cells[count++]=c;
            }
            continue;
        }
        if(!t.has_animal || !t.yield_units || (events&worker::Harvest))continue;
        if(stage==3) { loss[c]=t.yield_units;cells[count++]=c;continue; }
        const auto& animal=ANIMALS[t.what-GOOSE]; const int age=dawn.day+1-t.planted_day;
        const bool fed=events&worker::Feed;
        if(!fed && t.consecutive_dry) {
            // The intent permits escape, but existing output can still be
            // collected before the animal leaves. Preserve that same fate.
            loss[c]=t.yield_units;
        } else {
            if(age<animal.first_yield_day || (age-animal.first_yield_day)%animal.interval)continue;
            loss[c]=std::min(int(t.yield_units),std::max(0,int(t.yield_units)+1+(fed?t.pending_care_bonus:0)-animal.max_held));
        }
        if(loss[c])cells[count++]=c;
    }
    if(!count || out.diagnostics.candidates>=options.max_candidates || std::chrono::steady_clock::now()>=deadline)continue;
    const auto began=std::chrono::steady_clock::now(); out.diagnostics.collection_tried=true;
    auto product=[&](int c) {
        const auto& tile=dawn.self().tiles[c/10][c%10];
        return fertilizer?FERTILIZER:tile.has_animal?ANIMALS[tile.what-GOOSE].product:tile.what;
    };
    auto priority=[&](int c) {
        const int visit=out.worker_input.events[c]?1:1+2*shed_distance(c);
        return double(loss[c])*dawn.market.prices[product(c)]/visit;
    };
    std::sort(cells,cells+count,[&](int a,int b) {
        return priority(a)!=priority(b)?priority(a)>priority(b):a<b;
    });
    auto baseline=out; auto base_end=observed_state(dawn,config); double cash=0,margin=0;
    const RivalPath rivals(dawn,history,out.market,out.resources.hours,config,out.diagnostics.covers_visible_funding,
                          options.cover_visible_wheat,out.diagnostics.covers_visible_fertilizer);
    if(!evaluate_schedule(dawn,history,baseline,executor_,rivals,config,deadline,cash,margin,&base_end))continue;
    int previous_count=0;double best_score=0;bool full_extra_verified=false;
    for(int variant=0;variant<(stage==0?5:4);++variant) {
        if(variant==4&&full_extra_verified)continue;
        int added=variant==0 || variant>=3?count:variant==1?(count+1)/2:1;
        int chosen[100];std::copy_n(cells,count,chosen);
        int return_product=-1;
        if(variant==4) {
            double scores[N_PRODUCTS]{};
            for(int n=0;n<count;++n)scores[product(cells[n])]+=priority(cells[n]);
            return_product=int(std::max_element(scores,scores+N_PRODUCTS)-scores);
            added=0;
            for(int n=0;n<count;++n)if(product(cells[n])==return_product)chosen[added++]=cells[n];
        }
        const int extra_hire=variant>=3?std::min(2,hire_limit-baseline.diagnostics.hires):0;
        if((!extra_hire && added==previous_count) || (variant>=3 && baseline.diagnostics.hires>=hire_limit))continue;
        previous_count=added;
        if(out.diagnostics.candidates>=options.max_candidates || uint64_t(out.diagnostics.worker_attempts)>=options.max_worker_attempts ||
           std::chrono::steady_clock::now()>=deadline)break;
        auto candidate=baseline;
        auto& constraints=candidate.worker_constraints;
        for(int n=0;n<added;++n) {
            const int cell=chosen[n];const auto& tile=dawn.self().tiles[cell/10][cell%10];
            candidate.worker_input.events[cell]|=fertilizer?worker::CollectFertilizer:worker::Harvest;
            if(!fertilizer && tile.kind==T_PLANT && tile.max_lifespan_step>=0) {
                const int expiry=tile.max_lifespan_step-dawn.step;
                const int first_loss=expiry<0?(-expiry&1):expiry;
                const int last_harvest=first_loss+2*(int(tile.yield_units)-1);
                constraints.harvest_deadline[cell]=std::min(int(constraints.harvest_deadline[cell]),
                    std::clamp(last_harvest,0,candidate.resources.hours-1));
            }
        }
        if(return_product>=0) {
            int quantity=0;
            for(int c=0;c<100;++c)if(product(c)==return_product && (candidate.worker_input.events[c]&worker::Harvest))
                quantity+=planned_harvest_yield(candidate.worker_input.grid[c],0,candidate.worker_input.events[c]);
            for(int h=20;h<24;++h)candidate.worker_input.returns[h][return_product]=
                std::max(candidate.worker_input.returns[h][return_product],quantity);
        }
        constraints.max_hires=constraints.first_hires=baseline.diagnostics.hires+extra_hire;
        const uint64_t attempts=extra_hire?2*options.attempts_per_candidate:options.attempts_per_candidate;
        constraints.max_attempts=std::min(attempts,options.max_worker_attempts-out.diagnostics.worker_attempts);
        constraints.budget.soft_deadline=constraints.budget.hard_deadline=deadline;
        ++out.diagnostics.candidates;
        const auto answer=workers_.solve(candidate.worker_input,constraints);
        if(options.trace_candidates)std::fprintf(stderr,"collection_search stage=%d added=%d extra_hire=%d status=%d attempts=%llu hires=%d\n",
            stage,added,extra_hire,int(answer.status),static_cast<unsigned long long>(answer.attempts),answer.hires);
        out.diagnostics.worker_attempts+=answer.attempts;
        if(answer.status!=worker::SolveStatus::Success)continue;
        if(!describe_resources(dawn,answer.schedule,candidate.resources,config))continue;
        candidate.resources.flexible_inputs=true;
        auto ending=base_end;
        if(!evaluate_schedule(dawn,history,candidate,executor_,rivals,config,deadline,cash,margin,&ending))continue;
        const auto value=compare_collection_value(ending,base_end,dawn.player);
        if(variant==3&&value.valid())full_extra_verified=true;
        if(options.trace_candidates)std::fprintf(stderr,"collection stage=%d added=%d hires=%d valid=%d rejection=%d units=%d margin=%.0f score=%.0f\n",
            stage,added,answer.hires,value.valid(),value.rejection,value.extra_units,value.restored_margin,value.score);
        if(options.trace_candidates && value.rejection==2)for(int p=CARROT;p<=WOOL;++p) {
            auto stock=[&](const Farm& farm) {
                int total=farm.shed[p]+farm.sold_units[p];
                for(const auto& row:farm.tiles)for(const auto& tile:row)
                    if((tile.kind==T_PLANT && tile.what==p) || (tile.has_animal && ANIMALS[tile.what-GOOSE].product==p))total+=tile.yield_units;
                return total;
            };
            const auto& a=ending.st.farms[dawn.player];const auto& b=base_end.st.farms[dawn.player];
            if(stock(a)<stock(b))std::fprintf(stderr,"collection_stock product=%d delta=%d discarded_delta=%d\n",p,stock(a)-stock(b),a.discarded[p]-b.discarded[p]);
        }
        if(!value.valid() || value.score<=best_score)continue;
        best_score=value.score;
        out.resources=candidate.resources; out.worker_input=candidate.worker_input; out.worker_constraints=constraints;
        out.worker_constraints.budget={}; std::copy_n(candidate.actions,24,out.actions);
        out.diagnostics.hires=answer.hires; out.diagnostics.projected_cash=cash; out.diagnostics.projected_margin=margin;
        out.diagnostics.collection_kept=true; out.diagnostics.collection_added=baseline.diagnostics.collection_added+added;
        out.diagnostics.collection_extra_units=baseline.diagnostics.collection_extra_units+value.extra_units;
        out.diagnostics.collection_score=baseline.diagnostics.collection_score+value.score;
        // A complete full batch at the original hire cap is the cheap first
        // choice. Subsets and one extra hire are fallbacks, not a larger search.
        if(variant==0)break;
    }
    out.diagnostics.collection_ms+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-began).count();
    }
}
void DayCompiler::refine_inputs(const Observation& dawn,const History& history,DaySchedule& out,const Configuration& config,
                                const CompileOptions& options,std::chrono::steady_clock::time_point deadline) {
    if(out.diagnostics.candidates>=options.max_candidates || std::chrono::steady_clock::now()>=deadline)return;
    const auto started=std::chrono::steady_clock::now();
    out.diagnostics.ordinary_trading_tried=true;
    const RivalPath rivals(dawn,history,out.market,out.resources.hours,config,out.diagnostics.covers_visible_funding,
                          options.cover_visible_wheat,out.diagnostics.covers_visible_fertilizer);
    auto baseline=out;
    auto base_end=observed_state(dawn,config);
    double base_cash=0,base_margin=0,best_gain=0;
    const bool base_valid=evaluate_schedule(dawn,history,baseline,executor_,rivals,config,deadline,base_cash,base_margin,&base_end);
    auto refine=[&](bool wheat) {
        if(!base_valid || out.diagnostics.candidates>=options.max_candidates || std::chrono::steady_clock::now()>=deadline)return;
        ++out.diagnostics.candidates;
        auto candidate=out;
        if(wheat) candidate.ordinary_wheat=true; else candidate.ordinary_sales=true;
        // Each coordinate keeps the complete incumbent. In particular, wheat
        // must see the H6 sales we verified, not an obsolete liquidation tape.
        // Both candidates retain the original shared deadline and candidate cap.
        const auto* program=out.diagnostics.ordinary_trading_kept?out.actions:baseline.actions;
        std::copy_n(program,24,candidate.actions);
        std::copy_n(program,24,candidate.input_program);
        auto candidate_end=base_end; double cash=0,margin=0;
        if(evaluate_schedule(dawn,history,candidate,executor_,rivals,config,deadline,cash,margin,&candidate_end)) {
            const auto settled=settle_stock(candidate_end,base_end.st.farms[dawn.player],dawn.player);
            const double gain=margin-base_margin+settled.cash-cash;
            if(!out.diagnostics.ordinary_trading_kept)
                out.diagnostics.ordinary_trading_settled_margin=settled.valid?gain:-1e100;
            if(settled.valid && gain>best_gain) {
                best_gain=gain;
                out.ordinary_wheat=candidate.ordinary_wheat; out.ordinary_sales=candidate.ordinary_sales;
                std::copy_n(candidate.actions,24,out.actions);
                std::copy_n(candidate.input_program,24,out.input_program);
                out.diagnostics.projected_cash=cash; out.diagnostics.projected_margin=margin;
                out.diagnostics.ordinary_trading_settled_margin=gain;
                out.diagnostics.ordinary_trading_kept=true;
            }
        }
        out.diagnostics.ordinary_wheat_applied=candidate.diagnostics.ordinary_wheat_applied;
        out.diagnostics.ordinary_wheat_fallbacks=candidate.diagnostics.ordinary_wheat_fallbacks;
        out.diagnostics.ordinary_wheat_incomplete=candidate.diagnostics.ordinary_wheat_incomplete;
    };
    if(options.refine_ordinary_sales)refine(false);
    if(options.refine_ordinary_wheat)refine(true);
    out.diagnostics.ordinary_trading_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count();
}
void DayCompiler::refine_wheat(const Observation& dawn,const History& history,DaySchedule& out,const Configuration& config,
                               const CompileOptions& options,std::chrono::steady_clock::time_point deadline) {
    if(out.diagnostics.candidates>=options.max_candidates || std::chrono::steady_clock::now()>=deadline) return;
    const auto started=std::chrono::steady_clock::now();
    out.diagnostics.wheat_refinement_tried=true;
    const RivalPath rivals(dawn,history,options.market,out.resources.hours,config,false,false);
    auto baseline=out,candidate=out; candidate.market=options.market;
    double baseline_cash=0,baseline_margin=-1e100,cash=0,margin=-1e100;
    const bool base_valid=evaluate_schedule(dawn,history,baseline,executor_,rivals,config,deadline,baseline_cash,baseline_margin);
    if(std::chrono::steady_clock::now()<deadline) {
        ++out.diagnostics.candidates;
        if(evaluate_schedule(dawn,history,candidate,executor_,rivals,config,deadline,cash,margin) && (!base_valid || margin>baseline_margin)) {
            out.market=candidate.market; std::copy_n(candidate.actions,24,out.actions);
            out.diagnostics.projected_cash=cash; out.diagnostics.projected_margin=margin;
            out.diagnostics.wheat_refinement_kept=true;
        }
    }
    out.diagnostics.wheat_refinement_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count();
}
void DayCompiler::refine_funding(const Observation& dawn,const History& history,const DayIntent& intent,DaySchedule& out,
                                 const Configuration& config,const CompileOptions& options,std::chrono::steady_clock::time_point deadline) {
    if(std::chrono::steady_clock::now()>=deadline) return;
    DaySchedule candidate=out;
    if(out.verified) {
        const RivalPath rivals(dawn,history,options.market,out.resources.hours,config,true,options.cover_visible_wheat,true);
        double cash=0,margin=0;
        if(evaluate_schedule(dawn,history,candidate,executor_,rivals,config,deadline,cash,margin)) {
            std::copy_n(candidate.actions,24,out.actions);
            out.diagnostics.projected_cash=cash; out.diagnostics.projected_margin=margin;
            out.diagnostics.covers_visible_funding=true; out.diagnostics.covers_visible_fertilizer=true;
            return;
        }
    }
    auto remaining=options; remaining.fallback_to_model=false;
    remaining.max_candidates-=out.diagnostics.candidates;
    remaining.max_worker_attempts-=std::min(remaining.max_worker_attempts,uint64_t(out.diagnostics.worker_attempts));
    remaining.milliseconds=std::chrono::duration<double,std::milli>(deadline-std::chrono::steady_clock::now()).count();
    if(remaining.max_candidates<=0 || !remaining.max_worker_attempts || remaining.milliseconds<=0) return;
    const auto previous=out.diagnostics;
    compile_once(dawn,history,intent,candidate,config,remaining);
    const auto extra=candidate.diagnostics;
    if(candidate.verified) {
        out=candidate; out.diagnostics.covers_visible_fertilizer=true;
        out.diagnostics.fertilizer_refinement_kept=true;
    } else out.diagnostics.fertilizer_fallback=true;
    out.diagnostics.fertilizer_refinement_tried=true;
    out.diagnostics.candidates=previous.candidates+extra.candidates;
    out.diagnostics.worker_attempts=previous.worker_attempts+extra.worker_attempts;
    out.diagnostics.worker_failures=previous.worker_failures+extra.worker_failures;
    out.diagnostics.funding_rejections=previous.funding_rejections+extra.funding_rejections;
    out.diagnostics.duplicate_proposals=previous.duplicate_proposals+extra.duplicate_proposals;
}
CompileStatus DayCompiler::compile_once(const Observation& dawn,const History& history,const DayIntent& intent,
                                       DaySchedule& out,const Configuration& config,const CompileOptions& options) {
    const auto started=std::chrono::steady_clock::now();
    const auto deadline=started+std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::duration<double,std::milli>(std::max(0.0,options.milliseconds)));
    out=DaySchedule();
    out.diagnostics.covers_visible_funding=options.cover_visible_funding && dawn.day!=29;
    out.diagnostics.covers_visible_fertilizer=options.cover_visible_funding && options.cover_visible_fertilizer && dawn.day!=29;
    out.market=options.market; out.sales_first=options.sales_first;
    auto finish=[&](CompileStatus status) {
        out.diagnostics.milliseconds=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count();
        return out.verified?CompileStatus::Success:status;
    };
    out.diagnostics.intent_error=binder_.bind(dawn,intent,out.intent,nullptr,options.today_first_binding);
    if(out.diagnostics.intent_error!=IntentError::None) return finish(CompileStatus::InvalidIntent);
    const RivalPath rivals(dawn,history,options.market,std::min(24,config.episode_steps-dawn.step-1),config,
                           options.cover_visible_funding,options.cover_visible_wheat,options.cover_visible_fertilizer);
    if(options.max_candidates<=0 || !options.max_worker_attempts || std::chrono::steady_clock::now()>=deadline)
        return finish(CompileStatus::Budget);
    // Establish the required-work incumbent before optional economic jobs.
    // Return improvements cannot consume the budget before a feasible base.
    Workload required_work;
    FundingProposal incumbent_funding;
    int required_returns[N_PRODUCTS]{};
    int tried_count=0;
    bool tried_worker_feasible[12]{};
    for(bool productive_newborns:{true,false})
    for(bool optional:{false,true}) for(auto calendar:{CalendarChoice::FewerInputs,CalendarChoice::FewerActions,CalendarChoice::LessWorkToday})
    for(bool field_inputs:{false,true}) {
        if(dawn.day==29 && (calendar!=CalendarChoice::FewerInputs || field_inputs || !productive_newborns)) continue;
        Workload work;
        if(!workloads_.build(dawn,out.intent,work,calendar,optional,field_inputs,dawn.day!=29,productive_newborns)) continue;
        if(dawn.day==29 && !optional) required_work=work;
        ReturnQuota quotas;
        for(int funding_variant=0;funding_variant<(dawn.day==29?(optional?7:2):7);++funding_variant) {
            const bool adapt=options.adapt_terminal_returns && dawn.day==29 && optional && funding_variant<5;
            if(adapt && quotas.upper<=quotas.lower) { funding_variant=4; continue; }
            auto feedback=[&](bool feasible) { if(adapt) quotas.record(feasible); };
            int return_percent=-1;
            if(out.diagnostics.candidates>=options.max_candidates || uint64_t(out.diagnostics.worker_attempts)>=options.max_worker_attempts ||
                std::chrono::steady_clock::now()>=deadline) return finish(CompileStatus::Budget);
            FundingProposal funding;
            if(dawn.day==29 && optional && funding_variant>=5) {
                if(!out.verified) continue;
                funding=incumbent_funding; int added=0;
                for(int cell=0;cell<100;++cell) {
                    const auto& tile=dawn.self().tiles[cell/10][cell%10]; auto& events=funding.input.events[cell];
                    if(!tile.has_animal || !tile.fertilizer_available || (events&worker::CollectFertilizer)) continue;
                    if(funding_variant==5 && !events) continue;
                    events|=worker::CollectFertilizer; ++added;
                }
                if(!added) continue;
                funding.input.returns[22][FERTILIZER]+=added; funding.input.returns[23][FERTILIZER]+=added;
            } else {
                const bool proposed=(funding_variant==0 || dawn.day==29)?propose_funding(dawn,out.intent,work,funding,0,config):
                    propose_cashflow(dawn,out.intent,work,funding,
                                     std::min(13,funding_workforce(work)+(funding_variant<=3?0:2)),
                                     2*((funding_variant-1)%3),config,rivals.flow);
                if(!proposed) { ++out.diagnostics.funding_rejections; feedback(false); continue; }
                if(dawn.day==29) {
                    // Grow from the funded required-return incumbent. A hard
                    // all-optional solve must not preempt the useful small ones.
                    constexpr int fractions[]={25,50,75,90,100};
                    return_percent=optional?(adapt?quotas.next:fractions[funding_variant]):100*funding_variant;
                    terminal_returns(funding,dawn,return_percent,
                                     required_work,work,optional?required_returns:nullptr);
                }
            }
            if(options.initial_hires>=0) funding.constraints.first_hires=std::min(options.initial_hires,funding.constraints.max_hires);
            else if(dawn.day==29 && funding.constraints.first_hires<0)
                funding.constraints.first_hires=std::min(13,funding.constraints.max_hires);
            bool duplicate=false,duplicate_success=false;
            for(int n=0;n<tried_count;++n) if(same_problem(funding,tried_[n])) { duplicate=true; duplicate_success=tried_worker_feasible[n]; break; }
            if(duplicate) { ++out.diagnostics.duplicate_proposals; feedback(duplicate_success); continue; }
            const int tried_index=tried_count<int(tried_.size())?tried_count:-1;
            if(tried_index>=0) tried_[tried_count++]=funding;
            ++out.diagnostics.candidates;
            funding.constraints.max_attempts=std::min(options.attempts_per_candidate,options.max_worker_attempts-out.diagnostics.worker_attempts);
            funding.constraints.budget.soft_deadline=deadline; funding.constraints.budget.hard_deadline=deadline;
            const auto answer=workers_.solve(funding.input,funding.constraints);
            if(options.trace_candidates) {
                int jobs=0; for(int events:funding.input.events) jobs+=std::popcount(unsigned(events));
                std::fprintf(stderr,"candidate=%d optional=%d variant=%d percent=%d jobs=%d hires=%d status=%d attempts=%llu worker_ms=%.3f returns=",
                    out.diagnostics.candidates,optional,funding_variant,return_percent,jobs,answer.hires,int(answer.status),
                    static_cast<unsigned long long>(answer.attempts),answer.microseconds/1000);
                for(int p=0;p<N_PRODUCTS;++p) std::fprintf(stderr,"%s%d",p?",":"",funding.input.returns[22][p]);
                std::fprintf(stderr,"\n");
            }
            out.diagnostics.worker_attempts+=answer.attempts;
            if(answer.status!=worker::SolveStatus::Success) { ++out.diagnostics.worker_failures; feedback(false); continue; }
            ResourceSchedule resources;
            if(!describe_resources(dawn,answer.schedule,resources,config)) { ++out.diagnostics.worker_failures; feedback(false); continue; }
            // Market timing or cash failures do not bound achievable worker output.
            if(tried_index>=0) tried_worker_feasible[tried_index]=true;
            feedback(true);
            resources.flexible_inputs=dawn.day!=29;
            DaySchedule candidate; candidate.intent=out.intent; candidate.resources=resources; candidate.verified=true; candidate.market=options.market; candidate.sales_first=options.sales_first;
            auto sim=observed_state(dawn,config); CommitmentAudit audit; audit.begin(dawn,out.intent);
            auto projected_history=history;
            bool valid=true;
            for(int h=0;h<resources.hours;++h) {
                if(std::chrono::steady_clock::now()>=deadline) return finish(CompileStatus::Budget);
                const auto o=agent::runtime::make_observation(sim,dawn.player); Action action;
                if(h) projected_history.observe(o,config);
                const auto error=executor_.act(o,projected_history,candidate,action,config);
                out.diagnostics.execution_error=error;
                if(error!=ExecutionError::None) { out.diagnostics.failure_hour=h; valid=false; break; }
                const auto rival=rivals.prepare(sim,dawn.player,h,config,&action);
                const auto joint=dawn.player==0?sim.diagnose_joint_actions(action,rival):sim.diagnose_joint_actions(rival,action);
                const auto& checked=joint.players[dawn.player];
                if(checked.requested_unit_actions!=checked.successful_unit_actions ||
                   checked.requested_order_units!=checked.successful_order_units) {
                    out.diagnostics.execution_error=ExecutionError::WorkerEffect;
                    out.diagnostics.failure_hour=h; valid=false; break;
                }
                const auto workers=worker_phase(o,action,config);
                candidate.actions[h]=action; audit.record(o,action,workers); projected_history.record(o,workers,action);
                if(dawn.player==0) sim.step(action,rival); else sim.step(rival,action);
            }
            if(!valid) continue;
            const auto report=audit.finish(agent::runtime::make_observation(sim,dawn.player));
            out.diagnostics.semantic_failures=report.total();
            if(report.total()) continue;
            const double cash=sim.st.farms[dawn.player].money;
            const double margin=cash-(sim.st.farms[dawn.player^1].money-dawn.opponent().money);
            if(options.trace_candidates) std::fprintf(stderr,"candidate=%d complete cash=%.0f margin=%.0f incumbent=%d\n",
                out.diagnostics.candidates,cash,margin,!out.verified || margin>out.diagnostics.projected_margin);
            if(!out.verified || margin>out.diagnostics.projected_margin) {
                out.resources=resources; std::copy_n(candidate.actions,24,out.actions); out.verified=true;
                out.worker_input=funding.input; out.worker_constraints=funding.constraints;
                out.worker_constraints.budget={};
                incumbent_funding=funding;
                if(dawn.day==29 && !optional) std::copy_n(funding.input.returns[22],N_PRODUCTS,required_returns);
                out.diagnostics.hires=answer.hires; out.diagnostics.projected_cash=cash;
                out.diagnostics.newborn_first_yield_shortfall=work.newborn_first_yield_shortfall;
                out.diagnostics.projected_margin=margin;
            }
            if(dawn.day!=29) return finish(CompileStatus::Success);
        }
    }
    return finish(CompileStatus::NoFundedSchedule);
}
}
