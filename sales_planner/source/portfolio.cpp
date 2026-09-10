#include "registry.hpp"
#include "compile_calendar.hpp"
#include "forecast_library.hpp"
#include "project_resources.hpp"
#include "agents/external/atakan_demand/source/base/agent.hpp"
#include <iomanip>

using namespace sales_planner;
using Course = compositions::atakan_portfolio::Agent;

struct WorldPrediction {
    uint64_t episode=0;
    int seat=0;
    double cash=0,margin=0,source_cash=0;
    std::array<int,2> missing{},commitments{};
    ContinuationTrace trace;
};
struct Prediction {
    double margin = 0, own_cash = 0, seconds = 0;
    int own_failed = 0, rival_failed = 0, work = 0, compile_faults = 0;
    bool complete = false;
    std::vector<WorldPrediction> worlds;
    std::vector<CalendarTurn> calendar;
    std::array<PurchaseRepairStats,2> repairs{};
};

static Prediction predict(const kag::agent::AgentObservation& start, const kag::agent::AgentConfig& config,
                          int branch, std::span<const HistoricalScenario> library,bool detail=false,int repair_sides=0,
                          RivalCashSource cash_source = RivalCashSource::current_observation) {
    const auto began = std::chrono::steady_clock::now();
    kag::agent::DecisionBudget budget; budget.max_expansions = 100000;
    Course course(branch);
    auto compiled = compile_calendar(start, config, course, 202609091234, budget);
    Prediction p; p.complete = compiled.complete;
    p.work = compiled.projections + compiled.simulated_turns;
    p.compile_faults = compiled.failed_work;
    if (compiled.complete) {
        PlannerObservation obs;
        obs.turn = start.step; obs.own = compiled.plan.starting_account; obs.resources = compiled.plan.starting_resources;
        obs.rival_cash = start.opponent().money; obs.n_shops = start.n_shops;
        std::copy_n(start.shops, start.n_shops, obs.shops.begin());
        std::copy_n(start.market.inventory, kag::N_PRODUCTS, obs.inventory.begin());
        const MarketRules rules{config.shed_capacity, config.max_orders, config.hire_mult,
                                config.turns_per_day, config.shop_sell_interval, config.center_sell_interval};
        apply(obs.own, obs.resources, compiled.plan.turns[obs.turn].before_market, rules.capacity);
        for (const auto& source : library) {
            const auto forecast = source.at(obs, cash_source);
            ContinuationTrace trace;
            const auto value = evaluate_continuation(obs, compiled.plan.turns, compiled.warm_orders, forecast, rules, nullptr, false,detail?&trace:nullptr,repair_sides);
            p.margin += value.margin(); p.own_cash += value.accounts[0].cash;
            p.work += value.evaluated_turns;
            bool own_gap = value.commitment_errors[0], rival_gap = value.commitment_errors[1];
            for (int n : value.resources[0].missing) own_gap |= n > 0;
            for (int n : value.resources[1].missing) rival_gap |= n > 0;
            p.own_failed += own_gap; p.rival_failed += rival_gap;
            for(int side=0;side<2;++side) {
                p.repairs[side].decisions+=value.repairs[side].decisions;p.repairs[side].orders+=value.repairs[side].orders;
                p.repairs[side].units+=value.repairs[side].units;p.repairs[side].rejected+=value.repairs[side].rejected;
            }
            if(detail) {
                WorldPrediction w;
                w.episode=source.source_episode;w.seat=source.source_seat;w.cash=value.accounts[0].cash;w.margin=value.margin();
                w.source_cash=source.accounts[obs.turn].cash;w.commitments=value.commitment_errors;w.trace=trace;
                for(int p=0;p<2;++p)for(int n:value.resources[p].missing)w.missing[p]+=n;
                p.worlds.push_back(w);
            }
        }
        p.margin /= library.size(); p.own_cash /= library.size();
    }
    if(repair_sides&1)p.calendar=std::move(compiled.plan.turns);
    p.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - began).count();
    return p;
}

