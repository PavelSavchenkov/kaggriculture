// Options diff (local, Imitation): on a recorded player's own dawns, the network's decoded day intent (BC_OPUS_MODEL with its sidecars, fed
// the recorded history as in teacher_day) vs the recorded player's label for the same day (convert_day), per existing group: one-shot crop
// options (harvest / water / fertilize / clear), ongoing crop retain / clear / fertilize / harvest, animal feed / care / collect. Each row
// carries the counts and a rough value of the difference at the dawn price: collect / ongoing harvest x units lost tonight (cap overflow, decay), one-shot harvest x held yield, water / fertilize /
// feed / care x one unit. Fresh (new) groups are skipped (new entities: see DUEL_PLANT).
// Gate 2 rows (Imitation): new_crop / new_animal per type and buy_land, the net's ask vs the label's (buy_land also on no-land days).
// One agent per game observes the recorded play and decodes at each scored dawn (act() at a dawn, then observe() of the same
// dawn: History::advance is idempotent per step), linear in days; OD_FRESH=1: a fresh agent per day re-observing the prefix (old).
// OD_FAST=1: decode only (no compile; = the OD_PRE intent without compile-gated herd reach / early pushes), ~10x faster.
// OD_INV_ADD="p u" (probe): the scored dawn's market inventory of product p + u (its price recomputed).
// OD_SHED_ADD="p u" (probe): the scored dawn's own shed stock of product p + u.
// OD_MONEY_ADD=x (probe): the scored dawn's own money + x (the prefix history is unchanged).
// OD_PRE=1: the intent before herd reach / earlycow / earlycrop (default: the final intent the compiler gets).
// usage: options_diff list.txt first_day last_day threads out.csv   (list line: trace seat; seat = the recorded player)
#include "agent/bc_overhaul/source/agent.hpp"
#include "agents/common/runtime/observation_builder.hpp"
#include "source/convert.hpp"
#include <atomic>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>

using namespace dc10;

namespace {
std::string sidecar(const std::string& model) {
    char text[4096] = {0};
    if (std::FILE* f = std::fopen((model + ".dc11").c_str(), "r")) {
        if (!std::fgets(text, sizeof text, f)) text[0] = 0;
        std::fclose(f);
    }
    std::string s = text;
    while (!s.empty() && (s.back() == '\n' || s.back() == ' ')) s.pop_back();
    return s;
}
}  // namespace

