"""Local additions to the vendored dc11 agent (agent/bc_overhaul/source), all off by default:
<model>.ensemble (whole-farm head logits averaged over the model and its members, days after the opening),
<model>.ensemble_opening (also on opening days, members get the opening style; rejected, kept for tests),
<model>.ensemble_fields (average only these head segments: counts land crop animal),
push_day/push_q_animal/push_q_crop/push_land (tools_dc11/ei_dc11). Each step is idempotent."""
from pathlib import Path

E = Path(__file__).resolve().parents[1] / "agent/bc_overhaul/source"
h, c = E / "agent.hpp", E / "agent.cpp"


def sub(text, old, new):
    assert text.count(old) == 1, f"anchor not found: {old[:70]}"
    return text.replace(old, new)


# Step 1: ensemble, opening ensemble, one-dawn decode push.
hs, cs = h.read_text(), c.read_text()
if "ensemble_opening_" not in hs:
    hs = sub(hs, "    std::string options_text;", """    std::string options_text;
    // tools/ei_dc11 (local addition, sep24_BC_opus): decode push on one dawn only (push_day -1: off). A pushed
    // new-animal quantile also skips that dawn's herd reach.
    int push_day = -1;
    double push_q_animal = -1, push_q_crop = -1, push_land = 0;""")
    hs = sub(hs, "    std::shared_ptr<const bc_opus::Model> model_;", """    std::shared_ptr<const bc_opus::Model> model_;
    std::vector<std::shared_ptr<const bc_opus::Model>> ensemble_;  // <model>.ensemble (local addition, sep24_BC_opus)
    bool ensemble_opening_ = false;  // <model>.ensemble_opening: also on opening days (members get the opening style)""")
    cs = sub(cs, "    model_ = model;\n", """    model_ = model;
    ensemble_.clear();  // <model>.ensemble: members whose whole-farm head logits are averaged with the model's
    ensemble_opening_ = std::filesystem::exists(path + ".ensemble_opening");
    if (std::FILE* file = std::fopen((path + ".ensemble").c_str(), "r")) {
        const size_t slash = path.rfind('/');
        for (char name[512]; std::fscanf(file, "%511s", name) == 1;) {
            auto member = std::make_shared<bc_opus::Model>();
            if (!member->load(name[0] == '/' || slash == std::string::npos ? name : path.substr(0, slash + 1) + name)) std::abort();
            ensemble_.push_back(member);
        }
        std::fclose(file);
    }
""")
    cs = sub(cs, "        if (obs.day < model_->opening_days && model_->opening_style >= 0) knobs.style = model_->opening_style;\n",
             """        if (obs.day < model_->opening_days && model_->opening_style >= 0) knobs.style = model_->opening_style;
        const bool pushed = obs.day == push_day;  // tools/ei_dc11 (local addition)
        if (pushed) {
            if (push_q_animal >= 0) knobs.q_animal = push_q_animal;
            if (push_q_crop >= 0) knobs.q_crop = push_q_crop;
            knobs.land_bias += push_land;
        }
""")
    cs = sub(cs, "        DayIntent intent = bc_opus::decode_intent(*model_, obs, history_, nullptr, nullptr, knobs);", """        std::vector<float> averaged;
        if (!ensemble_.empty() && (ensemble_opening_ || obs.day >= model_->opening_days)) {  // the opening keeps its style unless .ensemble_opening
            bc_opus::decode_intent(*model_, obs, history_, nullptr, &averaged, knobs);
            for (const auto& member : ensemble_) {
                std::vector<float> logits;
                bc_opus::DecodeKnobs member_knobs{};
                if (obs.day < model_->opening_days) member_knobs.style = knobs.style;  // .ensemble_opening
                bc_opus::decode_intent(*member, obs, history_, nullptr, &logits, member_knobs);
                if (logits.size() != averaged.size()) std::abort();
                for (size_t i = 0; i < logits.size(); ++i) averaged[i] += logits[i];
            }
            for (auto& v : averaged) v /= float(1 + ensemble_.size());
            knobs.head_override = &averaged;  // also carried into the herd-reach decodes
        }
        DayIntent intent = bc_opus::decode_intent(*model_, obs, history_, nullptr, nullptr, knobs);""")
    cs = sub(cs, "        if (obs.day >= reach_first_ && obs.day <= reach_last_ && complete(plan)) {",
             "        if (obs.day >= reach_first_ && obs.day <= reach_last_ && complete(plan) && !(pushed && push_q_animal >= 0)) {")
    if "#include <filesystem>" not in cs:
        cs = sub(cs, "#include <cstdio>\n", "#include <cstdio>\n#include <filesystem>\n")
    print("applied: ensemble, ensemble_opening, push")

# Step 2: <model>.ensemble_fields.
if "ensemble_fields_" not in hs:
    hs = sub(hs, "    bool ensemble_opening_ = false;", """    bool ensemble_opening_ = false;
    std::string ensemble_fields_;  // <model>.ensemble_fields: segments averaged (counts land crop animal; empty: all)""")
    cs = sub(cs, "    ensemble_opening_ = std::filesystem::exists(path + \".ensemble_opening\");\n", """    ensemble_opening_ = std::filesystem::exists(path + ".ensemble_opening");
    ensemble_fields_.clear();
    if (std::FILE* file = std::fopen((path + ".ensemble_fields").c_str(), "r")) {
        for (char word[32]; std::fscanf(file, "%31s", word) == 1;) ensemble_fields_ += std::string(word) + " ";
        std::fclose(file);
    }
""")
    cs = sub(cs, "            for (auto& v : averaged) v /= float(1 + ensemble_.size());\n", """            for (auto& v : averaged) v /= float(1 + ensemble_.size());
            if (!ensemble_fields_.empty()) {  // .ensemble_fields: the model's own logits outside the chosen segments
                std::vector<float> own;
                bc_opus::decode_intent(*model_, obs, history_, nullptr, &own, knobs);
                const int C = 101, counts = 11 * C, land = counts + 1, crop = land + C + 5, animal = crop + C + 3;
                auto keep = [&](const char* name, int from, int to) {
                    if (ensemble_fields_.find(std::string(name) + " ") == std::string::npos)
                        std::copy(own.begin() + from, own.begin() + std::min<int>(to, int(own.size())), averaged.begin() + from);
                };
                keep("counts", 0, counts), keep("land", counts, land), keep("crop", land, crop), keep("animal", crop, animal);
            }
""")
    print("applied: ensemble_fields")

h.write_text(hs)
c.write_text(cs)

# Step 3: learned opponent-sales forecast. dc11 market: a callback that may replace the day market's opponent flow
# (after dc11's own forecast); the agent sets it from <model>.forecast (dc11_local/learned_forecast.hpp).
D = Path(__file__).resolve().parents[1] / "dc11"
mh, mc = D / "market.hpp", D / "market.cpp"
mhs, mcs = mh.read_text(), mc.read_text()
if "learned;" not in mhs:
    mhs = sub(mhs, "    const double* oracle = nullptr;", """    // Learned opponent-sales forecast (local addition, sep24_BC_opus): replaces the day market's flow after forecast().
    std::function<void(const agent::AgentObservation&, const History&, double (*)[N_PRODUCTS])> learned;
    const double* oracle = nullptr;""")
    mhs = sub(mhs, '#include "source/history.hpp"\n', '#include "source/history.hpp"\n#include <functional>\n')
    mcs = sub(mcs, "    else forecast(dawn, history, m.rival, options.first_full, options.blend);\n",
              "    else forecast(dawn, history, m.rival, options.first_full, options.blend);\n"
              "    if (!oracle && options.learned) options.learned(dawn, history, m.rival);  // local addition (sep24_BC_opus)\n")
    mh.write_text(mhs); mc.write_text(mcs)
    print("applied: dc11 learned-forecast hook")
hs, cs = h.read_text(), c.read_text()
if "forecast_" not in hs:
    hs = sub(hs, '#include "dc11/compiler.hpp"\n', '#include "dc11/compiler.hpp"\n#include "dc11_local/learned_forecast.hpp"\n')
    hs = sub(hs, "    std::shared_ptr<const bc_opus::Model> model_;", """    std::shared_ptr<const bc_opus::Model> model_;
    std::shared_ptr<const fcast::LearnedForecast> forecast_;  // <model>.forecast (local addition, sep24_BC_opus)""")
    cs = sub(cs, "    options_ = options_text.empty() ? options_from_env() : options_text == \"-\" ? Options{} : parse_options(options_text.c_str());\n",
             """    options_ = options_text.empty() ? options_from_env() : options_text == "-" ? Options{} : parse_options(options_text.c_str());
    forecast_.reset();  // <model>.forecast: learned opponent-sales forecast for the day market
    if (auto learned = std::make_shared<fcast::LearnedForecast>(); learned->load(path + ".forecast")) forecast_ = learned;
    if (forecast_) {
        const auto learned = forecast_;
        options_.market.learned = [learned](const agent::AgentObservation& o, const dc10::History& hist, double (*rival)[N_PRODUCTS]) {
            learned->apply(o, hist, rival);
        };
    }
""")
    h.write_text(hs); c.write_text(cs)
    print("applied: agent <model>.forecast")

# Step 4: transformer opponent forecast (agent-losses session, dc11_local/fc_transformer) from <model>.forecast_tf.
# One prediction per dawn (days 1-28, fed in order; cached for dc11's repeated day-market builds), products 1-7 only;
# if the transformer rejects a dawn it is switched off for the rest of the game (dc11's forecast).
hs, cs = h.read_text(), c.read_text()
if "forecast_tf" not in cs:
    hs = sub(hs, '#include "dc11_local/learned_forecast.hpp"\n',
             '#include "dc11_local/learned_forecast.hpp"\n#include "dc11_local/fc_transformer/fc_transformer.hpp"\n')
    cs = sub(cs, "    if (forecast_) {\n", """    if (std::filesystem::exists(path + ".forecast_tf")) {  // <model>.forecast_tf: transformer opponent forecast
        struct TfState {
            fcmodel::Transformer tf;
            int day = 0;
            bool off = false;
            double out[HOURS][N_PRODUCTS]{};
            explicit TfState(const std::string& file) : tf(file) { tf.reset(); }
        };
        const auto st = std::make_shared<TfState>(path + ".forecast_tf");
        options_.market.learned = [st](const agent::AgentObservation& o, const dc10::History& hist, double (*rival)[N_PRODUCTS]) {
            if (st->off || o.day < 1 || o.day > 28) return;
            if (st->day != o.day) {
                double seller[HOURS][N_PRODUCTS];
                std::copy_n(&rival[0][0], HOURS * N_PRODUCTS, &seller[0][0]);
                try {
                    st->tf.predict(o, hist, seller, st->out);
                } catch (const std::exception& e) {
                    std::fprintf(stderr, "forecast_tf off: %s\\n", e.what());
                    st->off = true;
                    return;
                }
                st->day = o.day;
            }
            for (int t = 0; t < HOURS; ++t)
                for (int p = CARROT; p <= WOOL; ++p) rival[t][p] = st->out[t][p];
        };
    }
    if (forecast_) {
""")
    h.write_text(hs); c.write_text(cs)
    print("applied: agent <model>.forecast_tf")

# Step 5: <model>.forecast_tf_exact: the transformer gets the inputs it was trained on (kaggriculture-38, Sep 26): a
# second History with the hour-23 fix (History::exact_shed, observed every step) and dc11::forecast without first_sales
# (first_full 0) as the seller input. dc11's own day market keeps its History and forecast.
hs, cs = h.read_text(), c.read_text()
if "exact_history_" not in hs:
    hs = sub(hs, "    dc10::History history_;\n", "    dc10::History history_;\n"
             "    std::shared_ptr<dc10::History> exact_history_;  // <model>.forecast_tf_exact (local addition, sep24_BC_opus)\n")
    cs = sub(cs, "        const auto st = std::make_shared<TfState>(path + \".forecast_tf\");\n", """        const auto st = std::make_shared<TfState>(path + ".forecast_tf");
        exact_history_.reset();
        if (std::filesystem::exists(path + ".forecast_tf_exact")) {
            exact_history_ = std::make_shared<dc10::History>();
            exact_history_->exact_shed = true;
        }
        const auto exact = exact_history_;
        const double blend = options_.market.blend;
""")
    cs = sub(cs, "        options_.market.learned = [st](const agent::AgentObservation& o, const dc10::History& hist, double (*rival)[N_PRODUCTS]) {\n",
             "        options_.market.learned = [st, exact, blend](const agent::AgentObservation& o, const dc10::History& hist, double (*rival)[N_PRODUCTS]) {\n")
    cs = sub(cs, """                std::copy_n(&rival[0][0], HOURS * N_PRODUCTS, &seller[0][0]);
                try {
                    st->tf.predict(o, hist, seller, st->out);""", """                if (exact) dc11::forecast(o, *exact, seller, 0, blend);
                else std::copy_n(&rival[0][0], HOURS * N_PRODUCTS, &seller[0][0]);
                try {
                    st->tf.predict(o, exact ? *exact : hist, seller, st->out);""")
    cs = sub(cs, "    history_.observe(obs, action);\n}\n", "    history_.observe(obs, action);\n    if (exact_history_) exact_history_->observe(obs, action);\n}\n")
    h.write_text(hs); c.write_text(cs)
    print("applied: agent <model>.forecast_tf_exact")

# Step 6: the forecast_tf_exact History infers the previous step before acting (History::advance), so at dawn it
# already holds yesterday's hour 23, as in the transformer's training rows (kaggriculture-38).
cs = c.read_text()
if "exact_history_->advance" not in cs:
    cs = sub(cs, "    if (obs.hour == 0 && obs.day != planned_day_) {\n",
             "    if (exact_history_) exact_history_->advance(obs);  // <model>.forecast_tf_exact (local addition)\n"
             "    if (obs.hour == 0 && obs.day != planned_day_) {\n")
    c.write_text(cs)
    print("applied: forecast_tf_exact History::advance")

# Step 7: dc11 option "landtrim" (N, default 0; Weaknesses session): when the day's full plan is unfunded and the intent
# buys land, first trim up to N new entities (as trim_new_entity does) with the land kept, then fall back to NoLand as
# before (NoLand drops the land and every entity placed on it, e.g. 22 entities on v28's Kaggle day 6 vs DSM).
ch, cc = D / "compiler.hpp", D / "compiler.cpp"
chs, ccs = ch.read_text(), cc.read_text()
if "land_trims" not in chs:
    chs = sub(chs, "    bool cap_collect = true;\n};\n", "    bool cap_collect = true;\n"
              "    int land_trims = 0;  // landtrim (local addition, sep24_BC_opus): entities trimmed with the land kept before NoLand\n};\n")
    ccs = sub(ccs, """    plan = level_plan(intent, KeepAll);
    if (plan.status != CompileStatus::Ok && intent.buy_land) plan = level_plan(intent, NoLand);
""", """    plan = level_plan(intent, KeepAll);
    if (plan.status != CompileStatus::Ok && intent.buy_land && options.land_trims > 0) {  // landtrim (local addition)
        DayIntent trimmed = intent;
        for (int trims = 1; trims <= options.land_trims && !options.spent(prof.evaluations) && trim_new_entity(schema, trimmed); ++trims) {
            Plan p = level_plan(trimmed, KeepAll);
            if (p.status == CompileStatus::Ok) {
                p.trims = trims;
                plan = p;
                break;
            }
        }
    }
    if (plan.status != CompileStatus::Ok && intent.buy_land) plan = level_plan(intent, NoLand);
""")
    ccs = sub(ccs, '        else if (k == "budget") o.max_evaluations = long(value);\n',
              '        else if (k == "budget") o.max_evaluations = long(value);\n        else if (k == "landtrim") o.land_trims = int(value);  // local addition\n')
    ch.write_text(chs); cc.write_text(ccs)
    print("applied: dc11 landtrim option")

# Step 8: intra-day re-forecast (kaggriculture-38): <model>.forecast_tf_intra (needs .forecast_tf_exact and a model with an
# intra-day head). Every hour the executor's market gets today's remaining hours of products 1-7 from the model's
# intra-day head, given the opponent's sales so far today (the exact History). The funding simulation keeps the dawn
# forecast (no opponent sales of today exist there).
mhs, ccs = mh.read_text(), cc.read_text()
if "intraday;" not in mhs:
    mhs = sub(mhs, "    const double* oracle = nullptr;",
              "    // Intra-day re-forecast (local addition): called by the executor every hour on its copy of the day market.\n"
              "    std::function<void(const agent::AgentObservation&, double (*)[N_PRODUCTS])> intraday;\n"
              "    const double* oracle = nullptr;")
    ccs = sub(ccs, "    DayMarket market = market_;\n",
              "    DayMarket market = market_;\n"
              "    if (options_.market.intraday) options_.market.intraday(obs, market.rival);  // local addition (sep24_BC_opus)\n")
    ccs = sub(ccs, "    Executor executor;\n    executor.start(dawn, history, plan, options);\n",
              "    Executor executor;\n    Options simulated = options;\n"
              "    simulated.market.intraday = nullptr;  // local addition: no opponent sales of today in this simulation\n"
              "    executor.start(dawn, history, plan, simulated);\n")
    mh.write_text(mhs); cc.write_text(ccs)
    print("applied: dc11 intraday hook")