int main(int argc, char** argv) {
    require(argc >= 7, "usage: portfolio opponent seed_start games output scenario.calendar [...]");
    const std::string opponent = argv[1]; const uint64_t seed_start = std::stoull(argv[2]);
    const int games = std::stoi(argv[3]);
    std::ofstream out(argv[4]); require(bool(out), "cannot open results"); out << std::setprecision(12);
    std::vector<HistoricalScenario> library;
    bool detail=false;int repair_sides=0;
    RivalCashSource cash_source = RivalCashSource::current_observation;
    for (int i = 5; i < argc; ++i) {
        if(std::string(argv[i])=="--trace"){detail=true;continue;}
        if(std::string(argv[i])=="--repair-own"){repair_sides=1;continue;}
        if(std::string(argv[i])=="--repair-both"){repair_sides=3;continue;}
        if(std::string(argv[i])=="--historical-rival-cash"){cash_source=RivalCashSource::historical_case;continue;}
        auto c = read_case(argv[i]); certify_commitments(c);
        for (int seat = 0; seat < 2; ++seat) library.emplace_back(c, seat);
    }
    std::ofstream details;
    if(detail){details.open(std::string(argv[4])+".forecasts.jsonl");require(bool(details),"cannot open forecast details");details<<std::setprecision(12);}
    for (int game = 0; game < games; ++game) for (int seat = 0; seat < 2; ++seat) {
        const uint64_t seed = seed_start + game;
        kag::Config config; config.seed = seed;
        kag::Sim prefix(config); Course source(0); auto rival = make_agent(opponent);
        const auto init = kag::agent::runtime::make_agent_init(prefix, seat);
        source.reset(init); rival.reset(kag::agent::runtime::make_agent_init(prefix, seat ^ 1));
        kag::agent::DecisionBudget budget; budget.max_expansions = 100000;
        uint64_t shop_rng = seed ^ 0xa37108e62d045fb9ULL;
        std::array<uint8_t, 8> shops;
        for (auto& shop : shops) shop = compositions::random_word(shop_rng) % kag::N_SHOPS;
        while (prefix.st.step < 226) {
            std::copy_n(shops.begin(), prefix.st.n_shops, prefix.st.shops);
            kag::Action actions[2];
            source.act(kag::agent::runtime::make_observation(prefix, seat), budget, actions[seat]);
            rival.act(kag::agent::runtime::make_observation(prefix, seat ^ 1), budget, actions[seat ^ 1]);
            prefix.step(actions[0], actions[1]);
        }
        const auto start = kag::agent::runtime::make_observation(prefix, seat);
        std::array<Prediction, 3> predictions;
        Course demand(3), old_model(6); demand.reset(init); old_model.reset(init);
        kag::Action ignored;
        demand.act(start, budget, ignored); old_model.act(start, budget, ignored);
        for (int branch = 0; branch < 3; ++branch) predictions[branch] = predict(start, init.config, branch, library,detail,repair_sides,cash_source);
        int chosen = demand.branch(); double best = -1e100;
        for (int branch = 0; branch < 3; ++branch) {
            const auto& p = predictions[branch];
            if (p.complete && !p.own_failed && !p.rival_failed && p.margin > best) { best = p.margin; chosen = branch; }
        }
        for (int branch = 0; branch < 3; ++branch) {
            // Replay the common prefix for the rival's private policy memory.
            // Full-state simulator copies never enter the financial prediction.
            kag::Sim actual(config); Course fixed(branch); auto other = make_agent(opponent);
            fixed.reset(init); other.reset(kag::agent::runtime::make_agent_init(actual, seat ^ 1));
            int faults[2]{},worker_days=0;PurchaseRepairStats actual_repairs;
            const MarketRules rules{init.config.shed_capacity,init.config.max_orders,init.config.hire_mult,
                init.config.turns_per_day,init.config.shop_sell_interval,init.config.center_sell_interval};
            while (!actual.st.done) {
                std::copy_n(shops.begin(), actual.st.n_shops, actual.st.shops);
                kag::Action actions[2];
                const auto own_obs = kag::agent::runtime::make_observation(actual, seat);
                const auto rival_obs = kag::agent::runtime::make_observation(actual, seat ^ 1);
                if (actual.st.step == 226) require(actual.parity_hash() == prefix.parity_hash(), "fixed course prefix mismatch");
                fixed.act(own_obs, budget, actions[seat]); other.act(rival_obs, budget, actions[seat ^ 1]);
                if((repair_sides&1) && actual.st.step>=226) {
                    PlannerObservation obs;obs.turn=own_obs.step;
                    require(project_resources(own_obs,actions[seat],obs.own,obs.resources,rules.capacity,rules.turns_per_day),"unsupported repair projection");
                    std::copy_n(own_obs.market.inventory,kag::N_PRODUCTS,obs.inventory.begin());
                    Orders warm;warm.count=actions[seat].n_orders;std::copy_n(actions[seat].orders,warm.count,warm.values.begin());
                    const auto next=repair_purchases(obs,predictions[branch].calendar,warm,rules,actual_repairs);
                    actions[seat].n_orders=next.count;std::copy_n(next.values.begin(),next.count,actions[seat].orders);actions[seat].finalize();
                }
                if(actual.st.hour==23 || (actual.st.day==29 && actual.st.hour==22))worker_days+=actual.st.farms[seat].n_units;
                compositions::validate_action(actions[seat], own_obs); compositions::validate_action(actions[seat ^ 1], rival_obs);
                const auto diagnostics = actual.diagnose_joint_actions(actions[0], actions[1]);
                for (int p = 0; p < 2; ++p) faults[p] += diagnostics.players[p].requested_unit_actions - diagnostics.players[p].successful_unit_actions;
                actual.step(actions[0], actions[1]);
            }
            const auto& p = predictions[branch]; const auto& own = actual.st.farms[seat]; const auto& other_farm = actual.st.farms[seat ^ 1];
            if(detail)for(const auto& w:p.worlds)for(int side=0;side<2;++side) {
                const auto& z=w.trace;
                details<<"{\"seed\":"<<seed<<",\"seat\":"<<seat<<",\"opponent\":\""<<opponent<<"\",\"branch\":"<<branch
                    <<",\"source_episode\":"<<w.episode<<",\"source_seat\":"<<w.seat<<",\"side\":"<<side
                    <<",\"current_rival_cash\":"<<start.opponent().money<<",\"source_rival_cash\":"<<w.source_cash
                    <<",\"predicted_cash\":"<<w.cash<<",\"predicted_margin\":"<<w.margin
                    <<",\"missing\":"<<w.missing[side]<<",\"commitment_errors\":"<<w.commitments[side]
                    <<",\"first_missing_turn\":"<<z.first_missing_turn[side]<<",\"first_missing_item\":"<<z.first_missing_item[side]
                    <<",\"first_missing_phase\":"<<z.first_missing_phase[side]<<",\"cash_at_missing\":"<<z.cash_at_missing[side]
                    <<",\"first_commitment_turn\":"<<z.first_commitment_turn[side]
                    <<",\"wanted_hires\":"<<z.wanted_hires[side]<<",\"actual_hires\":"<<z.actual_hires[side]
                    <<",\"wanted_land\":"<<z.wanted_land[side]<<",\"actual_land\":"<<z.actual_land[side]
                    <<",\"cash_at_commitment\":"<<z.cash_at_commitment[side]
                    <<",\"first_short_order_turn\":"<<z.first_short_order_turn[side]<<",\"short_order_op\":"<<z.short_order_op[side]
                    <<",\"short_order_item\":"<<z.short_order_item[side]<<",\"requested\":"<<z.requested[side]<<",\"accepted\":"<<z.accepted[side]
                    <<",\"cash_at_short_order\":"<<z.cash_at_short_order[side]<<"}\n";
            }
            out << "{\"seed\":" << seed << ",\"seat\":" << seat << ",\"opponent\":\"" << opponent
                << "\",\"branch\":" << branch << ",\"chosen\":" << chosen << ",\"demand\":" << demand.branch()
                << ",\"old_model\":" << old_model.branch() << ",\"predicted_margin\":" << p.margin
                << ",\"predicted_cash\":" << p.own_cash << ",\"complete\":" << p.complete
                << ",\"own_forecast_failures\":" << p.own_failed << ",\"rival_forecast_failures\":" << p.rival_failed
                << ",\"compile_faults\":" << p.compile_faults << ",\"work\":" << p.work << ",\"seconds\":" << p.seconds
                << ",\"cash\":" << own.money << ",\"rival_cash\":" << other_farm.money
                << ",\"margin\":" << own.money - other_farm.money << ",\"own_faults\":" << faults[seat]
                << ",\"rival_faults\":" << faults[seat ^ 1]
                << ",\"repair_sides\":"<<repair_sides<<",\"repair_decisions\":"<<actual_repairs.decisions<<",\"repair_units\":"<<actual_repairs.units
                << ",\"historical_rival_cash\":" << (cash_source == RivalCashSource::historical_case)
                << ",\"forecast_own_repair_decisions\":"<<p.repairs[0].decisions<<",\"forecast_rival_repair_decisions\":"<<p.repairs[1].decisions
                << ",\"worker_days\":"<<worker_days<<",\"produced\":[";
            for(int item=0;item<kag::N_ITEMS;++item){if(item)out<<',';out<<own.produced[item];}
            out<<"]}\n";
        }
        out.flush();
    }
}
