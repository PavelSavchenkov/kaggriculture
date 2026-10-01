#include "agent/bc_overhaul/source/agent.hpp"
#include "dc11_local/sellmodel.hpp"
#include <array>
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <cstdlib>
#include <sstream>

namespace kag::agents::bc_overhaul {
using namespace dc11;

void Agent::reset(const agent::AgentInit&) {
    const std::string path = !model_path.empty() ? model_path : BC_OVERHAUL_DEFAULT_MODEL;
    auto model = std::make_shared<bc_opus::Model>();
    if (!model->load(path)) std::abort();
    model_ = model;
    ensemble_.clear();  // <model>.ensemble: members whose whole-farm head logits are averaged with the model's
    ensemble_opening_ = std::filesystem::exists(path + ".ensemble_opening");
    ensemble_from_ = 0;
    if (std::FILE* file = std::fopen((path + ".ensemble_from").c_str(), "r")) {
        if (std::fscanf(file, "%d", &ensemble_from_) != 1) std::abort();
        std::fclose(file);
    }
    ensemble_fields_.clear();
    if (std::FILE* file = std::fopen((path + ".ensemble_fields").c_str(), "r")) {
        for (char word[32]; std::fscanf(file, "%31s", word) == 1;) ensemble_fields_ += std::string(word) + " ";
        std::fclose(file);
    }
    if (std::FILE* file = std::fopen((path + ".ensemble").c_str(), "r")) {
        const size_t slash = path.rfind('/');
        for (char name[512]; std::fscanf(file, "%511s", name) == 1;) {
            auto member = std::make_shared<bc_opus::Model>();
            if (!member->load(name[0] == '/' || slash == std::string::npos ? name : path.substr(0, slash + 1) + name)) std::abort();
            ensemble_.push_back(member);
        }
        std::fclose(file);
    }
    reach_q_.clear();
    reach_first_ = 0, reach_last_ = -1, max_land_ = 4, reach_stress_ = false;
    match_first_ = 0, match_last_ = -1, match_bias_ = 0;
    v219_day_ = -1, plot_day_ = -1, q4opp_first_ = 0, q4opp_last_ = -1, earlycow_first_ = 0, earlycow_last_ = -1;
    q4mix_first_ = 0, q4mix_last_ = -1, crop_bias_crop_ = -1, earlycow_species_ = 1, service_first_ = 0, service_last_ = -1;
    landcash_last_ = -1, qpush_first_ = 0, qpush_last_ = -1, landpush_first_ = 0, landpush_last_ = -1, animal_bias_sp_ = -1;
    cowcut_first_ = 0, cowcut_last_ = -1, cowbank_ = 0, earlycrop_first_ = 0, earlycrop_last_ = -1, earlycrop_n_ = 0;
    pertype_first_ = 0, pertype_last_ = -1;
    mainshare_first_ = 0, mainshare_last_ = -1;  // BC step 64 (ported)
    landpush_tomato_ = 0, landpush_fix_ = 0, harvestall_crop_ = -1, optrate_ = 0;
    share_biases_.clear();
    landin_mode_ = 0, landin_solo_ = 0;
    qdays_.clear();
    qdayopps_.clear();
    yarn_biases_.clear();
    solo_days_.clear(), shop_biases_.clear(), main_weight_ = 0;
    if (std::FILE* file = std::fopen((path + ".decode").c_str(), "r")) {
        char key[64];
        for (double a = 0, b = 0; std::fscanf(file, "%63s", key) == 1;) {
            const std::string k = key;
            if (k == "reach_days" && std::fscanf(file, "%lf %lf", &a, &b) == 2) reach_first_ = int(a), reach_last_ = int(b);
            else if (k == "reach")
                for (double q; std::fscanf(file, "%lf", &q) == 1;) reach_q_.push_back(q);
            else if (k == "reach_stress" && std::fscanf(file, "%lf", &a) == 1) reach_stress_ = a != 0;
            else if (k == "max_land" && std::fscanf(file, "%lf", &a) == 1) max_land_ = int(a);
            else if (k == "land_match" && std::fscanf(file, "%d %d %lf", &match_first_, &match_last_, &match_bias_) == 3) {}
            else if (k == "v219" && std::fscanf(file, "%d %d %d %d %d", &v219_day_, &v219_money_, &v219_price_, &v219_shops_, &v219_n_) == 5) {}
            else if (k == "plot" && std::fscanf(file, "%d %d %d %d %d %d", &plot_day_, &plot_money_, &plot_crop_, &plot_price_, &plot_shops_, &plot_n_) == 6) {}
            else if (k == "q4opp" && std::fscanf(file, "%d %d %d", &q4opp_first_, &q4opp_last_, &q4opp_money_) == 3) {}
            else if (k == "earlycow" && std::fscanf(file, "%d %d %d", &earlycow_first_, &earlycow_last_, &earlycow_n_) == 3) {}
            else if (k == "earlyanimal" && std::fscanf(file, "%d %d %d %d", &earlycow_species_, &earlycow_first_, &earlycow_last_, &earlycow_n_) == 4) {}
            else if (k == "q4mix" && std::fscanf(file, "%d %d %lf %d", &q4mix_first_, &q4mix_last_, &q4mix_shift_, &q4mix_sheep_) == 4) {}
            else if (k == "crop_bias" && std::fscanf(file, "%d %lf %d %d", &crop_bias_crop_, &crop_bias_, &crop_bias_first_, &crop_bias_last_) == 4) {}
            else if (k == "landcash" && std::fscanf(file, "%d %lf", &landcash_last_, &landcash_frac_) == 2) {}
            else if (k == "cowcut" && std::fscanf(file, "%d %d", &cowcut_first_, &cowcut_last_) == 2) {}
            else if (k == "landpushtom" && std::fscanf(file, "%d", &landpush_tomato_) == 1) {}  // local (sep28_top_lb_imitation)
            else if (k == "landpushfix" && std::fscanf(file, "%d", &landpush_fix_) == 1) {}  // local (sep28_top_lb_imitation)
            else if (k == "optrate" && std::fscanf(file, "%d", &optrate_) == 1) {}  // step 68
            else if (k == "harvestall" && std::fscanf(file, "%d %d %d", &harvestall_crop_, &harvestall_first_, &harvestall_last_) == 3) {}  // local (sep28_top_lb_imitation)
            else if (k == "pertype" && std::fscanf(file, "%d %d", &pertype_first_, &pertype_last_) == 2) {}
            else if (int a, b; k == "solo" && std::fscanf(file, "%d %d", &a, &b) == 2) solo_days_.push_back({a, b});
            else if (k == "mainweight" && std::fscanf(file, "%lf", &main_weight_) == 1) {}
            else if (ShopBias s{}; (k == "shopcbias" || k == "shopabias") &&
                     std::fscanf(file, "%d %lf %d %d %d %d", &s.index, &s.bias, &s.first, &s.last, &s.product, &s.max_shops) == 6) {
                s.animal = k == "shopabias";
                if (s.index < 0 || s.index >= (s.animal ? N_ANIMALS : N_CROPS) || s.product < 0 || s.product >= N_PRODUCTS) std::abort();
                shop_biases_.push_back(s);
            }
            else if (YarnBias y{}; k == "yarnbias" && std::fscanf(file, "%d %lf %d %d %d", &y.index, &y.bias, &y.first, &y.last, &y.max_shops) == 5) {
                if (y.index < 0 || y.index >= N_ANIMALS) std::abort();
                yarn_biases_.push_back(y);
            }
            else if (ShareBias b{}; (k == "cbias" || k == "abias") && std::fscanf(file, "%d %lf %d %d", &b.index, &b.bias, &b.first, &b.last) == 4) {
                b.animal = k == "abias";
                if (b.index < 0 || b.index >= (b.animal ? N_ANIMALS : N_CROPS)) std::abort();
                share_biases_.push_back(b);
            }
            else if (k == "mainshare" && std::fscanf(file, "%d %d", &mainshare_first_, &mainshare_last_) == 2) {}  // BC step 64 (ported)
            else if (k == "earlycrop" && std::fscanf(file, "%d %d %d %d", &earlycrop_crop_, &earlycrop_first_, &earlycrop_last_, &earlycrop_n_) == 4) {
                if (earlycrop_crop_ < 0 || earlycrop_crop_ >= N_CROPS) std::abort();  // earlycrop one-shot crops too (step 41)
            }
            else if (k == "animal_bias" && std::fscanf(file, "%d %lf %d %d", &animal_bias_sp_, &animal_bias_, &animal_bias_first_, &animal_bias_last_) == 4) {}
            else if (k == "landpush" && std::fscanf(file, "%d %d %lf", &landpush_first_, &landpush_last_, &landpush_bias_) == 3) {}
            else if (k == "qpush" && std::fscanf(file, "%d %d %lf %lf", &qpush_first_, &qpush_last_, &qpush_crop_, &qpush_animal_) == 4) {}
            else if (k == "landin" && std::fscanf(file, "%d %d", &landin_mode_, &landin_solo_) == 2) {}
            else if (QDayOpp q{}; k == "qdayopp" && std::fscanf(file, "%d %lf %lf %d %d", &q.day, &q.crop, &q.animal, &q.crop_index, &q.max_count) == 5) qdayopps_.push_back(q);
            else if (QDay q{}; k == "qday" && std::fscanf(file, "%d %lf %lf", &q.day, &q.crop, &q.animal) == 3) qdays_.push_back(q);
            else if (k == "service" && std::fscanf(file, "%d %d %d %d %d", &service_first_, &service_last_, &service_feed_, &service_care_, &service_q4_) == 5) {}
            else if (k == "land_push" && std::fscanf(file, "%d %d %lf", &push_first_, &push_last_, &push_bias_) == 3) {}
            else std::abort();
        }
        std::fclose(file);
    }
    env_ = options_text.empty();  // experiment overrides from the environment only without explicit options
    if (const char* m = env_ ? std::getenv("DC11_MAX_LAND") : nullptr) max_land_ = std::atoi(m);
    if (const char* r = env_ ? std::getenv("DC11_REACH") : nullptr) {  // "first last q ..." (herd reach days and quantiles)
        std::istringstream in(r);
        reach_q_.clear();
        if (!(in >> reach_first_ >> reach_last_)) std::abort();
        for (double q; in >> q;) reach_q_.push_back(q);
    }
    if (const char* r = env_ ? std::getenv("DC11_REACH_STRESS") : nullptr) reach_stress_ = std::atoi(r) != 0;
    options_ = options_text.empty() ? options_from_env() : options_text == "-" ? Options{} : parse_options(options_text.c_str());
    sell_model_.reset();
    if (options_.sell_model_on) {  // sellmodel=1: <model>.sellmodel must load (fail fast)
        auto m = std::make_shared<bcsell::Model>();
        if (!m->load(path + ".sellmodel")) std::abort();
        sell_model_ = m;
        options_.sell_model = sell_model_.get();
        sell_tracker_ = std::make_shared<bcsell::Tracker>();
        options_.sell_tracker = sell_tracker_.get();
    }
    forecast_.reset();  // <model>.forecast: learned opponent-sales forecast for the day market
    if (auto learned = std::make_shared<fcast::LearnedForecast>(); learned->load(path + ".forecast")) forecast_ = learned;
    if (std::filesystem::exists(path + ".forecast_tf")) {  // <model>.forecast_tf: transformer opponent forecast
        struct TfState {
            fcmodel::Transformer tf;
            std::vector<fcmodel::Transformer> extra;  // <model>.forecast_tf.2, .3, ... (local addition): averaged
            std::vector<fcmodel::Transformer> next;   // <model>.forecast_tf_next (local addition): next-morning supply only
            int day = 0;
            bool off = false;
            double out[HOURS][N_PRODUCTS]{};
            double learned_days[30][HOURS][N_PRODUCTS]{}, own_days[30][HOURS][N_PRODUCTS]{};  // .forecast_tf_select
            bool stored[30]{};
            bool pick[N_PRODUCTS] = {true, true, true, true, true, true, true, true, true};
            explicit TfState(const std::string& file) : tf(file) { tf.reset(); }
        };
        const auto st = std::make_shared<TfState>(path + ".forecast_tf");
        if (std::filesystem::exists(path + ".forecast_tf_next")) st->next.emplace_back(path + ".forecast_tf_next");
        for (int k = 2; std::filesystem::exists(path + ".forecast_tf." + std::to_string(k)); ++k)
            st->extra.emplace_back(path + ".forecast_tf." + std::to_string(k));
        exact_history_.reset();
        if (std::filesystem::exists(path + ".forecast_tf_exact")) {
            exact_history_ = std::make_shared<dc10::History>();
            exact_history_->exact_shed = true;
        }
        const auto exact = exact_history_;
        auto use = std::make_shared<std::array<bool, N_PRODUCTS>>();  // <model>.forecast_tf_products (local addition)
        for (int p = CARROT; p <= WOOL; ++p) (*use)[p] = true;
        if (std::FILE* file = std::fopen((path + ".forecast_tf_products").c_str(), "r")) {
            use->fill(false);
            for (int p; std::fscanf(file, "%d", &p) == 1;)
                if (p >= CARROT && p <= WOOL) (*use)[p] = true;
            std::fclose(file);
        }
        const double blend = options_.market.blend;
        double select = 0;  // <model>.forecast_tf_select (local addition): 0 off
        if (std::FILE* file = std::fopen((path + ".forecast_tf_select").c_str(), "r")) {
            if (std::fscanf(file, "%lf", &select) != 1) std::abort();
            std::fclose(file);
        }
        options_.market.learned = [st, exact, blend, use, select](const agent::AgentObservation& o, const dc10::History& hist, double (*rival)[N_PRODUCTS]) {
            if (st->off || o.day < 1 || o.day > 28) return;
            if (st->day != o.day) {
                double seller[HOURS][N_PRODUCTS];
                if (exact) dc11::forecast(o, *exact, seller, 0, blend);
                else std::copy_n(&rival[0][0], HOURS * N_PRODUCTS, &seller[0][0]);
                try {
                    st->tf.predict(o, exact ? *exact : hist, seller, st->out);
                    for (auto& tf : st->extra) {
                        double more[HOURS][N_PRODUCTS]{};
                        tf.predict(o, exact ? *exact : hist, seller, more);
                        for (int t = 0; t < HOURS; ++t)
                            for (int p = CARROT; p <= WOOL; ++p) st->out[t][p] += more[t][p];
                    }
                    for (int t = 0; t < HOURS; ++t)
                        for (int p = CARROT; p <= WOOL; ++p) st->out[t][p] /= double(1 + st->extra.size());
                    for (auto& tf : st->next) {  // keeps the next-morning model's dawn state current
                        double unused[HOURS][N_PRODUCTS]{};
                        tf.predict(o, exact ? *exact : hist, seller, unused);
                    }
                } catch (const std::exception& e) {
                    std::fprintf(stderr, "forecast_tf off: %s\n", e.what());
                    st->off = true;
                    return;
                }
                st->day = o.day;
                if (select > 0 && o.day < 30) {  // .forecast_tf_select: per product, learned vs dc11's forecast
                    std::copy_n(&st->out[0][0], HOURS * N_PRODUCTS, &st->learned_days[o.day][0][0]);
                    std::copy_n(&rival[0][0], HOURS * N_PRODUCTS, &st->own_days[o.day][0][0]);
                    st->stored[o.day] = true;
                    const dc10::History& actual = exact ? *exact : hist;
                    for (int p = CARROT; p <= WOOL; ++p) {
                        double err_l = 0, err_o = 0;
                        int n = 0;
                        for (int k = std::max(1, o.day - 3); k < o.day; ++k) {
                            if (!st->stored[k]) continue;
                            double cl = 0, co = 0, ca = 0;
                            for (int h = 0; h < HOURS; ++h) {
                                cl += std::max(0.0, st->learned_days[k][h][p]), co += std::max(0.0, st->own_days[k][h][p]);
                                ca += std::max(0, actual.flow_at(k * HOURS + h, p));
                                err_l += std::abs(cl - ca), err_o += std::abs(co - ca);
                            }
                            ++n;
                        }
                        st->pick[p] = n == 0 || err_l <= select * err_o;
                    }
                }
            }
            for (int t = 0; t < HOURS; ++t)
                for (int p = CARROT; p <= WOOL; ++p)
                    if ((*use)[p] && st->pick[p]) rival[t][p] = st->out[t][p];
        };
        if (std::filesystem::exists(path + ".forecast_tf_nextm")) {  // local addition: learned next-morning supply (FCT5)
            if (!exact || (st->next.empty() ? st->tf.next_hours() : st->next[0].next_hours()) <= 0) std::abort();
            options_.market.next_morning = [st, exact](const agent::AgentObservation& o, const double (*rival)[N_PRODUCTS], int* hold) {
                if (st->off || st->day != o.day) return;
                const fcmodel::Transformer& nm = st->next.empty() ? st->tf : st->next[0];
                const int H = std::min(nm.next_hours(), HOURS / 2);
                int visible[N_PRODUCTS];
                dc10::visible_supply(o, visible);
                double all_seen[HOURS]{};
                for (int h = 0; h < o.hour; ++h)
                    for (int q = CARROT; q <= WOOL; ++q) all_seen[h] += std::max(0, exact->flow_at(o.day * HOURS + h, q));
                for (int p = CARROT; p <= WOOL; ++p) {
                    double seen[HOURS]{}, out[HOURS]{}, next[HOURS]{}, head = 0, today = 0;
                    for (int h = 0; h < o.hour; ++h) seen[h] = std::max(0, exact->flow_at(o.day * HOURS + h, p));
                    nm.intraday(p, seen, o.hour, out, exact->opponent_stock()[p], visible[p], all_seen, next);
                    const size_t n = st->next.empty() ? 1 + st->extra.size() : 1;
                    if (st->next.empty())
                        for (const auto& tf : st->extra) {
                            double o2[HOURS]{}, n2[HOURS]{};
                            tf.intraday(p, seen, o.hour, o2, exact->opponent_stock()[p], visible[p], all_seen, n2);
                            for (int h = 0; h < H; ++h) next[h] += n2[h];
                        }
                    for (int h = 0; h < H; ++h) head += next[h] / double(n), today += std::max(0.0, rival[h][p]);
                    hold[p] += int(std::lround(head - today));  // may be negative (tomorrow=2 form)
                }
            };
        }
        if (std::filesystem::exists(path + ".forecast_tf_intra")) {  // intra-day head (needs the exact History)
            if (!exact || !st->tf.has_intraday()) std::abort();
            for (const auto& tf : st->extra)
                if (!tf.has_intraday()) std::abort();
            options_.market.intraday = [st, exact, use](const agent::AgentObservation& o, double (*rival)[N_PRODUCTS]) {
                if (st->off || st->day != o.day || o.hour < 1) return;
                int visible[N_PRODUCTS];  // stock / xseen inputs of FCT4 models (local addition)
                dc10::visible_supply(o, visible);
                double all_seen[HOURS]{};
                for (int h = 0; h < o.hour; ++h)
                    for (int q = CARROT; q <= WOOL; ++q) all_seen[h] += std::max(0, exact->flow_at(o.day * HOURS + h, q));
                for (int p = CARROT; p <= WOOL; ++p) {
                    if (!(*use)[p] || !st->pick[p]) continue;
                    double seen[HOURS]{}, out[HOURS]{};
                    for (int h = 0; h < o.hour; ++h) seen[h] = std::max(0, exact->flow_at(o.day * HOURS + h, p));
                    st->tf.intraday(p, seen, o.hour, out, exact->opponent_stock()[p], visible[p], all_seen);
                    for (const auto& tf : st->extra) {
                        double more[HOURS]{};
                        tf.intraday(p, seen, o.hour, more, exact->opponent_stock()[p], visible[p], all_seen);
                        for (int h = o.hour; h < HOURS; ++h) out[h] += more[h];
                    }
                    for (int h = o.hour; h < HOURS; ++h) out[h] /= double(1 + st->extra.size());
                    for (int h = o.hour; h < HOURS; ++h) rival[h][p] = out[h];
                }
            };
        }
    }
    if (forecast_) {
        const auto learned = forecast_;
        options_.market.learned = [learned](const agent::AgentObservation& o, const dc10::History& hist, double (*rival)[N_PRODUCTS]) {
            learned->apply(o, hist, rival);
        };
    }
    if (dc11_oracle_flow) {
        if (const char* mode = std::getenv("DC12_ORACLE_MODE"))  // probe: forecast / truth mixes (market.hpp oracle_mix)
            options_.market.oracle_mix = dc11_oracle_flow, options_.market.oracle_mode = std::atoi(mode);
            if (const char* mask = std::getenv("DC12_ORACLE_PRODUCTS")) options_.market.oracle_products = unsigned(std::atoi(mask));
            if (const char* band = std::getenv("DC12_ORACLE_HOURS")) std::sscanf(band, "%d-%d", &options_.market.oracle_first, &options_.market.oracle_last);
        else options_.market.oracle = dc11_oracle_flow, options_.market.intraday = nullptr;
    }
    history_ = dc10::History{};
    executor_ = Executor{};
    planned_day_ = -1;
    reports_.clear();
}

void Agent::observe(const agent::AgentObservation& obs, const Action& submitted) {  // tools_dc11/teacher_day (local)
    if (exact_history_) exact_history_->advance(obs);
    if (obs.hour == 0 && options_.market.learned) {  // the learned forecasters take every dawn in order, as act()'s compiles do
        dc10::History seen = history_;
        if (options_.fresh_history) seen.observe(obs, Action{});
        double rival[HOURS][N_PRODUCTS]{};
        options_.market.learned(obs, seen, rival);
    }
    if (exact_history_) exact_history_->observe(obs, submitted);
    history_.observe(obs, submitted);
}

dc11::DayMarket Agent::seller_market(const agent::AgentObservation& dawn) {  // local, Imitation (tools_dc11/duel_mm)
    dc10::History seen = history_;
    if (options_.fresh_history) seen.observe(dawn, Action{});
    return dc11::day_market(dawn, seen, options_.market);
}

bool Agent::model_flags(const agent::AgentObservation& obs, bool flag[N_PRODUCTS]) {
    std::fill_n(flag, N_PRODUCTS, false);
    if (sell_tracker_) sell_tracker_->update(obs);  // every hour (level inputs)
    if (!sell_model_ || options_.sell_model_mode < 3 || obs.day > options_.sell_model_last || obs.day < options_.sell_model_first) return false;
    if (obs.day != rec_day_) rec_day_ = obs.day, std::fill_n(rec_sold_, N_PRODUCTS, 0);
    unsigned long long seed = 1469598103934665603ull ^ obs.player;
    for (int k = 0; k < obs.n_shops; ++k) seed = (seed ^ (obs.shops[k] + 1)) * 1099511628211ull;
    int units[N_PRODUCTS];
    bcsell::sell_units(*sell_model_, obs, history_, rec_sold_, seed, units, true, sell_tracker_.get());
    for (int p : {int(STRAWBERRY), int(EGG), int(MILK), int(WOOL)}) flag[p] = units[p] > 0;
    return true;
}

void Agent::model_record(const agent::AgentObservation& obs, const int sell[N_PRODUCTS]) {
    if (!sell_model_ || options_.sell_model_mode < 3) return;
    if (obs.day != rec_day_) rec_day_ = obs.day, std::fill_n(rec_sold_, N_PRODUCTS, 0);
    for (int p = 0; p < N_PRODUCTS; ++p) rec_sold_[p] += std::min<int>(sell[p], obs.own.shed[p]);
}

void Agent::model_sales(const agent::AgentObservation& obs, int sell[N_PRODUCTS], bool room_binds) {
    if (sell_tracker_) sell_tracker_->update(obs);  // every hour (level inputs)
    const int mm = options_.sell_model_mode;
    if (!sell_model_ || (mm > 2 && mm != 5) || obs.day > options_.sell_model_last || obs.day < options_.sell_model_first) return;
    if (obs.day != rec_day_) rec_day_ = obs.day, std::fill_n(rec_sold_, N_PRODUCTS, 0);
    unsigned long long seed = 1469598103934665603ull ^ obs.player;
    for (int k = 0; k < obs.n_shops; ++k) seed = (seed ^ (obs.shops[k] + 1)) * 1099511628211ull;
    int units[N_PRODUCTS];
    bcsell::sell_units(*sell_model_, obs, history_, rec_sold_, seed, units, mm == 2 || mm == 5, sell_tracker_.get());
    for (int p : {int(STRAWBERRY), int(EGG), int(MILK), int(WOOL)}) {
        sell[p] = room_binds && mm != 5 ? std::max(units[p], sell[p]) : units[p];
        rec_sold_[p] += std::min<int>(sell[p], obs.own.shed[p]);
    }
}

void Agent::seller_hour(const agent::AgentObservation& obs, dc11::DayMarket& market) {  // local, Imitation (tools_dc11/duel_mm)
    // Mirrors the Executor's per-hour market setup (dc11/compiler.cpp, before choose_sales), except the parts that need our own
    // compiled plan (scenario gating on planned purchases, regime pockets, holdown): the recorded farm has no plan.
    const int h = obs.hour;
    if (options_.market.intraday) options_.market.intraday(obs, market.rival);
    dc11::mix_oracle(options_.market, obs.day, h, market.rival);
    if (options_.market.stock_cap && !options_.market.oracle) dc11::cap_by_stock(obs, history_, market.rival, h, HOURS);
    if (market.lumpy)
        for (int t = 0; t < HOURS; ++t)
            for (int p = 0; p < N_PRODUCTS; ++p) market.lump_q[t][p] = history_.sell_frequency(obs.day, 5, t, p);
    if (market.race || market.response || market.leader) {
        int visible[N_PRODUCTS];
        visible_supply(obs, visible);
        int animals = 0;
        for (int y = 0; y < BOARD; ++y)
            for (int x = 0; x < BOARD; ++x) animals += obs.opponent().tiles[y][x].has_animal;
        for (int p = 0; p < N_PRODUCTS; ++p) {
            market.race_level[p] = history_.mean_inventory(obs.step, HOURS, p) - (market.race ? 2 : 0);
            market.race_opp[p] = p == FERTILIZER ? animals : history_.opponent_stock()[p] + visible[p];
            market.race_stock[p] = p == FERTILIZER ? animals : history_.opponent_stock()[p];
        }
        if (market.leader > 0) history_.own_expected(obs.day, 3, market.own_hist);
    }
    if (options_.regime_on(obs.day)) {
        market.carry_wait = options_.regime_wait;
        market.carry_hold = options_.regime_hold;
        for (int p = 0; p < N_PRODUCTS; ++p) market.carry[p] = options_.regime_carry >> p & 1, market.dawn[p] = options_.regime_dawn >> p & 1;
    }
    if (options_.market.next_morning && obs.day < LAST_DAY - 1) options_.market.next_morning(obs, market.rival, market.hold_supply);
    if (obs.day >= LAST_DAY && !options_.market.oracle)
        for (int p = 0; p < N_PRODUCTS; ++p)
            if (p != WHEAT && p != FERTILIZER) market.rival[h % HOURS][p] += history_.opponent_stock()[p];
}

void Agent::act(const agent::AgentObservation& obs, const agent::DecisionBudget& budget, Action& action) {
    if (exact_history_) exact_history_->advance(obs);  // <model>.forecast_tf_exact (local addition)
    if (obs.hour == 0 && obs.day != planned_day_) {
        planned_day_ = obs.day;
        if (sell_model_) {  // sellmodel: a per-game sampling seed from the shop draws (day-0 dawns look alike)
            unsigned long long seed = 1469598103934665603ull ^ obs.player;
            for (int k = 0; k < obs.n_shops; ++k) seed = (seed ^ (obs.shops[k] + 1)) * 1099511628211ull;
            options_.sell_seed = seed;
        }
        bc_opus::DecodeKnobs knobs{};
        knobs.max_quadrants = max_land_;
        knobs.opt_rate = optrate_ != 0;  // .decode optrate (step 68)
        if (obs.day >= qpush_first_ && obs.day <= qpush_last_) knobs.q_crop = qpush_crop_, knobs.q_animal = qpush_animal_;  // .decode qpush
        for (const QDay& q : qdays_)  // .decode qday (step 49)
            if (obs.day == q.day) knobs.q_crop = q.crop, knobs.q_animal = q.animal;
        for (const QDayOpp& q : qdayopps_) {  // .decode qdayopp (step 50)
            if (obs.day != q.day) continue;
            int plants = 0;
            for (int y = 0; y < BOARD; ++y)
                for (int x = 0; x < BOARD; ++x) {
                    const auto& t = obs.opponent().tiles[y][x];
                    plants += t.kind == T_PLANT && t.what == q.crop_index;
                }
            if (plants <= q.max_count) knobs.q_crop = q.crop, knobs.q_animal = q.animal;
        }
        int tomato_shops = 0;  // landpushtom (local, sep28_top_lb_imitation)
        for (int k = 0; k < obs.n_shops; ++k) tomato_shops += (SHOP_MASK[obs.shops[k]] >> TOMATO) & 1;
        const bool landpush = obs.day >= landpush_first_ && obs.day <= landpush_last_ && tomato_shops >= landpush_tomato_;
        if (landpush)  // .decode landpush (local addition)
            knobs.max_quadrants = 4, knobs.land_bias = landpush_bias_, knobs.push_first = landpush_first_, knobs.push_last = landpush_last_;
        if (obs.day <= landcash_last_ && obs.self().n_quadrants < 4 &&  // .decode landcash (local addition)
            obs.self().money < landcash_frac_ * LAND_PRICES[std::min(2, obs.self().n_quadrants - 1)])
            knobs.max_quadrants = std::min(knobs.max_quadrants, obs.self().n_quadrants);
        if (crop_bias_crop_ >= 0 && obs.day >= crop_bias_first_ && obs.day <= crop_bias_last_)  // .decode crop_bias (local addition)
            knobs.crop_bias[crop_bias_crop_] = float(crop_bias_);
        if (!yarn_biases_.empty()) {  // .decode yarnbias (step 48): shops that consume wool
            int wool_shops = 0;
            for (int k = 0; k < obs.n_shops; ++k) wool_shops += (SHOP_MASK[obs.shops[k]] >> WOOL) & 1;
            for (const YarnBias& y : yarn_biases_)
                if (obs.day >= y.first && obs.day <= y.last && wool_shops <= y.max_shops) knobs.animal_bias[y.index] += float(y.bias);
        }
        for (const ShopBias& s : shop_biases_) {  // .decode shopcbias / shopabias (step 53)
            if (obs.day < s.first || obs.day > s.last) continue;
            int shops = 0;
            for (int k = 0; k < obs.n_shops; ++k) shops += (SHOP_MASK[obs.shops[k]] >> s.product) & 1;
            if (shops <= s.max_shops) (s.animal ? knobs.animal_bias : knobs.crop_bias)[s.index] += float(s.bias);
        }
        for (const ShareBias& b : share_biases_)  // .decode cbias / abias (local addition)
            if (obs.day >= b.first && obs.day <= b.last) (b.animal ? knobs.animal_bias : knobs.crop_bias)[b.index] += float(b.bias);
        if (animal_bias_sp_ >= 0 && obs.day >= animal_bias_first_ && obs.day <= animal_bias_last_)  // .decode animal_bias (local addition)
            knobs.animal_bias[animal_bias_sp_] = float(animal_bias_);
        if (obs.day >= service_first_ && obs.day <= service_last_ && (!service_q4_ || obs.self().n_quadrants >= 4))  // .decode service (local addition)
            knobs.feed_all = service_feed_ != 0, knobs.care_all = service_care_ != 0;
        knobs.push_first = push_first_, knobs.push_last = push_last_, knobs.land_bias = push_bias_;  // decode config land_push
        if (landpush && landpush_fix_)  // .decode landpushfix (local, sep28_top_lb_imitation): the line above cleared landpush's bias
            knobs.push_first = landpush_first_, knobs.push_last = landpush_last_, knobs.land_bias = landpush_bias_;
        if (const char* push = env_ ? std::getenv("DC11_LAND_PUSH") : nullptr)  // experiment: "first last bias" (land logit)
            if (std::sscanf(push, "%d %d %lf", &knobs.push_first, &knobs.push_last, &knobs.land_bias) != 3) std::abort();
        if (obs.day < knobs.push_first || obs.day > knobs.push_last) knobs.land_bias = 0;  // decode_intent does not gate it
        // Land match (peer session, Sep 26): an opponent that owns the 4th quadrant is matched.
        if (obs.day >= match_first_ && obs.day <= match_last_ && obs.opponent().n_quadrants >= 4 && obs.self().n_quadrants == 3)
            knobs.max_quadrants = 4, knobs.land_bias = match_bias_;
        if (obs.day < model_->opening_days && model_->opening_style >= 0) knobs.style = model_->opening_style;
        knobs.per_type = obs.day >= pertype_first_ && obs.day <= pertype_last_;  // .decode pertype (step 47)
        if (landin_mode_ > 0 && model_->land_scale > 0) {  // .decode landin (step 52): today's land decision as the network's input
            auto fires = [&](int day, int money, int crop, int price, int min_shops) {  // as the v219 / plot lambdas
                int shops = 0;
                for (int k = 0; k < obs.n_shops; ++k) shops += (SHOP_MASK[obs.shops[k]] >> crop) & 1;
                return obs.day == day && obs.self().n_quadrants == 3 && obs.self().money >= money && obs.market.prices[crop] >= price &&
                       shops >= min_shops;
            };
            if (fires(v219_day_, v219_money_, TOMATO, v219_price_, v219_shops_) || fires(plot_day_, plot_money_, plot_crop_, plot_price_, plot_shops_))
                knobs.land_input = 1;
            else if (landin_mode_ >= 2 && obs.self().n_quadrants >= knobs.max_quadrants)
                knobs.land_input = -1;
        }
        const bool pushed = obs.day == push_day;  // tools/ei_dc11 (local addition)
        if (pushed) {
            if (push_q_animal >= 0) knobs.q_animal = push_q_animal;
            if (push_q_crop >= 0) knobs.q_crop = push_q_crop;
            knobs.land_bias += push_land;
        }
        std::vector<float> averaged;
        if (!ensemble_.empty() && obs.day >= ensemble_from_ && (ensemble_opening_ || obs.day >= model_->opening_days) &&
            !(landin_solo_ && knobs.land_input > 0) &&  // .decode landin solo (step 52)
            std::none_of(solo_days_.begin(), solo_days_.end(), [&](const auto& r) { return obs.day >= r.first && obs.day <= r.second; })) {  // .decode solo (step 53)  // the opening keeps its style unless .ensemble_opening
            bc_opus::DecodeKnobs head_knobs = knobs;  // logits only (step 46)
            head_knobs.logits_only = true;
            bc_opus::decode_intent(*model_, obs, history_, nullptr, &averaged, head_knobs);
            const std::vector<float> main_logits = averaged;  // .decode mainweight (step 53)
            for (const auto& member : ensemble_) {
                std::vector<float> logits;
                bc_opus::DecodeKnobs member_knobs{};
                if (obs.day < model_->opening_days) member_knobs.style = knobs.style;  // .ensemble_opening
                member_knobs.logits_only = true;
                bc_opus::decode_intent(*member, obs, history_, nullptr, &logits, member_knobs);
                if (logits.size() != averaged.size()) std::abort();
                for (size_t i = 0; i < logits.size(); ++i) averaged[i] += logits[i];
            }
            if (main_weight_ > 0)  // .decode mainweight (step 53): w * main + (1 - w) * mean of the members
                for (size_t i = 0; i < averaged.size(); ++i)
                    averaged[i] = float(main_weight_ * main_logits[i] + (1 - main_weight_) * (averaged[i] - main_logits[i]) / ensemble_.size());
            else
                for (auto& v : averaged) v /= float(1 + ensemble_.size());
            if (obs.day >= mainshare_first_ && obs.day <= mainshare_last_)  // .decode mainshare (BC step 64, ported)
                for (int i = 0; i < N_CROPS; ++i) averaged[11 * 101 + 1 + 101 + i] = main_logits[11 * 101 + 1 + 101 + i];
            if (!ensemble_fields_.empty()) {  // .ensemble_fields: the model's own logits outside the chosen segments
                std::vector<float> own;
                bc_opus::decode_intent(*model_, obs, history_, nullptr, &own, head_knobs);
                const int C = 101, counts = 11 * C, land = counts + 1, crop = land + C + 5, animal = crop + C + 3;
                auto keep = [&](const char* name, int from, int to) {
                    if (ensemble_fields_.find(std::string(name) + " ") == std::string::npos)
                        std::copy(own.begin() + from, own.begin() + std::min<int>(to, int(own.size())), averaged.begin() + from);
                };
                keep("counts", 0, counts), keep("land", counts, land), keep("crop", land, crop), keep("animal", crop, animal);
            }
            knobs.head_override = &averaged;  // also carried into the herd-reach decodes
        }
        auto v219 = [&](DayIntent& in) {  // .decode v219 (local addition): tomato-rich dawn buys the 4th quadrant
            if (obs.day != v219_day_ || obs.self().n_quadrants != 3 || obs.self().money < v219_money_ ||
                obs.market.prices[TOMATO] < v219_price_) return;
            int shops = 0;
            for (int k = 0; k < obs.n_shops; ++k) shops += (SHOP_MASK[obs.shops[k]] >> TOMATO) & 1;
            if (shops < v219_shops_) return;
            in.buy_land = true, in.new_crop[TOMATO] = int16_t(in.new_crop[TOMATO] + v219_n_);
            if (v219_n_ > 0 && !CROPS[TOMATO].ongoing) {  // v219 partition (step 51)
                auto& o = in.options[describe(obs).new_crop_group[TOMATO]][4];
                o = int16_t(o + v219_n_);
            }
        };
        auto plot = [&](DayIntent& in) {  // .decode plot (local addition): v219 for any crop
            if (obs.day != plot_day_ || obs.self().n_quadrants != 3 || obs.self().money < plot_money_ ||
                obs.market.prices[plot_crop_] < plot_price_) return;
            int shops = 0;
            for (int k = 0; k < obs.n_shops; ++k) shops += (SHOP_MASK[obs.shops[k]] >> plot_crop_) & 1;
            if (shops < plot_shops_) return;
            in.buy_land = true, in.new_crop[plot_crop_] = int16_t(in.new_crop[plot_crop_] + plot_n_);
            if (plot_n_ > 0 && !CROPS[plot_crop_].ongoing) {  // plot partition (step 51)
                auto& o = in.options[describe(obs).new_crop_group[plot_crop_]][4];
                o = int16_t(o + plot_n_);
            }
        };
        DayIntent intent = bc_opus::decode_intent(*model_, obs, history_, nullptr, nullptr, knobs);
        auto q4opp = [&](DayIntent& in) {  // .decode q4opp (local addition)
            if (obs.day >= q4opp_first_ && obs.day <= q4opp_last_ && obs.self().n_quadrants == 3 && obs.opponent().n_quadrants >= 4 &&
                obs.self().money >= q4opp_money_) in.buy_land = true;
        };
        auto fresh_fix = [&](DayIntent& in, int species) {  // keep fresh-group counts valid after a lowered new_animal (local addition)
            const Schema s = describe(obs);
            const int n = in.new_animal[species];
            for (int i = 0; i < s.n_animals; ++i)
                if (s.animals[i].fresh && s.animals[i].species == species) {
                    in.feed[i] = int16_t(std::min<int>(in.feed[i], n)), in.collect[i] = int16_t(std::min<int>(in.collect[i], n));
                    in.care[i] = int16_t(std::min<int>(in.care[i], in.feed[i]));
                }
        };
        auto q4mix = [&](DayIntent& in) {  // .decode q4mix (local addition)
            if (obs.day < q4mix_first_ || obs.day > q4mix_last_ || obs.self().n_quadrants < 4) return;
            int move = int(std::lround(q4mix_shift_ * (in.new_crop[TOMATO] + in.new_crop[WHEAT])));
            for (const int crop : {int(TOMATO), int(WHEAT)}) {
                const int m = std::min<int>(move, in.new_crop[crop]);
                in.new_crop[crop] = int16_t(in.new_crop[crop] - m), in.new_crop[STRAWBERRY] = int16_t(in.new_crop[STRAWBERRY] + m), move -= m;
            }
            if (q4mix_sheep_ >= 0) in.new_animal[2] = int16_t(std::min<int>(in.new_animal[2], q4mix_sheep_)), fresh_fix(in, 2);  // sheep
        };
        auto harvestall = [&](DayIntent& in) {  // .decode harvestall (local, sep28_top_lb_imitation): harvest every group of this
            // ongoing crop that holds product (the top teams harvest strawberries every production cycle)
            if (harvestall_crop_ < 0 || obs.day < harvestall_first_ || obs.day > harvestall_last_) return;
            const Schema s = describe(obs);
            for (int i = 0; i < s.n_crops; ++i) {
                const auto& g = s.crops[i];
                if (g.crop == harvestall_crop_ && !g.fresh && CROPS[g.crop].ongoing && g.yield > 0) in.harvest[i] = int16_t(g.size);
            }
        };
        auto cowcut = [&](DayIntent& in, bool commit) {  // .decode cowcut (local addition): banked early cows come off later cow asks
            if (obs.day < cowcut_first_ || obs.day > cowcut_last_ || cowbank_ <= 0) return;
            const int cut = std::min<int>(cowbank_, in.new_animal[1]);
            in.new_animal[1] = int16_t(in.new_animal[1] - cut);
            fresh_fix(in, 1);
            if (commit) cowbank_ -= cut;  // the main intent commits; herd-reach retries reuse the same bank
        };
        v219(intent), plot(intent), q4opp(intent), q4mix(intent), cowcut(intent, true), harvestall(intent);
        const bool teacher = obs.day == teacher_day;  // tools_dc11/teacher_day (local, sep28_top_lb_imitation)
        if (teacher) intent = teacher_intent;
        const bool planted = plant_mask && obs.day < int(plant_ok.size()) && plant_ok[obs.day];
        if (planted) {  // tools_dc11/duel_dc11 DUEL_PLANT (local, sep28_top_lb_imitation)
            const Schema s = describe(obs);
            const DayIntent& rec = plant_intent[obs.day];
            DayIntent in = intent;
            if (plant_mask & 4) in.buy_land = rec.buy_land && s.land_sites > 0;
            if (plant_mask & 1)
                for (int a = 0; a < N_ANIMALS; ++a) {
                    const int old = in.new_animal[a], n = std::max<int>(rec.new_animal[a], s.unplaced_dawn[a] - in.reserve[a]);
                    const int i = s.new_animal_group[a];
                    in.new_animal[a] = int16_t(n);
                    in.feed[i] = int16_t(old ? std::min<int>(n, std::lround(double(in.feed[i]) * n / old)) : n);
                    in.care[i] = int16_t(care_fixed(s.animals[i], obs.day) ? 0 : old ? std::min<int>(in.feed[i], std::lround(double(in.care[i]) * n / old)) : 0);
                    in.collect[i] = 0;
                }
            auto set_crop = [&](int c, int n) {
                const int i = s.new_crop_group[c];
                if (!CROPS[c].ongoing) {
                    const bool w = !option_fixed(s.crops[i], obs.day, 4), wf = !option_fixed(s.crops[i], obs.day, 6);
                    if (!w && !wf) n = 0;
                    const int old = in.new_crop[c];
                    const int f = !wf ? 0 : !w ? n : old ? std::min<int>(n, std::lround(double(in.options[i][6]) * n / old)) : 0;
                    in.options[i] = {};
                    in.options[i][6] = int16_t(f), in.options[i][4] = int16_t(n - f);
                }
                in.new_crop[c] = int16_t(n);
            };
            if (plant_mask & 2)
                for (int c = 0; c < N_CROPS; ++c) set_crop(c, rec.new_crop[c]);
            // Trim to today's sites: the largest new crop first, then animals above the dawn stock.
            while (new_entities(in) > free_sites(s, in)) {
                int best = -1;
                for (int c = 0; c < N_CROPS; ++c)
                    if (in.new_crop[c] > 0 && (best < 0 || in.new_crop[c] > in.new_crop[best])) best = c;
                if (best >= 0) { set_crop(best, in.new_crop[best] - 1); continue; }
                int a = -1;
                for (int k = 0; k < N_ANIMALS; ++k)
                    if (in.new_animal[k] + in.reserve[k] > s.unplaced_dawn[k] && in.new_animal[k] > 0) a = k;
                if (a < 0) break;
                in.new_animal[a] = int16_t(in.new_animal[a] - 1), fresh_fix(in, a);
            }
            if (validate(s, in).empty()) intent = in, ++plant_used;
            else ++plant_failed;
        }
        dc10::History seen = history_;  // dc11's view: with fresh_history, caught up to this observation
        if (options_.fresh_history) seen.observe(obs, Action{});
        Options options = options_;
        options.deadline = budget.soft_deadline;
        last_intent = intent, last_intent_day = obs.day;  // tools_dc11/options_diff (local, Imitation)
        if (intent_only) {
            final_intent = intent;
            return;
        }
        Plan plan = compile_day(obs, seen, intent, options);
        auto complete = [](const Plan& p) { return p.status == dc11::CompileStatus::Ok && p.fallback == dc11::KeepAll && !p.dropped; };
        // Herd reach: a larger new-animal quantile when the plan with it is also complete.
        if (!teacher && !planted && obs.day >= reach_first_ && obs.day <= reach_last_ && complete(plan) && !(pushed && push_q_animal >= 0)) {
            auto animals = [](const DayIntent& in) { return in.new_animal[0] + in.new_animal[1] + in.new_animal[2]; };
            double spent = plan.compile_ms;
            long used = plan.evaluations;  // the dawn's work budget covers the retries too
            for (const double q : reach_q_) {
                if (options.spent(used)) break;
                bc_opus::DecodeKnobs more = knobs;
                more.q_animal = q;
                DayIntent bigger = bc_opus::decode_intent(*model_, obs, history_, nullptr, nullptr, more);
                v219(bigger), plot(bigger), q4opp(bigger), q4mix(bigger), cowcut(bigger, false), harvestall(bigger);
                if (animals(bigger) <= animals(intent)) continue;
                Options left = options;
                left.max_evaluations = options.max_evaluations - used;
                Plan tried = compile_day(obs, seen, bigger, left);
                spent += tried.compile_ms;
                used += tried.evaluations;
                if (std::getenv("DC11_DAYLOG"))
                    std::fprintf(stderr, "reach d%d q%.2f animals %d -> %d status %d fallback %d dropped %d stress %d | %s\n", obs.day, q,
                                 animals(intent), animals(bigger), int(tried.status), tried.fallback, tried.dropped, int(tried.stress_funded),
                                 tried.reason.substr(0, 300).c_str());
                if (complete(tried) && (!reach_stress_ || tried.stress_funded)) {
                    tried.reason = "herd reach;" + tried.reason;
                    intent = bigger, plan = tried;
                    break;
                }
            }
            plan.compile_ms = spent;
        }
        if (!teacher && obs.day >= earlycow_first_ && obs.day <= earlycow_last_ && earlycow_n_ > 0 && complete(plan) &&
            plan.evaluations < options.max_evaluations) {  // .decode earlycow (local addition; earlycow after herd reach, step 43)
            DayIntent more = intent;
            more.new_animal[earlycow_species_] = int16_t(more.new_animal[earlycow_species_] + earlycow_n_);  // cows unless earlyanimal
            Options left = options;
            left.max_evaluations = options.max_evaluations - plan.evaluations;
            Plan tried = compile_day(obs, seen, more, left);
            tried.compile_ms += plan.compile_ms, tried.evaluations += plan.evaluations;
            if (complete(tried)) {
                tried.reason = "early cow;" + tried.reason, intent = more, plan = tried;
                if (earlycow_species_ == 1) cowbank_ += earlycow_n_;  // .decode cowcut bank (local addition)
            }
            else plan.compile_ms = tried.compile_ms, plan.evaluations = tried.evaluations;
        }
        if (!teacher && obs.day >= earlycrop_first_ && obs.day <= earlycrop_last_ && earlycrop_n_ > 0 && complete(plan)) {  // .decode earlycrop (local addition; earlycrop after herd reach, step 42)
            for (int n = earlycrop_n_; n >= 1 && plan.evaluations < options.max_evaluations; --n) {
                DayIntent more = intent;
                more.new_crop[earlycrop_crop_] = int16_t(more.new_crop[earlycrop_crop_] + n);
                if (!CROPS[earlycrop_crop_].ongoing) {  // one-shot: the new units get the fresh group's water option
                    auto& o = more.options[describe(obs).new_crop_group[earlycrop_crop_]][4];
                    o = int16_t(o + n);
                }
                Options left = options;
                left.max_evaluations = options.max_evaluations - plan.evaluations;
                Plan tried = compile_day(obs, seen, more, left);
                tried.compile_ms += plan.compile_ms, tried.evaluations += plan.evaluations;
                if (complete(tried)) {
                    tried.reason = "early crop;" + tried.reason, intent = more, plan = tried;
                    break;
                }
                plan.compile_ms = tried.compile_ms, plan.evaluations = tried.evaluations;
            }
        }
        final_intent = intent;  // tools_dc11/options_diff (local, Imitation)
        if (static const bool log = std::getenv("DC11_INTENTLOG") != nullptr; log)  // local (Imitation): asked intent per day (pre / final)
            std::fprintf(stderr, "intent p%d d%d money %.0f pre %d %d %d %d %d %d %d %d %d final %d %d %d %d %d %d %d %d %d status %d fallback %d dropped %d trims %d\n",
                         obs.player, obs.day, obs.self().money, last_intent.new_crop[0], last_intent.new_crop[1], last_intent.new_crop[2],
                         last_intent.new_crop[3], last_intent.new_crop[4], last_intent.new_animal[0], last_intent.new_animal[1],
                         last_intent.new_animal[2], int(last_intent.buy_land), intent.new_crop[0], intent.new_crop[1], intent.new_crop[2],
                         intent.new_crop[3], intent.new_crop[4], intent.new_animal[0], intent.new_animal[1], intent.new_animal[2],
                         int(intent.buy_land), int(plan.status), plan.fallback, plan.dropped, plan.trims);
        DayReport report;
        report.day = obs.day;
        report.status = int(plan.status);
        report.fallback = plan.fallback, report.hires = plan.hires, report.dropped = plan.dropped, report.trims = plan.trims;
        for (int c = 0; c < N_CROPS; ++c) report.new_crops += intent.new_crop[c];
        for (int h = 0; h < plan.hours && report.land_hour < 0; ++h)
            for (int k = 0; k < plan.actions[h].n_orders; ++k)
                if (plan.actions[h].orders[k].op == M_BUY_LAND) report.land_hour = h;
        report.stress_ok = plan.stress_funded;
        report.ms_route = plan.ms_route, report.ms_realize = plan.ms_realize, report.ms_fund = plan.ms_fund;
        for (int a = 0; a < N_ANIMALS; ++a) report.asked[a] = intent.new_animal[a], report.planned[a] = plan.new_animals[a];
        {
            const Schema s = describe(obs);
            for (int g = 0; g < s.n_animals; ++g) {
                report.animals += s.animals[g].size, report.feed_intent += intent.feed[g];
                report.groups += " s" + std::to_string(s.animals[g].species) + "n" + std::to_string(s.animals[g].size) + "u" +
                                 std::to_string(s.animals[g].unfed) + "f" + std::to_string(intent.feed[g]);
            }
        }
        report.stress_funded = plan.stress_funded;
        report.moves = plan.moves, report.unit_actions = plan.unit_actions;
        report.compile_ms = plan.compile_ms;
        report.night = plan.night_carried;
        report.skipped = plan.skipped;
        report.reason = plan.reason;
        if (std::getenv("DC11_DAYLOG"))
            std::fprintf(stderr, "daylog p%d d%d money %.0f status %s fallback %d hires %d dropped %d skipped %d trims %d stress %d ms %.1f | %s\n",
                         obs.player, obs.day, obs.self().money, status_name(plan.status), plan.fallback, plan.hires, plan.dropped, plan.skipped,
                         plan.trims, int(plan.stress_funded), plan.compile_ms, plan.reason.c_str());
        if (std::getenv("DC11_INTENTLOG")) {  // local addition (step 45)
            std::fprintf(stderr, "intentlog p%d d%d money %.0f animals %d %d %d crops", obs.player, obs.day, obs.self().money,
                         int(intent.new_animal[0]), int(intent.new_animal[1]), int(intent.new_animal[2]));
            for (int k = 0; k < N_CROPS; ++k) std::fprintf(stderr, " %d", int(intent.new_crop[k]));
            std::fprintf(stderr, " status %d fallback %d dropped %d trims %d | %s\n", int(plan.status), plan.fallback, plan.dropped,
                         plan.trims, plan.reason.c_str());
        }
        if (std::getenv("DC11_LANDHOUR"))  // local (Imitation): the network's land ask, the plan's land order hour (-1: none), dawn cash
            std::fprintf(stderr, "landhour p%d d%d ask %d hour %d money %.0f quads %d\n", obs.player, obs.day, int(intent.buy_land),
                         report.land_hour, obs.self().money, obs.self().n_quadrants);
        if (std::getenv("DC11_COLLECTLOG")) {  // animals, ready (held > 0) and intended collections vs bound and dropped
            const Schema schema = describe(obs);
            int animals = 0, ready = 0, want = 0, unfed = 0, capped = 0, lost = 0;  // capped: at the held cap, producing tonight
            int fed = 0, cared = 0, care_unfed = 0, bonus_lost = 0;  // intent: fed, cared, cared but unfed, bonus due tonight but unfed
            for (int i = 0; i < schema.n_animals; ++i) {
                const auto& g = schema.animals[i];
                if (g.fresh) continue;
                animals += g.size, unfed += g.unfed >= 1 ? g.size : 0;
                if (g.held > 0) ready += g.size, want += std::min<int>(intent.collect[i], g.size);
                const auto& a = ANIMALS[g.species];
                const int since = g.age + 1 - a.first_yield_day;
                if (g.held >= a.max_held && since >= 0 && since % a.interval == 0)
                    capped += g.size, lost += g.size - std::min<int>(intent.collect[i], g.size);
                const int f = std::min<int>(intent.feed[i], g.size), c = std::min<int>(intent.care[i], g.size);
                fed += f, cared += c, care_unfed += std::max(0, c - f);
                if (g.bonus > 0 && since >= 0 && since % a.interval == 0) bonus_lost += g.size - f;
            }
            int drops = 0, idle_eve = 0, moves_am = 0;  // deposit visits; hired idle turns h19+; moves h0-12
            for (int h = 0; h < plan.hours; ++h)
                for (int u = 0; u < plan.actions[h].n_units; ++u) {
                    const int op = plan.actions[h].units[u].op;
                    drops += op == OP_DROP;
                    idle_eve += u >= 1 && h >= 19 && op == OP_PASS;
                    moves_am += h <= 12 && op >= OP_NORTH && op <= OP_WEST;
                }
            std::fprintf(stderr, "collectlog p%d d%d quads %d animals %d ready %d want %d bound %d dropped %d unfed %d capped %d lost %d fallback %d status %d moves %d actions %d hires %d drops %d visits %d tiles %d revisits %d idle_eve %d moves_am %d fed %d cared %d care_unfed %d bonus_lost %d\n",
                         obs.player, obs.day, obs.self().n_quadrants, animals, ready, want, plan.collects, plan.collects_dropped, unfed,
                         capped, lost, plan.fallback, int(plan.status), plan.moves, plan.unit_actions, plan.hires, drops, plan.visits,
                         plan.tiles_worked, plan.revisits, idle_eve, moves_am, fed, cared, care_unfed, bonus_lost);
        }
        if (const char* d = std::getenv("DC11_PLAN"); d && std::atoi(d) == obs.day) {  // the day's plan, hour by hour
            std::fprintf(stderr, "night items:");
            for (int p = 0; p < N_PRODUCTS; ++p) std::fprintf(stderr, " %d", plan.night_items[p]);
            std::fprintf(stderr, "\n");
            for (int h = 0; h < plan.hours; ++h) {
                std::fprintf(stderr, "h%d:", h);
                for (int u = 0; u < plan.actions[h].n_units; ++u) std::fprintf(stderr, " %d", plan.actions[h].units[u].op);
                std::fprintf(stderr, " | orders");
                for (int k = 0; k < plan.actions[h].n_orders; ++k)
                    std::fprintf(stderr, " %d:%d:%d", plan.actions[h].orders[k].op, plan.actions[h].orders[k].item, plan.actions[h].orders[k].n);
                std::fprintf(stderr, " | receipts");
                for (int p = 0; p < N_PRODUCTS; ++p) std::fprintf(stderr, " %d", plan.receipts[h][p]);
                std::fprintf(stderr, "\n");
            }
        }
        if (plan.status != dc11::CompileStatus::Ok) {  // no schedule: markets only
            Plan idle;
            idle.hours = last_hour(obs.day) + 1;
            idle.status = plan.status;
            plan = idle;
        }
        reports_.push_back(report);
        executor_.start(obs, seen, plan, options_);
    }
    dc11::live_seller = true;  // probes log the live executor only
    executor_.act(obs, history_, action);
    dc11::live_seller = false;
    history_.observe(obs, action);
    if (exact_history_) exact_history_->observe(obs, action);
}
}
