#include "baseline.hpp"
#include "timing.hpp"
#include "compact.hpp"
#include "storage.hpp"
#include "value_storage.hpp"
#include "forecast_library.hpp"
#include "case.hpp"
#include "immediate_sales.hpp"
#include "sale_priority.hpp"
#include <chrono>

using namespace sales_planner;

struct Result {
    std::array<double, 2> cash{}, receipts{}, spending{};
    std::array<int, 2> missing{}, commitments{}, discarded{};
    int first_gap = -1, first_own_gap = -1, first_own_commitment_gap = -1, first_own_gap_item = -1;
    double seconds = 0;
    TimingMemory timing;
    int compacted_orders = 0;
    StorageStats storage;
    ValueStats value;
};

static Result run(const Case& c, int seat, int policy, std::span<const HistoricalScenario> library = {}, bool mixed_timing = false) {
    auto state = c.initial.financial; auto resources = c.initial.resources;
    const auto rules = rules_for(c.config);
    std::vector<CalendarTurn> own_calendar;
    std::vector<Orders> own_warm;
    for (const auto& t : c.turns) { own_calendar.push_back(t.calendar[seat]); own_warm.push_back(t.original_orders[seat]); }
    const BaselineOptions options[] = {{24, 8, 24, true}, {1, 1, 1, false}, {8, 8, 8, false},
        {24, 8, 24, false}, {2, 2, 2, false}, {4, 4, 4, false},
        {8, 8, 8, false, true}, {24, 8, 24, false, true},
        {2, 2, 2, false, false, true}, {4, 4, 4, false, false, true},
        {2, 2, 2, false, false, false, true}, {4, 4, 4, false, false, false, true}};
    Result result;
    const auto started = std::chrono::steady_clock::now();
    for (int t = 0; t < int(c.turns.size()); ++t) {
        const auto& turn = c.turns[t];
        state.n_shops = turn.n_shops; state.shops = turn.shops;
        for (int p = 0; p < 2; ++p)
            apply(state.accounts[p], resources[p], turn.calendar[p].before_market, rules.capacity);
        auto orders = turn.original_orders;
        if (policy) {
            PlannerObservation obs{t, state.accounts[seat], resources[seat], state.inventory,
                                   state.shops, state.n_shops, state.accounts[seat ^ 1].cash};
            if (policy >= 24) {
                auto warm=make_room(obs,own_calendar,turn.original_orders[seat],rules,result.storage);
                orders[seat]=policy==26 ? sale_priority(obs.own,obs.inventory,warm) :
                    immediate_sales(obs.own,obs.inventory,warm,rules,policy==24);
            } else if (policy >= 22) {
                StorageStats preflight;
                const auto proposal = make_room(obs, own_calendar, turn.original_orders[seat], rules, preflight);
                orders[seat] = proposal;
                if (preflight.decisions) {
                    std::array<MarketForecast, 16> forecasts;
                    for (int k = 0; k < int(library.size()); ++k) forecasts[k] = library[k].at(obs);
                    orders[seat] = valued_storage_scenarios(obs, own_calendar, own_warm, rules,
                        std::span<const MarketForecast>(forecasts.data(), library.size()), result.storage, result.value, policy == 23);
                }
            } else if (policy == 21) {
                orders[seat] = valued_storage(obs, own_calendar, own_warm, rules, result.storage, result.value);
            } else if (policy >= 18) {
                const int horizons[] = {0, 4, terminal_turn};
                orders[seat] = make_room(obs, own_calendar, turn.original_orders[seat], rules, result.storage, horizons[policy - 18]);
            } else if (policy >= 15) {
                orders[seat] = compact_orders(obs.own, turn.original_orders[seat], policy - 15);
                result.compacted_orders += turn.original_orders[seat].count - orders[seat].count;
            } else if (policy >= 13) {
                const Orders next = t + 1 < int(c.turns.size()) ? c.turns[t + 1].original_orders[seat] : Orders{};
                orders[seat] = delay_for_demand(obs, own_calendar, turn.original_orders[seat], next,
                                               turn.public_ready[seat ^ 1], result.timing, rules, policy == 14, 20, mixed_timing);
            } else orders[seat] = baseline_orders(obs, own_calendar, rules, options[policy - 1], &turn.original_orders[seat]);
        }
        const auto trades = trade(state, orders, rules); consume(state, rules);
        for (int p = 0; p < 2; ++p) {
            apply(state.accounts[p], resources[p], turn.calendar[p].after_market, rules.capacity);
            result.receipts[p] += trades.receipts[p]; result.spending[p] += trades.spending[p];
            int required[3]{}, actual[3]{};
            for (int slot = 0; slot < turn.calendar[p].commitments.count; ++slot) {
                const auto op = turn.calendar[p].commitments.values[slot].op;
                if (op == kag::M_HIRE || op == kag::M_BUY_LAND) ++required[op];
            }
            for (int slot = 0; slot < orders[p].count; ++slot) {
                const auto op = orders[p].values[slot].op;
                if (op == kag::M_HIRE || op == kag::M_BUY_LAND) actual[op] += trades.accepted[p][slot];
            }
            const int mismatch = std::abs(actual[kag::M_HIRE] - required[kag::M_HIRE]) +
                                 std::abs(actual[kag::M_BUY_LAND] - required[kag::M_BUY_LAND]);
            result.commitments[p] += mismatch;
            if (mismatch && p == seat && result.first_own_commitment_gap < 0) result.first_own_commitment_gap = t;
            int missing = 0; for (int n : resources[p].missing) missing += n;
            if (missing && result.first_gap < 0) result.first_gap = t;
            if (missing && p == seat && result.first_own_gap < 0) {
                result.first_own_gap = t;
                for (int item = 0; item < kag::N_ITEMS; ++item)
                    if (resources[p].missing[item]) { result.first_own_gap_item = item; break; }
            }
        }
        advance(state, rules);

    }
    result.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
    for (int p = 0; p < 2; ++p) {
        result.cash[p] = state.accounts[p].cash;
        for (int n : resources[p].missing) result.missing[p] += n;
        for (int n : resources[p].discarded) result.discarded[p] += n;
    }
    return result;
}