cs = c.read_text()
if "forecast_tf_intra" not in cs:
    cs = sub(cs, "                for (int p = CARROT; p <= WOOL; ++p) rival[t][p] = st->out[t][p];\n        };\n",
             """                for (int p = CARROT; p <= WOOL; ++p) rival[t][p] = st->out[t][p];
        };
        if (std::filesystem::exists(path + ".forecast_tf_intra")) {  // intra-day head (needs the exact History)
            if (!exact || !st->tf.has_intraday()) std::abort();
            options_.market.intraday = [st, exact](const agent::AgentObservation& o, double (*rival)[N_PRODUCTS]) {
                if (st->off || st->day != o.day || o.hour < 1) return;
                for (int p = CARROT; p <= WOOL; ++p) {
                    double seen[HOURS]{}, out[HOURS]{};
                    for (int h = 0; h < o.hour; ++h) seen[h] = std::max(0, exact->flow_at(o.day * HOURS + h, p));
                    st->tf.intraday(p, seen, o.hour, out);
                    for (int h = o.hour; h < HOURS; ++h) rival[h][p] = out[h];
                }
            };
        }
""")
    c.write_text(cs)
    print("applied: agent <model>.forecast_tf_intra")

# Step 9: <model>.forecast_tf_products: the learned forecast (dawn and intra-day) only for these product indices
# (e.g. "6 7": milk, wool); dc11's own forecast for the rest. Default: all of carrot..wool.
cs = c.read_text()
if "forecast_tf_products" not in cs:
    cs = sub(cs, "        const auto exact = exact_history_;\n", """        const auto exact = exact_history_;
        auto use = std::make_shared<std::array<bool, N_PRODUCTS>>();  // <model>.forecast_tf_products (local addition)
        for (int p = CARROT; p <= WOOL; ++p) (*use)[p] = true;
        if (std::FILE* file = std::fopen((path + ".forecast_tf_products").c_str(), "r")) {
            use->fill(false);
            for (int p; std::fscanf(file, "%d", &p) == 1;)
                if (p >= CARROT && p <= WOOL) (*use)[p] = true;
            std::fclose(file);
        }
""")
    cs = sub(cs, "        options_.market.learned = [st, exact, blend](", "        options_.market.learned = [st, exact, blend, use](")
    cs = sub(cs, "                for (int p = CARROT; p <= WOOL; ++p) rival[t][p] = st->out[t][p];\n",
             "                for (int p = CARROT; p <= WOOL; ++p)\n                    if ((*use)[p]) rival[t][p] = st->out[t][p];\n")
    cs = sub(cs, "            options_.market.intraday = [st, exact](", "            options_.market.intraday = [st, exact, use](")
    cs = sub(cs, "                for (int p = CARROT; p <= WOOL; ++p) {\n                    double seen[HOURS]{}, out[HOURS]{};\n",
             "                for (int p = CARROT; p <= WOOL; ++p) {\n                    if (!(*use)[p]) continue;\n                    double seen[HOURS]{}, out[HOURS]{};\n")
    if "#include <array>" not in cs:
        cs = sub(cs, "#include <cstdio>\n", "#include <array>\n#include <cstdio>\n")
    c.write_text(cs)
    print("applied: <model>.forecast_tf_products")

# Step 10: dc11 options "dawnsell" (H) and "dawnstart" (S) (Weaknesses session: port_dawn_sell_dc / port_dawn_start): on
# days 1-28 the hourly seller sells all shed stock above reserves during hours S..S+H-1 and otherwise holds (prohibitive
# hold value) unless the night's shed room forces sales. For bed clones that sell like the current top teams (h8-10).
mhs, mcs, ccs = mh.read_text(), mc.read_text(), cc.read_text()
if "dawn_sell" not in mhs:
    mhs = sub(mhs, "    bool tie_all = false;     // ties between selling now and later sell now for every product (else melons only)\n};",
              "    bool tie_all = false;     // ties between selling now and later sell now for every product (else melons only)\n"
              "    int dawn_hours = 0, dawn_start = 0;  // dawnsell / dawnstart (local addition)\n};")
    mhs = sub(mhs, "    const double* oracle = nullptr;",
              "    int dawn_sell = 0, dawn_start = 0;  // dawnsell / dawnstart (local addition, Weaknesses session): bed clones sell at h S..S+H-1\n"
              "    const double* oracle = nullptr;")
    mcs = sub(mcs, "    m.tie_all = options.tie_all;\n",
              "    m.tie_all = options.tie_all;\n"
              "    m.dawn_hours = dawn.day >= 1 && dawn.day < LAST_DAY ? options.dawn_sell : 0;  // local addition\n"
              "    m.dawn_start = options.dawn_start;\n")
    mcs = sub(mcs, "    const int remaining = hours - h;\n    if (remaining <= 0) return;\n",
              "    const int remaining = hours - h;\n    if (remaining <= 0) return;\n"
              "    if (market.dawn_hours > 0 && h >= market.dawn_start && h < market.dawn_start + market.dawn_hours) {  // dawnsell (local addition)\n"
              "        for (int p = 0; p < N_PRODUCTS; ++p) sell[p] = std::max(0, obs.own.shed[p] - reserve[p]);\n"
              "        return;\n"
              "    }\n")
    mcs = sub(mcs, "    // Night room: a per-unit charge on stock left after the last market, as small as fits.\n",
              "    if (market.dawn_hours > 0)  // dawnsell: keep stock for tomorrow's window unless it does not fit tonight\n"
              "        for (auto& sale : sales) sale.hold_discount = 10;\n"
              "    // Night room: a per-unit charge on stock left after the last market, as small as fits.\n")
    ccs = sub(ccs, '        else if (k == "landtrim") o.land_trims = int(value);  // local addition\n',
              '        else if (k == "landtrim") o.land_trims = int(value);  // local addition\n'
              '        else if (k == "dawnsell") o.market.dawn_sell = int(value);  // local addition\n'
              '        else if (k == "dawnstart") o.market.dawn_start = int(value);  // local addition\n')
    mh.write_text(mhs); mc.write_text(mcs); cc.write_text(ccs)
    print("applied: dc11 dawnsell / dawnstart")

# Step 11: <model>.forecast_tf_select (factor f; Weaknesses session's lfselect): each dawn both forecasts (learned and
# dc11's own) are stored; per product the learned one is used today (dawn and intra-day) unless its timing error over the
# last 3 days (sum over hours of |cumulative forecast - cumulative actual opponent sales|) exceeds f x dc11's error.
cs = c.read_text()
if "forecast_tf_select" not in cs:
    cs = sub(cs, "            double out[HOURS][N_PRODUCTS]{};\n",
             "            double out[HOURS][N_PRODUCTS]{};\n"
             "            double learned_days[30][HOURS][N_PRODUCTS]{}, own_days[30][HOURS][N_PRODUCTS]{};  // .forecast_tf_select\n"
             "            bool stored[30]{};\n"
             "            bool pick[N_PRODUCTS] = {true, true, true, true, true, true, true, true, true};\n")
    cs = sub(cs, "        options_.market.learned = [st, exact, blend, use](",
             "        double select = 0;  // <model>.forecast_tf_select (local addition): 0 off\n"
             "        if (std::FILE* file = std::fopen((path + \".forecast_tf_select\").c_str(), \"r\")) {\n"
             "            if (std::fscanf(file, \"%lf\", &select) != 1) std::abort();\n"
             "            std::fclose(file);\n"
             "        }\n"
             "        options_.market.learned = [st, exact, blend, use, select](")
    cs = sub(cs, "                st->day = o.day;\n",
             "                st->day = o.day;\n"
             "                if (select > 0 && o.day < 30) {  // .forecast_tf_select: per product, learned vs dc11's forecast\n"
             "                    std::copy_n(&st->out[0][0], HOURS * N_PRODUCTS, &st->learned_days[o.day][0][0]);\n"
             "                    std::copy_n(&rival[0][0], HOURS * N_PRODUCTS, &st->own_days[o.day][0][0]);\n"
             "                    st->stored[o.day] = true;\n"
             "                    const dc10::History& actual = exact ? *exact : hist;\n"
             "                    for (int p = CARROT; p <= WOOL; ++p) {\n"
             "                        double err_l = 0, err_o = 0;\n"
             "                        int n = 0;\n"
             "                        for (int k = std::max(1, o.day - 3); k < o.day; ++k) {\n"
             "                            if (!st->stored[k]) continue;\n"
             "                            double cl = 0, co = 0, ca = 0;\n"
             "                            for (int h = 0; h < HOURS; ++h) {\n"
             "                                cl += std::max(0.0, st->learned_days[k][h][p]), co += std::max(0.0, st->own_days[k][h][p]);\n"
             "                                ca += std::max(0, actual.flow_at(k * HOURS + h, p));\n"
             "                                err_l += std::abs(cl - ca), err_o += std::abs(co - ca);\n"
             "                            }\n"
             "                            ++n;\n"
             "                        }\n"
             "                        st->pick[p] = n == 0 || err_l <= select * err_o;\n"
             "                    }\n"
             "                }\n")
    cs = sub(cs, "                    if ((*use)[p]) rival[t][p] = st->out[t][p];\n",
             "                    if ((*use)[p] && st->pick[p]) rival[t][p] = st->out[t][p];\n")
    cs = sub(cs, "                    if (!(*use)[p]) continue;\n", "                    if (!(*use)[p] || !st->pick[p]) continue;\n")
    c.write_text(cs)
    print("applied: <model>.forecast_tf_select")

# Step 12: the intra-day call passes the opponent's inferred shed stock and visible output of the product and its sales of
# all products 1-7 so far today (FCT4 models with stock / xseen inputs, e.g. stk_xs_w1_l5; older models ignore them).
cs = c.read_text()
if "all_seen" not in cs:
    cs = sub(cs, "                if (st->off || st->day != o.day || o.hour < 1) return;\n                for (int p = CARROT; p <= WOOL; ++p) {\n",
             "                if (st->off || st->day != o.day || o.hour < 1) return;\n"
             "                int visible[N_PRODUCTS];  // stock / xseen inputs of FCT4 models (local addition)\n"
             "                dc10::visible_supply(o, visible);\n"
             "                double all_seen[HOURS]{};\n"
             "                for (int h = 0; h < o.hour; ++h)\n"
             "                    for (int q = CARROT; q <= WOOL; ++q) all_seen[h] += std::max(0, exact->flow_at(o.day * HOURS + h, q));\n"
             "                for (int p = CARROT; p <= WOOL; ++p) {\n")
    cs = sub(cs, "                    st->tf.intraday(p, seen, o.hour, out);\n",
             "                    st->tf.intraday(p, seen, o.hour, out, exact->opponent_stock()[p], visible[p], all_seen);\n")
    c.write_text(cs)
    print("applied: intra-day stock / visible / xseen inputs")

# Step 13: dc11 option "tieadapt" (S; Weaknesses session's port_tie_adapt.py): sell on ties only for products the opponent
# sold mostly early over the last 3 days (share of its units before h8 >= S). tieall helps vs morning sellers (real
# top-30) and loses vs evening sellers (our lineage on the Local-LB -3.7k).
mhs, mcs, ccs = mh.read_text(), mc.read_text(), cc.read_text()
if "tie_adapt" not in mhs and "    bool tie_all = false;            // DayMarket::tie_all\n" in mhs:  # closed option; anchor gone from dc11 v38+
    mhs = sub(mhs, "    bool tie_all = false;     // ties between selling now and later sell now for every product (else melons only)\n",
              "    bool tie_all = false;     // ties between selling now and later sell now for every product (else melons only)\n"
              "    bool tie[N_PRODUCTS]{};   // tieadapt (local addition): sell on ties for this product\n")
    mhs = sub(mhs, "    bool tie_all = false;            // DayMarket::tie_all\n",
              "    bool tie_all = false;            // DayMarket::tie_all\n"
              "    double tie_adapt = 0;            // tieadapt (local addition): ties sell now where the opponent sold >= this share before h8\n")
    mcs = sub(mcs, "    m.tie_all = options.tie_all;\n", """    m.tie_all = options.tie_all;
    if (options.tie_adapt > 0)  // tieadapt (local addition): the opponent's share of units sold before h8, last 3 days
        for (int p = 0; p < N_PRODUCTS; ++p) {
            double early = 0, total = 0;
            for (int d = std::max(0, dawn.day - 3); d < dawn.day; ++d)
                for (int h = 0; h < HOURS; ++h) {
                    const int f = std::max(0, history.flow_at(d * HOURS + h, p));
                    total += f, early += h < 8 ? f : 0;
                }
            m.tie[p] = total >= 3 && early >= options.tie_adapt * total;
        }
""")
    assert mcs.count("sale.tie_now = p == MELON || market.tie_all;") == 2
    mcs = mcs.replace("sale.tie_now = p == MELON || market.tie_all;", "sale.tie_now = p == MELON || market.tie_all || market.tie[p];")
    ccs = sub(ccs, '        else if (k == "tieall") o.market.tie_all = value != 0;\n',
              '        else if (k == "tieall") o.market.tie_all = value != 0;\n'
              '        else if (k == "tieadapt") o.market.tie_adapt = value;  // local addition\n')
    mh.write_text(mhs); mc.write_text(mcs); cc.write_text(ccs)
    print("applied: dc11 tieadapt")

# Step 14: forecaster seed ensemble: <model>.forecast_tf.2, .forecast_tf.3, ... (same format as <model>.forecast_tf) are
# averaged with it (mean of expected units per hour, dawn and intra-day; kaggriculture-38's 3-seed big2 ensemble).
cs = c.read_text()
if "forecast_tf.\"" not in cs:
    cs = sub(cs, "            fcmodel::Transformer tf;\n",
             "            fcmodel::Transformer tf;\n"
             "            std::vector<fcmodel::Transformer> extra;  // <model>.forecast_tf.2, .3, ... (local addition): averaged\n")
    cs = sub(cs, "        const auto st = std::make_shared<TfState>(path + \".forecast_tf\");\n",
             "        const auto st = std::make_shared<TfState>(path + \".forecast_tf\");\n"
             "        for (int k = 2; std::filesystem::exists(path + \".forecast_tf.\" + std::to_string(k)); ++k)\n"
             "            st->extra.emplace_back(path + \".forecast_tf.\" + std::to_string(k));\n")
    cs = sub(cs, "                    st->tf.predict(o, exact ? *exact : hist, seller, st->out);\n",
             "                    st->tf.predict(o, exact ? *exact : hist, seller, st->out);\n"
             "                    for (auto& tf : st->extra) {\n"
             "                        double more[HOURS][N_PRODUCTS]{};\n"
             "                        tf.predict(o, exact ? *exact : hist, seller, more);\n"
             "                        for (int t = 0; t < HOURS; ++t)\n"
             "                            for (int p = CARROT; p <= WOOL; ++p) st->out[t][p] += more[t][p];\n"
             "                    }\n"
             "                    for (int t = 0; t < HOURS; ++t)\n"
             "                        for (int p = CARROT; p <= WOOL; ++p) st->out[t][p] /= double(1 + st->extra.size());\n")
    cs = sub(cs, "            if (!exact || !st->tf.has_intraday()) std::abort();\n",
             "            if (!exact || !st->tf.has_intraday()) std::abort();\n"
             "            for (const auto& tf : st->extra)\n"
             "                if (!tf.has_intraday()) std::abort();\n")
    cs = sub(cs, "                    st->tf.intraday(p, seen, o.hour, out, exact->opponent_stock()[p], visible[p], all_seen);\n",
             "                    st->tf.intraday(p, seen, o.hour, out, exact->opponent_stock()[p], visible[p], all_seen);\n"
             "                    for (const auto& tf : st->extra) {\n"
             "                        double more[HOURS]{};\n"
             "                        tf.intraday(p, seen, o.hour, more, exact->opponent_stock()[p], visible[p], all_seen);\n"
             "                        for (int h = o.hour; h < HOURS; ++h) out[h] += more[h];\n"
             "                    }\n"
             "                    for (int h = o.hour; h < HOURS; ++h) out[h] /= double(1 + st->extra.size());\n")
    c.write_text(cs)
    print("applied: forecaster seed ensemble")