int main(int argc, char** argv) {
    if (argc != 6) {
        std::cerr << "usage: options_diff list.txt first_day last_day threads out.csv\n";
        return 2;
    }
    setenv("DC10_NO_DEADLINE", "1", 0);
    std::vector<std::pair<std::string, int>> games;
    std::ifstream list(argv[1]);
    for (std::string line; std::getline(list, line);) {
        std::istringstream w(line);
        std::string t;
        int s;
        if (w >> t >> s) games.push_back({t, s});
    }
    const int first = std::atoi(argv[2]), last = std::atoi(argv[3]), threads = std::atoi(argv[4]);
    const char* model = std::getenv("BC_OPUS_MODEL");
    if (!model) {
        std::cerr << "BC_OPUS_MODEL is required\n";
        return 2;
    }
    std::ofstream out(argv[5]);
    out << "trace,day,kind,what,label,net,value\n";
    std::mutex lock;
    std::atomic<int> next{0};
    std::vector<std::thread> pool;
    for (int t = 0; t < threads; ++t)
        pool.emplace_back([&] {
            for (int g; (g = next++) < int(games.size());) {
                const auto& [trace, seat] = games[g];
                const Replay replay = load_replay(trace);
                const auto states = replay_states(replay);
                std::ostringstream rows;
                const bool fresh = std::getenv("OD_FRESH") != nullptr, pre = std::getenv("OD_PRE") != nullptr;
                const std::string opts = sidecar(model);
                auto make = [&] {
                    auto a = std::make_unique<kag::agents::bc_overhaul::Agent>();
                    a->model_path = model;
                    a->options_text = !opts.empty() ? opts : "-";
                    a->intent_only = std::getenv("OD_FAST") != nullptr;
                    a->reset(kag::agent::runtime::make_agent_init(states[0], seat));
                    return a;
                };
                auto seq = make();
                int seen = 0;  // steps observed by seq
                for (int day = first; day <= last && (day + 1) * HOURS < int(states.size()); ++day) {
                    const Conversion label = convert_day(states, replay.turns, seat, day);
                    for (; !fresh && seen < day * HOURS; ++seen)
                        seq->observe(kag::agent::runtime::make_observation(states[seen], seat), replay.turns[seen][seat]);
                    if (label.status != ConvertStatus::Ok) continue;
                    std::unique_ptr<kag::agents::bc_overhaul::Agent> own = fresh ? make() : nullptr;
                    if (fresh)
                        for (int k = 0; k < day * HOURS; ++k)
                            own->observe(kag::agent::runtime::make_observation(states[k], seat), replay.turns[k][seat]);
                    auto& agent = fresh ? *own : *seq;
                    kag::agent::DecisionBudget budget;
                    budget.max_expansions = 256;
                    Action action;
                    auto dawn = kag::agent::runtime::make_observation(states[day * HOURS], seat);
                    if (const char* add = std::getenv("OD_MONEY_ADD")) dawn.farms[seat].money += std::atof(add);  // probe: the net's cash sensitivity
                    if (const char* add = std::getenv("OD_INV_ADD")) {  // probe "product units": the dawn's market inventory + units (price follows)
                        int p = 0, u = 0;
                        if (std::sscanf(add, "%d %d", &p, &u) != 2) std::abort();
                        dawn.market.inventory[p] += u;
                        dawn.market.prices[p] = int32_t(market_price(p, dawn.market.inventory[p]));
                    }
                    if (const char* add = std::getenv("OD_SHED_ADD")) {  // probe "product units": the scored dawn's own shed + units
                        int p = 0, u = 0;
                        if (std::sscanf(add, "%d %d", &p, &u) != 2) std::abort();
                        dawn.own.shed[p] = decltype(dawn.own.shed[p])(std::max(0, int(dawn.own.shed[p]) + u));
                    }
                    agent.act(dawn, budget, action);
                    if (agent.last_intent_day != day) continue;
                    const DayIntent& net = pre ? agent.last_intent : agent.final_intent;
                    const DayIntent& lab = label.intent;
                    const Schema& s = label.schema;
                    auto row = [&](const char* kind, int what, int a, int b, double unit) {
                        if (a || b) rows << trace << ',' << day << ',' << kind << ',' << what << ',' << a << ',' << b << ',' << (b - a) * unit << '\n';
                    };
                    // new entities and land (gate 2): the net's asks vs the label's
                    for (int c = 0; c < N_CROPS; ++c) row("new_crop", c, lab.new_crop[c], net.new_crop[c], 0);
                    for (int a = 0; a < N_ANIMALS; ++a) row("new_animal", a, lab.new_animal[a], net.new_animal[a], 0);
                    row("buy_land", 0, int(lab.buy_land), int(net.buy_land), 0);
                    if (!lab.buy_land && !net.buy_land) rows << trace << ',' << day << ",buy_land,0,0,0,0\n";  // count agreement on no-land days
                    for (int i = 0; i < s.n_crops; ++i) {
                        const auto& c = s.crops[i];
                        if (c.fresh) continue;
                        const double price = market_price(c.crop, dawn.market.inventory[c.crop]);
                        if (!CROPS[c.crop].ongoing) {
                            int la[4]{}, nb[4]{};  // harvest, water, fertilize, clear
                            for (int o = 0; o < OPTIONS; ++o) {
                                const int x = lab.options[i][o], y = net.options[i][o];
                                if (o == CLEAR) la[3] += x, nb[3] += y;
                                else {
                                    if (has_harvest(o)) la[0] += x, nb[0] += y;
                                    if (has_water(o)) la[1] += x, nb[1] += y;
                                    if (has_fertilize(o)) la[2] += x, nb[2] += y;
                                }
                            }
                            row("oneshot_harvest", c.crop, la[0], nb[0], c.yield * price);
                            row("oneshot_water", c.crop, la[1], nb[1], price);
                            row("oneshot_fertilize", c.crop, la[2], nb[2], price);
                            row("oneshot_clear", c.crop, la[3], nb[3], 0);
                        } else {
                            row("ongoing_retain", c.crop, lab.retain[i], net.retain[i], 0);
                            row("ongoing_clear", c.crop, lab.clear[i], net.clear[i], 0);
                            row("ongoing_fertilize", c.crop, lab.fertilize[i], net.fertilize[i], price);
                            // not harvesting today loses only the yield of a decaying plant or what tonight's production pushes over the cap
                            const CropDef& cd = CROPS[c.crop];
                            const int since = c.age + 1 - cd.first_yield_day;
                            const bool produces = since >= 0 && since % cd.interval == 0 && since / cd.interval + 1 <= cd.max_yield;
                            const int lost = c.decaying ? c.yield : produces ? std::max(0, c.yield + 1 - cd.max_yield) : 0;
                            row("ongoing_harvest", c.crop, lab.harvest[i], net.harvest[i], lost * price);
                        }
                    }
                    for (int i = 0; i < s.n_animals; ++i) {
                        const auto& a = s.animals[i];
                        if (a.fresh) continue;
                        const int product = GOOSE + a.species < N_ITEMS ? ANIMALS[a.species].product : 0;
                        const double price = market_price(product, dawn.market.inventory[product]);
                        row("animal_feed", a.species, lab.feed[i], net.feed[i], price);
                        row("animal_care", a.species, lab.care[i], net.care[i], price);
                        // not collecting today loses only what tonight's production pushes over the held cap (engine rule)
                        const AnimalDef& ad = ANIMALS[a.species];
                        const int since = a.age + 1 - ad.first_yield_day;
                        const int lost = since >= 0 && since % ad.interval == 0 ? std::max(0, a.held + 1 + a.bonus - ad.max_held) : 0;
                        row("animal_collect", a.species, lab.collect[i], net.collect[i], lost * price);
                    }
                }
                std::lock_guard<std::mutex> guard(lock);
                out << rows.str();
            }
        });
    for (auto& t : pool) t.join();
}
