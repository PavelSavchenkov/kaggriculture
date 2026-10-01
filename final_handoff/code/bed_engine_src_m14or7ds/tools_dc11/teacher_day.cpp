// Teacher-intent day replays (local, sep28_top_lb_imitation): for each listed game and day, our dc11 agent (BC_OPUS_MODEL,
// options from DC11_OPTIONS else <model>.dc11) starts from the recorded dawn with the recorded history, compiles the
// teacher's own day intent (convert_day label of the recorded day) and plays that one day against the opponent's recorded
// actions. One row for our day and one for the teacher's recorded day: unit actions by kind, late work, hires, sales by
// product and hour band, revenue, and money / stock at the next dawn. Shows how our compiler executes the same intent.
// usage: teacher_day list.txt first_day last_day threads out.csv   (list line: trace seat, seat = the teacher's seat)
//   env: TEACHER_NET=1 our network's own intent instead of the label; TEACHER_DAYS=k play k days from the dawn (later days
//   always decode our own intent) and compare at dawn day + k. TEACHER_HOURLY=1: per-hour CSV lines on stderr, prefix "hourcsv,":
//   trace,day,hour,who,units,idle,moves,created,water,harvest,feed,care,collect,pickup,deposit,hires,sold,sold_wheat,revenue,spend,
//   money_after,sold_fertilizer,plant_orders (read with scripts/hour_ref.py).
#include "agent/bc_overhaul/source/agent.hpp"
#include "agents/common/runtime/observation_builder.hpp"
#include "source/convert.hpp"
#include <atomic>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>

using namespace dc10;

namespace {
std::string dc11_sidecar(const std::string& model) {
    std::string s;
    if (std::FILE* file = std::fopen((model + ".dc11").c_str(), "r")) {
        for (char chunk[512]; std::fgets(chunk, sizeof chunk, file);) {  // step 67: the whole first line (no 255-character limit)
            s += chunk;
            if (s.back() == '\n') break;
        }
        std::fclose(file);
    }
    while (!s.empty() && (s.back() == '\n' || s.back() == ' ')) s.pop_back();
    return s;
}

constexpr int SOLD[] = {WHEAT, TOMATO, STRAWBERRY, EGG, MILK, WOOL, FERTILIZER};
constexpr const char* SOLD_NAME[] = {"wheat", "tomato", "strawberry", "egg", "milk", "wool", "fertilizer"};
constexpr int N_SOLD = 7, N_BANDS = 4;
int band(int hour) { return hour <= 2 ? 0 : hour <= 11 ? 1 : hour <= 20 ? 2 : 3; }

struct Day {
    int status = -1, fallback = -1, dropped = -1, hires = 0;
    int ops[OP_INVALID + 1] = {0};
    int late_work = 0;  // non-move, non-pass unit actions at h18-23
    int plants[N_CROPS] = {0};  // plant creations (a tile becoming a plant), not PLANT orders (failed orders excluded)
    int plant_orders = 0;
    int sold[N_SOLD][N_BANDS] = {{0}};
    int deposited[N_SOLD][N_BANDS] = {{0}};  // units reaching the shed (sellable from that hour), by hour band
    double revenue = 0, spend = 0, money_next = 0, value_next = 0, opp_money_next = 0;
    int stock_next = 0, discarded = 0;
    int plants_next = 0, weeds_next = 0, dry_next = 0, field_units_next = 0, held_next = 0;  // field at the next dawn
    double field_value_next = 0;  // crops' stored yield at the next dawn's prices
    int animals_next[3] = {0, 0, 0}, quadrants_next = 0, trims = -1;  // geese / cows / sheep owned (placed + carried + shed)
    // Yield waterings (rules): one-shot waters that added yield, and ongoing crops watered with fertilizer active on the day
    // before a production (tiles marked per step, counted once per day). Other waters only keep the crop alive.
    int water_yield = 0;
    bool ongoing_yield[BOARD][BOARD] = {};