# Step 15: .decode key "v219 <day> <money> <tomato price> <tomato shops> <n>" (Weaknesses session's port_v219.py): on <day> with 3
# quadrants, money >= <money>, tomato price >= <price> and >= <tomato shops> open shops that consume tomatoes, the decoded intent
# (and the herd-reach retries) buys the 4th quadrant and plants <n> extra tomatoes; the land-only arm is "v219 11 6000 65 2 0".
hs, cs = h.read_text(), c.read_text()
if "v219_day_" not in hs:
    hs = sub(hs, "    double match_bias_ = 0;\n",
             "    double match_bias_ = 0;\n"
             "    int v219_day_ = -1, v219_money_ = 0, v219_price_ = 0, v219_shops_ = 0, v219_n_ = 0;  // .decode v219 (local addition)\n")
    cs = sub(cs, "    match_first_ = 0, match_last_ = -1, match_bias_ = 0;\n",
             "    match_first_ = 0, match_last_ = -1, match_bias_ = 0;\n    v219_day_ = -1;\n")
    cs = sub(cs, '            else if (k == "land_match" && std::fscanf(file, "%d %d %lf", &match_first_, &match_last_, &match_bias_) == 3) {}\n',
             '            else if (k == "land_match" && std::fscanf(file, "%d %d %lf", &match_first_, &match_last_, &match_bias_) == 3) {}\n'
             '            else if (k == "v219" && std::fscanf(file, "%d %d %d %d %d", &v219_day_, &v219_money_, &v219_price_, &v219_shops_, &v219_n_) == 5) {}\n')
    cs = sub(cs, "        DayIntent intent = bc_opus::decode_intent(*model_, obs, history_, nullptr, nullptr, knobs);\n",
             "        auto v219 = [&](DayIntent& in) {  // .decode v219 (local addition): tomato-rich dawn buys the 4th quadrant\n"
             "            if (obs.day != v219_day_ || obs.self().n_quadrants != 3 || obs.self().money < v219_money_ ||\n"
             "                obs.market.prices[TOMATO] < v219_price_) return;\n"
             "            int shops = 0;\n"
             "            for (int k = 0; k < obs.n_shops; ++k) shops += (SHOP_MASK[obs.shops[k]] >> TOMATO) & 1;\n"
             "            if (shops >= v219_shops_) in.buy_land = true, in.new_crop[TOMATO] = int16_t(in.new_crop[TOMATO] + v219_n_);\n"
             "        };\n"
             "        DayIntent intent = bc_opus::decode_intent(*model_, obs, history_, nullptr, nullptr, knobs);\n"
             "        v219(intent);\n")
    cs = sub(cs, "                const DayIntent bigger = bc_opus::decode_intent(*model_, obs, history_, nullptr, nullptr, more);\n",
             "                DayIntent bigger = bc_opus::decode_intent(*model_, obs, history_, nullptr, nullptr, more);\n"
             "                v219(bigger);\n")
    h.write_text(hs); c.write_text(cs)
    print("applied: .decode v219")

# Step 16: .decode key "plot <day> <money> <crop> <price> <shops> <n>" (Weaknesses session's port_plot.py): v219 for any crop - on <day>
# with 3 quadrants, money >= <money>, the crop's price >= <price> and >= <shops> open shops consuming it, buy the 4th quadrant and
# plant <n> extra of the crop. The strawberry-demand land trigger is "plot 11 6000 3 0 2 0".
hs, cs = h.read_text(), c.read_text()
if "plot_day_" not in hs:
    hs = sub(hs, "    int v219_day_ = -1, v219_money_ = 0, v219_price_ = 0, v219_shops_ = 0, v219_n_ = 0;  // .decode v219 (local addition)\n",
             "    int v219_day_ = -1, v219_money_ = 0, v219_price_ = 0, v219_shops_ = 0, v219_n_ = 0;  // .decode v219 (local addition)\n"
             "    int plot_day_ = -1, plot_money_ = 0, plot_crop_ = 0, plot_price_ = 0, plot_shops_ = 0, plot_n_ = 0;  // .decode plot (local addition)\n")
    cs = sub(cs, "    v219_day_ = -1;\n", "    v219_day_ = -1, plot_day_ = -1;\n")
    cs = sub(cs, '            else if (k == "v219" && std::fscanf(file, "%d %d %d %d %d", &v219_day_, &v219_money_, &v219_price_, &v219_shops_, &v219_n_) == 5) {}\n',
             '            else if (k == "v219" && std::fscanf(file, "%d %d %d %d %d", &v219_day_, &v219_money_, &v219_price_, &v219_shops_, &v219_n_) == 5) {}\n'
             '            else if (k == "plot" && std::fscanf(file, "%d %d %d %d %d %d", &plot_day_, &plot_money_, &plot_crop_, &plot_price_, &plot_shops_, &plot_n_) == 6) {}\n')
    cs = sub(cs, "            if (shops >= v219_shops_) in.buy_land = true, in.new_crop[TOMATO] = int16_t(in.new_crop[TOMATO] + v219_n_);\n        };\n",
             "            if (shops >= v219_shops_) in.buy_land = true, in.new_crop[TOMATO] = int16_t(in.new_crop[TOMATO] + v219_n_);\n        };\n"
             "        auto plot = [&](DayIntent& in) {  // .decode plot (local addition): v219 for any crop\n"
             "            if (obs.day != plot_day_ || obs.self().n_quadrants != 3 || obs.self().money < plot_money_ ||\n"
             "                obs.market.prices[plot_crop_] < plot_price_) return;\n"
             "            int shops = 0;\n"
             "            for (int k = 0; k < obs.n_shops; ++k) shops += (SHOP_MASK[obs.shops[k]] >> plot_crop_) & 1;\n"
             "            if (shops >= plot_shops_) in.buy_land = true, in.new_crop[plot_crop_] = int16_t(in.new_crop[plot_crop_] + plot_n_);\n"
             "        };\n")
    cs = sub(cs, "        v219(intent);\n", "        v219(intent), plot(intent);\n")
    cs = sub(cs, "                v219(bigger);\n", "                v219(bigger), plot(bigger);\n")
    h.write_text(hs); c.write_text(cs)
    print("applied: .decode plot")

# Step 17: .decode key "q4opp <first> <last> <money>": on days first..last with 3 quadrants, money >= <money> and an opponent that
# already owns 4 quadrants, the decoded intent (and the herd-reach retries) buys the 4th quadrant (a forced land_match; the
# unconditional day-11 Q4 provokes land_match opponents into their own 4th quadrant).
hs, cs = h.read_text(), c.read_text()
if "q4opp_first_" not in hs:
    hs = sub(hs, "    int plot_day_ = -1, plot_money_ = 0, plot_crop_ = 0, plot_price_ = 0, plot_shops_ = 0, plot_n_ = 0;  // .decode plot (local addition)\n",
             "    int plot_day_ = -1, plot_money_ = 0, plot_crop_ = 0, plot_price_ = 0, plot_shops_ = 0, plot_n_ = 0;  // .decode plot (local addition)\n"
             "    int q4opp_first_ = 0, q4opp_last_ = -1, q4opp_money_ = 0;  // .decode q4opp (local addition)\n")
    cs = sub(cs, "    v219_day_ = -1, plot_day_ = -1;\n", "    v219_day_ = -1, plot_day_ = -1, q4opp_first_ = 0, q4opp_last_ = -1;\n")
    cs = sub(cs, '            else if (k == "plot" && std::fscanf(file, "%d %d %d %d %d %d", &plot_day_, &plot_money_, &plot_crop_, &plot_price_, &plot_shops_, &plot_n_) == 6) {}\n',
             '            else if (k == "plot" && std::fscanf(file, "%d %d %d %d %d %d", &plot_day_, &plot_money_, &plot_crop_, &plot_price_, &plot_shops_, &plot_n_) == 6) {}\n'
             '            else if (k == "q4opp" && std::fscanf(file, "%d %d %d", &q4opp_first_, &q4opp_last_, &q4opp_money_) == 3) {}\n')
    cs = sub(cs, "        v219(intent), plot(intent);\n",
             "        auto q4opp = [&](DayIntent& in) {  // .decode q4opp (local addition)\n"
             "            if (obs.day >= q4opp_first_ && obs.day <= q4opp_last_ && obs.self().n_quadrants == 3 && obs.opponent().n_quadrants >= 4 &&\n"
             "                obs.self().money >= q4opp_money_) in.buy_land = true;\n"
             "        };\n"
             "        v219(intent), plot(intent), q4opp(intent);\n")
    cs = sub(cs, "                v219(bigger), plot(bigger);\n", "                v219(bigger), plot(bigger), q4opp(bigger);\n")
    h.write_text(hs); c.write_text(cs)
    print("applied: .decode q4opp")

# Step 18: ST2's cash-denial and final-day rival terms (Local-LB pavel-bc-opus-v12-dc11v29-st2, fork by Codex), as dc11 options:
# "liquidity=B": the sale DP's rival weight is rival + B x clamp(1 - opponent cash / 2000, 0, 1) x clamp((29 - day) / 14, 0, 1),
# at dawn and again every hour in the executor (public current cash only); "terminalrival=W": the rival weight of the final-day
# sale (0 before). Both 0 = off (v34 play).
mhs, mcs, ccs = mh.read_text(), mc.read_text(), cc.read_text()
if "liquidity_bonus" not in mhs and "scenario_lot = 1;  // MarketOptions::scenarios" in mhs:  # rejected option; anchor gone from dc11 v38+
    mhs = sub(mhs, "    int scenarios = 0, scenario_lot = 1;  // MarketOptions::scenarios / scenario_lot\n",
              "    int scenarios = 0, scenario_lot = 1;  // MarketOptions::scenarios / scenario_lot\n"
              "    double terminal_rival_weight = 0;  // MarketOptions::terminal_rival_weight (local addition)\n")
    mhs = sub(mhs, "    double rival_weight = 0.5;\n",
              "    double rival_weight = 0.5;\n"
              "    double liquidity_bonus = 0;        // liquidity (local addition, ST2): extra denial while the opponent's cash < $2k\n"
              "    double terminal_rival_weight = 0;  // terminalrival (local addition, ST2): rival weight of the final-day sale\n")
    mhs = sub(mhs, "DayMarket day_market(const agent::AgentObservation& dawn, const History& history, const MarketOptions& options);\n",
              "DayMarket day_market(const agent::AgentObservation& dawn, const History& history, const MarketOptions& options);\n"
              "double denial_weight(const agent::AgentObservation& obs, const MarketOptions& options);  // local addition (ST2 liquidity)\n")
    mcs = sub(mcs, "DayMarket day_market(const agent::AgentObservation& dawn, const History& history, const MarketOptions& options) {\n",
              "double denial_weight(const agent::AgentObservation& obs, const MarketOptions& options) {  // local addition (ST2 liquidity)\n"
              "    const double shortage = std::clamp(1.0 - obs.opponent().money / 2000.0, 0.0, 1.0);\n"
              "    const double horizon = std::clamp((LAST_DAY - obs.day) / 14.0, 0.0, 1.0);\n"
              "    return options.rival_weight + options.liquidity_bonus * shortage * horizon;\n"
              "}\n\n"
              "DayMarket day_market(const agent::AgentObservation& dawn, const History& history, const MarketOptions& options) {\n")
    mcs = sub(mcs, "    m.rival_weight = options.rival_weight;\n",
              "    m.rival_weight = denial_weight(dawn, options);  // = rival_weight unless liquidity (local addition)\n"
              "    m.terminal_rival_weight = options.terminal_rival_weight;\n")
    assert mcs.count("sale.rival_weight = sale.terminal ? 0 : market.rival_weight;") == 2
    mcs = mcs.replace("sale.rival_weight = sale.terminal ? 0 : market.rival_weight;",
                      "sale.rival_weight = sale.terminal ? market.terminal_rival_weight : market.rival_weight;")
    ccs = sub(ccs, "    DayMarket market = market_;\n",
              "    DayMarket market = market_;\n"
              "    if (options_.market.liquidity_bonus > 0) market.rival_weight = denial_weight(obs, options_.market);  // local addition (ST2)\n")
    ccs = sub(ccs, '        else if (k == "rival") o.market.rival_weight = value;\n',
              '        else if (k == "rival") o.market.rival_weight = value;\n'
              '        else if (k == "liquidity") o.market.liquidity_bonus = value;  // local addition (ST2)\n'
              '        else if (k == "terminalrival") o.market.terminal_rival_weight = value;  // local addition (ST2)\n')
    mh.write_text(mhs); mc.write_text(mcs); cc.write_text(ccs)
    print("applied: dc11 liquidity / terminalrival")

# Step 19: .decode key "earlycow <first> <last> <n>" (ST5's early-cow candidate, Local-LB PR 214): on days first..last, when the day's plan
# is complete, recompile with <n> extra cows and keep that plan only if it is complete too (every planned entity kept, funded).
hs, cs = h.read_text(), c.read_text()
if "earlycow_first_" not in hs:
    hs = sub(hs, "    int q4opp_first_ = 0, q4opp_last_ = -1, q4opp_money_ = 0;  // .decode q4opp (local addition)\n",
             "    int q4opp_first_ = 0, q4opp_last_ = -1, q4opp_money_ = 0;  // .decode q4opp (local addition)\n"
             "    int earlycow_first_ = 0, earlycow_last_ = -1, earlycow_n_ = 0;  // .decode earlycow (local addition)\n")
    cs = sub(cs, "    v219_day_ = -1, plot_day_ = -1, q4opp_first_ = 0, q4opp_last_ = -1;\n",
             "    v219_day_ = -1, plot_day_ = -1, q4opp_first_ = 0, q4opp_last_ = -1, earlycow_first_ = 0, earlycow_last_ = -1;\n")
    cs = sub(cs, '            else if (k == "q4opp" && std::fscanf(file, "%d %d %d", &q4opp_first_, &q4opp_last_, &q4opp_money_) == 3) {}\n',
             '            else if (k == "q4opp" && std::fscanf(file, "%d %d %d", &q4opp_first_, &q4opp_last_, &q4opp_money_) == 3) {}\n'
             '            else if (k == "earlycow" && std::fscanf(file, "%d %d %d", &earlycow_first_, &earlycow_last_, &earlycow_n_) == 3) {}\n')
    cs = sub(cs, "        auto complete = [](const Plan& p) { return p.status == dc11::CompileStatus::Ok && p.fallback == dc11::KeepAll && !p.dropped; };\n",
             "        auto complete = [](const Plan& p) { return p.status == dc11::CompileStatus::Ok && p.fallback == dc11::KeepAll && !p.dropped; };\n"
             "        if (obs.day >= earlycow_first_ && obs.day <= earlycow_last_ && earlycow_n_ > 0 && complete(plan) &&\n"
             "            plan.evaluations < options.max_evaluations) {  // .decode earlycow (local addition)\n"
             "            DayIntent more = intent;\n"
             "            more.new_animal[1] = int16_t(more.new_animal[1] + earlycow_n_);  // cows\n"
             "            Options left = options;\n"
             "            left.max_evaluations = options.max_evaluations - plan.evaluations;\n"
             "            Plan tried = compile_day(obs, seen, more, left);\n"
             "            tried.compile_ms += plan.compile_ms, tried.evaluations += plan.evaluations;\n"
             "            if (complete(tried)) tried.reason = \"early cow;\" + tried.reason, intent = more, plan = tried;\n"
             "            else plan.compile_ms = tried.compile_ms, plan.evaluations = tried.evaluations;\n"
             "        }\n")
    h.write_text(hs); c.write_text(cs)
    print("applied: .decode earlycow")

