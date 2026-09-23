#include "agent.hpp"
#include <algorithm>
#include <chrono>

#ifndef BC_DEFAULT_MODEL_PATH
#define BC_DEFAULT_MODEL_PATH "agent_sep23/model/model.bin"
#endif

namespace kag::agents::agent_sep23 {
namespace {
double elapsed(std::chrono::steady_clock::time_point start) {
    return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
}
void globals(const dc::DayIntent& intent,int* result) {
    for(int p=0;p<5;++p)result[p]=intent.new_crops(p);
    for(int s=0;s<3;++s)result[5+s]=intent.animal_target(GOOSE+s);
    result[8]=intent.buy_next_land_today;
}
void set_targets(dc::DayIntent& intent,const int* targets) {
    intent.target_geese_next_dawn=targets[0];intent.target_cows_next_dawn=targets[1];intent.target_sheep_next_dawn=targets[2];
}
bool reduce_animal_growth(const dc::DayIntent& intent,const dc::IntentSchema& schema,dc::DayIntent& less) {
    int retained[3]{},targets[3]{},chosen=-1;
    for(int g=0;g<schema.animal_group_count;++g)
        retained[schema.animals[g].state.species]+=schema.animals[g].count-intent.animals[g].allow_escape_tonight_count;
    for(int s=0;s<3;++s) {
        targets[s]=intent.animal_target(GOOSE+s);
        if(targets[s]>retained[s]&&(chosen<0||ANIMALS[s].cost>ANIMALS[chosen].cost))chosen=s;
    }
    if(chosen<0)return false;
    less=intent;--targets[chosen];set_targets(less,targets);return true;
}
double search_milliseconds(double normal,const agent::DecisionBudget& budget) {
    const auto limit=std::min(budget.soft_deadline,budget.hard_deadline);
    // A node-only comparison must not change with machine load.
    if(limit==agent::DecisionBudget::Clock::time_point::max())return 3600000.;
    const double left=std::chrono::duration<double,std::milli>(limit-agent::DecisionBudget::Clock::now()).count();
    return std::max(0.,std::min(normal,left-1));
}
}
Agent::Agent():Agent([]{static const auto model=std::make_shared<const bc::Model>(BC_DEFAULT_MODEL_PATH);return model;}()){}
Agent::Agent(std::shared_ptr<const bc::Model> model):model_(std::move(model)),scratch_(std::make_unique<bc::Scratch>()),
    compiler_(std::make_unique<dc::DayCompiler>()),schedule_(std::make_unique<dc::DaySchedule>()),executor_(std::make_unique<dc::ReactiveExecutor>()){}
agent::AgentInfo Agent::info(){return {"agent_sep23"};}
void Agent::reset(const agent::AgentInit& init) {
    config_=init.config;history_={};trajectory_={};random_.seed(2301+init.player);reports_={};
    day_=-1;observed_step_=-1;active_=false;requested_bound_=false;day_open_=false;
    budget_step_=-1;attempts_used_=0;
}
void Agent::observe(const agent::AgentObservation& o) {
    if(observed_step_!=o.step) {
        history_.observe(o,config_);observed_step_=o.step;
        if(model_->trajectory_enabled||model_->recurrent_enabled||
           (late_model_&&(late_model_->trajectory_enabled||late_model_->recurrent_enabled)))trajectory_.observe(o,config_);
    }
}
bc::Features Agent::features(const agent::AgentObservation& o,const dc::IntentSchema& schema) const {
    auto history=history_;if(observed_step_!=o.step)history.observe(o,config_);
    auto result=bc::encode(o,history,schema,config_);trajectory_.fill(result);return result;
}
void Agent::observe_past(const agent::AgentObservation& o,const Action& action) {
    observe(o);history_.record(o,dc::worker_phase(o,action,config_),action);
}
void Agent::finish(const agent::AgentObservation& o) {
    if(!day_open_)return;
    auto& r=reports_[day_];
    if(requested_bound_)r.requested_missing=requested_audit_.finish(o).total();
    if(active_) {
        r.executed_missing=executed_audit_.finish(o).total();
        r.repairs=executor_->diagnostics().repaired;r.repair_failures=executor_->diagnostics().failed;
    }
    day_open_=false;
}
void Agent::begin_day(const agent::AgentObservation& o) {
    finish(o);observe(o);day_=o.day;reports_[day_].day=day_;
    active_=false;requested_bound_=false;day_open_=true;
}
bool Agent::compile_intent(const agent::AgentObservation& o,const dc::DayIntent& intent,
                           const agent::DecisionBudget& budget,int candidate,uint64_t attempt_limit,bool require_stressed_funding) {
    if(day_!=o.day||!day_open_)begin_day(o);
    if(budget_step_!=o.step){budget_step_=o.step;attempts_used_=0;}
    auto& report=reports_[day_];
    if(candidate==0) {
        globals(intent,report.raw_global);dc::detail::BoundIntent bound;
        requested_bound_=binder_.bind(o,intent,bound)==dc::IntentError::None;
        if(requested_bound_)requested_audit_.begin(o,bound);
    }
    if(budget.soft_expired()||budget.hard_expired())return false;
    auto options=dc::recommended_compile_options();
    if(recovery_==5||require_stressed_funding)options.fallback_to_model=false;
    options.refine_ordinary_collections=collections_;
    options.max_worker_attempts=std::min(options.max_worker_attempts,budget.max_expansions-std::min(budget.max_expansions,attempts_used_));
    options.max_worker_attempts=std::min(options.max_worker_attempts,attempt_limit);
    options.milliseconds=search_milliseconds(options.milliseconds,budget);
    const auto start=std::chrono::steady_clock::now();const auto status=compiler_->compile(o,history_,intent,*schedule_,config_,options);
    report.compile_ms+=elapsed(start);++report.attempts;
    attempts_used_+=schedule_->diagnostics.worker_attempts;
    report.compile_worker_attempts+=schedule_->diagnostics.worker_attempts;
    report.max_step_attempts=std::max(report.max_step_attempts,attempts_used_);
    if(attempts_used_>budget.max_expansions)std::abort();
    if(candidate==0){report.raw_status=int(status);report.raw_intent_error=int(schedule_->diagnostics.intent_error);
        report.raw_forecast_fallback=schedule_->diagnostics.forecast_fallback;}
    if(status!=dc::CompileStatus::Success)return false;
    if(!executor_->begin(o,*schedule_))return false;
    active_=true;report.selected=candidate;globals(intent,report.executed_global);
    report.forecast_fallback=schedule_->diagnostics.forecast_fallback;
    // The compiler may legally reassign interchangeable group members.
    // Audit its chosen assignment, not a second binding of the same counts.
    if(candidate==0){requested_bound_=true;requested_audit_.begin(o,schedule_->intent);}
    executed_audit_.begin(o,schedule_->intent);return true;
}
dc::DayIntent Agent::conservative(const agent::AgentObservation& o,const dc::IntentSchema& schema,int mode) const {
    dc::DayIntent intent;int targets[3]{};
    for(int g=0;g<schema.crop_group_count;++g) {
        const auto& group=schema.crops[g];int selected=group.option_count-1,best=1000000;
        if(mode==0)for(int k=0;k<group.option_count;++k) {
            const auto& option=group.options[k];
            if(option.goal.mode==dc::CropMode::Retire||option.maximum_count<group.count)continue;
            const int cost=3*option.fertilizer_actions+option.water_actions;
            if(cost<best){best=cost;selected=k;}
        }
        intent.crops[g].counts[selected]=group.count;
    }
    for(int g=0;g<schema.animal_group_count;++g) {
        const auto& group=schema.animals[g];targets[group.state.species]+=group.count;
        const bool endangered=group.state.unfed>=1&&!group.state.fed_today;
        intent.animals[g].serve_today_count=o.day<29&&(mode==0||endangered)?group.count:0;
    }
    set_targets(intent,targets);return intent;
}
void Agent::act(const agent::AgentObservation& o,const agent::DecisionBudget& budget,Action& action) {
    action.clear();action.n_units=o.self().n_units;
    std::fill_n(action.units,action.n_units,UnitAction{});action.finalize();
    if(o.hour==0&&day_!=o.day) {
        begin_day(o);dc::IntentSchema schema;
        if(binder_.describe(o,schema)!=dc::IntentError::None){execute(o,budget,action);return;}
        const auto start=std::chrono::steady_clock::now();const auto features=this->features(o,schema);
        const auto& predictor=late_model_&&o.day>=late_day_?late_model_:model_;
        const auto prediction=predictor->predict(features,*scratch_);reports_[day_].inference_ms=elapsed(start);
        // Reserve fallback search in mode3, or only under low cash in mode4.
        // Rich farms retain the full search for profitable collection work.
        // Every call shares one decision budget, including failed candidates.
        const bool reserve=recovery_==3 || (recovery_==4 && o.self().money<1000) || (recovery_==5 && o.day<29);
        const auto primary_limit=reserve?std::max(uint64_t{1},budget.max_expansions/(recovery_==3?4:2)):UINT64_MAX;
        const auto middle_limit=reserve?std::max(uint64_t{1},budget.max_expansions/(recovery_==3?8:4)):UINT64_MAX;
        const auto maintenance_limit=reserve?std::max(uint64_t{1},budget.max_expansions/8):UINT64_MAX;
        const bool compiled=compile_intent(o,prediction.intent,budget,0,primary_limit);
        if(compiled&&recovery_==6&&o.day<29&&attempts_used_+8<budget.max_expansions) {
            const auto& diagnostics=schedule_->diagnostics;
            dc::DayIntent less;
            if((diagnostics.forecast_fallback||diagnostics.fertilizer_fallback)&&diagnostics.projected_cash<100&&
               reduce_animal_growth(prediction.intent,schema,less)) {
                // Use only the unused search budget. If the smaller intent does
                // not pass the stress check, retain the verified original.
                const auto original=std::make_unique<dc::DaySchedule>(*schedule_);
                if(!compile_intent(o,less,budget,1,UINT64_MAX,true)) {
                    *schedule_=*original;
                    if(!executor_->begin(o,*schedule_))std::abort();
                }
            }
        }
        if(!compiled&&recovery_>0) {
            int candidate=1;
            if(recovery_==5) {
                // Try one less new animal before dropping all expansion.
                // Current animals and their service commitments are preserved.
                dc::DayIntent less;
                if(reduce_animal_growth(prediction.intent,schema,less)) {
                    compile_intent(o,less,budget,candidate++,std::max(uint64_t{1},budget.max_expansions/4));
                }
            }
            if(recovery_==2)for(int k=0;k<2&&!active_&&!budget.soft_expired();++k) {
                const auto alternative=predictor->predict(features,*scratch_,&random_,.6f);
                compile_intent(o,alternative.intent,budget,candidate++);
            }
            if(!active_) {
                auto trim=prediction.intent;trim.new_wheat_count=trim.new_carrot_count=trim.new_tomato_count=trim.new_strawberry_count=trim.new_melon_count=0;
                trim.buy_next_land_today=false;int targets[3]{};
                for(int g=0;g<schema.animal_group_count;++g)targets[schema.animals[g].state.species]+=schema.animals[g].count-trim.animals[g].allow_escape_tonight_count;
                set_targets(trim,targets);
                bool changed=prediction.intent.buy_next_land_today;
                for(int p=0;p<5;++p)changed|=prediction.intent.new_crops(p)>0;
                for(int s=0;s<3;++s)changed|=prediction.intent.animal_target(GOOSE+s)!=targets[s];
                if(!reserve||changed)compile_intent(o,trim,budget,candidate,middle_limit);
                ++candidate;
            }
            for(int mode=0;mode<2&&!active_&&!budget.soft_expired();++mode)
                compile_intent(o,conservative(o,schema,mode),budget,candidate++,mode==0?maintenance_limit:UINT64_MAX);
        }
    }
    execute(o,budget,action);
}
void Agent::execute(const agent::AgentObservation& o,const agent::DecisionBudget& budget,Action& action) {
    if(day_!=o.day||!day_open_)begin_day(o);
    if(budget_step_!=o.step){budget_step_=o.step;attempts_used_=0;}
    observe(o);
    auto& report=reports_[day_];const auto start=std::chrono::steady_clock::now();
    bool fallback=!active_||budget.hard_expired();
    if(!fallback) {
        auto options=dc::recommended_repair_options();
        options.max_worker_attempts=std::min(options.max_worker_attempts,budget.max_expansions-std::min(budget.max_expansions,attempts_used_));
        options.milliseconds=search_milliseconds(options.milliseconds,budget);
        const auto previous=executor_->diagnostics().worker_attempts;
        fallback=executor_->act(o,history_,action,config_,options)!=dc::ExecutionError::None;
        attempts_used_+=executor_->diagnostics().worker_attempts-previous;
        report.max_step_attempts=std::max(report.max_step_attempts,attempts_used_);
        if(attempts_used_>budget.max_expansions)std::abort();
        report.execution_errors+=fallback;
        // The reactive executor records this action before returning. Replacing
        // it outside the compiler also makes its remaining-work ledger stale.
        if(trust_executor_)fallback=false;
    }
    if(fallback){emergency(o,action);++report.emergency_hours;}
    const auto after=dc::worker_phase(o,action,config_);
    if(requested_bound_)requested_audit_.record(o,action,after);
    if(active_)executed_audit_.record(o,action,after);
    history_.record(o,after,action);report.execute_ms+=elapsed(start);
}
void Agent::emergency(const agent::AgentObservation& o,Action& action) const {
    action.clear();action.n_units=o.self().n_units;
    std::fill_n(action.units,action.n_units,UnitAction{});
    for(int u=0;u<action.n_units;++u) {
        action.finalize();const auto current=dc::worker_phase(o,action,config_);
        const int x=current.pos_x[u],y=current.pos_y[u],cell=y*10+x;const auto& tile=current.tiles[y][x];
        int cargo=0;for(int item=0;item<N_ITEMS;++item)cargo+=current.inv[u][item];
        if(dc::shed_distance(cell)==0&&cargo&&current.shed_total+ cargo<=config_.shed_capacity)action.units[u].op=OP_DROP;
        else if(tile.has_animal&&!tile.fed_today&&current.inv[u][WHEAT]>0)action.units[u].op=OP_FEED;
        else if(tile.has_animal&&tile.fed_today&&!tile.cared_today&&!dc::zero_value_care(tile,o.day))action.units[u].op=OP_CARE;
        else if(tile.kind==T_PLANT&&!tile.watered_today&&(tile.consecutive_dry>=1||tile.planted_day==o.day))action.units[u].op=OP_WATER;
        else if(cargo&&dc::shed_distance(cell)>0) {
            const int tx=std::clamp(x,4,5),ty=std::clamp(y,4,5);
            action.units[u].op=x<tx?OP_EAST:x>tx?OP_WEST:y<ty?OP_SOUTH:OP_NORTH;
        }
    }
    action.finalize();const auto after=dc::worker_phase(o,action,config_);
    for(int product=0;product<N_PRODUCTS;++product)if(after.shed[product]>0) {
        if(action.n_orders>=config_.max_orders)break;
        if(o.day<29&&(product==WHEAT||product==FERTILIZER))continue;
        action.orders[action.n_orders++]={M_SELL,uint8_t(product),after.shed[product]};
    }
    action.finalize();
}
}