    void step(const Sim& before, const Action& a, const Sim& after, int seat) {
        const int hour = before.st.step % HOURS;
        for (int u = 0; u < a.n_units; ++u) {
            const int op = std::min<int>(a.units[u].op, OP_INVALID);
            ++ops[op];
            if (op >= OP_PICKUP && op != OP_INVALID && hour >= 18) ++late_work;
            plant_orders += op == OP_PLANT;
        }
        const Farm &f0 = before.st.farms[seat], &f1 = after.st.farms[seat];
        for (int y = 0; y < BOARD; ++y)
            for (int x = 0; x < BOARD; ++x)
                if (f1.tiles[y][x].kind == T_PLANT && (f0.tiles[y][x].kind != T_PLANT || f1.tiles[y][x].planted_day != f0.tiles[y][x].planted_day) &&
                    f1.tiles[y][x].what < N_CROPS)
                    ++plants[f1.tiles[y][x].what];
        const int day = before.st.step / HOURS;
        for (int y = 0; y < BOARD; ++y)
            for (int x = 0; x < BOARD; ++x) {
                const Tile &t0 = f0.tiles[y][x], &t1 = f1.tiles[y][x];
                if (t1.kind != T_PLANT || t1.what >= N_CROPS || !t1.watered_today) continue;
                const CropDef& cd = CROPS[t1.what];
                if (!cd.ongoing) {
                    water_yield += !t0.watered_today && t0.kind == T_PLANT && t0.planted_day == t1.planted_day && t1.yield_units > t0.yield_units;
                    continue;
                }
                const int since = day + 1 - t1.planted_day - cd.first_yield_day;
                if (after.st.step / HOURS == day && t1.fertilized_until_day >= day && since >= 0 && since % cd.interval == 0 &&
                    since / cd.interval + 1 <= cd.max_yield && !ongoing_yield[y][x])
                    ongoing_yield[y][x] = true, ++water_yield;
            }
        for (int k = 0; k < N_SOLD; ++k) sold[k][band(hour)] += f1.sold_units[SOLD[k]] - f0.sold_units[SOLD[k]];
        for (int k = 0; k < N_SOLD; ++k)
            deposited[k][band(hour)] += std::max(0, int(f1.shed[SOLD[k]]) - int(f0.shed[SOLD[k]]) + (f1.sold_units[SOLD[k]] - f0.sold_units[SOLD[k]]));
        revenue += f1.sell_revenue - f0.sell_revenue, spend += f1.total_spend - f0.total_spend;
        hires = std::max(hires, f1.hires_today);
        for (int i = 0; i < N_ITEMS; ++i) discarded += f1.discarded[i] - f0.discarded[i];
    }
    void finish(const Sim& next, int seat) {
        const Farm& f = next.st.farms[seat];
        money_next = f.money, opp_money_next = next.st.farms[1 - seat].money, stock_next = f.shed_total;
        value_next = f.money;
        for (int p = 0; p < N_PRODUCTS; ++p) value_next += f.shed[p] * double(next.st.market.prices[p]);
        for (int y = 0; y < BOARD; ++y)
            for (int x = 0; x < BOARD; ++x) {
                const Tile& tile = f.tiles[y][x];
                if (tile.kind == T_WEED) ++weeds_next;
                if (tile.kind == T_PLANT) {
                    ++plants_next, dry_next += tile.consecutive_dry >= 1, field_units_next += tile.yield_units;
                    field_value_next += tile.yield_units * double(next.st.market.prices[tile.what]);
                }
                if (tile.has_animal) held_next += tile.yield_units, ++animals_next[tile.what - GOOSE];  // what = the animal item
            }
        for (int s = 0; s < 3; ++s) {
            animals_next[s] += f.shed[GOOSE + s];
            for (int u = 0; u < f.n_units; ++u) animals_next[s] += f.inv[u][GOOSE + s];
        }
        quadrants_next = f.n_quadrants;
    }
};

void header(std::ostream& out) {
    out << "trace,seat,day,who,status,fallback,dropped,hires,idle,moves,pickup,drop,place,plant,water,harvest,fertilize,dig,feed,"
           "care,collect,late_work";
    for (int c = 0; c < N_CROPS; ++c) out << ",plant_c" << c;
    for (int k = 0; k < N_SOLD; ++k)
        for (int b = 0; b < N_BANDS; ++b) out << ",sold_" << SOLD_NAME[k] << "_b" << b;
    out << ",plant_orders,revenue,spend,money_next,value_next,stock_next,discarded,opp_money_next,plants_next,weeds_next,dry_next,"
           "field_units_next,field_value_next,held_next,geese_next,cows_next,sheep_next,quadrants_next,trims,water_yield";
    for (int k = 0; k < N_SOLD; ++k)
        for (int b = 0; b < N_BANDS; ++b) out << ",dep_" << SOLD_NAME[k] << "_b" << b;
    out << '\n';
}

void row(std::ostream& out, const std::string& trace, int seat, int day, const char* who, const Day& d) {
    const int moves = d.ops[OP_NORTH] + d.ops[OP_SOUTH] + d.ops[OP_EAST] + d.ops[OP_WEST];
    out << trace << ',' << seat << ',' << day << ',' << who << ',' << d.status << ',' << d.fallback << ',' << d.dropped << ','
        << d.hires << ',' << d.ops[OP_PASS] << ',' << moves << ',' << d.ops[OP_PICKUP] << ',' << d.ops[OP_DROP] << ','
        << d.ops[OP_PLACE] << ',' << d.ops[OP_PLANT] << ',' << d.ops[OP_WATER] << ',' << d.ops[OP_HARVEST] << ','
        << d.ops[OP_FERTILIZE] << ',' << d.ops[OP_DIG] << ',' << d.ops[OP_FEED] << ',' << d.ops[OP_CARE] << ','
        << d.ops[OP_COLLECT_FERTILIZER] << ',' << d.late_work;
    for (int c = 0; c < N_CROPS; ++c) out << ',' << d.plants[c];
    for (int k = 0; k < N_SOLD; ++k)
        for (int b = 0; b < N_BANDS; ++b) out << ',' << d.sold[k][b];
    out << ',' << d.plant_orders << ',' << d.revenue << ',' << d.spend << ',' << d.money_next << ',' << d.value_next << ',' << d.stock_next << ','
        << d.discarded << ',' << d.opp_money_next << ',' << d.plants_next << ',' << d.weeds_next << ',' << d.dry_next << ','
        << d.field_units_next << ',' << d.field_value_next << ',' << d.held_next << ',' << d.animals_next[0] << ','
        << d.animals_next[1] << ',' << d.animals_next[2] << ',' << d.quadrants_next << ',' << d.trims << ',' << d.water_yield;
    for (int k = 0; k < N_SOLD; ++k)
        for (int b = 0; b < N_BANDS; ++b) out << ',' << d.deposited[k][b];
    out << '\n';
}
}  // namespace