# Step 20: learned next-morning supply (kaggriculture-38's nextm, FCT5 forecasters with a tomorrow head): <model>.forecast_tf_nextm (needs
# .forecast_tf_intra). Every hour of days < 28 the executor calls market.next_morning, which adds (sum of the head's tomorrow hours 0..11 -
# sum of today's forecast hours 0..11, i.e. what the hold value wraps around) to hold_supply for products 1-7. Off in funded().
mhs, ccs, cs = mh.read_text(), cc.read_text(), c.read_text()
if "next_morning" not in mhs:
    mhs = sub(mhs, "    std::function<void(const agent::AgentObservation&, double (*)[N_PRODUCTS])> intraday;\n",
              "    std::function<void(const agent::AgentObservation&, double (*)[N_PRODUCTS])> intraday;\n"
              "    // local addition (nextm): adds the learned next-morning opponent supply correction to hold_supply (days < 28)\n"
              "    std::function<void(const agent::AgentObservation&, const double (*)[N_PRODUCTS], int*)> next_morning;\n")
    ccs = sub(ccs, "    simulated.market.intraday = nullptr;  // no opponent sales of today exist in this simulation\n",
              "    simulated.market.intraday = nullptr;  // no opponent sales of today exist in this simulation\n"
              "    simulated.market.next_morning = nullptr;  // local addition (nextm)\n")
    ccs = sub(ccs, "    if (options_.market.intraday) options_.market.intraday(obs, market.rival);\n",
              "    if (options_.market.intraday) options_.market.intraday(obs, market.rival);\n"
              "    if (options_.market.next_morning && obs.day < LAST_DAY - 1) options_.market.next_morning(obs, market.rival, market.hold_supply);  // local addition\n")
    mh.write_text(mhs); cc.write_text(ccs)
    print("applied: dc11 next_morning hook")
if "forecast_tf_nextm" not in cs:
    cs = sub(cs, "        if (std::filesystem::exists(path + \".forecast_tf_intra\")) {  // intra-day head (needs the exact History)\n",
             "        if (std::filesystem::exists(path + \".forecast_tf_nextm\")) {  // local addition: learned next-morning supply (FCT5)\n"
             "            if (!exact || st->tf.next_hours() <= 0) std::abort();\n"
             "            options_.market.next_morning = [st, exact](const agent::AgentObservation& o, const double (*rival)[N_PRODUCTS], int* hold) {\n"
             "                if (st->off || st->day != o.day) return;\n"
             "                const int H = std::min(st->tf.next_hours(), HOURS / 2);\n"
             "                int visible[N_PRODUCTS];\n"
             "                dc10::visible_supply(o, visible);\n"
             "                double all_seen[HOURS]{};\n"
             "                for (int h = 0; h < o.hour; ++h)\n"
             "                    for (int q = CARROT; q <= WOOL; ++q) all_seen[h] += std::max(0, exact->flow_at(o.day * HOURS + h, q));\n"
             "                for (int p = CARROT; p <= WOOL; ++p) {\n"
             "                    double seen[HOURS]{}, out[HOURS]{}, next[HOURS]{}, head = 0, today = 0;\n"
             "                    for (int h = 0; h < o.hour; ++h) seen[h] = std::max(0, exact->flow_at(o.day * HOURS + h, p));\n"
             "                    st->tf.intraday(p, seen, o.hour, out, exact->opponent_stock()[p], visible[p], all_seen, next);\n"
             "                    for (const auto& tf : st->extra) {\n"
             "                        double o2[HOURS]{}, n2[HOURS]{};\n"
             "                        tf.intraday(p, seen, o.hour, o2, exact->opponent_stock()[p], visible[p], all_seen, n2);\n"
             "                        for (int h = 0; h < H; ++h) next[h] += n2[h];\n"
             "                    }\n"
             "                    for (int h = 0; h < H; ++h) head += next[h] / double(1 + st->extra.size()), today += std::max(0.0, rival[h][p]);\n"
             "                    hold[p] = std::max(0, hold[p] + int(std::lround(head - today)));\n"
             "                }\n"
             "            };\n"
             "        }\n"
             "        if (std::filesystem::exists(path + \".forecast_tf_intra\")) {  // intra-day head (needs the exact History)\n")
    c.write_text(cs)
    print("applied: agent <model>.forecast_tf_nextm")

# Step 21: split next-morning model (kaggriculture-38's nextsplit): <model>.forecast_tf_next = an FCT5 file used only for the next-morning
# supply of step 20 (fed every dawn so its state is current); the main <model>.forecast_tf (+ .2, .3) keeps today's forecast and intra-day.
cs = c.read_text()
if "forecast_tf_next\"" not in cs:
    cs = sub(cs, "            std::vector<fcmodel::Transformer> extra;  // <model>.forecast_tf.2, .3, ... (local addition): averaged\n",
             "            std::vector<fcmodel::Transformer> extra;  // <model>.forecast_tf.2, .3, ... (local addition): averaged\n"
             "            std::vector<fcmodel::Transformer> next;   // <model>.forecast_tf_next (local addition): next-morning supply only\n")
    cs = sub(cs, "        for (int k = 2; std::filesystem::exists(path + \".forecast_tf.\" + std::to_string(k)); ++k)\n",
             "        if (std::filesystem::exists(path + \".forecast_tf_next\")) st->next.emplace_back(path + \".forecast_tf_next\");\n"
             "        for (int k = 2; std::filesystem::exists(path + \".forecast_tf.\" + std::to_string(k)); ++k)\n")
    cs = sub(cs, "                    for (int t = 0; t < HOURS; ++t)\n                        for (int p = CARROT; p <= WOOL; ++p) st->out[t][p] /= double(1 + st->extra.size());\n",
             "                    for (int t = 0; t < HOURS; ++t)\n                        for (int p = CARROT; p <= WOOL; ++p) st->out[t][p] /= double(1 + st->extra.size());\n"
             "                    for (auto& tf : st->next) {  // keeps the next-morning model's dawn state current\n"
             "                        double unused[HOURS][N_PRODUCTS]{};\n"
             "                        tf.predict(o, exact ? *exact : hist, seller, unused);\n"
             "                    }\n")
    cs = sub(cs, "            if (!exact || st->tf.next_hours() <= 0) std::abort();\n",
             "            if (!exact || (st->next.empty() ? st->tf.next_hours() : st->next[0].next_hours()) <= 0) std::abort();\n")
    cs = sub(cs, "                const int H = std::min(st->tf.next_hours(), HOURS / 2);\n",
             "                const fcmodel::Transformer& nm = st->next.empty() ? st->tf : st->next[0];\n"
             "                const int H = std::min(nm.next_hours(), HOURS / 2);\n")
    cs = sub(cs, "                    st->tf.intraday(p, seen, o.hour, out, exact->opponent_stock()[p], visible[p], all_seen, next);\n"
                 "                    for (const auto& tf : st->extra) {\n"
                 "                        double o2[HOURS]{}, n2[HOURS]{};\n"
                 "                        tf.intraday(p, seen, o.hour, o2, exact->opponent_stock()[p], visible[p], all_seen, n2);\n"
                 "                        for (int h = 0; h < H; ++h) next[h] += n2[h];\n"
                 "                    }\n"
                 "                    for (int h = 0; h < H; ++h) head += next[h] / double(1 + st->extra.size()), today += std::max(0.0, rival[h][p]);\n",
             "                    nm.intraday(p, seen, o.hour, out, exact->opponent_stock()[p], visible[p], all_seen, next);\n"
             "                    const size_t n = st->next.empty() ? 1 + st->extra.size() : 1;\n"
             "                    if (st->next.empty())\n"
             "                        for (const auto& tf : st->extra) {\n"
             "                            double o2[HOURS]{}, n2[HOURS]{};\n"
             "                            tf.intraday(p, seen, o.hour, o2, exact->opponent_stock()[p], visible[p], all_seen, n2);\n"
             "                            for (int h = 0; h < H; ++h) next[h] += n2[h];\n"
             "                        }\n"
             "                    for (int h = 0; h < H; ++h) head += next[h] / double(n), today += std::max(0.0, rival[h][p]);\n")
    c.write_text(cs)
    print("applied: <model>.forecast_tf_next (split next-morning model)")

# Step 22: the next-morning correction may be negative (as in kaggriculture-38's wrapped-flow form and dc11 tomorrow=2): no clamp at 0.
cs = c.read_text()
old = "                    hold[p] = std::max(0, hold[p] + int(std::lround(head - today)));\n"
if old in cs:
    cs = sub(cs, old, "                    hold[p] += int(std::lround(head - today));  // may be negative (tomorrow=2 form)\n")
    c.write_text(cs)
    print("applied: next-morning correction without clamp")

# Step 23: .decode key "q4mix <first> <last> <shift> <sheep>" (live ledgers of Kaggle 56613566: the 4th quadrant went to tomatoes / wheat and extra
# sheep, not strawberries): on days first..last while we own 4 quadrants, move round(shift x (new tomatoes + new wheat)) new plants to strawberries
# (tomatoes first) and cap new sheep at <sheep> per day. Applied after the other decode rules, also to the herd-reach retries.
hs, cs = h.read_text(), c.read_text()
if "q4mix_first_" not in hs:
    hs = sub(hs, "    int earlycow_first_ = 0, earlycow_last_ = -1, earlycow_n_ = 0;  // .decode earlycow (local addition)\n",
             "    int earlycow_first_ = 0, earlycow_last_ = -1, earlycow_n_ = 0;  // .decode earlycow (local addition)\n"
             "    int q4mix_first_ = 0, q4mix_last_ = -1, q4mix_sheep_ = -1;  // .decode q4mix (local addition)\n"
             "    double q4mix_shift_ = 0;\n")
    cs = sub(cs, "    v219_day_ = -1, plot_day_ = -1, q4opp_first_ = 0, q4opp_last_ = -1, earlycow_first_ = 0, earlycow_last_ = -1;\n",
             "    v219_day_ = -1, plot_day_ = -1, q4opp_first_ = 0, q4opp_last_ = -1, earlycow_first_ = 0, earlycow_last_ = -1;\n"
             "    q4mix_first_ = 0, q4mix_last_ = -1;\n")
    cs = sub(cs, '            else if (k == "earlycow" && std::fscanf(file, "%d %d %d", &earlycow_first_, &earlycow_last_, &earlycow_n_) == 3) {}\n',
             '            else if (k == "earlycow" && std::fscanf(file, "%d %d %d", &earlycow_first_, &earlycow_last_, &earlycow_n_) == 3) {}\n'
             '            else if (k == "q4mix" && std::fscanf(file, "%d %d %lf %d", &q4mix_first_, &q4mix_last_, &q4mix_shift_, &q4mix_sheep_) == 4) {}\n')
    cs = sub(cs, "        v219(intent), plot(intent), q4opp(intent);\n",
             "        auto q4mix = [&](DayIntent& in) {  // .decode q4mix (local addition)\n"
             "            if (obs.day < q4mix_first_ || obs.day > q4mix_last_ || obs.self().n_quadrants < 4) return;\n"
             "            int move = int(std::lround(q4mix_shift_ * (in.new_crop[TOMATO] + in.new_crop[WHEAT])));\n"
             "            for (const int crop : {int(TOMATO), int(WHEAT)}) {\n"
             "                const int m = std::min<int>(move, in.new_crop[crop]);\n"
             "                in.new_crop[crop] = int16_t(in.new_crop[crop] - m), in.new_crop[STRAWBERRY] = int16_t(in.new_crop[STRAWBERRY] + m), move -= m;\n"
             "            }\n"
             "            if (q4mix_sheep_ >= 0) in.new_animal[2] = int16_t(std::min<int>(in.new_animal[2], q4mix_sheep_));  // sheep\n"
             "        };\n"
             "        v219(intent), plot(intent), q4opp(intent), q4mix(intent);\n")
    cs = sub(cs, "                v219(bigger), plot(bigger), q4opp(bigger);\n", "                v219(bigger), plot(bigger), q4opp(bigger), q4mix(bigger);\n")
    h.write_text(hs); c.write_text(cs)
    print("applied: .decode q4mix")

# Step 24: .decode key "crop_bias <crop> <bias> <first> <last>" (Weaknesses session, port_crop_bias.py): on days first..last add <bias> to
# the whole-farm head's share logit of <crop> (0 wheat .. 4 melon); the total of new crops is unchanged. Lead: strawberries on 4 quadrants.
O = Path(__file__).resolve().parents[1] / "agent/bc_opus/source"
oh, oc = O / "agent.hpp", O / "agent.cpp"
ohs, ocs, hs, cs = oh.read_text(), oc.read_text(), h.read_text(), c.read_text()
if "crop_bias" not in ohs:  # bc_opus decoder (not re-vendored)
    ohs = sub(ohs, "    int max_quadrants = 4;  // land is bought only below this many quadrants\n",
              "    int max_quadrants = 4;  // land is bought only below this many quadrants\n"
              "    float crop_bias[5] = {};  // added to the crop share logits (.decode crop_bias, dc11 agent)\n")
    ocs = sub(ocs, "                softmax_masked(&head[offsets[t] + CLASSES], fixed, k, p);\n",
              "                float biased[5];\n"
              "                for (int i = 0; i < k; ++i) biased[i] = head[offsets[t] + CLASSES + i] + (t ? 0.0f : knobs.crop_bias[i]);\n"
              "                softmax_masked(biased, fixed, k, p);\n")
    oh.write_text(ohs); oc.write_text(ocs)
if "crop_bias_crop_" not in hs:  # bc_overhaul agent (re-vendored)
    hs = sub(hs, "    double q4mix_shift_ = 0;\n",
             "    double q4mix_shift_ = 0;\n"
             "    int crop_bias_crop_ = -1, crop_bias_first_ = 0, crop_bias_last_ = -1;  // .decode crop_bias (local addition)\n"
             "    double crop_bias_ = 0;\n")
    cs = sub(cs, "    q4mix_first_ = 0, q4mix_last_ = -1;\n", "    q4mix_first_ = 0, q4mix_last_ = -1, crop_bias_crop_ = -1;\n")
    cs = sub(cs, '            else if (k == "q4mix" && std::fscanf(file, "%d %d %lf %d", &q4mix_first_, &q4mix_last_, &q4mix_shift_, &q4mix_sheep_) == 4) {}\n',
             '            else if (k == "q4mix" && std::fscanf(file, "%d %d %lf %d", &q4mix_first_, &q4mix_last_, &q4mix_shift_, &q4mix_sheep_) == 4) {}\n'
             '            else if (k == "crop_bias" && std::fscanf(file, "%d %lf %d %d", &crop_bias_crop_, &crop_bias_, &crop_bias_first_, &crop_bias_last_) == 4) {}\n')
    cs = sub(cs, "        knobs.max_quadrants = max_land_;\n",
             "        knobs.max_quadrants = max_land_;\n"
             "        if (crop_bias_crop_ >= 0 && obs.day >= crop_bias_first_ && obs.day <= crop_bias_last_)  // .decode crop_bias (local addition)\n"
             "            knobs.crop_bias[crop_bias_crop_] = float(crop_bias_);\n")
    h.write_text(hs); c.write_text(cs)
    print("applied: .decode crop_bias")

# Step 25: .decode key "earlyanimal <species 0 goose / 1 cow / 2 sheep> <first> <last> <n>": ST5's early cow for any species (earlycow = species 1).
hs, cs = h.read_text(), c.read_text()
if "earlycow_species_" not in hs:
    hs = sub(hs, "    int earlycow_first_ = 0, earlycow_last_ = -1, earlycow_n_ = 0;  // .decode earlycow (local addition)\n",
             "    int earlycow_first_ = 0, earlycow_last_ = -1, earlycow_n_ = 0, earlycow_species_ = 1;  // .decode earlycow / earlyanimal (local addition)\n")
    cs = sub(cs, "    q4mix_first_ = 0, q4mix_last_ = -1, crop_bias_crop_ = -1;\n", "    q4mix_first_ = 0, q4mix_last_ = -1, crop_bias_crop_ = -1, earlycow_species_ = 1;\n")
    cs = sub(cs, '            else if (k == "earlycow" && std::fscanf(file, "%d %d %d", &earlycow_first_, &earlycow_last_, &earlycow_n_) == 3) {}\n',
             '            else if (k == "earlycow" && std::fscanf(file, "%d %d %d", &earlycow_first_, &earlycow_last_, &earlycow_n_) == 3) {}\n'
             '            else if (k == "earlyanimal" && std::fscanf(file, "%d %d %d %d", &earlycow_species_, &earlycow_first_, &earlycow_last_, &earlycow_n_) == 4) {}\n')
    cs = sub(cs, "            more.new_animal[1] = int16_t(more.new_animal[1] + earlycow_n_);  // cows\n",
             "            more.new_animal[earlycow_species_] = int16_t(more.new_animal[earlycow_species_] + earlycow_n_);  // cows unless earlyanimal\n")
    h.write_text(hs); c.write_text(cs)
    print("applied: .decode earlyanimal")

