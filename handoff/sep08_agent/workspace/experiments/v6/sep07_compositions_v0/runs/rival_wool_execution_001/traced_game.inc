template<class AgentA, class AgentB>
Outcome traced_game(AgentA& a, AgentB& b, uint64_t seed, int seat, const Options& o, std::ostream& trace) {
    kag::Config config; config.seed = seed;
    kag::Sim sim(config);
    a.reset(kag::agent::runtime::make_agent_init(sim, seat));
    b.reset(kag::agent::runtime::make_agent_init(sim, seat ^ 1));
    kag::agent::DecisionBudget budget; budget.max_expansions = o.expansions;
    Outcome result; result.seed = seed; result.seat = seat;
    if(o.profile) result.profile=std::make_unique<DetailedProfile>();
    uint64_t shop_rng = seed ^ 0xa37108e62d045fb9ULL;
    for (auto& shop : result.shops) shop = random_word(shop_rng) % kag::N_SHOPS;
    while (!sim.st.done) {
        // Sample shops independently of policy-dependent weed RNG consumption.
        // They are exposed only when the exact engine unlocks each shop.
        if (!o.native_shops) std::copy_n(result.shops.begin(), sim.st.n_shops, sim.st.shops);
        const auto oa = kag::agent::runtime::make_observation(sim, seat);
        const auto ob = kag::agent::runtime::make_observation(sim, seat ^ 1);
        kag::Action acts[2];
        a.act(oa, budget, acts[seat]); b.act(ob, budget, acts[seat ^ 1]);
        validate_action(acts[seat], oa); validate_action(acts[seat ^ 1], ob);
        for (int p = 0; p < 2; ++p) {
            hash_action(result.hash[p], acts[p]);
            result.max_workers[p] = std::max(result.max_workers[p], sim.st.farms[p].n_units);
            if (sim.st.hour == 22 || sim.st.hour == 23) {
                if (sim.st.hour == 23 || sim.st.day == 29) result.worker_days[p] += sim.st.farms[p].n_units;
            }
        }
        trace_step(sim,oa,acts,a,trace);
        if (o.validate) {
            const auto d = sim.diagnose_joint_actions(acts[0], acts[1]);
            for (int p = 0; p < 2; ++p) {
                result.actions[p] += d.players[p].requested_unit_actions;
                result.faults[p] += d.players[p].requested_unit_actions - d.players[p].successful_unit_actions;
            }
        }
        if(o.profile) {
            const auto before=sim;
            sim.step(acts[0],acts[1]);
            result.profile->observe(before,sim,acts);
        } else sim.step(acts[0], acts[1]);
    }
    result.turns = sim.st.step;
    if (result.turns != 719 || sim.st.n_shops != 8) std::abort();
    if (o.native_shops) std::copy_n(sim.st.shops, 8, result.shops.begin());
    for (int p = 0; p < 2; ++p) {
        const auto& f = sim.st.farms[p];
        result.cash[p] = f.money; result.revenue[p] = f.sell_revenue; result.spend[p] = f.total_spend;
        std::copy_n(f.produced, kag::N_ITEMS, result.produced[p]);
        std::copy_n(f.sold_units, kag::N_ITEMS, result.sold[p]);
        std::copy_n(f.discarded, kag::N_ITEMS, result.discarded[p]);
    }
    return result;
}
