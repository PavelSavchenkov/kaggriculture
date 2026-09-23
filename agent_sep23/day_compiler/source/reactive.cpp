#include "reactive.hpp"
#include "purchases.hpp"
#include "partial_recovery.hpp"
#include "rival_path.hpp"
#include "worker/source/verify.hpp"
#include "agents/common/runtime/observation_builder.hpp"
#include <algorithm>
#include <chrono>

namespace kag::day_compiler {
namespace {
bool effective_workers(const Observation& o,const Action& action,const Farm& after,const Configuration& config) {
    if(action.n_units!=o.self().n_units) return false;
    Config c; c.shed_capacity=config.shed_capacity;
    Sim sim(c); sim.st.day=o.day; sim.st.farms[o.player]=own_farm(o);
    auto workers=action; workers.n_orders=0; workers.finalize();
    const auto check=sim.diagnose_solo_action(o.player,workers,true);
    if(check.requested_unit_actions!=check.successful_unit_actions)return false;
    for(int u=0;u<action.n_units;++u) {
        const auto a=action.units[u];
        if(a.op==OP_PICKUP && int(after.inv[u][a.arg])-o.own.inv[u][a.arg]<a.n) return false;
    }
    return true;
}
bool fixed_purchases(const worker::DayInput& input,const Action* previous,Action* result) {
    bool fits=true;
    for(int h=0;h<24;++h) {
        result[h]=previous[h]; result[h].n_orders=0;
        if(h<input.start_hour || h>=input.hours)continue;
        auto emit=[&](int op,int item,int n) {
            if(!n)return;
            if(result[h].n_orders>=10) { fits=false; return; }
            result[h].orders[result[h].n_orders++]={uint8_t(op),uint8_t(item),n};
        };
        for(int p=0;p<N_CROPS;++p)emit(M_BUY_SEED,p,input.buy_seeds[h][p]);
        for(int a=0;a<3;++a)emit(M_BUY_ANIMAL,GOOSE+a,input.buy_animals[h][a]);
        emit(M_BUY_PRODUCT,WHEAT,input.buy_wheat[h]); emit(M_BUY_PRODUCT,FERTILIZER,input.buy_fertilizer[h]);
        if(input.land_hour==h)emit(M_BUY_LAND,0,1);
        for(int k=0;k<previous[h].n_orders;++k)if(previous[h].orders[k].op==M_HIRE)emit(M_HIRE,0,1);
        result[h].finalize();
    }
    return fits;
}
}
bool ReactiveExecutor::begin(const Observation& dawn,const DaySchedule& schedule) {
    plan_=schedule; diagnostics_={}; expected_={}; audit_.begin(dawn,schedule.intent);
    active_=schedule.verified && remaining_.begin(dawn,schedule.worker_input,schedule.worker_constraints);
    return active_;
}
ExecutionError ReactiveExecutor::act(const Observation& o,const History& history,Action& action,
                                     const Configuration& config,const RepairOptions& options) {
    const auto began=std::chrono::steady_clock::now();
    const auto deadline=began+std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double,std::milli>(std::max(0.0,options.milliseconds)));
    auto fallback=[&] {
        action.clear(); action.n_units=o.self().n_units;
        std::fill_n(action.units,action.n_units,UnitAction{}); action.finalize();
    };
    fallback();
    auto error=executor_.act(o,history,plan_,action,config);
    input_diagnostics_=executor_.input_diagnostics();
    const auto original_error=error; const Action original_action=action;
    if(error==ExecutionError::None && !effective_workers(o,action,worker_phase(o,action,config),config))error=ExecutionError::WorkerEffect;
    if(active_ && expected_.day==o.day && expected_.hour==o.hour) {
        const bool changed=expected_.workers!=o.self().n_units || expected_.quadrants!=o.self().n_quadrants ||
            !std::equal(expected_.seeds,expected_.seeds+N_CROPS,o.own.seeds) ||
            !std::equal(expected_.shed,expected_.shed+N_ITEMS,o.own.shed);
        if(changed) {
            ++diagnostics_.changed_fills;
            if(error==ExecutionError::None)error=ExecutionError::Funding;
        }
    }
    const bool projected=active_ && error==ExecutionError::None && options.monitor_funding && options.max_candidates>0 &&
        funding_changed(o,history,config,deadline);
    if(projected) { error=ExecutionError::Funding; ++diagnostics_.projected_failures; }
    if(active_ && error!=ExecutionError::None) {
        diagnostics_.trigger=error; ++diagnostics_.triggers;
        auto remaining_budget=options;
        remaining_budget.milliseconds=std::max(0.0,options.milliseconds-std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-began).count());
        if(repair(o,history,config,remaining_budget,projected)) {
            error=executor_.act(o,history,plan_,action,config);
            input_diagnostics_=executor_.input_diagnostics();
            if(error==ExecutionError::None && !effective_workers(o,action,worker_phase(o,action,config),config))error=ExecutionError::WorkerEffect;
            if(error==ExecutionError::None)++diagnostics_.repaired; else ++diagnostics_.failed;
        } else {
            ++diagnostics_.failed;
            // A forecast rejection is not an actual failed action. Keep the
            // current executable program if no verified replacement is found.
            if(projected)error=original_error;
        }
    }
    if(error!=ExecutionError::None) {
        // An unsuccessful full repair must not suppress other work that the
        // existing executor can still perform. The error remains explicit.
        if(original_error==ExecutionError::None)action=original_action;
        else if(plan_.verified && options.max_candidates>0 && options.milliseconds>0)
            partial_recovery(o,plan_,action,config);
        else fallback();
    }
    if(active_ && !remaining_.record(o,action,config)) { active_=false; ++diagnostics_.progress_failures; }
    // A tracking mismatch disables verified repair, not the original executor.
    // Keep its physical actions and expose that the contract is no longer tracked.
    if(plan_.verified && !active_)error=ExecutionError::WorkerEffect;
    const auto after=worker_phase(o,action,config); audit_.record(o,action,after);
    expected_.day=o.day; expected_.hour=o.hour+1; expected_.workers=after.n_units; expected_.quadrants=after.n_quadrants;
    std::copy_n(after.seeds,N_CROPS,expected_.seeds); std::copy_n(after.shed,N_ITEMS,expected_.shed);
    for(int k=0;k<action.n_orders;++k) {
        const auto order=action.orders[k];
        if(order.op==M_HIRE)++expected_.workers;
        if(order.op==M_BUY_LAND)++expected_.quadrants;
        if(order.op==M_BUY_SEED)expected_.seeds[order.item]+=order.n;
        if(order.op==M_BUY_PRODUCT || order.op==M_BUY_ANIMAL)expected_.shed[order.item]+=order.n;
        if(order.op==M_SELL)expected_.shed[order.item]-=order.n;
    }
    if(o.hour<24)plan_.actions[o.hour]=action;
    return error;
}
bool ReactiveExecutor::funding_changed(const Observation& o,const History& history,const Configuration& config,
                                       std::chrono::steady_clock::time_point deadline) {
    if(!o.hour)return false;
    double fixed_bill=0; int last=-1,hires=o.self().n_units-1;
    for(int h=o.hour;h<plan_.resources.hours;++h)for(int k=0;k<plan_.resources.workers[h].n_orders;++k) {
        const auto order=plan_.resources.workers[h].orders[k];
        if(order.op==M_HIRE)fixed_bill+=config.hire_mult*double(fib(hires++));
        else if(order.op==M_BUY_SEED)fixed_bill+=order.n*CROPS[order.item].seed;
        else if(order.op==M_BUY_ANIMAL)fixed_bill+=order.n*ANIMALS[order.item-GOOSE].cost;
        else if(order.op==M_BUY_LAND)fixed_bill+=LAND_PRICES[o.self().n_quadrants-1];
        else continue;
        last=h;
    }
    if(last<o.hour || fixed_bill<=o.self().money)return false;
    const auto began=std::chrono::steady_clock::now(); ++diagnostics_.funding_checks;
    auto finish=[&](bool failed) {
        diagnostics_.funding_check_ms+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-began).count();
        return failed;
    };
    const RivalPath rivals(o,history,plan_.market,plan_.resources.hours,config,plan_.diagnostics.covers_visible_funding,true,
                          plan_.diagnostics.covers_visible_fertilizer);
    auto sim=observed_state(o,config); auto projected_history=history;
    for(int h=o.hour;h<=last;++h) {
        if(std::chrono::steady_clock::now()>=deadline)return finish(false);
        const auto current=agent::runtime::make_observation(sim,o.player);
        if(h>o.hour)projected_history.observe(current,config);
        Action action;
        if(executor_.act(current,projected_history,plan_,action,config)!=ExecutionError::None)return finish(true);
        const auto rival=rivals.prepare(sim,o.player,h,config,&action);
        const auto checked=(o.player==0?sim.diagnose_joint_actions(action,rival):sim.diagnose_joint_actions(rival,action)).players[o.player];
        if(checked.requested_unit_actions!=checked.successful_unit_actions || checked.requested_order_units!=checked.successful_order_units)return finish(true);
        projected_history.record(current,worker_phase(current,action,config),action);
        if(o.player==0)sim.step(action,rival); else sim.step(rival,action);
    }
    return finish(false);
}
bool ReactiveExecutor::repair(const Observation& o,const History& history,const Configuration& config,const RepairOptions& options,bool projected) {
    const auto began=std::chrono::steady_clock::now();
    const auto deadline=began+std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double,std::milli>(std::max(0.0,options.milliseconds)));
    auto finish=[&](bool success) {
        diagnostics_.milliseconds+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-began).count(); return success;
    };
    if(options.max_candidates<=0 || std::chrono::steady_clock::now()>=deadline)return finish(false);
    Workload work;
    if(!remaining_.build(o,work))return finish(false);
    const bool cover=plan_.diagnostics.covers_visible_funding || (projected && options.cover_projected_supply);
    const bool fertilizer=plan_.diagnostics.covers_visible_fertilizer || (projected && options.cover_projected_supply);
    const RivalPath rivals(o,history,plan_.market,work.input.hours,config,cover,true,fertilizer);
    int candidates=0,attempts=0;
    auto stopped=[&] { return candidates>=options.max_candidates || std::chrono::steady_clock::now()>=deadline; };
    auto accept=[&](DaySchedule& candidate) {
        if(stopped())return false;
        ++candidates; ++diagnostics_.candidates;
        auto sim=observed_state(o,config); auto projected_history=history; auto audit=audit_;
        for(int h=o.hour;h<candidate.resources.hours;++h) {
            if(std::chrono::steady_clock::now()>=deadline)return false;
            const auto current=agent::runtime::make_observation(sim,o.player);
            if(h>o.hour)projected_history.observe(current,config);
            Action action;
            if(executor_.act(current,projected_history,candidate,action,config)!=ExecutionError::None)return false;
            const auto rival=rivals.prepare(sim,o.player,h,config,&action);
            const auto checked=(o.player==0?sim.diagnose_joint_actions(action,rival):sim.diagnose_joint_actions(rival,action)).players[o.player];
            if(checked.requested_unit_actions!=checked.successful_unit_actions || checked.requested_order_units!=checked.successful_order_units)return false;
            const auto after=worker_phase(current,action,config);
            if(!effective_workers(current,action,after,config))return false;
            audit.record(current,action,after); projected_history.record(current,after,action); candidate.actions[h]=action;
            if(o.player==0)sim.step(action,rival); else sim.step(rival,action);
        }
        if(audit.finish(agent::runtime::make_observation(sim,o.player)).total())return false;
        candidate.worker_constraints.budget={};
        if(!remaining_.begin(o,candidate.worker_input,candidate.worker_constraints))return false;
        candidate.diagnostics.covers_visible_funding=cover;
        candidate.diagnostics.covers_visible_fertilizer=fertilizer;
        plan_=candidate; return true;
    };
    // Input timing may be the only changed dependency. Preserve the available
    // worker path before asking the worker solver to rebuild assignments.
    DaySchedule candidate=plan_; candidate.ordinary_wheat=candidate.ordinary_sales=false;
    candidate.worker_input=work.input; candidate.worker_constraints=work.constraints;
    if(describe_resources(o,plan_.resources.workers,candidate.resources,config,o.day!=29)) {
        candidate.resources.flexible_inputs=plan_.resources.flexible_inputs;
        if(accept(candidate)) { ++diagnostics_.preserved; return finish(true); }
    }
    Action delayed[24];
    if(delay_seed_purchases(o,plan_.resources.workers,delayed,work.input.hours,config)) {
        bool changed=false;
        for(int h=o.hour;h<work.input.hours;++h)for(int p=0;p<N_CROPS;++p) {
            int before=0,after=0;
            for(int k=0;k<plan_.resources.workers[h].n_orders;++k) {
                const auto x=plan_.resources.workers[h].orders[k];
                if(x.op==M_BUY_SEED && x.item==p)before+=x.n;
            }
            for(int k=0;k<delayed[h].n_orders;++k) {
                const auto x=delayed[h].orders[k];
                if(x.op==M_BUY_SEED && x.item==p)after+=x.n;
            }
            candidate.worker_input.buy_seeds[h][p]=after; changed|=before!=after;
        }
        if(changed && describe_resources(o,delayed,candidate.resources,config,o.day!=29)) {
            candidate.resources.flexible_inputs=plan_.resources.flexible_inputs;
            if(accept(candidate)) {
                ++diagnostics_.preserved; ++diagnostics_.retimed_seed_purchases; return finish(true);
            }
        }
    }
    auto intent=plan_.intent; intent.buy_land=remaining_.needs_land(o);
    const int existing=o.self().n_units-1;
    const int wanted=std::max(existing,plan_.diagnostics.hires);
    int deferred=0;
    for(int service=0;service<2 && !stopped();++service) {
        if(service && (!options.defer_newborn_service || !(deferred=defer_newborn_service(o,plan_.intent,work))))break;
        for(int formulation=0;formulation<3 && !stopped();++formulation) {
            FundingProposal funding;
            const bool proposed=formulation==0?propose_funding(o,intent,work,funding,o.hour,config):
                propose_cashflow(o,intent,work,funding,wanted,formulation-1,config,rivals.flow);
            if(!proposed)continue;
            auto constraints=funding.constraints;
            constraints.max_hires=std::min(constraints.max_hires,wanted); constraints.first_hires=constraints.max_hires;
            constraints.budget.soft_deadline=constraints.budget.hard_deadline=deadline;
            Action previous[24]; const bool fits=fixed_purchases(funding.input,plan_.resources.workers,previous);
            worker::SolveResult preserved;
            auto prepare=[&](const worker::SolveResult& result) {
                candidate=plan_; candidate.worker_input=funding.input; candidate.worker_constraints=constraints;
                // The old projected market program belongs to the old worker
                // contract. Rebuild a complete baseline before input refinement.
                candidate.ordinary_wheat=candidate.ordinary_sales=false;
                candidate.diagnostics.hires=result.hires;
                candidate.diagnostics.newborn_first_yield_shortfall+=work.newborn_first_yield_shortfall;
                if(!describe_resources(o,result.schedule,candidate.resources,config,o.day!=29))return false;
                candidate.resources.flexible_inputs=plan_.resources.flexible_inputs; candidate.verified=true;
                const bool accepted=accept(candidate);
                if(accepted)diagnostics_.deferred_newborn_service+=deferred;
                return accepted;
            };
            if(fits && worker::replay_schedule(funding.input,previous,preserved,constraints).valid && prepare(preserved)) {
                ++diagnostics_.preserved; return finish(true);
            }
            for(int placement=0;placement<2 && !stopped();++placement) {
                if(uint64_t(attempts)>=options.max_worker_attempts)break;
                constraints.placement=placement?worker::PlacementStyle::Legacy:worker::PlacementStyle::Staged;
                constraints.max_attempts=std::min(options.attempts_per_candidate,options.max_worker_attempts-uint64_t(attempts));
                const auto result=workers_.solve(funding.input,constraints);
                attempts+=result.attempts; diagnostics_.worker_attempts+=result.attempts;
                if(result.status==worker::SolveStatus::Success && prepare(result))return finish(true);
            }
        }
    }
    return finish(false);
}
}