# Step 26: .decode key "service <first> <last> <feed_all> <care_all> <q4 only>": on those days (with 4 quadrants only when <q4 only>) the intent feeds
# (and cares for) every animal (DecodeKnobs feed_all / care_all). Lead: the live Q4 agent serves animals less on 4-quadrant days (Weaknesses).
hs, cs = h.read_text(), c.read_text()
if "service_first_" not in hs:
    hs = sub(hs, "    double crop_bias_ = 0;\n",
             "    double crop_bias_ = 0;\n"
             "    int service_first_ = 0, service_last_ = -1, service_feed_ = 0, service_care_ = 0, service_q4_ = 0;  // .decode service (local addition)\n")
    cs = sub(cs, "    q4mix_first_ = 0, q4mix_last_ = -1, crop_bias_crop_ = -1, earlycow_species_ = 1;\n",
             "    q4mix_first_ = 0, q4mix_last_ = -1, crop_bias_crop_ = -1, earlycow_species_ = 1, service_first_ = 0, service_last_ = -1;\n")
    cs = sub(cs, '            else if (k == "crop_bias" && std::fscanf(file, "%d %lf %d %d", &crop_bias_crop_, &crop_bias_, &crop_bias_first_, &crop_bias_last_) == 4) {}\n',
             '            else if (k == "crop_bias" && std::fscanf(file, "%d %lf %d %d", &crop_bias_crop_, &crop_bias_, &crop_bias_first_, &crop_bias_last_) == 4) {}\n'
             '            else if (k == "service" && std::fscanf(file, "%d %d %d %d %d", &service_first_, &service_last_, &service_feed_, &service_care_, &service_q4_) == 5) {}\n')
    cs = sub(cs, "            knobs.crop_bias[crop_bias_crop_] = float(crop_bias_);\n",
             "            knobs.crop_bias[crop_bias_crop_] = float(crop_bias_);\n"
             "        if (obs.day >= service_first_ && obs.day <= service_last_ && (!service_q4_ || obs.self().n_quadrants >= 4))  // .decode service (local addition)\n"
             "            knobs.feed_all = service_feed_ != 0, knobs.care_all = service_care_ != 0;\n")
    h.write_text(hs); c.write_text(cs)
    print("applied: .decode service")

# Step 27: goal-conditioned BC (train.py --goal): <model>.goal lines "first last cows6 animals14 q4" set global slots 219-222 (known, cows at
# dawn 6 / 4, animals at dawn 14 / 20, 4 quadrants by dawn 11) on those days; no file or no line for the day: unknown (zeros).
oh, oc = O / "agent.hpp", O / "agent.cpp"
ohs, ocs = oh.read_text(), oc.read_text()
if "goals;" not in ohs:
    ohs = sub(ohs, "    int opening_days = 0, opening_style = -1;\n    bool load(const std::string& path);\n",
              "    int opening_days = 0, opening_style = -1;\n"
              "    // Goal inputs (<model>.goal, models trained with train.py --goal): per line first day, last day, cows6, animals14, q4.\n"
              "    std::vector<std::array<float, 5>> goals;\n"
              "    bool load(const std::string& path);\n")
    ocs = sub(ocs, "    if (std::FILE* style_file = std::fopen((path + \".style\").c_str(), \"r\")) {\n",
              "    goals.clear();\n"
              "    if (std::FILE* goal_file = std::fopen((path + \".goal\").c_str(), \"r\")) {\n"
              "        for (std::array<float, 5> r; std::fscanf(goal_file, \"%f %f %f %f %f\", &r[0], &r[1], &r[2], &r[3], &r[4]) == 5;) goals.push_back(r);\n"
              "        std::fclose(goal_file);\n"
              "        if (goals.empty()) return false;\n"
              "    }\n"
              "    if (std::FILE* style_file = std::fopen((path + \".style\").c_str(), \"r\")) {\n")
    ocs = sub(ocs, "    const auto g = mlp(m, 0, g_in, true);\n",
              "    for (const auto& r : m.goals)  // <model>.goal: hindsight goal inputs (known, cows6 / 4, animals14 / 20, q4)\n"
              "        if (dawn.day >= r[0] && dawn.day <= r[1]) {\n"
              "            g_in[219] = 1.0f, g_in[220] = r[2] / 4.0f, g_in[221] = r[3] / 20.0f, g_in[222] = r[4];\n"
              "            break;\n"
              "        }\n"
              "    const auto g = mlp(m, 0, g_in, true);\n")
    if "#include <array>" not in ohs:
        ohs = ohs.replace("#include <string>\n", "#include <array>\n#include <string>\n", 1)
    oh.write_text(ohs); oc.write_text(ocs)
    print("applied: <model>.goal")

# Step 28: dc11 option "trimnet=1" (kaggriculture-38 triage): trims drop the network's least-wanted unit first (DayIntent::trim_order from the
# decode, cursor trim_next; the old compiler's trim_model) instead of the most expensive new entity; the cost order remains the fallback.
D11 = Path(__file__).resolve().parents[1] / "dc11"
ch, cc2 = D11 / "compiler.hpp", D11 / "compiler.cpp"
chs, ccs2 = ch.read_text(), cc2.read_text()
if "trim_net" not in chs and "\"trimnet\"" not in ccs2:  # v40+ carries trimnet itself
    chs = sub(chs, "    int land_trims = 0;\n};\n",
              "    int land_trims = 0;\n"
              "    bool trim_net = false;  // trimnet (local addition): trims follow the network's trim_order\n};\n")
    ccs2 = sub(ccs2, "bool trim_new_entity(const Schema& s, DayIntent& in) {\n",
               "bool trim_new_entity(const Schema& s, DayIntent& in, bool net = false) {\n"
               "    if (net)  // trimnet (local addition): the network's least-wanted unit first\n"
               "        while (in.trim_next < in.trim_count) {\n"
               "            const int i = in.trim_order[in.trim_next++];\n"
               "            if (i < N_CROPS ? in.new_crop[i] > 0 : in.new_animal[i - N_CROPS] > 0)\n"
               "                return i < N_CROPS ? drop_crop(s, in, i) : drop_animal(s, in, i - N_CROPS);\n"
               "        }\n")
    assert ccs2.count("trim_new_entity(schema, trimmed)") == 2
    ccs2 = ccs2.replace("trim_new_entity(schema, trimmed)", "trim_new_entity(schema, trimmed, options.trim_net)")
    ccs2 = sub(ccs2, '        else if (k == "landtrim") o.land_trims = int(value);\n',
               '        else if (k == "landtrim") o.land_trims = int(value);\n'
               '        else if (k == "trimnet") o.trim_net = value != 0;  // local addition\n')
    ch.write_text(chs); cc2.write_text(ccs2)
    print("applied: dc11 trimnet")

# Step 29: .decode key "landcash <last day> <fraction>" (kaggriculture-38 triage: on day 1 the network asks for land with ~$12 in 35% of
# games; dc11 then falls back to NoLand plus a trim): on days <= last, when dawn money < fraction x the next quadrant's price, the decode
# masks the land decision (max_quadrants = current), so the network decodes a plan without the land.
hs, cs = h.read_text(), c.read_text()
if "landcash_last_" not in hs:
    hs = sub(hs, "    double crop_bias_ = 0;\n",
             "    double crop_bias_ = 0;\n"
             "    int landcash_last_ = -1;  // .decode landcash (local addition)\n"
             "    double landcash_frac_ = 0;\n")
    cs = sub(cs, "    q4mix_first_ = 0, q4mix_last_ = -1, crop_bias_crop_ = -1, earlycow_species_ = 1, service_first_ = 0, service_last_ = -1;\n",
             "    q4mix_first_ = 0, q4mix_last_ = -1, crop_bias_crop_ = -1, earlycow_species_ = 1, service_first_ = 0, service_last_ = -1;\n"
             "    landcash_last_ = -1;\n")
    cs = sub(cs, '            else if (k == "crop_bias" && std::fscanf(file, "%d %lf %d %d", &crop_bias_crop_, &crop_bias_, &crop_bias_first_, &crop_bias_last_) == 4) {}\n',
             '            else if (k == "crop_bias" && std::fscanf(file, "%d %lf %d %d", &crop_bias_crop_, &crop_bias_, &crop_bias_first_, &crop_bias_last_) == 4) {}\n'
             '            else if (k == "landcash" && std::fscanf(file, "%d %lf", &landcash_last_, &landcash_frac_) == 2) {}\n')
    cs = sub(cs, "        knobs.max_quadrants = max_land_;\n",
             "        knobs.max_quadrants = max_land_;\n"
             "        if (obs.day <= landcash_last_ && obs.self().n_quadrants < 4 &&  // .decode landcash (local addition)\n"
             "            obs.self().money < landcash_frac_ * LAND_PRICES[std::min(2, obs.self().n_quadrants - 1)])\n"
             "            knobs.max_quadrants = std::min(knobs.max_quadrants, obs.self().n_quadrants);\n")
    h.write_text(hs); c.write_text(cs)
    print("applied: .decode landcash")

# Step 30: <model>.goalraw lines "first last v219 v220 v221 v222 v223": raw global inputs at slots 219-223 on those days (any train.py --goal
# schema; schema 2 = known, cows at dawn 6 / 4, geese at dawn 10 / 6, cash at dawn 6 / 1000, 4 quadrants by dawn 11).
oh, oc = O / "agent.hpp", O / "agent.cpp"
ohs, ocs = oh.read_text(), oc.read_text()
if "goals_raw" not in ohs:
    ohs = sub(ohs, "    std::vector<std::array<float, 5>> goals;\n",
              "    std::vector<std::array<float, 5>> goals;\n"
              "    std::vector<std::array<float, 7>> goals_raw;  // <model>.goalraw: first day, last day, raw slots 219-223\n")
    ocs = sub(ocs, "    goals.clear();\n",
              "    goals.clear();\n"
              "    goals_raw.clear();\n"
              "    if (std::FILE* goal_file = std::fopen((path + \".goalraw\").c_str(), \"r\")) {\n"
              "        for (std::array<float, 7> r; std::fscanf(goal_file, \"%f %f %f %f %f %f %f\", &r[0], &r[1], &r[2], &r[3], &r[4], &r[5], &r[6]) == 7;)\n"
              "            goals_raw.push_back(r);\n"
              "        std::fclose(goal_file);\n"
              "        if (goals_raw.empty()) return false;\n"
              "    }\n")
    ocs = sub(ocs, "    const auto g = mlp(m, 0, g_in, true);\n",
              "    for (const auto& r : m.goals_raw)  // <model>.goalraw\n"
              "        if (dawn.day >= r[0] && dawn.day <= r[1]) {\n"
              "            std::copy_n(r.data() + 2, 5, g_in + 219);\n"
              "            break;\n"
              "        }\n"
              "    const auto g = mlp(m, 0, g_in, true);\n")
    oh.write_text(ohs); oc.write_text(ocs)
    print("applied: <model>.goalraw")

# Step 31: .decode key "qpush <first> <last> <q_crop> <q_animal>": decode quantiles of the new-crop / new-animal totals on those days
# (0.5 = the median, as default; > 0.5 asks for more). Lead: idle cash at dawn 6 ($1,029 vs the top 3's $68-560).
hs, cs = h.read_text(), c.read_text()
if "qpush_first_" not in hs:
    hs = sub(hs, "    double crop_bias_ = 0;\n",
             "    double crop_bias_ = 0;\n"
             "    int qpush_first_ = 0, qpush_last_ = -1;  // .decode qpush (local addition)\n"
             "    double qpush_crop_ = 0.5, qpush_animal_ = 0.5;\n")
    cs = sub(cs, "    landcash_last_ = -1;\n", "    landcash_last_ = -1, qpush_first_ = 0, qpush_last_ = -1;\n")
    cs = sub(cs, '            else if (k == "landcash" && std::fscanf(file, "%d %lf", &landcash_last_, &landcash_frac_) == 2) {}\n',
             '            else if (k == "landcash" && std::fscanf(file, "%d %lf", &landcash_last_, &landcash_frac_) == 2) {}\n'
             '            else if (k == "qpush" && std::fscanf(file, "%d %d %lf %lf", &qpush_first_, &qpush_last_, &qpush_crop_, &qpush_animal_) == 4) {}\n')
    cs = sub(cs, "        knobs.max_quadrants = max_land_;\n",
             "        knobs.max_quadrants = max_land_;\n"
             "        if (obs.day >= qpush_first_ && obs.day <= qpush_last_) knobs.q_crop = qpush_crop_, knobs.q_animal = qpush_animal_;  // .decode qpush\n")
    h.write_text(hs); c.write_text(cs)
    print("applied: .decode qpush")

# Step 32: .decode key "landpush <first> <last> <bias>": on those days the decode may buy up to 4 quadrants and adds <bias> to the land logit
# (the land decision is made inside the decode, so the free-site cap includes the next quadrant's tiles). Lead: intent audit on fresh top-15
# games - on day 10 the network asks ~3 crops fewer than the teams (who plant the day-10 4th quadrant the same day).
hs, cs = h.read_text(), c.read_text()
if "landpush_first_" not in hs:
    hs = sub(hs, "    double crop_bias_ = 0;\n",
             "    double crop_bias_ = 0;\n"
             "    int landpush_first_ = 0, landpush_last_ = -1;  // .decode landpush (local addition)\n"
             "    double landpush_bias_ = 0;\n")
    cs = sub(cs, "    landcash_last_ = -1, qpush_first_ = 0, qpush_last_ = -1;\n",
             "    landcash_last_ = -1, qpush_first_ = 0, qpush_last_ = -1, landpush_first_ = 0, landpush_last_ = -1;\n")
    cs = sub(cs, '            else if (k == "landcash" && std::fscanf(file, "%d %lf", &landcash_last_, &landcash_frac_) == 2) {}\n',
             '            else if (k == "landcash" && std::fscanf(file, "%d %lf", &landcash_last_, &landcash_frac_) == 2) {}\n'
             '            else if (k == "landpush" && std::fscanf(file, "%d %d %lf", &landpush_first_, &landpush_last_, &landpush_bias_) == 3) {}\n')
    cs = sub(cs, "        if (obs.day >= qpush_first_ && obs.day <= qpush_last_) knobs.q_crop = qpush_crop_, knobs.q_animal = qpush_animal_;  // .decode qpush\n",
             "        if (obs.day >= qpush_first_ && obs.day <= qpush_last_) knobs.q_crop = qpush_crop_, knobs.q_animal = qpush_animal_;  // .decode qpush\n"
             "        if (obs.day >= landpush_first_ && obs.day <= landpush_last_)  // .decode landpush (local addition)\n"
             "            knobs.max_quadrants = 4, knobs.land_bias = landpush_bias_, knobs.push_first = landpush_first_, knobs.push_last = landpush_last_;\n")
    h.write_text(hs); c.write_text(cs)
    print("applied: .decode landpush")

# Step 33: .decode key "animal_bias <species 0 goose / 1 cow / 2 sheep> <bias> <first> <last>": added to the whole-farm head's animal share
# logit (total of new animals unchanged). Lead: intent audit - the top 3 buy 2.6 cows / 2.5 sheep at day 0, the network asks 2.0 / 3.0.
ohs, ocs = oh.read_text(), oc.read_text()
if "animal_bias" not in ohs:  # bc_opus decoder
    ohs = sub(ohs, "    float crop_bias[5] = {};  // added to the crop share logits (.decode crop_bias, dc11 agent)\n",
              "    float crop_bias[5] = {};  // added to the crop share logits (.decode crop_bias, dc11 agent)\n"
              "    float animal_bias[3] = {};  // added to the animal share logits (.decode animal_bias, dc11 agent)\n")
    ocs = sub(ocs, "                for (int i = 0; i < k; ++i) biased[i] = head[offsets[t] + CLASSES + i] + (t ? 0.0f : knobs.crop_bias[i]);\n",
              "                for (int i = 0; i < k; ++i) biased[i] = head[offsets[t] + CLASSES + i] + (t ? knobs.animal_bias[i] : knobs.crop_bias[i]);\n")
    oh.write_text(ohs); oc.write_text(ocs)