int main(int argc, char** argv) {
    require(argc > 1, "usage: benchmark_baselines case.calendar [...]");
    const char* names[] = {"original", "warm_sales24", "cold1", "cold8", "cold24",
                           "cold2", "cold4", "cold8_bridge", "cold24_bridge", "cold2_keep", "cold4_keep",
                           "cold2_priority", "cold4_priority", "wait_demand", "wait_no_ready",
                           "compact_all", "compact_sales", "compact_nonbuyable", "room_keep", "room4", "room_all", "room_value_quiet", "room_value_scenarios", "room_value_all_gains",
                           "immediate_sales", "immediate_all", "sale_priority"};
    const bool mixed_timing = std::string(argv[1]) == "--mixed-timing";
    const bool timing_only = std::string(argv[1]) == "--timing" || mixed_timing;
    if (mixed_timing) { names[13] = "wait_mixed_demand"; names[14] = "wait_mixed_no_ready"; }
    const bool compact_only = std::string(argv[1]) == "--compact";
    const bool storage_only = std::string(argv[1]) == "--storage";
    const bool scenarios_only = std::string(argv[1]) == "--scenarios";
    const bool patterns_only = std::string(argv[1]) == "--patterns";
    int first_file = timing_only || compact_only || storage_only || patterns_only ? 2 : 1;
    std::vector<HistoricalScenario> library;
    if (scenarios_only) {
        require(argc > 3, "missing scenario files");
        const int count = std::stoi(argv[2]);
        require(count > 0 && count <= 8 && argc > count + 3, "invalid scenario count");
        for (int i = 0; i < count; ++i) {
            auto source = read_case(argv[i + 3]); certify_commitments(source);
            for (int seat = 0; seat < 2; ++seat) library.emplace_back(source, seat);
        }
        first_file = count + 3;
    }
    for (int i = first_file; i < argc; ++i) {
        auto c = read_case(argv[i]);
        certify_commitments(c);
        for (const auto& source : library) require(source.source_episode != c.episode, "scenario source overlaps evaluation");
        for (int seat = 0; seat < 2; ++seat)
            for (int policy = 0; policy < 27; ++policy) {
                if(patterns_only && policy!=0 && policy!=18 && policy<24)continue;
                if(!patterns_only && policy>=24)continue;
                if (timing_only && policy != 0 && (policy < 13 || policy > 14)) continue;
                if (compact_only && policy != 0 && (policy < 15 || policy > 17)) continue;
                if (storage_only && policy != 0 && policy != 16 && policy < 18) continue;
                if (!scenarios_only && policy >= 22 && policy < 24) continue;
                if (scenarios_only && policy != 0 && policy != 16 && policy != 18 && policy < 21) continue;
                const auto r = run(c, seat, policy, library, mixed_timing);
                std::printf("{\"episode\":%llu,\"seat\":%d,\"policy\":\"%s\",\"cash\":%.0f,\"rival_cash\":%.0f,\"margin\":%.0f,\"own_missing\":%d,\"rival_missing\":%d,\"own_commitment_failures\":%d,\"rival_commitment_failures\":%d,\"own_discarded\":%d,\"first_gap\":%d,\"first_own_gap\":%d,\"first_own_commitment_gap\":%d,\"first_own_gap_item\":%d,\"delayed_units\":%llu,\"delay_decisions\":%llu,\"predicted_own_gain\":%.0f,\"compacted_orders\":%d,\"storage_decisions\":%d,\"storage_sold\":%d,\"storage_predicted_saved\":%d,\"storage_evaluations\":%d,\"value_decisions\":%d,\"value_accepted\":%d,\"value_evaluations\":%d,\"value_turns\":%d,\"value_unknown\":%d,\"value_predicted_gain\":%.0f,\"seconds\":%.9f}\n",
                            (unsigned long long)c.episode, seat, names[policy], r.cash[seat], r.cash[seat ^ 1],
                            r.cash[seat] - r.cash[seat ^ 1], r.missing[seat], r.missing[seat ^ 1],
                            r.commitments[seat], r.commitments[seat ^ 1], r.discarded[seat], r.first_gap,
                            r.first_own_gap, r.first_own_commitment_gap, r.first_own_gap_item,
                            (unsigned long long)r.timing.delayed_units, (unsigned long long)r.timing.decisions,
                            r.timing.predicted_gain, r.compacted_orders, r.storage.decisions, r.storage.sold,
                            r.storage.predicted_saved, r.storage.evaluations, r.value.decisions, r.value.accepted,
                            r.value.evaluations, r.value.evaluated_turns, r.value.unknown, r.value.predicted_gain, r.seconds);
            }
    }
}
