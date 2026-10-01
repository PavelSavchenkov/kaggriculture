#pragma once
// BC DayIntent policy (frozen bc_opus network and decoding) + day compiler dc11.
#include "agents/common/api/agent_api.hpp"
#include "agent/bc_opus/source/agent.hpp"
#include "dc11/compiler.hpp"
#include "dc11_local/learned_forecast.hpp"
#include "dc11_local/fc_transformer/fc_transformer.hpp"
#include <memory>
#include <string>
#include <vector>

namespace kag::agents::bc_overhaul {

struct DayReport {
    int day = 0;
    int status = 0;  // dc11::CompileStatus
    int fallback = 0, hires = 0, dropped = 0, trims = 0;
    int new_crops = 0;  // the compiled intent's new plantings (all crops)
    int land_hour = -1, stress_ok = 0;  // the plan's land order hour; the plan was stress-funded
    double ms_route = 0, ms_realize = 0, ms_fund = 0;  // compile profile
    int animals = 0, feed_intent = 0;  // placed animals at dawn (schema) and the intent's feeds
    std::string groups;                // animal groups: species / size / unfed / feed asked
    int asked[N_ANIMALS]{}, planned[N_ANIMALS]{};  // new animals: the intent's ask, the compiled plan's (after trims / drops)
    bool stress_funded = false;
    int moves = 0, unit_actions = 0, night = 0, skipped = 0;
    double compile_ms = 0;
    std::string reason;
};

// Probe only (tools/replay_games REPLAY_ORACLE, tools_dc11/teacher_day TEACHER_ORACLE): the true opponent flow
// [day][hour][product] for this thread's game; set before reset(). Replaces the day and intra-day forecasts.
inline thread_local const double* dc11_oracle_flow = nullptr;

class Agent {
public:
    static agent::AgentInfo info() { return {"bc_overhaul"}; }
    void reset(const agent::AgentInit& init);
    void act(const agent::AgentObservation& observation, const agent::DecisionBudget& budget, Action& action);
    std::string model_path;    // set before reset; empty: the build default
    // Compiler options ("rival=0.5 ..."); "-": defaults; empty: DC11_OPTIONS and the DC11_* probe
    // overrides of the environment (else the environment is not read).
    std::string options_text;
    // tools/ei_dc11 (local addition, sep24_BC_opus): decode push on one dawn only (push_day -1: off). A pushed
    // new-animal quantile also skips that dawn's herd reach.
    int push_day = -1;
    double push_q_animal = -1, push_q_crop = -1, push_land = 0;
    // tools_dc11/teacher_day (local, sep28_top_lb_imitation): on teacher_day compile teacher_intent instead of the decoded
    // intent (no herd reach / early pushes); observe() feeds recorded history for continuations.
    int teacher_day = -1;
    dc10::DayIntent teacher_intent{};
    // tools_dc11/duel_mm DUEL_PLANT (ported from sep28_top_lb_imitation's duel_dc11): on days with plant_ok, the decoded intent's new
    // animals (plant_mask 1), new crops (2) and land (4) come from plant_intent[day] (the recorded player's label), trimmed to today's
    // sites; no herd reach. plant_used counts the days applied, plant_failed the days whose fitted intent did not validate.
    std::vector<dc10::DayIntent> plant_intent;
    std::vector<char> plant_ok;
    int plant_mask = 0, plant_used = 0, plant_failed = 0;
    void observe(const agent::AgentObservation& obs, const Action& submitted);
    const std::vector<DayReport>& reports() const { return reports_; }
    dc11::Options& options() { return options_; }
    // tools_dc11/options_diff (local, Imitation): the final decoded intent (after decode patches) of the last compiled dawn.
    dc10::DayIntent last_intent{};
    int last_intent_day = -1;
    dc10::DayIntent final_intent{};
    bool intent_only = false;  // options_diff OD_FAST: act() at a dawn stops after the decode (no compile, no herd reach / early pushes)  // after herd reach / earlycow / earlycrop: the intent the compiler gets (options_diff)
    // Seller replay (local, Imitation; tools_dc11/duel_mm): the day market the executor starts from on this dawn, and its hourly update.
    dc11::DayMarket seller_market(const agent::AgentObservation& dawn);
    void seller_hour(const agent::AgentObservation& obs, dc11::DayMarket& market);
    void seller_advance(const agent::AgentObservation& obs) { if (exact_history_) exact_history_->advance(obs); }
    void seller_record(const agent::AgentObservation& obs, const Action& submitted) {
        if (exact_history_) exact_history_->observe(obs, submitted);
        history_.observe(obs, submitted);
    }