hs, cs = h.read_text(), c.read_text()
if "animal_bias_sp_" not in hs:  # bc_overhaul agent
    hs = sub(hs, "    double crop_bias_ = 0;\n",
             "    double crop_bias_ = 0;\n"
             "    int animal_bias_sp_ = -1, animal_bias_first_ = 0, animal_bias_last_ = -1;  // .decode animal_bias (local addition)\n"
             "    double animal_bias_ = 0;\n")
    cs = sub(cs, "    landcash_last_ = -1, qpush_first_ = 0, qpush_last_ = -1, landpush_first_ = 0, landpush_last_ = -1;\n",
             "    landcash_last_ = -1, qpush_first_ = 0, qpush_last_ = -1, landpush_first_ = 0, landpush_last_ = -1, animal_bias_sp_ = -1;\n")
    cs = sub(cs, '            else if (k == "landcash" && std::fscanf(file, "%d %lf", &landcash_last_, &landcash_frac_) == 2) {}\n',
             '            else if (k == "landcash" && std::fscanf(file, "%d %lf", &landcash_last_, &landcash_frac_) == 2) {}\n'
             '            else if (k == "animal_bias" && std::fscanf(file, "%d %lf %d %d", &animal_bias_sp_, &animal_bias_, &animal_bias_first_, &animal_bias_last_) == 4) {}\n')
    cs = sub(cs, "            knobs.crop_bias[crop_bias_crop_] = float(crop_bias_);\n",
             "            knobs.crop_bias[crop_bias_crop_] = float(crop_bias_);\n"
             "        if (animal_bias_sp_ >= 0 && obs.day >= animal_bias_first_ && obs.day <= animal_bias_last_)  // .decode animal_bias (local addition)\n"
             "            knobs.animal_bias[animal_bias_sp_] = float(animal_bias_);\n")
    h.write_text(hs); c.write_text(cs)
    print("applied: .decode animal_bias")

# Step 34: .decode key "cowcut <first> <last>": cows added by earlycow / earlyanimal (species 1) are banked, and on days first..last the
# network's new-cow asks are reduced by the banked amount (the same herd, bought earlier; top teams buy cows ~4 days before we do).
hs, cs = h.read_text(), c.read_text()
if "cowbank_" not in hs:
    hs = sub(hs, "    double crop_bias_ = 0;\n",
             "    double crop_bias_ = 0;\n"
             "    int cowcut_first_ = 0, cowcut_last_ = -1, cowbank_ = 0;  // .decode cowcut (local addition)\n")
    cs = sub(cs, "    landcash_last_ = -1, qpush_first_ = 0, qpush_last_ = -1, landpush_first_ = 0, landpush_last_ = -1, animal_bias_sp_ = -1;\n",
             "    landcash_last_ = -1, qpush_first_ = 0, qpush_last_ = -1, landpush_first_ = 0, landpush_last_ = -1, animal_bias_sp_ = -1;\n"
             "    cowcut_first_ = 0, cowcut_last_ = -1, cowbank_ = 0;\n")
    cs = sub(cs, '            else if (k == "landcash" && std::fscanf(file, "%d %lf", &landcash_last_, &landcash_frac_) == 2) {}\n',
             '            else if (k == "landcash" && std::fscanf(file, "%d %lf", &landcash_last_, &landcash_frac_) == 2) {}\n'
             '            else if (k == "cowcut" && std::fscanf(file, "%d %d", &cowcut_first_, &cowcut_last_) == 2) {}\n')
    cs = sub(cs, "        v219(intent), plot(intent), q4opp(intent), q4mix(intent);\n",
             "        auto cowcut = [&](DayIntent& in, bool commit) {  // .decode cowcut (local addition): banked early cows come off later cow asks\n"
             "            if (obs.day < cowcut_first_ || obs.day > cowcut_last_ || cowbank_ <= 0) return;\n"
             "            const int cut = std::min<int>(cowbank_, in.new_animal[1]);\n"
             "            in.new_animal[1] = int16_t(in.new_animal[1] - cut);\n"
             "            if (commit) cowbank_ -= cut;  // the main intent commits; herd-reach retries reuse the same bank\n"
             "        };\n"
             "        v219(intent), plot(intent), q4opp(intent), q4mix(intent), cowcut(intent, true);\n")
    cs = sub(cs, "                v219(bigger), plot(bigger), q4opp(bigger), q4mix(bigger);\n",
             "                v219(bigger), plot(bigger), q4opp(bigger), q4mix(bigger), cowcut(bigger, false);\n")
    cs = sub(cs, '            if (complete(tried)) tried.reason = "early cow;" + tried.reason, intent = more, plan = tried;\n',
             '            if (complete(tried)) {\n'
             '                tried.reason = "early cow;" + tried.reason, intent = more, plan = tried;\n'
             '                if (earlycow_species_ == 1) cowbank_ += earlycow_n_;  // .decode cowcut bank (local addition)\n'
             '            }\n')
    h.write_text(hs); c.write_text(cs)
    print("applied: .decode cowcut")

# Step 35: .decode key "earlycrop <crop> <first> <last> <n>" (kaggriculture-38: top-30 teams plant strawberries on days 2-3 from the day's
# first sales; we hold $250-310 idle and plant 1-2 days later, losing the harvest lead): on those days, when the day's plan is complete,
# recompile with +n, then +n-1, ... of <crop> (ongoing crops only: a plain count change) and keep the first plan that is still complete.
hs, cs = h.read_text(), c.read_text()
if "earlycrop_n_" not in hs:
    hs = sub(hs, "    double crop_bias_ = 0;\n",
             "    double crop_bias_ = 0;\n"
             "    int earlycrop_crop_ = 3, earlycrop_first_ = 0, earlycrop_last_ = -1, earlycrop_n_ = 0;  // .decode earlycrop (local addition)\n")
    cs = sub(cs, "    cowcut_first_ = 0, cowcut_last_ = -1, cowbank_ = 0;\n",
             "    cowcut_first_ = 0, cowcut_last_ = -1, cowbank_ = 0, earlycrop_first_ = 0, earlycrop_last_ = -1, earlycrop_n_ = 0;\n")
    cs = sub(cs, '            else if (k == "cowcut" && std::fscanf(file, "%d %d", &cowcut_first_, &cowcut_last_) == 2) {}\n',
             '            else if (k == "cowcut" && std::fscanf(file, "%d %d", &cowcut_first_, &cowcut_last_) == 2) {}\n'
             '            else if (k == "earlycrop" && std::fscanf(file, "%d %d %d %d", &earlycrop_crop_, &earlycrop_first_, &earlycrop_last_, &earlycrop_n_) == 4) {\n'
             '                if (earlycrop_crop_ < 0 || earlycrop_crop_ >= N_CROPS || !CROPS[earlycrop_crop_].ongoing) std::abort();  // ongoing crops only\n'
             '            }\n')
    cs = sub(cs, "        // Herd reach: a larger new-animal quantile when the plan with it is also complete.\n",
             "        if (obs.day >= earlycrop_first_ && obs.day <= earlycrop_last_ && earlycrop_n_ > 0 && complete(plan)) {  // .decode earlycrop (local addition)\n"
             "            for (int n = earlycrop_n_; n >= 1 && plan.evaluations < options.max_evaluations; --n) {\n"
             "                DayIntent more = intent;\n"
             "                more.new_crop[earlycrop_crop_] = int16_t(more.new_crop[earlycrop_crop_] + n);\n"
             "                Options left = options;\n"
             "                left.max_evaluations = options.max_evaluations - plan.evaluations;\n"
             "                Plan tried = compile_day(obs, seen, more, left);\n"
             "                tried.compile_ms += plan.compile_ms, tried.evaluations += plan.evaluations;\n"
             "                if (complete(tried)) {\n"
             "                    tried.reason = \"early crop;\" + tried.reason, intent = more, plan = tried;\n"
             "                    break;\n"
             "                }\n"
             "                plan.compile_ms = tried.compile_ms, plan.evaluations = tried.evaluations;\n"
             "            }\n"
             "        }\n"
             "        // Herd reach: a larger new-animal quantile when the plan with it is also complete.\n")
    h.write_text(hs); c.write_text(cs)
    print("applied: .decode earlycrop")

# Step 36 (fix, Weaknesses session): lowering intent.new_animal[a] by hand (q4mix sheep cap, cowcut) left the fresh group's feed / care /
# collect above the new count, so validate() rejected the intent ("animal count out of range") and the agent did nothing that day.
# fresh_fix clamps every fresh group of that species: feed, collect <= new count; care <= feed.
cs = c.read_text()
if "fresh_fix" not in cs:
    cs = sub(cs, "        auto q4mix = [&](DayIntent& in) {  // .decode q4mix (local addition)\n",
             "        auto fresh_fix = [&](DayIntent& in, int species) {  // keep fresh-group counts valid after a lowered new_animal (local addition)\n"
             "            const Schema s = describe(obs);\n"
             "            const int n = in.new_animal[species];\n"
             "            for (int i = 0; i < s.n_animals; ++i)\n"
             "                if (s.animals[i].fresh && s.animals[i].species == species) {\n"
             "                    in.feed[i] = int16_t(std::min<int>(in.feed[i], n)), in.collect[i] = int16_t(std::min<int>(in.collect[i], n));\n"
             "                    in.care[i] = int16_t(std::min<int>(in.care[i], in.feed[i]));\n"
             "                }\n"
             "        };\n"
             "        auto q4mix = [&](DayIntent& in) {  // .decode q4mix (local addition)\n")
    cs = sub(cs, "            if (q4mix_sheep_ >= 0) in.new_animal[2] = int16_t(std::min<int>(in.new_animal[2], q4mix_sheep_));  // sheep\n",
             "            if (q4mix_sheep_ >= 0) in.new_animal[2] = int16_t(std::min<int>(in.new_animal[2], q4mix_sheep_)), fresh_fix(in, 2);  // sheep\n")
    cs = sub(cs, "            in.new_animal[1] = int16_t(in.new_animal[1] - cut);\n",
             "            in.new_animal[1] = int16_t(in.new_animal[1] - cut);\n"
             "            fresh_fix(in, 1);\n")
    c.write_text(cs)
    print("applied: fresh_fix")

# Step 37: dc11 v42+ next_days hook from kaggriculture-38's 48 h head (fcexport/big2_nx48.bin as <model>.forecast_tf_next; outputs 0-23
# tomorrow, 24-47 the day after): <model>.forecast_tf_days enables it (HANDOFF_next_days.md; no two-day forecast from day 27 on). The nextm
# lambda's next buffer is widened to 48 so a 48-output next model cannot overrun it.
cs = c.read_text()
if "forecast_tf_days" not in cs and "next_days" in (Path(__file__).resolve().parents[1] / "dc11/market.hpp").read_text():
    cs = sub(cs, "                    double seen[HOURS]{}, out[HOURS]{}, next[HOURS]{}, head = 0, today = 0;\n",
             "                    double seen[HOURS]{}, out[HOURS]{}, next[2 * HOURS]{}, head = 0, today = 0;  // 2 x HOURS: 48-output next models\n")
    cs = sub(cs, "        if (std::filesystem::exists(path + \".forecast_tf_intra\")) {  // intra-day head (needs the exact History)\n",
             "        if (std::filesystem::exists(path + \".forecast_tf_days\")) {  // local addition: two-day opponent supply (big2_nx48 as forecast_tf_next)\n"
             "            if (!exact || st->next.empty() || st->next[0].next_hours() < 2 * HOURS) std::abort();\n"
             "            options_.market.next_days = [st, exact](const agent::AgentObservation& o, double (*tomorrow)[N_PRODUCTS], double (*day_after)[N_PRODUCTS]) {\n"
             "                if (st->off || st->day != o.day || o.day >= 27) return false;  // the head saw the day after only on days 1-26\n"
             "                const fcmodel::Transformer& nm = st->next[0];\n"
             "                int visible[N_PRODUCTS];\n"
             "                dc10::visible_supply(o, visible);\n"
             "                double all_seen[HOURS]{};\n"
             "                for (int h = 0; h < o.hour; ++h)\n"
             "                    for (int q = CARROT; q <= WOOL; ++q) all_seen[h] += std::max(0, exact->flow_at(o.day * HOURS + h, q));\n"
             "                for (int p = CARROT; p <= WOOL; ++p) {\n"
             "                    double seen[HOURS]{}, out[HOURS]{}, next[2 * HOURS]{};\n"
             "                    for (int h = 0; h < o.hour; ++h) seen[h] = std::max(0, exact->flow_at(o.day * HOURS + h, p));\n"
             "                    nm.intraday(p, seen, o.hour, out, exact->opponent_stock()[p], visible[p], all_seen, next);\n"
             "                    for (int h = 0; h < HOURS; ++h) tomorrow[h][p] = next[h], day_after[h][p] = next[HOURS + h];\n"
             "                }\n"
             "                return true;\n"
             "            };\n"
             "        }\n"
             "        if (std::filesystem::exists(path + \".forecast_tf_intra\")) {  // intra-day head (needs the exact History)\n")
    c.write_text(cs)
    print("applied: next_days (forecast_tf_days)")

# Step 38: multi-entry share biases: .decode keys "cbias <crop> <bias> <first> <last>" and "abias <species> <bias> <first> <last>" may repeat;
# every entry active on the day adds to the crop / animal share logits (on top of crop_bias / animal_bias). Lead: kaggriculture-38's live gap
# decomposition - the top 3 run a wheat -> animals feed chain (wheat, milk, eggs up; tomatoes down) against the same opponents.
hs, cs = h.read_text(), c.read_text()
if "share_biases_" not in hs:
    hs = sub(hs, "    double crop_bias_ = 0;\n",
             "    double crop_bias_ = 0;\n"
             "    struct ShareBias { int animal, index, first, last; double bias; };  // .decode cbias / abias (local addition)\n"
             "    std::vector<ShareBias> share_biases_;\n")
    cs = sub(cs, "    cowcut_first_ = 0, cowcut_last_ = -1, cowbank_ = 0, earlycrop_first_ = 0, earlycrop_last_ = -1, earlycrop_n_ = 0;\n",
             "    cowcut_first_ = 0, cowcut_last_ = -1, cowbank_ = 0, earlycrop_first_ = 0, earlycrop_last_ = -1, earlycrop_n_ = 0;\n"
             "    share_biases_.clear();\n")
    cs = sub(cs, '            else if (k == "cowcut" && std::fscanf(file, "%d %d", &cowcut_first_, &cowcut_last_) == 2) {}\n',
             '            else if (k == "cowcut" && std::fscanf(file, "%d %d", &cowcut_first_, &cowcut_last_) == 2) {}\n'
             '            else if (ShareBias b{}; (k == "cbias" || k == "abias") && std::fscanf(file, "%d %lf %d %d", &b.index, &b.bias, &b.first, &b.last) == 4) {\n'
             '                b.animal = k == "abias";\n'
             '                if (b.index < 0 || b.index >= (b.animal ? N_ANIMALS : N_CROPS)) std::abort();\n'
             '                share_biases_.push_back(b);\n'
             '            }\n')
    cs = sub(cs, "        if (animal_bias_sp_ >= 0 && obs.day >= animal_bias_first_ && obs.day <= animal_bias_last_)  // .decode animal_bias (local addition)\n",
             "        for (const ShareBias& b : share_biases_)  // .decode cbias / abias (local addition)\n"
             "            if (obs.day >= b.first && obs.day <= b.last) (b.animal ? knobs.animal_bias : knobs.crop_bias)[b.index] += float(b.bias);\n"
             "        if (animal_bias_sp_ >= 0 && obs.day >= animal_bias_first_ && obs.day <= animal_bias_last_)  // .decode animal_bias (local addition)\n")
    h.write_text(hs); c.write_text(cs)
    print("applied: .decode cbias / abias")

# Step 39: <model>.gridoff "first last": the tile-grid input (both farms) is zeroed on those days, as train.py --grid-dropout zeroes it in
# training. Lead: counterfactuals on our live states - the own-farm layout after days 0-1 (compiler placement) switches the network to a
# weaker team's day 2-5 plan (strawberries 1.5 vs 4.75 per dawn); money and the other global inputs do not.
oh, oc = O / "agent.hpp", O / "agent.cpp"
ohs, ocs = oh.read_text(), oc.read_text()
if "gridoff_first" not in ohs:
    ohs = sub(ohs, "    std::vector<std::array<float, 7>> goals_raw;  // <model>.goalraw: first day, last day, raw slots 219-223\n",
              "    std::vector<std::array<float, 7>> goals_raw;  // <model>.goalraw: first day, last day, raw slots 219-223\n"
              "    int gridoff_first = 0, gridoff_last = -1;  // <model>.gridoff: grid input zeroed on these days (train.py --grid-dropout)\n")
    ocs = sub(ocs, "    goals_raw.clear();\n",
              "    goals_raw.clear();\n"
              "    gridoff_first = 0, gridoff_last = -1;\n"
              "    if (std::FILE* gridoff_file = std::fopen((path + \".gridoff\").c_str(), \"r\")) {\n"
              "        const bool read = std::fscanf(gridoff_file, \"%d %d\", &gridoff_first, &gridoff_last) == 2;\n"
              "        std::fclose(gridoff_file);\n"
              "        if (!read) return false;\n"
              "    }\n")
    ocs = sub(ocs, "    bcopus::grid_features(farm, day, x.data());\n",
              "    if (day < m.gridoff_first || day > m.gridoff_last) bcopus::grid_features(farm, day, x.data());  // <model>.gridoff: zeros\n")
    oh.write_text(ohs); oc.write_text(ocs)
    print("applied: <model>.gridoff")

