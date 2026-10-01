// Full-agent replay continuation: from a replay dawn to the end of the game, our agent
// plays the perspective's seat against the opponent's recorded actions. The agent's
// history is fed with the replay's earlier observations and actions. Scores the exact
// final margin against the replay (a fixed-opponent diagnostic).
// usage: continuation corpus.txt out.csv start_day [start_day...]   (corpus: episode seat ... trace)
#include "agent/bc_opus/source/agent.hpp"
#include "source/convert.hpp"
#include <iostream>
#include <sstream>

using namespace dc10;

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "usage: continuation corpus.txt out.csv start_day...\n";
        return 2;
    }
    std::ifstream corpus(argv[1]);
    std::ofstream out(argv[2]);
    out << "episode,seat,start_day,margin,replay_margin,own_cash,replay_cash,opponent_cash,replay_opponent_cash,"
           "unit_failed,replay_unit_failed,order_units_failed,discarded,replay_discarded,sold_units,replay_sold_units,"
           "uncompiled_days,fallback_days,invalid_intents,compile_ms_max\n";
    std::string line;
    while (std::getline(corpus, line)) {
        std::istringstream words(line);
        long episode = 0;
        int seat = 0;
        std::string field, path;
        words >> episode >> seat;
        while (words >> field) path = field;
        const Replay replay = load_replay(path);
        const auto states = replay_states(replay);
        const int end_step = int(replay.turns.size());
        for (int a = 3; a < argc; ++a) {
            const int start_day = std::atoi(argv[a]);
            kag::agents::bc_opus::Agent agent;
            const char* trace_key = std::getenv("BC_TRACE");
            const bool trace = trace_key && std::to_string(episode) + ":" + std::to_string(seat) == trace_key;
            agent.reset(kag::agent::runtime::make_agent_init(states[0], seat));
            for (int k = 0; k < start_day * HOURS; ++k)
                agent.observe(agent::runtime::make_observation(states[k], seat), replay.turns[k][seat]);
            Sim sim = states[start_day * HOURS];
            int unit_failed = 0, replay_unit_failed = 0, orders_failed = 0;
            kag::agent::DecisionBudget budget;
            for (int step = start_day * HOURS; step < end_step; ++step) {
                Action mine;
                agent.act(agent::runtime::make_observation(sim, seat), budget, mine);
                if (trace && step % HOURS == 0) {
                    // Our decoded intent (from the agent's report) vs the replay's label today.
                    const int day = step / HOURS;
                    const auto label = convert_day(states, replay.turns, seat, day);
                    auto summary = [&](const DayIntent& in, const Schema& s) {
                        std::ostringstream o;
                        o << "new";
                        for (int c = 0; c < N_CROPS; ++c) o << ' ' << in.new_crop[c];
                        o << " | animals";
                        for (int a = 0; a < N_ANIMALS; ++a) o << ' ' << in.new_animal[a] << '+' << in.reserve[a];
                        int harvest = 0, feed = 0, care = 0, collect = 0, water = 0, retain = 0, clear = 0;
                        for (int i = 0; i < s.n_crops; ++i) {
                            for (int op = 0; op < OPTIONS; ++op) {
                                harvest += has_harvest(op) * in.options[i][op];
                                water += has_water(op) * in.options[i][op];
                                clear += (op == CLEAR) * in.options[i][op];
                            }
                            harvest += in.harvest[i], retain += in.retain[i], clear += in.clear[i];
                        }
                        for (int i = 0; i < s.n_animals; ++i) feed += in.feed[i], care += in.care[i], collect += in.collect[i];
                        o << " | land " << in.buy_land << " harvest " << harvest << " water " << water << " retain " << retain
                          << " clear " << clear << " feed " << feed << " care " << care << " collect " << collect;
                        return o.str();
                    };
                    const auto& r = agent.reports().back();
                    int lost = 0;
                    for (int it = 0; it < N_ITEMS; ++it) lost += sim.st.farms[seat].discarded[it];
                    std::fprintf(stderr, "d%02d lost %d return %d cash %8.0f/%8.0f opp %8.0f/%8.0f status %d fallback %d\n  ours:   %s\n  replay: %s\n", lost, r.return_percent,
                                 day, sim.st.farms[seat].money, states[step].st.farms[seat].money, sim.st.farms[1 - seat].money,
                                 states[step].st.farms[1 - seat].money, r.status, r.fallback,
                                 summary(agent.last_intent(), agent.last_schema()).c_str(),
                                 label.status == ConvertStatus::Ok ? summary(label.intent, label.schema).c_str() : "n/a");
                }
                std::array<Action, 2> joint = replay.turns[step];
                joint[seat] = mine;
                const auto diag = sim.diagnose_joint_actions(joint[0], joint[1]).players[seat];
                unit_failed += diag.requested_unit_actions - diag.successful_unit_actions;
                const auto rdiag = states[step].diagnose_joint_actions(replay.turns[step][0], replay.turns[step][1]).players[seat];
                replay_unit_failed += rdiag.requested_unit_actions - rdiag.successful_unit_actions;
                const auto accepted = sim.sanitize_joint_actions(joint[0], joint[1])[seat];
                for (int o = 0; o < mine.n_orders; ++o)
                    if (mine.orders[o].op != M_SELL) {
                        const int want = (mine.orders[o].op == M_HIRE || mine.orders[o].op == M_BUY_LAND) ? 1 : mine.orders[o].n;
                        orders_failed += std::max(0, want - accepted.orders[o].n);
                    }
                const char* hour_day = std::getenv("BC_HOUR_TRACE");
                const bool hourly = trace && hour_day && step / HOURS == std::atoi(hour_day);
                int lost_before = 0, carried = 0;
                for (int it = 0; it < N_ITEMS; ++it) lost_before += sim.st.farms[seat].discarded[it];
                for (int u = 0; u < sim.st.farms[seat].n_units; ++u)
                    for (int it = 0; it < N_ITEMS; ++it) carried += sim.st.farms[seat].inv[u][it];
                const int shed_before = sim.st.farms[seat].shed_total;
                sim.step(joint[0], joint[1]);
                if (hourly) {
                    int lost = -lost_before;
                    for (int it = 0; it < N_ITEMS; ++it) lost += sim.st.farms[seat].discarded[it];
                    std::fprintf(stderr, "  h%02d shed %3d carried %3d lost %3d | orders:", step % HOURS, shed_before, carried, lost);
                    for (int o = 0; o < accepted.n_orders; ++o)
                        if (accepted.orders[o].n) std::fprintf(stderr, " %d:%d:%d", accepted.orders[o].op, accepted.orders[o].item, accepted.orders[o].n);
                    std::fprintf(stderr, "\n");
                }
            }
            int uncompiled = 0, fallback = 0, invalid = 0;
            double compile_max = 0;
            for (const auto& r : agent.reports()) {
                uncompiled += r.status != int(CompileStatus::Ok);
                fallback += r.status == int(CompileStatus::Ok) && r.fallback != 0;
                invalid += !r.invalid.empty();
                compile_max = std::max(compile_max, r.compile_ms);
            }
            const Farm& mine = sim.st.farms[seat];
            const Farm& theirs = states[end_step].st.farms[seat];
            const Farm& dawn = states[start_day * HOURS].st.farms[seat];
            int discarded = 0, replay_discarded = 0, sold = 0, replay_sold = 0;
            for (int i = 0; i < N_ITEMS; ++i) {
                discarded += mine.discarded[i] - dawn.discarded[i];
                replay_discarded += theirs.discarded[i] - dawn.discarded[i];
                if (i < N_PRODUCTS) sold += mine.sold_units[i] - dawn.sold_units[i];
                if (i < N_PRODUCTS) replay_sold += theirs.sold_units[i] - dawn.sold_units[i];
            }
            const double opponent = sim.st.farms[1 - seat].money, replay_opponent = states[end_step].st.farms[1 - seat].money;
            out << episode << ',' << seat << ',' << start_day << ',' << mine.money - opponent << ','
                << theirs.money - replay_opponent << ',' << mine.money << ',' << theirs.money << ',' << opponent << ','
                << replay_opponent << ',' << unit_failed << ',' << replay_unit_failed << ',' << orders_failed << ','
                << discarded << ',' << replay_discarded << ',' << sold << ',' << replay_sold << ',' << uncompiled << ','
                << fallback << ',' << invalid << ',' << compile_max << '\n';
            out.flush();
        }
    }
    return 0;
}