int main(int argc, char** argv) {
    if (argc != 6) {
        std::cerr << "usage: teacher_day list.txt first_day last_day threads out.csv\n";
        return 2;
    }
    std::vector<std::pair<std::string, int>> games;
    {
        std::ifstream list(argv[1]);
        std::string trace;
        int seat;
        for (std::string line; std::getline(list, line);) {
            std::istringstream words(line);
            if (words >> trace >> seat) games.push_back({trace, seat});
        }
    }
    const int first = std::atoi(argv[2]), last = std::atoi(argv[3]), threads = std::atoi(argv[4]);
    const bool net = std::getenv("TEACHER_NET") != nullptr;
    const int span = std::getenv("TEACHER_DAYS") ? std::atoi(std::getenv("TEACHER_DAYS")) : 1;
    const bool hourly = std::getenv("TEACHER_HOURLY") != nullptr;
    // TEACHER_LABELS=1 (with TEACHER_DAYS): every later dawn of the span compiles M&M's own label for that day when our farm's crop
    // and animal groups match its schema (type, age, size); otherwise our network's intent. The multi-day compiler test.
    const bool labels_all = std::getenv("TEACHER_LABELS") != nullptr;
    const char* model = std::getenv("BC_OPUS_MODEL");
    if (!model) {
        std::cerr << "BC_OPUS_MODEL is required\n";
        return 2;
    }
    std::ofstream out(argv[5]);
    header(out);
    std::mutex lock;
    std::atomic<int> next{0};
    std::vector<std::thread> pool;
    for (int t = 0; t < threads; ++t)
        pool.emplace_back([&] {
            for (int g; (g = next++) < int(games.size());) {
                const auto& [trace, seat] = games[g];
                const Replay replay = load_replay(trace);
                const auto states = replay_states(replay);
                std::vector<double> flow(30 * HOURS * N_PRODUCTS, 0.0);  // TEACHER_ORACLE: the opponent's accepted sells - buys per hour
                if (std::getenv("TEACHER_ORACLE")) {
                    for (size_t s = 0; s < replay.turns.size() && s + 1 < states.size() && s < 30 * HOURS; ++s) {
                        Sim at = states[s];
                        const auto acc = at.sanitize_joint_actions(replay.turns[s][0], replay.turns[s][1])[1 - seat];
                        for (int k = 0; k < acc.n_orders; ++k) {
                            const auto& o = acc.orders[k];
                            if (o.op == M_SELL && o.item < N_PRODUCTS) flow[s * N_PRODUCTS + o.item] += o.n;
                            if (o.op == M_BUY_PRODUCT && o.item < N_PRODUCTS) flow[s * N_PRODUCTS + o.item] -= o.n;
                        }
                    }
                    kag::agents::bc_overhaul::dc11_oracle_flow = flow.data();
                } else kag::agents::bc_overhaul::dc11_oracle_flow = nullptr;
                std::ostringstream rows;
                for (int day = first; day <= last && (day + span) * HOURS < int(states.size()); ++day) {
                    const Conversion label = convert_day(states, replay.turns, seat, day);
                    if (label.status == ConvertStatus::Failed) continue;
                    kag::agents::bc_overhaul::Agent agent;
                    agent.model_path = model;
                    if (!std::getenv("DC11_OPTIONS")) agent.options_text = dc11_sidecar(model);
                    agent.reset(kag::agent::runtime::make_agent_init(states[0], seat));
                    for (int k = 0; k < day * HOURS; ++k)
                        agent.observe(kag::agent::runtime::make_observation(states[k], seat), replay.turns[k][seat]);
                    if (!net) agent.teacher_day = day, agent.teacher_intent = label.intent;
                    if (std::getenv("TEACHER_DUMP")) {  // the label's intent: new crops / animals, and per-group harvest, water, feed, ...
                        const DayIntent& in = label.intent;
                        const Schema& s = label.schema;
                        std::ostringstream o;
                        o << "label " << trace << " d" << day << " new";
                        for (int c = 0; c < N_CROPS; ++c) o << ' ' << in.new_crop[c];
                        o << " | animals";
                        for (int a = 0; a < N_ANIMALS; ++a) o << ' ' << in.new_animal[a];
                        for (int i = 0; i < s.n_crops; ++i) {
                            int harvest = in.harvest[i], water = 0;
                            for (int op = 0; op < OPTIONS; ++op) harvest += has_harvest(op) * in.options[i][op], water += has_water(op) * in.options[i][op];
                            o << " | crop" << i << " c" << int(s.crops[i].crop) << " n" << int(s.crops[i].size) << " h" << harvest << " w" << water;
                        }
                        for (int i = 0; i < s.n_animals; ++i) o << " | animal" << i << " feed" << in.feed[i] << " care" << in.care[i] << " coll" << in.collect[i];
                        std::fprintf(stderr, "%s\n", o.str().c_str());
                    }
                    kag::agent::DecisionBudget budget;
                    budget.max_expansions = 256;
                    Sim sim = states[day * HOURS];
                    Day ours, teacher;
                    double prof0[16];
                    std::copy_n(dc11::route_prof, 16, prof0);
                    int labels_used = 0, labels_missed = 0;  // TEACHER_LABELS: later dawns compiled with M&M's own label
                    for (int k = day * HOURS; k < (day + span) * HOURS; ++k) {
                        if (labels_all && k > day * HOURS && k % HOURS == 0) {
                            const int d = k / HOURS;
                            const Conversion lab = convert_day(states, replay.turns, seat, d);
                            const Schema ours = describe(kag::agent::runtime::make_observation(sim, seat));
                            bool same = lab.status != ConvertStatus::Failed && ours.n_crops == lab.schema.n_crops && ours.n_animals == lab.schema.n_animals;
                            for (int g = 0; same && g < ours.n_crops; ++g) {
                                const auto &a = ours.crops[g], &b = lab.schema.crops[g];
                                same = a.crop == b.crop && a.age == b.age && a.size == b.size && a.fresh == b.fresh;
                            }
                            for (int g = 0; same && g < ours.n_animals; ++g) {
                                const auto &a = ours.animals[g], &b = lab.schema.animals[g];
                                same = a.species == b.species && a.size == b.size && a.fresh == b.fresh;
                            }
                            if (same) agent.teacher_day = d, agent.teacher_intent = lab.intent, ++labels_used;
                            else ++labels_missed;
                            if (!same && std::getenv("TEACHER_LABELS_WHY")) {  // labelwhy: our groups vs the label's (crop / species, age, size, fresh)
                                std::string w = "labelwhy," + trace + "," + std::to_string(d) + " ours";
                                for (int g = 0; g < ours.n_crops; ++g) w += " c" + std::to_string(ours.crops[g].crop) + "a" + std::to_string(ours.crops[g].age) + "n" + std::to_string(ours.crops[g].size) + (ours.crops[g].fresh ? "f" : "");
                                for (int g = 0; g < ours.n_animals; ++g) w += " s" + std::to_string(ours.animals[g].species) + "n" + std::to_string(ours.animals[g].size) + (ours.animals[g].fresh ? "f" : "");
                                w += " | label";
                                for (int g = 0; g < lab.schema.n_crops; ++g) w += " c" + std::to_string(lab.schema.crops[g].crop) + "a" + std::to_string(lab.schema.crops[g].age) + "n" + std::to_string(lab.schema.crops[g].size) + (lab.schema.crops[g].fresh ? "f" : "");
                                for (int g = 0; g < lab.schema.n_animals; ++g) w += " s" + std::to_string(lab.schema.animals[g].species) + "n" + std::to_string(lab.schema.animals[g].size) + (lab.schema.animals[g].fresh ? "f" : "");
                                std::fprintf(stderr, "%s\n", w.c_str());
                            }
                        }
                        Action mine;
                        agent.act(kag::agent::runtime::make_observation(sim, seat), budget, mine);
                        std::array<Action, 2> joint = replay.turns[k];
                        joint[seat] = mine;
                        const Sim before = sim;
                        sim.step(joint[0], joint[1]);
                        ours.step(before, mine, sim, seat);
                        teacher.step(states[k], replay.turns[k][seat], states[k + 1], seat);
                        if (hourly) {  // TEACHER_HOURLY: one CSV line per hour and side (prefix "hourcsv,")
                            auto line = [&](const char* who, const Sim& b, const Action& a, const Sim& n) {
                                int ops[OP_INVALID + 1] = {0}, created = 0, sold = 0;
                                for (int u = 0; u < a.n_units; ++u) ++ops[std::min<int>(a.units[u].op, OP_INVALID)];
                                const Farm &f0 = b.st.farms[seat], &f1 = n.st.farms[seat];
                                for (int y = 0; y < BOARD; ++y)
                                    for (int x = 0; x < BOARD; ++x)
                                        created += f1.tiles[y][x].kind == T_PLANT &&
                                                   (f0.tiles[y][x].kind != T_PLANT || f1.tiles[y][x].planted_day != f0.tiles[y][x].planted_day);
                                for (int p = 0; p < N_PRODUCTS; ++p) sold += f1.sold_units[p] - f0.sold_units[p];
                                const int moves = ops[OP_NORTH] + ops[OP_SOUTH] + ops[OP_EAST] + ops[OP_WEST];
                                std::fprintf(stderr, "hourcsv,%s,%d,%d,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%.0f,%.0f,%.0f,%d,%d\n", trace.c_str(),
                                             k / HOURS, k % HOURS, who, a.n_units, ops[OP_PASS], moves, created, ops[OP_WATER], ops[OP_HARVEST],
                                             ops[OP_FEED], ops[OP_CARE], ops[OP_COLLECT_FERTILIZER], ops[OP_PICKUP], ops[OP_DROP] + ops[OP_PLACE],
                                             f1.hires_today - f0.hires_today, sold, f1.sold_units[WHEAT] - f0.sold_units[WHEAT],
                                             f1.sell_revenue - f0.sell_revenue, f1.total_spend - f0.total_spend, f1.money,
                                             f1.sold_units[FERTILIZER] - f0.sold_units[FERTILIZER], ops[OP_PLANT]);
                            };
                            line("ours", before, mine, sim), line("teacher", states[k], replay.turns[k][seat], states[k + 1]);
                        }
                    }
                    ours.finish(sim, seat), teacher.finish(states[(day + span) * HOURS], seat);
                    const auto& r = agent.reports()[agent.reports().size() - span];
                    ours.status = r.status, ours.fallback = r.fallback, ours.dropped = r.dropped, ours.trims = r.trims;
                    if (labels_all)  // labels,trace,seat,day,used,missed
                        std::fprintf(stderr, "labels,%s,%d,%d,%d,%d\n", trace.c_str(), seat, day, labels_used, labels_missed);
                    if (std::getenv("DC12_ROUTEPROF")) {  // routeprof,trace,seat,day, then route_prof deltas [0..15]
                        std::string line = "routeprof," + trace + "," + std::to_string(seat) + "," + std::to_string(day);
                        for (int k = 0; k < 16; ++k) line += "," + std::to_string(dc11::route_prof[k] - prof0[k]);
                        std::fprintf(stderr, "%s\n", line.c_str());
                    }
                    if (std::getenv("TEACHER_REASON")) {  // the first day's compile report: reason,trace,seat,day,hires,dropped,reason
                        std::fprintf(stderr, "reason,%s,%d,%d,%d,%d,%s\n", trace.c_str(), seat, day, ours.hires, r.dropped, r.reason.c_str());
                        for (int j = 0; j < span; ++j) {  // every day: report,trace,seat,start,day,new_crops,fallback,trims,dropped,hires
                            const auto& q = agent.reports()[agent.reports().size() - span + j];
                            std::fprintf(stderr, "report,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%.1f,%.1f,%.1f,%.1f,%s\n", trace.c_str(), seat, day, q.day,
                                         q.new_crops, q.fallback, q.trims, q.dropped, q.hires, q.land_hour, q.stress_ok, q.compile_ms, q.ms_route,
                                         q.ms_realize, q.ms_fund, q.reason.c_str());
                        }
                    }
                    row(rows, trace, seat, day, "ours", ours), row(rows, trace, seat, day, "teacher", teacher);
                }
                std::lock_guard<std::mutex> guard(lock);
                out << rows.str();
                out.flush();
            }
        });
    for (auto& t : pool) t.join();
    return 0;
}