# Step 41: .decode earlycrop also for one-shot crops (Weaknesses' port): the extra units get the fresh group's water option (a new crop
# must be watered on its planting day). Lead: the top teams' day-6 melon top-up to ~12 standing melons; "earlycrop 4 6 6 2" on v17 main
# +401 (SE 214) over 80 reactive games, pinned "up to 4" +955 clean (own money). Ongoing crops behave as before.
cs = c.read_text()
if "earlycrop one-shot" not in cs:
    cs = sub(cs, "                if (earlycrop_crop_ < 0 || earlycrop_crop_ >= N_CROPS || !CROPS[earlycrop_crop_].ongoing) std::abort();  // ongoing crops only\n",
             "                if (earlycrop_crop_ < 0 || earlycrop_crop_ >= N_CROPS) std::abort();  // earlycrop one-shot crops too (step 41)\n")
    cs = sub(cs, "                more.new_crop[earlycrop_crop_] = int16_t(more.new_crop[earlycrop_crop_] + n);\n",
             "                more.new_crop[earlycrop_crop_] = int16_t(more.new_crop[earlycrop_crop_] + n);\n"
             "                if (!CROPS[earlycrop_crop_].ongoing) {  // one-shot: the new units get the fresh group's water option\n"
             "                    auto& o = more.options[describe(obs).new_crop_group[earlycrop_crop_]][4];\n"
             "                    o = int16_t(o + n);\n"
             "                }\n")
    c.write_text(cs)
    print("applied: .decode earlycrop one-shot")

# Step 42: the earlycrop push runs after herd reach (Weaknesses): herd reach re-decodes the day with a larger animal quantile and, when that
# plan is complete, replaced the pushed plan, so the extra melons were silently lost (the push survived in ~40% of live27 games).
cs = c.read_text()
if "earlycrop after herd reach" not in cs:
    start = cs.index("        if (obs.day >= earlycrop_first_ && obs.day <= earlycrop_last_ && earlycrop_n_ > 0 && complete(plan)) {")
    herd = cs.index("        // Herd reach: a larger new-animal quantile when the plan with it is also complete.\n")
    block = cs[start:herd].replace("// .decode earlycrop (local addition)", "// .decode earlycrop (local addition; earlycrop after herd reach, step 42)")
    cs = cs[:start] + cs[herd:]
    anchor = "        DayReport report;\n        report.day = obs.day;\n        report.status = int(plan.status);\n"
    if cs.count(anchor) != 1: raise SystemExit("step 42: DayReport anchor not found once")
    cs = cs.replace(anchor, block + anchor)
    c.write_text(cs)
    print("applied: earlycrop after herd reach")

# Step 43: the earlycow / earlyanimal push also runs after herd reach (as earlycrop, step 42): on days 6-14 herd reach re-decoded the day and
# replaced the pushed plan, so animal pushes there were lost (Weaknesses' "+1 goose on day 8" fired 0.12 times per game). E's "earlycow 2 4 1"
# (days 2-4, before the reach window) plays exactly as before.
cs = c.read_text()
if "earlycow after herd reach" not in cs:
    start = cs.index("        if (obs.day >= earlycow_first_ && obs.day <= earlycow_last_ && earlycow_n_ > 0 && complete(plan) &&\n")
    herd = cs.index("        // Herd reach: a larger new-animal quantile when the plan with it is also complete.\n")
    block = cs[start:herd].replace("// .decode earlycow (local addition)", "// .decode earlycow (local addition; earlycow after herd reach, step 43)")
    cs = cs[:start] + cs[herd:]
    anchor = "        if (obs.day >= earlycrop_first_ && obs.day <= earlycrop_last_ && earlycrop_n_ > 0 && complete(plan)) {"
    if cs.count(anchor) != 1: raise SystemExit("step 43: earlycrop anchor not found once")
    cs = cs.replace(anchor, block + anchor)
    c.write_text(cs)
    print("applied: earlycow after herd reach")

# Step 44: <model>.ensemble_from "day": the members' logits are averaged in only from that day on (still never on opening days unless
# .ensemble_opening). Lead (reports/nft/ens_cf.py on econm6-like own states): the four v12-era members veto the fresh main's plan after
# the opening - day-6 melons main 2.45 -> ensemble 0.01, day-8 geese 1.32 -> 0.34 - while on the top teams' states the main matches them.
hs, cs = h.read_text(), c.read_text()
if "ensemble_from_" not in hs:
    hs = sub(hs, "    bool ensemble_opening_ = false;\n",
             "    bool ensemble_opening_ = false;\n"
             "    int ensemble_from_ = 0;  // <model>.ensemble_from: members only from this day on (local addition, step 44)\n")
    cs = sub(cs, "    ensemble_opening_ = std::filesystem::exists(path + \".ensemble_opening\");\n",
             "    ensemble_opening_ = std::filesystem::exists(path + \".ensemble_opening\");\n"
             "    ensemble_from_ = 0;\n"
             "    if (std::FILE* file = std::fopen((path + \".ensemble_from\").c_str(), \"r\")) {\n"
             "        if (std::fscanf(file, \"%d\", &ensemble_from_) != 1) std::abort();\n"
             "        std::fclose(file);\n"
             "    }\n")
    cs = sub(cs, "        if (!ensemble_.empty() && (ensemble_opening_ || obs.day >= model_->opening_days)) {",
             "        if (!ensemble_.empty() && obs.day >= ensemble_from_ && (ensemble_opening_ || obs.day >= model_->opening_days)) {")
    h.write_text(hs); c.write_text(cs)
    print("applied: <model>.ensemble_from")

# Step 45: DC11_INTENTLOG (debug, environment only): the final day intent's new animals and crops next to DC11_DAYLOG's compile summary,
# to see which asked purchases the compiler drops (lead: econm6's day-3 cow is decoded in ~80% of games but bought in 25%).
cs = c.read_text()
if "DC11_INTENTLOG" not in cs:
    cs = sub(cs, "        if (std::getenv(\"DC11_COLLECTLOG\")) {",
             "        if (std::getenv(\"DC11_INTENTLOG\")) {  // local addition (step 45)\n"
             "            std::fprintf(stderr, \"intentlog p%d d%d money %.0f animals %d %d %d crops\", obs.player, obs.day, obs.self().money,\n"
             "                         int(intent.new_animal[0]), int(intent.new_animal[1]), int(intent.new_animal[2]));\n"
             "            for (int k = 0; k < N_CROPS; ++k) std::fprintf(stderr, \" %d\", int(intent.new_crop[k]));\n"
             "            std::fprintf(stderr, \" status %d fallback %d dropped %d trims %d | %s\\n\", int(plan.status), plan.fallback, plan.dropped,\n"
             "                         plan.trims, plan.reason.c_str());\n"
             "        }\n"
             "        if (std::getenv(\"DC11_COLLECTLOG\")) {")
    c.write_text(cs)
    print("applied: DC11_INTENTLOG")

# Step 46 (Compiler Overhaul's profile, work/sep28_bcv62/logits_only.diff): the ensemble averaging pass and the members only need the
# whole-farm head logits, so decode_intent stops right after them (DecodeKnobs::logits_only) instead of running the per-group count
# decoders and discarding the intent. Exact (games identical); ~-15% CPU per game with E's four members.
ohs, ocs, cs = oh.read_text(), oc.read_text(), c.read_text()
if "logits_only" not in ohs:
    ohs = sub(ohs, "    const std::vector<float>* head_override = nullptr;\n",
              "    const std::vector<float>* head_override = nullptr;\n"
              "    bool logits_only = false;  // stop after the whole-farm head (ensemble averaging needs only global_logits)\n")
    ocs = sub(ocs, "    if (global_logits) *global_logits = head;\n",
              "    if (global_logits) *global_logits = head;\n"
              "    if (knobs.logits_only) return {};\n")
    oh.write_text(ohs); oc.write_text(ocs)
    print("applied: DecodeKnobs::logits_only")
if "head_knobs.logits_only" not in cs:
    cs = sub(cs, "            bc_opus::decode_intent(*model_, obs, history_, nullptr, &averaged, knobs);\n",
             "            bc_opus::DecodeKnobs head_knobs = knobs;  // logits only (step 46)\n"
             "            head_knobs.logits_only = true;\n"
             "            bc_opus::decode_intent(*model_, obs, history_, nullptr, &averaged, head_knobs);\n")
    cs = sub(cs, "                if (obs.day < model_->opening_days) member_knobs.style = knobs.style;  // .ensemble_opening\n",
             "                if (obs.day < model_->opening_days) member_knobs.style = knobs.style;  // .ensemble_opening\n"
             "                member_knobs.logits_only = true;\n")
    cs = sub(cs, "                bc_opus::decode_intent(*model_, obs, history_, nullptr, &own, knobs);\n",
             "                bc_opus::decode_intent(*model_, obs, history_, nullptr, &own, head_knobs);\n")
    c.write_text(cs)
    print("applied: ensemble logits only")

# Step 47: .decode "pertype first last": on those days the whole-farm counts are decoded from the per-type count heads (as BC_PER_TYPE)
# instead of the factored total + shares. Lead (DC11_INTENTLOG / layout_cf.py): on day 0 the factored split turns the fresh main's 18 crops
# into 11 wheat + 7 melons (per-type heads: 11.94 / 6.00, the top teams' 12 + 6); the funding trim then drops a melon, and the 11 + 6
# layout moves the network off the shared top-team plan (day-1 melons 3.9 vs 2.0, no cash for the day-3 cow). Per-type: 12 + 6, day 1
# two melons, dawn-3 cash $146-265 instead of $24-63.
ohs, ocs, hs, cs = oh.read_text(), oc.read_text(), h.read_text(), c.read_text()
if "bool per_type" not in ohs:
    ohs = sub(ohs, "    bool logits_only = false;",
              "    bool per_type = false;  // per-type count heads instead of the factored total + shares (as BC_PER_TYPE; step 47)\n"
              "    bool logits_only = false;")
    ocs = sub(ocs, "&& !std::getenv(\"BC_PER_TYPE\");", "&& !std::getenv(\"BC_PER_TYPE\") && !knobs.per_type;")
    oh.write_text(ohs); oc.write_text(ocs)
    print("applied: DecodeKnobs::per_type")
if "pertype_first_" not in hs:
    hs = sub(hs, "    int cowcut_first_ = 0, cowcut_last_ = -1, cowbank_ = 0;  // .decode cowcut (local addition)\n",
             "    int cowcut_first_ = 0, cowcut_last_ = -1, cowbank_ = 0;  // .decode cowcut (local addition)\n"
             "    int pertype_first_ = 0, pertype_last_ = -1;  // .decode pertype (local addition, step 47)\n")
    cs = sub(cs, "    cowcut_first_ = 0, cowcut_last_ = -1, cowbank_ = 0, earlycrop_first_ = 0, earlycrop_last_ = -1, earlycrop_n_ = 0;\n",
             "    cowcut_first_ = 0, cowcut_last_ = -1, cowbank_ = 0, earlycrop_first_ = 0, earlycrop_last_ = -1, earlycrop_n_ = 0;\n"
             "    pertype_first_ = 0, pertype_last_ = -1;\n")
    cs = sub(cs, '            else if (k == "cowcut" && std::fscanf(file, "%d %d", &cowcut_first_, &cowcut_last_) == 2) {}\n',
             '            else if (k == "cowcut" && std::fscanf(file, "%d %d", &cowcut_first_, &cowcut_last_) == 2) {}\n'
             '            else if (k == "pertype" && std::fscanf(file, "%d %d", &pertype_first_, &pertype_last_) == 2) {}\n')
    cs = sub(cs, "        if (obs.day < model_->opening_days && model_->opening_style >= 0) knobs.style = model_->opening_style;\n",
             "        if (obs.day < model_->opening_days && model_->opening_style >= 0) knobs.style = model_->opening_style;\n"
             "        knobs.per_type = obs.day >= pertype_first_ && obs.day <= pertype_last_;  // .decode pertype (step 47)\n")
    h.write_text(hs); c.write_text(cs)
    print("applied: .decode pertype")

# Step 48: .decode "yarnbias <animal> <bias> <first> <last> <max wool shops>": on those days, while at most <max wool shops> open shops
# consume wool, <bias> is added to that animal's share logit (animal 2 = sheep; negative = fewer). Lead (Weaknesses, v17 live top-30):
# with 0 Yarn Stores we sell 67 wool at $73 vs the opponents' 44 at $127; with 1 we keep 8.6 sheep vs 6.4.
hs, cs = h.read_text(), c.read_text()
if "yarn_biases_" not in hs:
    hs = sub(hs, "    std::vector<ShareBias> share_biases_;\n",
             "    std::vector<ShareBias> share_biases_;\n"
             "    struct YarnBias { int index, first, last, max_shops; double bias; };  // .decode yarnbias (local addition, step 48)\n"
             "    std::vector<YarnBias> yarn_biases_;\n")
    cs = sub(cs, "    share_biases_.clear();\n", "    share_biases_.clear();\n    yarn_biases_.clear();\n")
    cs = sub(cs, '            else if (ShareBias b{}; (k == "cbias" || k == "abias")',
             '            else if (YarnBias y{}; k == "yarnbias" && std::fscanf(file, "%d %lf %d %d %d", &y.index, &y.bias, &y.first, &y.last, &y.max_shops) == 5) {\n'
             '                if (y.index < 0 || y.index >= N_ANIMALS) std::abort();\n'
             '                yarn_biases_.push_back(y);\n'
             '            }\n'
             '            else if (ShareBias b{}; (k == "cbias" || k == "abias")')
    cs = sub(cs, "        for (const ShareBias& b : share_biases_)  // .decode cbias / abias (local addition)\n",
             "        if (!yarn_biases_.empty()) {  // .decode yarnbias (step 48): shops that consume wool\n"
             "            int wool_shops = 0;\n"
             "            for (int k = 0; k < obs.n_shops; ++k) wool_shops += (SHOP_MASK[obs.shops[k]] >> WOOL) & 1;\n"
             "            for (const YarnBias& y : yarn_biases_)\n"
             "                if (obs.day >= y.first && obs.day <= y.last && wool_shops <= y.max_shops) knobs.animal_bias[y.index] += float(y.bias);\n"
             "        }\n"
             "        for (const ShareBias& b : share_biases_)  // .decode cbias / abias (local addition)\n")
    h.write_text(hs); c.write_text(cs)
    print("applied: .decode yarnbias")

# Step 49: .decode "qday <day> <crop quantile> <animal quantile>", repeatable: decode quantiles for single days on top of the one qpush
# range (lead: the day-3 crop push "qpush 3 3 0.8 0.5" passed the gate on v17d; a per-day sweep needs several days at once).
hs, cs = h.read_text(), c.read_text()
if "qdays_" not in hs:
    hs = sub(hs, "    double qpush_crop_ = 0.5, qpush_animal_ = 0.5;\n",
             "    double qpush_crop_ = 0.5, qpush_animal_ = 0.5;\n"
             "    struct QDay { int day; double crop, animal; };  // .decode qday (local addition, step 49)\n"
             "    std::vector<QDay> qdays_;\n")
    cs = sub(cs, "    share_biases_.clear();\n", "    share_biases_.clear();\n    qdays_.clear();\n")
    cs = sub(cs, '            else if (k == "qpush" && std::fscanf(file, "%d %d %lf %lf", &qpush_first_, &qpush_last_, &qpush_crop_, &qpush_animal_) == 4) {}\n',
             '            else if (k == "qpush" && std::fscanf(file, "%d %d %lf %lf", &qpush_first_, &qpush_last_, &qpush_crop_, &qpush_animal_) == 4) {}\n'
             '            else if (QDay q{}; k == "qday" && std::fscanf(file, "%d %lf %lf", &q.day, &q.crop, &q.animal) == 3) qdays_.push_back(q);\n')
    cs = sub(cs, "        if (obs.day >= qpush_first_ && obs.day <= qpush_last_) knobs.q_crop = qpush_crop_, knobs.q_animal = qpush_animal_;  // .decode qpush\n",
             "        if (obs.day >= qpush_first_ && obs.day <= qpush_last_) knobs.q_crop = qpush_crop_, knobs.q_animal = qpush_animal_;  // .decode qpush\n"
             "        for (const QDay& q : qdays_)  // .decode qday (step 49)\n"
             "            if (obs.day == q.day) knobs.q_crop = q.crop, knobs.q_animal = q.animal;\n")
    h.write_text(hs); c.write_text(cs)
    print("applied: .decode qday")

