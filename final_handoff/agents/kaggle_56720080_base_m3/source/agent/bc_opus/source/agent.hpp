#pragma once
// BC DayIntent policy + day compiler (designs/day_intent.md, designs/day_compiler.md).
#include "agents/common/api/agent_api.hpp"
#include "source/compiler.hpp"
#include <cstdlib>
#include <memory>
#include <array>
#include <string>
#include <vector>

namespace kag::agents::bc_opus {

struct Linear {
    int rows = 0, cols = 0;
    std::vector<float> w, b;
};

// Immutable FP32 weights exported by scripts/train.py (module order).
struct Conv {
    int out = 0, in = 0, k = 0;
    std::vector<float> w, b;
};

struct Model {
    std::vector<Conv> convs;  // optional tile CNN (shared by both farms)
    std::vector<Linear> layers;
    int style = -1;  // default teacher style for style-conditioned models (<model>.style), -1: none
    int features = 3;  // input feature version (<model>.features; bcopus::FEATURES_VERSION)
    // Strength/recency conditioning inputs (<model>.condition: known, strength / 200,
    // day index / 40) at global slots 216-218; absent for unconditioned models.
    bool conditioned = false;
    float condition[3]{};
    // Default opening (<model>.opening: "days style"): teacher style for days < days.
    int opening_days = 0, opening_style = -1;
    // Goal inputs (<model>.goal, models trained with train.py --goal): per line first day, last day, cows6, animals14, q4.
    std::vector<std::array<float, 5>> goals;
    std::vector<std::array<float, 7>> goals_raw;  // <model>.goalraw: first day, last day, raw slots 219-223
    float land_scale = 0;  // <model>.landcond (train.py --land-cond / --land-scale): slot 215 = land_input * scale; 0: not conditioned (step 52)
    int gridoff_first = 0, gridoff_last = -1;  // <model>.gridoff: grid input zeroed on these days (train.py --grid-dropout)
    bool load(const std::string& path);
};

struct DayReport {
    int day = 0;
    int status = 0;    // dc10::CompileStatus
    int fallback = 0;
    int return_percent = 0;
    int hires = 0;
    double compile_ms = 0;
    std::string invalid;  // non-empty if the decoded intent failed validation
    std::string reason;   // compiler attempts log
};

// True if an opening experiment is set (BC_OPENING_*); opponents then use no opening, otherwise
// their model's own (<model>.opening).
inline bool opening_experiment() {
    return std::getenv("BC_OPENING_DAYS") || std::getenv("BC_OPENING_STYLE") || std::getenv("BC_OPENING_MODEL");
}

// Decision pushes (flexibility experiments and per-dawn search); defaults from BC_* env.
struct DecodeKnobs {
    double q_crop = 0.5, q_animal = 0.5;  // quantiles of the new-entity total distributions
    double land_bias = 0;                 // added to the buy-land logit
    bool feed_all = false, care_all = false, collect_all = false, harvest_all = false;
    // Sampling instead of medians: per dawn, quantiles of the new-entity totals and the
    // land decision come from a hash of the dawn state (deterministic per game).
    bool sample = false;
    int style = -1;  // teacher style override (-1: BC_OPUS_STYLE, else the model's default)
    int push_first = 0, push_last = 1 << 20;  // days the quantile and land pushes apply (BC_PUSH_DAYS=a-b)
    int max_quadrants = 4;  // land is bought only below this many quadrants
    float crop_bias[5] = {};  // added to the crop share logits (.decode crop_bias, dc11 agent)
    float animal_bias[3] = {};  // added to the animal share logits (.decode animal_bias, dc11 agent)
    // Whole-farm head logits to decode from instead of the model's own (ensembles).
    const std::vector<float>* head_override = nullptr;
    bool per_type = false;  // per-type count heads instead of the factored total + shares (as BC_PER_TYPE; step 47)
    float land_input = 0;  // land-conditioned models: +1 buying land today, -1 not, 0 unknown (step 52)
    bool logits_only = false;  // stop after the whole-farm head (ensemble averaging needs only global_logits)
    static DecodeKnobs from_env();
};

// Decodes one valid DayIntent from the dawn observation and causal history.
dc10::DayIntent decode_intent(const Model& model, const agent::AgentObservation& dawn, const dc10::History& history,
                              std::string* invalid = nullptr, std::vector<float>* global_logits = nullptr,
                              const DecodeKnobs& knobs = DecodeKnobs::from_env());

// One-line summary of an intent: new counts, reserves, land and per-field totals.
std::string summarize(const dc10::DayIntent& in, const dc10::Schema& s);

class Agent {
public:
    static agent::AgentInfo info() { return {"bc_opus"}; }
    void reset(const agent::AgentInit& init);
    void act(const agent::AgentObservation& observation, const agent::DecisionBudget& budget, Action& action);
    Agent() = default;
    Agent(const Agent& other);  // a copy for rollouts (own solver scratch state)
    Agent& operator=(const Agent&) = delete;
    DecodeKnobs knobs = DecodeKnobs::from_env();  // used at every dawn
    std::string model_path;  // set before reset; empty: BC_OPUS_MODEL, else the default model
    // Opening phase (days < opening_days): another model and/or teacher style.
    // Defaults from BC_OPENING_DAYS, BC_OPENING_MODEL, BC_OPENING_STYLE.
    int opening_days = -1;       // -1: read the environment at reset
    std::string opening_model;   // empty: the main model
    // "<model>.land_model": "path style": the model and style used on the land-push day and, once the
    // agent owns 4 quadrants, on later days (e.g. a fine-tune on games of teams that buy the 4th quadrant).
    std::string land_model;
    int land_style = -1;
    int opening_style = -1;
    // Ensemble: whole-farm head logits averaged over the main model and these models
    // (BC_ENSEMBLE=path,path; each keeps its own sidecars). Group fields: main model.
    std::vector<std::string> ensemble;
    // <model>.decode ("q_crop x", "q_animal x", "days a b"): quantiles of the new-entity totals on
    // days a..b, used when no experiment knob (BC_Q_*) overrides them.
    double decode_q_crop = 0.5, decode_q_animal = 0.5;
    int decode_first = 0, decode_last = -1;
    // Budget-aware herd ("reach_days a b", "reach q ..."): on days a..b, when today's plan compiles
    // in full, try these higher quantiles of the new-animal total (first to last) and keep the first
    // plan that also compiles in full (funded, next-dawn reserve kept).
    std::vector<double> reach_q;
    int reach_first = 0, reach_last = -1;
    // The same for the new-crop total ("creach_days a b", "creach q ...").
    std::vector<double> creach_q;
    int creach_first = 0, creach_last = -1;
    // "reach_stress 1": a reach plan must also be funded under the stress forecast (the opponent sells
    // all its visible output at hour 2); plans funded only under the expected forecast are not optional
    // growth (seed 1404: herd reach on $1.1k cash, $20 next dawn, 7 animals escaped).
    bool reach_stress = false;
    // "max_land n": never own more than n quadrants. With the 4th quadrant our compiler overfills the
    // shed (plans carry 91-203 units into the night) and every Q4 test lost (forced -5..-7k median,
    // natural purchases in the Local-LB replay -14k).
    int max_land = 4;
    // "stress_down q ...": on reach days, a base plan funded only under the expected forecast is
    // replaced by the first plan at these lower new-animal quantiles that is stress-funded.
    std::vector<double> stress_down_q;
    // "land_push a b bias": on days a..b, add bias to the network's land logit (value probe for the
    // 4th quadrant: the latest top agents buy it on day 10; the downstream heads plan the new land).
    int land_push_first = 0, land_push_last = -1;
    double land_push_bias = 0;
    // Continuations: feed observed history without acting (replay steps before takeover).
    void observe(const agent::AgentObservation& observation, const Action& submitted) { history_.observe(observation, submitted); }
    const std::vector<DayReport>& reports() const { return reports_; }
    const dc10::DayIntent& last_intent() const { return last_intent_; }
    const dc10::Schema& last_schema() const { return last_schema_; }
    // Remaining overage time (s) reported by the environment; below 25 s the compiler
    // searches faster, below 10 s it also caps route executions.
    void set_time_left(double seconds) { time_left_ = seconds; }
    // Compiler options for the next dawns (per-dawn search over compiler variants).
    dc10::CompileOptions& compile_options() { return options_; }

private:
    std::shared_ptr<const Model> model_, opening_model_, land_model_;
    std::vector<std::shared_ptr<const Model>> ensemble_;
    dc10::History history_;
    dc10::CompileOptions options_;
    dc10::DayExecutor executor_;
    std::unique_ptr<dc10::dp::Solver> solver_;
    int planned_day_ = -1;
    double time_left_ = 1e9;
    std::vector<DayReport> reports_;
    dc10::DayIntent last_intent_{};
    dc10::Schema last_schema_{};
};
}