    // sellmodel (I1): the learned M&M seller on the same-stock bed's seller path (duel_mm REC): replaces the DP's thin-product lots
    // unless the night room binds (then the larger).
    void model_sales(const agent::AgentObservation& obs, int sell[N_PRODUCTS], bool room_binds);
    // sellmodel=3 / 4 on the seller path: the model's hours (flag), and the mode; model_record adds this hour's sales to today's count.
    bool model_flags(const agent::AgentObservation& obs, bool flag[N_PRODUCTS]);
    int model_mode() const { return options_.sell_model_mode; }
    void model_record(const agent::AgentObservation& obs, const int sell[N_PRODUCTS]);

private:
    std::shared_ptr<const bcsell::Model> sell_model_;  // <model>.sellmodel (sellmodel=1)
    std::shared_ptr<bcsell::Tracker> sell_tracker_;     // its trailing market levels (level models)
    int rec_sold_[N_PRODUCTS]{};                        // model_sales: our units sold today on the seller path
    int rec_day_ = -1;
    std::shared_ptr<const bc_opus::Model> model_;
    std::shared_ptr<const fcast::LearnedForecast> forecast_;  // <model>.forecast (local addition, sep24_BC_opus)
    std::vector<std::shared_ptr<const bc_opus::Model>> ensemble_;  // <model>.ensemble (local addition, sep24_BC_opus)
    bool ensemble_opening_ = false;
    int ensemble_from_ = 0;  // <model>.ensemble_from: members only from this day on (local addition, step 44)
    std::string ensemble_fields_;  // <model>.ensemble_fields: segments averaged (counts land crop animal; empty: all)  // <model>.ensemble_opening: also on opening days (members get the opening style)
    // <model>.decode: herd reach ("reach_days a b", "reach q ..."), "reach_stress", "max_land n",
    // "land_match a b bias" (days a..b: the 4th quadrant when the opponent owns 4 and we own 3).
    std::vector<double> reach_q_;
    int reach_first_ = 0, reach_last_ = -1, max_land_ = 4;
    bool reach_stress_ = false;
    int match_first_ = 0, match_last_ = -1;
    double match_bias_ = 0;
    int v219_day_ = -1, v219_money_ = 0, v219_price_ = 0, v219_shops_ = 0, v219_n_ = 0;  // .decode v219 (local addition)
    int plot_day_ = -1, plot_money_ = 0, plot_crop_ = 0, plot_price_ = 0, plot_shops_ = 0, plot_n_ = 0;  // .decode plot (local addition)
    int q4opp_first_ = 0, q4opp_last_ = -1, q4opp_money_ = 0;  // .decode q4opp (local addition)
    int earlycow_first_ = 0, earlycow_last_ = -1, earlycow_n_ = 0, earlycow_species_ = 1;  // .decode earlycow / earlyanimal (local addition)
    int q4mix_first_ = 0, q4mix_last_ = -1, q4mix_sheep_ = -1;  // .decode q4mix (local addition)
    double q4mix_shift_ = 0;
    int crop_bias_crop_ = -1, crop_bias_first_ = 0, crop_bias_last_ = -1;  // .decode crop_bias (local addition)
    double crop_bias_ = 0;
    struct ShareBias { int animal, index, first, last; double bias; };  // .decode cbias / abias (local addition)
    std::vector<ShareBias> share_biases_;
    struct YarnBias { int index, first, last, max_shops; double bias; };  // .decode yarnbias (local addition, step 48)
    std::vector<YarnBias> yarn_biases_;
    std::vector<std::pair<int, int>> solo_days_;  // .decode solo (local addition, step 53)
    double main_weight_ = 0;  // .decode mainweight (step 53; 0: equal weights)
    struct ShopBias { bool animal; int index, first, last, product, max_shops; double bias; };  // .decode shopcbias / shopabias (step 53)
    std::vector<ShopBias> shop_biases_;
    int earlycrop_crop_ = 3, earlycrop_first_ = 0, earlycrop_last_ = -1, earlycrop_n_ = 0;  // .decode earlycrop (local addition)
    int cowcut_first_ = 0, cowcut_last_ = -1, cowbank_ = 0;  // .decode cowcut (local addition)
    int harvestall_crop_ = -1, harvestall_first_ = 0, harvestall_last_ = -1;  // .decode harvestall (local, sep28_top_lb_imitation)
    int pertype_first_ = 0, pertype_last_ = -1;
    int mainshare_first_ = 0, mainshare_last_ = -1;  // .decode mainshare (BC step 64; ported by Imitation)  // .decode pertype (local addition, step 47)
    int animal_bias_sp_ = -1, animal_bias_first_ = 0, animal_bias_last_ = -1;  // .decode animal_bias (local addition)
    double animal_bias_ = 0;
    int landpush_first_ = 0, landpush_last_ = -1;  // .decode landpush (local addition)
    int landpush_tomato_ = 0;  // .decode landpushtom N (local, sep28_top_lb_imitation): landpush only with >= N tomato-consuming shops
    int landpush_fix_ = 0;  // .decode landpushfix 1 (local, sep28_top_lb_imitation): keep landpush's land bias (land_push used to clear it)
    int optrate_ = 0;  // .decode optrate (step 68)
    double landpush_bias_ = 0;
    int qpush_first_ = 0, qpush_last_ = -1;  // .decode qpush (local addition)
    double qpush_crop_ = 0.5, qpush_animal_ = 0.5;
    struct QDay { int day; double crop, animal; };  // .decode qday (local addition, step 49)
    std::vector<QDay> qdays_;
    int landin_mode_ = 0, landin_solo_ = 0;  // .decode landin (local addition, step 52)
    struct QDayOpp { int day, crop_index, max_count; double crop, animal; };  // .decode qdayopp (local addition, step 50)
    std::vector<QDayOpp> qdayopps_;
    int landcash_last_ = -1;  // .decode landcash (local addition)
    double landcash_frac_ = 0;
    int service_first_ = 0, service_last_ = -1, service_feed_ = 0, service_care_ = 0, service_q4_ = 0;  // .decode service (local addition)
    int push_first_ = 0, push_last_ = -1;  // decode config "land_push first last bias": land logit push on these days
    double push_bias_ = 0;
    bool env_ = true;
    dc10::History history_;
    std::shared_ptr<dc10::History> exact_history_;  // <model>.forecast_tf_exact (local addition, sep24_BC_opus)
    dc11::Options options_;
    dc11::Executor executor_;
    int planned_day_ = -1;
    std::vector<DayReport> reports_;
};
}