# Step 50: .decode "qdayopp <day> <crop q> <animal q> <crop index> <max>", repeatable: like qday, but only while the opponent has at most
# <max> plants of <crop index> on its farm at that dawn. Lead: the day-3 crop push (d3crop) gains +1.0k per changed Local-LB game when
# the rival has no strawberries at dawn 3 and loses -0.6k when it has some (strawberry market competition).
hs, cs = h.read_text(), c.read_text()
if "qdayopps_" not in hs:
    hs = sub(hs, "    std::vector<QDay> qdays_;\n",
             "    std::vector<QDay> qdays_;\n"
             "    struct QDayOpp { int day, crop_index, max_count; double crop, animal; };  // .decode qdayopp (local addition, step 50)\n"
             "    std::vector<QDayOpp> qdayopps_;\n")
    cs = sub(cs, "    qdays_.clear();\n", "    qdays_.clear();\n    qdayopps_.clear();\n")
    cs = sub(cs, '            else if (QDay q{}; k == "qday"',
             '            else if (QDayOpp q{}; k == "qdayopp" && std::fscanf(file, "%d %lf %lf %d %d", &q.day, &q.crop, &q.animal, &q.crop_index, &q.max_count) == 5) qdayopps_.push_back(q);\n'
             '            else if (QDay q{}; k == "qday"')
    cs = sub(cs, "            if (obs.day == q.day) knobs.q_crop = q.crop, knobs.q_animal = q.animal;\n",
             "            if (obs.day == q.day) knobs.q_crop = q.crop, knobs.q_animal = q.animal;\n"
             "        for (const QDayOpp& q : qdayopps_) {  // .decode qdayopp (step 50)\n"
             "            if (obs.day != q.day) continue;\n"
             "            int plants = 0;\n"
             "            for (int y = 0; y < BOARD; ++y)\n"
             "                for (int x = 0; x < BOARD; ++x) {\n"
             "                    const auto& t = obs.opponent().tiles[y][x];\n"
             "                    plants += t.kind == T_PLANT && t.what == q.crop_index;\n"
             "                }\n"
             "            if (plants <= q.max_count) knobs.q_crop = q.crop, knobs.q_animal = q.animal;\n"
             "        }\n")
    h.write_text(hs); c.write_text(cs)
    print("applied: .decode qdayopp")

# Step 51 (Weaknesses' request): the v219 / plot extra crops (n > 0) are partition-consistent, as earlycrop is since step 41: one-shot
# crops added on the buy day also get the fresh group's water option. Before, "v219 10 0 0 0 2" / "plot 10 0 0 0 0 7" made the day-10
# intent invalid ("one-shot partition does not sum to group size") and the whole day was lost. n = 0 (all current packages) is unchanged.
cs = c.read_text()
if "v219 partition (step 51)" not in cs:
    cs = sub(cs, "            if (shops >= v219_shops_) in.buy_land = true, in.new_crop[TOMATO] = int16_t(in.new_crop[TOMATO] + v219_n_);\n",
             "            if (shops < v219_shops_) return;\n"
             "            in.buy_land = true, in.new_crop[TOMATO] = int16_t(in.new_crop[TOMATO] + v219_n_);\n"
             "            if (v219_n_ > 0 && !CROPS[TOMATO].ongoing) {  // v219 partition (step 51)\n"
             "                auto& o = in.options[describe(obs).new_crop_group[TOMATO]][4];\n"
             "                o = int16_t(o + v219_n_);\n"
             "            }\n")
    cs = sub(cs, "            if (shops >= plot_shops_) in.buy_land = true, in.new_crop[plot_crop_] = int16_t(in.new_crop[plot_crop_] + plot_n_);\n",
             "            if (shops < plot_shops_) return;\n"
             "            in.buy_land = true, in.new_crop[plot_crop_] = int16_t(in.new_crop[plot_crop_] + plot_n_);\n"
             "            if (plot_n_ > 0 && !CROPS[plot_crop_].ongoing) {  // plot partition (step 51)\n"
             "                auto& o = in.options[describe(obs).new_crop_group[plot_crop_]][4];\n"
             "                o = int16_t(o + plot_n_);\n"
             "            }\n")
    c.write_text(cs)
    print("applied: v219 / plot partition")

# Step 52: land-conditioned networks (train.py --land-cond; the Imitation session's proposal). Models with a <model>.landcond sidecar read
# global slot 215 = today's buy-land decision (+1 buy, -1 no buy, 0 unknown); the bc_opus decoder writes DecodeKnobs::land_input there.
# .decode "landin <mode> <solo>": mode 1 sets +1 on the v219 / plot buy dawn (the crop heads plan for the 4th quadrant the day it is bought);
# mode 2 also sets -1 when no land can be bought (n_quadrants >= max_quadrants). solo 1: that buy dawn decodes from the main model
# alone (ensemble members are not land-conditioned and would dilute the input). Without the line (all current packages) nothing changes.
ohs, ocs, hs, cs = oh.read_text(), oc.read_text(), h.read_text(), c.read_text()
if "land_input" not in ohs:
    ohs = sub(ohs, "    int gridoff_first = 0, gridoff_last = -1;",
              "    float land_scale = 0;  // <model>.landcond (train.py --land-cond / --land-scale): slot 215 = land_input * scale; 0: not conditioned (step 52)\n"
              "    int gridoff_first = 0, gridoff_last = -1;")
    ohs = sub(ohs, "    bool logits_only = false;",
              "    float land_input = 0;  // land-conditioned models: +1 buying land today, -1 not, 0 unknown (step 52)\n"
              "    bool logits_only = false;")
    ocs = sub(ocs, "    if (std::FILE* condition_file = std::fopen((path + \".condition\").c_str(), \"r\")) {\n",
              "    land_scale = 0;\n"
              "    if (std::FILE* land_file = std::fopen((path + \".landcond\").c_str(), \"r\")) {  // step 52\n"
              "        const bool read = std::fscanf(land_file, \"%f\", &land_scale) == 1;\n"
              "        std::fclose(land_file);\n"
              "        if (!read || land_scale <= 0) return false;\n"
              "    }\n"
              "    if (std::FILE* condition_file = std::fopen((path + \".condition\").c_str(), \"r\")) {\n")
    ocs = sub(ocs, "    const auto g = mlp(m, 0, g_in, true);\n",
              "    if (m.land_scale > 0) g_in[215] = knobs.land_input * m.land_scale;  // today's land decision (step 52)\n"
              "    const auto g = mlp(m, 0, g_in, true);\n")
    oh.write_text(ohs); oc.write_text(ocs)
if "landin_mode_" not in hs:
    hs = sub(hs, "    std::vector<QDay> qdays_;\n",
             "    std::vector<QDay> qdays_;\n"
             "    int landin_mode_ = 0, landin_solo_ = 0;  // .decode landin (local addition, step 52)\n")
    cs = sub(cs, "    share_biases_.clear();\n", "    share_biases_.clear();\n    landin_mode_ = 0, landin_solo_ = 0;\n")
    cs = sub(cs, '            else if (QDayOpp q{}; k == "qdayopp"',
             '            else if (k == "landin" && std::fscanf(file, "%d %d", &landin_mode_, &landin_solo_) == 2) {}\n'
             '            else if (QDayOpp q{}; k == "qdayopp"')
    cs = sub(cs, "        const bool pushed = obs.day == push_day;  // tools/ei_dc11 (local addition)\n",
             "        if (landin_mode_ > 0 && model_->land_scale > 0) {  // .decode landin (step 52): today's land decision as the network's input\n"
             "            auto fires = [&](int day, int money, int crop, int price, int min_shops) {  // as the v219 / plot lambdas\n"
             "                int shops = 0;\n"
             "                for (int k = 0; k < obs.n_shops; ++k) shops += (SHOP_MASK[obs.shops[k]] >> crop) & 1;\n"
             "                return obs.day == day && obs.self().n_quadrants == 3 && obs.self().money >= money && obs.market.prices[crop] >= price &&\n"
             "                       shops >= min_shops;\n"
             "            };\n"
             "            if (fires(v219_day_, v219_money_, TOMATO, v219_price_, v219_shops_) || fires(plot_day_, plot_money_, plot_crop_, plot_price_, plot_shops_))\n"
             "                knobs.land_input = 1;\n"
             "            else if (landin_mode_ >= 2 && obs.self().n_quadrants >= knobs.max_quadrants)\n"
             "                knobs.land_input = -1;\n"
             "        }\n"
             "        const bool pushed = obs.day == push_day;  // tools/ei_dc11 (local addition)\n")
    cs = sub(cs, "        if (!ensemble_.empty() && obs.day >= ensemble_from_ && (ensemble_opening_ || obs.day >= model_->opening_days)) {",
             "        if (!ensemble_.empty() && obs.day >= ensemble_from_ && (ensemble_opening_ || obs.day >= model_->opening_days) &&\n"
             "            !(landin_solo_ && knobs.land_input > 0)) {  // .decode landin solo (step 52)")
    h.write_text(hs); c.write_text(cs)
    print("applied: .decode landin")

# Step 53: per-day ensemble control and shop-conditional share biases.
# ".decode solo <first> <last>" (repeatable): those days decode from the main model alone (no member averaging). ".decode mainweight <w>":
# the main model's weight in the logit average (members share 1 - w; default: equal weights). Lead (reports/nft/ens_path_cf.py): on the
# top-4 teams' own states the fresh main reproduces their day-6 melons (2.98 vs actual 3.19) and day-8 wheat / geese (14.4 / 2.20 vs 14.5 /
# 2.21), while the package average with E's four v12-era members gives 0.24 melons and 10.8 wheat / 1.03 geese.
# ".decode shopcbias <crop> <bias> <first> <last> <product> <max shops>" / "shopabias <animal> ..." (repeatable, Weaknesses' request): adds
# <bias> to the crop's / animal's share logit on days first..last while at most <max shops> open shops consume <product> (yarnbias for any
# product; lead: M&M keeps Q4 without a tomato shop but grows carrots instead of tomatoes).
hs, cs = h.read_text(), c.read_text()
if "solo_days_" not in hs:
    hs = sub(hs, "    std::vector<YarnBias> yarn_biases_;\n",
             "    std::vector<YarnBias> yarn_biases_;\n"
             "    std::vector<std::pair<int, int>> solo_days_;  // .decode solo (local addition, step 53)\n"
             "    double main_weight_ = 0;  // .decode mainweight (step 53; 0: equal weights)\n"
             "    struct ShopBias { bool animal; int index, first, last, product, max_shops; double bias; };  // .decode shopcbias / shopabias (step 53)\n"
             "    std::vector<ShopBias> shop_biases_;\n")
    cs = sub(cs, "    yarn_biases_.clear();\n", "    yarn_biases_.clear();\n    solo_days_.clear(), shop_biases_.clear(), main_weight_ = 0;\n")
    cs = sub(cs, '            else if (YarnBias y{}; k == "yarnbias"',
             '            else if (int a, b; k == "solo" && std::fscanf(file, "%d %d", &a, &b) == 2) solo_days_.push_back({a, b});\n'
             '            else if (k == "mainweight" && std::fscanf(file, "%lf", &main_weight_) == 1) {}\n'
             '            else if (ShopBias s{}; (k == "shopcbias" || k == "shopabias") &&\n'
             '                     std::fscanf(file, "%d %lf %d %d %d %d", &s.index, &s.bias, &s.first, &s.last, &s.product, &s.max_shops) == 6) {\n'
             '                s.animal = k == "shopabias";\n'
             '                if (s.index < 0 || s.index >= (s.animal ? N_ANIMALS : N_CROPS) || s.product < 0 || s.product >= N_PRODUCTS) std::abort();\n'
             '                shop_biases_.push_back(s);\n'
             '            }\n'
             '            else if (YarnBias y{}; k == "yarnbias"')
    cs = sub(cs, "        for (const ShareBias& b : share_biases_)  // .decode cbias / abias (local addition)\n",
             "        for (const ShopBias& s : shop_biases_) {  // .decode shopcbias / shopabias (step 53)\n"
             "            if (obs.day < s.first || obs.day > s.last) continue;\n"
             "            int shops = 0;\n"
             "            for (int k = 0; k < obs.n_shops; ++k) shops += (SHOP_MASK[obs.shops[k]] >> s.product) & 1;\n"
             "            if (shops <= s.max_shops) (s.animal ? knobs.animal_bias : knobs.crop_bias)[s.index] += float(s.bias);\n"
             "        }\n"
             "        for (const ShareBias& b : share_biases_)  // .decode cbias / abias (local addition)\n")
    cs = sub(cs, "            !(landin_solo_ && knobs.land_input > 0)) {  // .decode landin solo (step 52)",
             "            !(landin_solo_ && knobs.land_input > 0) &&  // .decode landin solo (step 52)\n"
             "            std::none_of(solo_days_.begin(), solo_days_.end(), [&](const auto& r) { return obs.day >= r.first && obs.day <= r.second; })) {  // .decode solo (step 53)")
    cs = sub(cs, "            bc_opus::decode_intent(*model_, obs, history_, nullptr, &averaged, head_knobs);\n            for (const auto& member : ensemble_) {\n",
             "            bc_opus::decode_intent(*model_, obs, history_, nullptr, &averaged, head_knobs);\n"
             "            const std::vector<float> main_logits = averaged;  // .decode mainweight (step 53)\n"
             "            for (const auto& member : ensemble_) {\n")
    cs = sub(cs, "            for (auto& v : averaged) v /= float(1 + ensemble_.size());\n",
             "            if (main_weight_ > 0)  // .decode mainweight (step 53): w * main + (1 - w) * mean of the members\n"
             "                for (size_t i = 0; i < averaged.size(); ++i)\n"
             "                    averaged[i] = float(main_weight_ * main_logits[i] + (1 - main_weight_) * (averaged[i] - main_logits[i]) / ensemble_.size());\n"
             "            else\n"
             "                for (auto& v : averaged) v /= float(1 + ensemble_.size());\n")
    if "#include <algorithm>" not in cs:
        cs = sub(cs, "#include <cstdio>\n", "#include <algorithm>\n#include <cstdio>\n")
    h.write_text(hs); c.write_text(cs)
    print("applied: .decode solo / mainweight / shopcbias / shopabias")

# Step 54 (bug found by the Imitation session): ".decode landpush first last bias" never applied its land bias. The landpush block set
# knobs.land_bias / push_first / push_last, and the unconditional "decode config land_push" line a few lines later reset them to the
# land_push defaults (0, -1, 0), so the day gate zeroed the bias and landpush only raised max_quadrants to 4. ".decode landpushfix 1"
# re-applies landpush's bias and days after that line; without the key (all packages so far) play is unchanged, so old arms reproduce.
hs, cs = h.read_text(), c.read_text()
if "landpush_fix_" not in hs:
    hs = sub(hs, "    double landpush_bias_ = 0;\n",
             "    double landpush_bias_ = 0;\n"
             "    int landpush_fix_ = 0;  // .decode landpushfix (local addition, step 54)\n")
    cs = sub(cs, "    landin_mode_ = 0, landin_solo_ = 0;\n", "    landin_mode_ = 0, landin_solo_ = 0, landpush_fix_ = 0;\n")
    cs = sub(cs, '            else if (k == "landpush" && std::fscanf(file, "%d %d %lf", &landpush_first_, &landpush_last_, &landpush_bias_) == 3) {}\n',
             '            else if (k == "landpush" && std::fscanf(file, "%d %d %lf", &landpush_first_, &landpush_last_, &landpush_bias_) == 3) {}\n'
             '            else if (k == "landpushfix" && std::fscanf(file, "%d", &landpush_fix_) == 1) {}\n')
    cs = sub(cs, "        knobs.push_first = push_first_, knobs.push_last = push_last_, knobs.land_bias = push_bias_;  // decode config land_push\n",
             "        knobs.push_first = push_first_, knobs.push_last = push_last_, knobs.land_bias = push_bias_;  // decode config land_push\n"
             "        if (landpush_fix_ && obs.day >= landpush_first_ && obs.day <= landpush_last_)  // .decode landpushfix (step 54): keep landpush's bias\n"
             "            knobs.push_first = landpush_first_, knobs.push_last = landpush_last_, knobs.land_bias = landpush_bias_;\n")
    h.write_text(hs); c.write_text(cs)
    print("applied: .decode landpushfix")
