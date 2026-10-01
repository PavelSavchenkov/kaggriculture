#include "agent.hpp"
#include "routes.hpp"
#include "placement.hpp"
#include <algorithm>
#include <cassert>
#include <climits>

namespace kag::agents::day_policy_contract {
int first_hire_wave(const DayPlan& plan, int order_limit) {
    if (plan.first_wave >= 0) return plan.first_wave;
    int inputs = plan.buy_land > 0;
    for (int c = 0; c < N_CROPS; ++c) inputs += plan.buy_seeds[c] > 0;
    for (int it = 0; it < N_ITEMS; ++it) inputs += plan.buy_items[it] > 0;
    return std::min({8, plan.hires, std::max(0, order_limit - inputs)});
}
int harvest_output(const Job& job, int cursor, Tile tile, int day, int product) {
    auto water=[&] {
        if(tile.kind!=T_PLANT || tile.watered_today)return;
        const auto& crop=CROPS[tile.what];const int age=day-tile.planted_day;
        if(!crop.ongoing && age>=(crop.max_yield_day+1)/2 && age<=crop.max_yield_day)
            tile.yield_units=std::min(crop.max_yield,tile.yield_units+(tile.fertilized_until_day>=day?2:1));
        tile.watered_today=true;
    };
    if(job.prior_service&2)tile.fertilized_until_day=day+2;
    if(job.prior_service&1)water();
    int result = 0;
    for (int k = cursor; k < job.count; ++k) {
        const auto a = job.steps[k];
        if (a.op == OP_FERTILIZE) tile.fertilized_until_day = day + 2;
        if (a.op == OP_WATER) water();
        if (a.op == OP_HARVEST && tile.kind == T_PLANT) {
            if (tile.what == product) result += tile.yield_units;
            if (!CROPS[tile.what].ongoing) tile = {}; else tile.yield_units = 0;
        }
        if (a.op == OP_HARVEST && tile.has_animal) {
            if (ANIMALS[tile.what - GOOSE].product == product) result += tile.yield_units;
            tile.yield_units = 0;
        }
        if (a.op == OP_COLLECT_FERTILIZER && product == FERTILIZER && tile.fertilizer_available) ++result;
        if (a.op == OP_DIG) tile = {};
        if (a.op == OP_PLANT) {
            tile = {}; tile.kind = T_PLANT; tile.what = a.arg; tile.planted_day = day;
            tile.yield_units = CROPS[a.arg].ongoing ? 0 : 1;
        }
    }
    return result;
}

int wheat_harvest(const Job& job, int cursor, Tile tile, int day) {
    return harvest_output(job, cursor, tile, day, WHEAT);
}

int choose_site(const Tile* tiles, const bool* reserved, int origin, bool animal, int structure) {
    int best = -1, score = INT_MAX;
    for (int c = 0; c < 100; ++c) {
        const auto& t = tiles[c];
        if (reserved[c]) continue;
        const bool reuse = animal && !t.has_animal && int(t.kind) == structure;
        if (t.kind != T_EMPTY && t.kind != T_WEED && !reuse) continue;
        const int cost = (t.kind == T_WEED ? 100 : 0) + (animal ? 100 * shed_distance(c) + (reuse ? 0 : 30)
                                : 100 * distance(origin, c) - shed_distance(c));
        if (cost < score) score = cost, best = c;
    }
    return best;
}

agent::AgentInfo Agent::info() { return {"day_policy_contract", 1}; }
void Agent::reset(const agent::AgentInit& init) {
    init_ = init;
    set_plan(DayPlan{});
}
void Agent::assert_returns([[maybe_unused]] const DayReturns& returns) {
    for (int h = 0; h < 24; ++h) for (int it = 0; it < N_PRODUCTS; ++it)
        assert(returns.target[h][it] >= (h ? returns.target[h-1][it] : 0));
}
void Agent::set_plan(const DayPlan& p) {
    assert(p.count >= 0 && p.count <= MAX_JOBS && p.hires >= 0 && p.hires < MAX_WORKERS);
    plan_ = p;
    returns_ = DayReturns{}; returns_supplied_ = false;
    fixed_orders_ = false;
    std::fill_n(returned_, N_PRODUCTS, 0);
    std::fill_n(cursor_, MAX_JOBS, 0);
    std::fill_n(owner_, MAX_JOBS, -1);
    std::fill_n(reserved_, 100, false);
    std::fill_n(workers_, MAX_WORKERS, Worker{});
    std::fill_n(region_, MAX_WORKERS, -1);
    std::fill_n(region_work_, 4, 0);
    std::fill_n(worker_done_, MAX_WORKERS, 0);
    std::fill_n(bought_seeds_, N_CROPS, 0);
    std::fill_n(bought_items_, N_ITEMS, 0);
    std::fill_n(pending_seeds_, N_CROPS, 0);
    std::fill_n(pending_items_, N_ITEMS, 0);
    std::fill_n(sold_, N_PRODUCTS, 0);
    std::fill_n(previous_sold_, N_PRODUCTS, 0);
    bought_land_ = 0;
    fresh_ = true;
    routes_ready_ = false;
    replanned_ = false;
    known_units_ = 0;
    progress_ = {};
    std::fill_n(progress_.placements, 100, -1);
    for (int j = 0; j < p.count; ++j) {
        assert(p.jobs[j].count > 0 && p.jobs[j].count <= MAX_STEPS);
        if (!p.jobs[j].depot) progress_.requested += p.jobs[j].count;
        const int cell = p.jobs[j].tile >= 0 ? p.jobs[j].tile : p.jobs[j].key;
        if (cell >= 0) region_work_[quadrant_of(cell % 10, cell / 10, 10)] += p.jobs[j].count + 1;
        if (p.jobs[j].tile >= 0 && !p.jobs[j].new_site) reserved_[p.jobs[j].tile] = true;
        if (p.relocate_new && p.jobs[j].new_site) plan_.jobs[j].tile = -1;
    }
}

void Agent::load_observation(const agent::AgentObservation& o) {
    const auto& f = o.self();
    std::copy_n(&f.tiles[0][0], 100, &local_.tiles[0][0]);
    local_.money = f.money;
    local_.n_units = f.n_units;
    local_.hires_today = f.hires_today;
    local_.n_quadrants = f.n_quadrants;
    std::copy_n(f.pos_x, f.n_units, local_.pos_x);
    std::copy_n(f.pos_y, f.n_units, local_.pos_y);
    std::copy_n(o.own.shed, N_ITEMS, local_.shed);
    std::copy_n(o.own.seeds, N_CROPS, local_.seeds);
    local_.shed_total = o.own.shed_total;
    for (int u = 0; u < f.n_units; ++u) {
        std::copy_n(o.own.inv[u], N_ITEMS, local_.inv[u]);
        local_.inv_nkeys[u] = o.own.inv_nkeys[u];
        std::copy_n(o.own.inv_keys[u], o.own.inv_nkeys[u], local_.inv_keys[u]);
    }
    for (int c = 0; c < N_CROPS; ++c) {
        bought_seeds_[c] += std::clamp(int(local_.seeds[c]) - previous_seeds_[c], 0, pending_seeds_[c]);
        pending_seeds_[c] = 0;
    }
    for (int it = 0; it < N_ITEMS; ++it) {
        int total = local_.shed[it];
        for (int u = 0; u < f.n_units; ++u) total += local_.inv[u][it];
        bought_items_[it] += std::clamp(total - previous_items_[it] + (it < N_PRODUCTS ? previous_sold_[it] : 0), 0, pending_items_[it]);
        pending_items_[it] = 0;
    }
    if (!fresh_) bought_land_ += std::max(0, f.n_quadrants - previous_land_);
    fresh_ = false;
    previous_land_ = f.n_quadrants;
    hour_ = o.hour;
    end_hour_ = std::min(23, init_.config.episode_steps - 2 - o.day * 24);
    remaining_feed_ = remaining_fertilize_ = 0;
    for (int j = 0; j < plan_.count; ++j)
        for (int k = cursor_[j]; k < plan_.jobs[j].count; ++k) {
            remaining_feed_ += plan_.jobs[j].steps[k].op == OP_FEED;
            remaining_fertilize_ += plan_.jobs[j].steps[k].op == OP_FERTILIZE;
        }
}

void Agent::assign_sites() {
    if (!plan_.relocate_new) return;
    for (int j = 0; j < plan_.count; ++j) {
        auto& job = plan_.jobs[j];
        if (job.tile >= 0 || !job.new_site || cursor_[j] == job.count) continue;
        const int op = job.steps[0].op;
        if (op != OP_BUILD_COOP && op != OP_BUILD_PASTURE) continue;
        const int structure = op == OP_BUILD_COOP ? T_COOP : T_PASTURE;
        // BUILD requires empty land; structure reuse is selected by plans with PLACE.
        job.tile = choose_site(&local_.tiles[0][0], reserved_, 44, true, -1);
        if (job.tile >= 0) reserved_[job.tile] = true;
        (void)structure;
    }
}

void Agent::bind_new_workers() {
    const int first = known_units_, end = local_.n_units;
    int selected[MAX_WORKERS], destination[MAX_WORKERS];
    bool worker_used[MAX_WORKERS]{}, route_used[MAX_WORKERS]{};
    if(options_.match_routes) {
        int cost[MAX_WORKERS][MAX_WORKERS]{};
        const int n=end-first;
        for(int u=0;u<n;++u)for(int r=0;r<n;++r) {
            const int worker=first+u,route=first+r;
            const int at=local_.pos_y[worker]*10+local_.pos_x[worker];
            const int target=routes_.count[route]?plan_.jobs[routes_.jobs[route][0]].tile:at;
            const int length=routes_.predicted_length[route]+distance(at,target)-distance(routes_.start[route],target);
            const int over=std::max(0,length-(24-hour_));
            cost[u][r]=10000*over*over+10*length*length+(worker!=route);
        }
        int u[MAX_WORKERS+1]{},v[MAX_WORKERS+1]{},p[MAX_WORKERS+1]{},way[MAX_WORKERS+1]{};
        for(int row=1;row<=n;++row) {
            p[0]=row;int col=0,minimum[MAX_WORKERS+1];bool seen[MAX_WORKERS+1]{};
            std::fill_n(minimum,n+1,INT_MAX);
            do {
                seen[col]=true;int delta=INT_MAX,next=0;
                for(int c=1;c<=n;++c)if(!seen[c]) {
                    const int value=cost[p[col]-1][c-1]-u[p[col]]-v[c];
                    if(value<minimum[c])minimum[c]=value,way[c]=col;
                    if(minimum[c]<delta)delta=minimum[c],next=c;
                }
                for(int c=0;c<=n;++c)if(seen[c])u[p[c]]+=delta,v[c]-=delta;else minimum[c]-=delta;
                col=next;
            }while(p[col]);
            do {const int next=way[col];p[col]=p[next];col=next;}while(col);
        }
        for(int c=1;c<=n;++c) {
            const int worker=first+p[c]-1,route=first+c-1;
            selected[worker]=route;destination[route]=worker;
        }
    } else {
    for (int count = first; count < end; ++count) {
        int worker = -1, route = -1, best = INT_MAX;
        for (int u = first; u < end; ++u) if (!worker_used[u])
            for (int r = first; r < end; ++r) if (!route_used[r]) {
                const int pos = local_.pos_y[u] * 10 + local_.pos_x[u];
                const int cost = 100 * distance(pos, routes_.start[r]) + (u != r);
                if (cost < best) best = cost, worker = u, route = r;
            }
        selected[worker] = route; destination[route] = worker;
        worker_used[worker] = route_used[route] = true;
    }
    }
    Routes previous;
    for (int u = first; u < end; ++u) {
        previous.start[u] = routes_.start[u]; previous.count[u] = routes_.count[u];
        previous.predicted_length[u] = routes_.predicted_length[u];
        std::copy_n(routes_.jobs[u], routes_.count[u], previous.jobs[u]);
    }
    for (int u = first; u < end; ++u) {
        const int r = selected[u];
        routes_.start[u] = previous.start[r]; routes_.count[u] = previous.count[r];
        routes_.predicted_length[u] = previous.predicted_length[r];
        std::copy_n(previous.jobs[r], previous.count[r], routes_.jobs[u]);
    }
    for (int j = 0; j < plan_.count; ++j)
        if (owner_[j] >= first && owner_[j] < end) owner_[j] = destination[owner_[j]];
}

bool Agent::ready(int j) const {
    const auto& job = plan_.jobs[j];
    if (job.tile < 0 || cursor_[j] == job.count) return false;
    if (job.predecessor >= 0 && cursor_[job.predecessor] < plan_.jobs[job.predecessor].count)
        return options_.preposition && job.new_site;
    if(job.service_predecessor>=0 && cursor_[job.service_predecessor]<plan_.jobs[job.service_predecessor].count) return options_.preposition;
    if (job.depot) return true;
    const auto& t = local_.tiles[job.tile / 10][job.tile % 10];
    const auto a = job.steps[cursor_[j]];
    // Movement onto locked land is legal. Fixed purchases allow prepositioning;
    // the field operation still waits for the actual unlock below.
    if (t.kind == T_LOCKED) return fixed_orders_ && job.new_site && plan_.buy_land;
    if (a.op == OP_PLANT || a.op == OP_BUILD_COOP || a.op == OP_BUILD_PASTURE) return t.kind == T_EMPTY || (job.new_site && t.kind == T_WEED);
    if (a.op == OP_FEED || a.op == OP_CARE) return t.has_animal;
    if (a.op == OP_COLLECT_FERTILIZER) return t.has_animal && t.fertilizer_available;
    if (a.op == OP_WATER || a.op == OP_FERTILIZE) return t.kind == T_PLANT;
    if (a.op == OP_HARVEST) return t.yield_units > 0 && (t.has_animal || (t.kind == T_PLANT && plan_.day - t.planted_day >= CROPS[t.what].first_yield_day));
    return true;
}

int Agent::work_left(int j) const {
    const auto& job = plan_.jobs[j];
    if (cursor_[j] == job.count) return 0;
    if (job.depot) return job.count - cursor_[j];
    return job.count - cursor_[j] + (job.new_site && cursor_[j] == 0 && job.steps[0].op != OP_DIG && job.tile >= 0 && local_.tiles[job.tile / 10][job.tile % 10].kind == T_WEED);
}

int Agent::supply_cost(int u, const Job& job) const {
    int need[N_ITEMS]{};
    const int j = int(&job - plan_.jobs);
    if (job.depot) {
        for (int it = 0; it < N_ITEMS; ++it) if (local_.inv[u][it] > 0) return 0;
        return 100000;
    }
    for (int k = cursor_[j]; k < job.count; ++k) {
        auto a = job.steps[k];
        if (a.op == OP_FEED) ++need[WHEAT];
        if (a.op == OP_FERTILIZE) ++need[FERTILIZER];
        if (a.op == OP_PLACE && is_animal(a.arg)) ++need[a.arg];
        if (a.op == OP_COLLECT_FERTILIZER) --need[FERTILIZER];
        if (a.op == OP_PLANT && local_.seeds[a.arg] <= 0 &&
            !(fixed_orders_ && plan_.buy_seeds[a.arg] > bought_seeds_[a.arg])) return 100000;
        if (a.op == OP_HARVEST) break; // Receipts may fund the following replacement.
    }
    int pickups = 0;
    for (int it = 0; it < N_ITEMS; ++it) if (need[it] > local_.inv[u][it]) {
        const int future=fixed_orders_?std::max(0,plan_.buy_items[it]-bought_items_[it]):0;
        if (local_.shed[it]+future < need[it] - local_.inv[u][it]) return 100000;
        ++pickups;
    }
    if (!pickups) return 0;
    const int pos = local_.pos_y[u] * 10 + local_.pos_x[u];
    const int depot = shed_cell(pos);
    return distance(pos, depot) + pickups + distance(depot, job.tile) - distance(pos, job.tile);
}

Agent::Choice Agent::choose(int u) {
    Choice best;
    const int pos = local_.pos_y[u] * 10 + local_.pos_x[u];
    for (int j = 0; j < plan_.count; ++j) {
        auto& job = plan_.jobs[j];
        if (owner_[j] >= 0 || cursor_[j] == job.count || job.key == AUXILIARY_SOURCE) continue;
        int site = job.tile;
        if (site < 0 && job.new_site && job.steps[cursor_[j]].op == OP_PLANT)
            site = choose_site(&local_.tiles[0][0], reserved_, pos, false);
        if (site < 0) continue;
        const int save = job.tile;
        job.tile = site;
        const auto a = job.steps[cursor_[j]];
        const auto& tile = local_.tiles[site / 10][site % 10];
        if (tile.kind == T_LOCKED) { job.tile = save; continue; }
        // Independent animal passes must wait for establishment, and cannot
        // feed/care an animal that escaped on an earlier failed rollout day.
        if ((a.op == OP_FEED || a.op == OP_CARE || a.op == OP_COLLECT_FERTILIZER) && !tile.has_animal) {
            job.tile = save; continue;
        }
        if (a.op == OP_COLLECT_FERTILIZER && !tile.fertilizer_available) { job.tile = save; continue; }
        if ((a.op == OP_WATER || a.op == OP_FERTILIZE) && tile.kind != T_PLANT) { job.tile = save; continue; }
        if (a.op == OP_HARVEST && tile.yield_units <= 0) { job.tile = save; continue; }
        if (a.op == OP_PLANT && tile.kind != T_EMPTY && !(job.new_site && tile.kind == T_WEED)) { job.tile = save; continue; }
        int supply = supply_cost(u, job), source = -1;
        // A crop may acquire fertilizer through one standalone collection job.
        bool needs_f = false;
        for (int k = cursor_[j]; k < job.count; ++k) needs_f |= job.steps[k].op == OP_FERTILIZE;
        if (needs_f && local_.inv[u][FERTILIZER] == 0) {
            for (int s = 0; s < plan_.count; ++s) {
                const auto& src = plan_.jobs[s];
                if (s == j || owner_[s] >= 0 || cursor_[s] == src.count || src.tile < 0) continue;
                if (src.count - cursor_[s] != 1 || src.steps[cursor_[s]].op != OP_COLLECT_FERTILIZER) continue;
                const auto& t = local_.tiles[src.tile / 10][src.tile % 10];
                if (!t.has_animal || !t.fertilizer_available) continue;
                const int extra = distance(pos, src.tile) + 1 + distance(src.tile, site) - distance(pos, site);
                if (extra < supply) supply = extra, source = s;
            }
        }
        int duration = distance(pos, site) + supply + work_left(j);
        if (duration > end_hour_ - hour_ + 1) { job.tile = save; continue; }
        int cost = 100 * (distance(pos, site) + supply) + options_.work_bias * (job.count - cursor_[j]);
        if (region_[u] >= 0 && quadrant_of(site % 10, site / 10, 10) != region_[u]) cost += 100 * options_.region_penalty;
        if (options_.outward) cost -= shed_distance(site);
        if (options_.far_start && worker_done_[u] == 0 && u % 4 != 0)
            cost -= 100 * options_.far_start * shed_distance(site);
        if (a.op == OP_FEED) cost -= 100 * options_.animal_bias;
        if (a.op == OP_FERTILIZE && local_.inv[u][FERTILIZER] > 0) cost -= 100 * options_.fertilizer_bias;
        if (a.op == OP_COLLECT_FERTILIZER && remaining_fertilize_ > 0) cost -= 40;
        if (job.deadline < end_hour_) cost -= 120 * std::max(0, hour_ + duration - int(job.deadline) + 2);
        if (cost < best.cost) best = {j, source, site, cost};
        job.tile = save;
    }
    return best;
}

Agent::Choice Agent::repair(int u) {
    Choice best;
    const int pos = local_.pos_y[u] * 10 + local_.pos_x[u];
    for (int j = 0; j < plan_.count; ++j) {
        auto& job = plan_.jobs[j];
        if (!ready(j) || job.depot || (job.key == AUXILIARY_SOURCE && owner_[j] < 0)) continue;
        int v = owner_[j];
        if (v == u) continue;
        int extra = 0, transfer_end = -1;
        if (job.carrier_bound) {
            if (!(options_.route_variant & 128) || job.predecessor >= 0 || cursor_[j] || v < 0) continue;
            bool started = false, valid = true;
            int at = job.tile;
            for (int k = 0; k < routes_.count[v]; ++k) {
                const int next = routes_.jobs[v][k];
                if (next == j) { started = true; continue; }
                if (!started) continue;
                const auto& visit = plan_.jobs[next];
                if (cursor_[next] || owner_[next] != v) { valid = false; break; }
                extra += distance(at, visit.tile) + visit.count; at = visit.tile;
                if (visit.depot) { transfer_end = next; break; }
            }
            if (!valid || transfer_end < 0) continue;
        }
        const int supply = supply_cost(u, job);
        const int finish = distance(pos, job.tile) + supply + work_left(j) + extra;
        if (finish > end_hour_ - hour_ + 1) continue;
        const auto& t = local_.tiles[job.tile / 10][job.tile % 10];
        if (t.kind == T_LOCKED) continue;
        int other_finish = 1000;
        if (v >= 0 && v < local_.n_units) {
            other_finish = v < u ? 1 : 0;
            int other_pos = local_.pos_y[v] * 10 + local_.pos_x[v];
            if (workers_[v].job == j) other_finish += distance(other_pos, job.tile) + supply_cost(v, job) + job.count - cursor_[j];
            else {
                const int active = workers_[v].job;
                if (active >= 0 && cursor_[active] < plan_.jobs[active].count) {
                    const auto& current = plan_.jobs[active];
                    other_finish += distance(other_pos, current.tile) + supply_cost(v, current) + current.count - cursor_[active];
                    other_pos = current.tile;
                }
                for (int k = 0; k < routes_.count[v]; ++k) {
                    int next = routes_.jobs[v][k];
                    const auto& visit = plan_.jobs[next];
                    if (next == active || cursor_[next] == visit.count || visit.tile < 0 || owner_[next] != v) continue;
                    other_finish += distance(other_pos, visit.tile) + visit.count - cursor_[next];
                    other_pos = visit.tile;
                    if (next == j) break;
                }
            }
        }
        if (v >= 0 && v < local_.n_units && workers_[v].job < 0 && supply_cost(v, job) >= 100000) other_finish += 1000;
        other_finish += extra;
        const bool take_unstarted = (options_.route_variant & 1024) && v >= 0 && workers_[v].job != j;
        if (!take_unstarted && finish >= other_finish) continue;
        int cost = finish * 100 - (other_finish > end_hour_ - hour_ + 1 ? 1000 : 0);
        if (cost < best.cost) best = {j, -1, job.tile, cost, transfer_end};
    }
    if (best.job >= 0) {
        const int v = owner_[best.job];
        if (v >= 0 && workers_[v].job == best.job) workers_[v].job = -1;
        if (v >= 0 && workers_[v].next == best.job) workers_[v].next = -1;
        owner_[best.job] = u;
        if (best.transfer_end >= 0) {
            bool started = false;
            for (int k = 0; k < routes_.count[v]; ++k) {
                const int j = routes_.jobs[v][k];
                if (j == best.job) started = true;
                if (!started) continue;
                owner_[j] = u;
                assert(routes_.count[u] < MAX_JOBS);
                routes_.jobs[u][routes_.count[u]++] = j;
                if (workers_[v].job == j) workers_[v].job = -1;
                if (workers_[v].next == j) workers_[v].next = -1;
                if (j == best.transfer_end) break;
            }
        }
    }
    return best;
}

Agent::Choice Agent::relocate_crop(int u) {
    Choice best;
    const int pos=local_.pos_y[u]*10+local_.pos_x[u];
    bool blocked[100]{};int assigned[100];std::fill_n(assigned,100,-1);
    for(int j=0;j<plan_.count;++j) {
        const auto& job=plan_.jobs[j];if(job.depot || job.tile<0)continue;
        if(job.new_site)assigned[job.tile]=j;
        else if(cursor_[j]<job.count)blocked[job.tile]=true;
    }
    for(int j=0;j<plan_.count;++j) {
        const auto& job=plan_.jobs[j];
        if(!job.new_site || cursor_[j] || job.carrier_bound || job.steps[0].op!=OP_PLANT)continue;
        if(supply_cost(u,job)>=100000)continue;
        const int v=owner_[j];
        int other=1000;
        if(v>=0 && v<local_.n_units && v!=u && workers_[v].job==j)
            other=distance(local_.pos_y[v]*10+local_.pos_x[v],job.tile)+job.count;
        for(int c=0;c<100;++c) {
            if(blocked[c] || (assigned[c]>=0 && assigned[c]!=j) || local_.tiles[c/10][c%10].kind!=T_EMPTY)continue;
            const int finish=distance(pos,c)+job.count;
            if(finish>end_hour_-hour_+1 || finish>=other)continue;
            const int cost=100*finish+distance(c,job.tile);
            if(cost<best.cost)best={j,-1,c,cost};
        }
    }
    if(best.job>=0) {
        auto& job=plan_.jobs[best.job];const int v=owner_[best.job];
        if(v>=0 && workers_[v].job==best.job)workers_[v].job=-1;
        if(v>=0 && workers_[v].next==best.job)workers_[v].next=-1;
        job.predecessor=-1;job.tile=best.site;owner_[best.job]=u;
        if(std::find(routes_.jobs[u],routes_.jobs[u]+routes_.count[u],best.job)==routes_.jobs[u]+routes_.count[u]) {
            assert(routes_.count[u]<MAX_JOBS);routes_.jobs[u][routes_.count[u]++]=best.job;
        }
    }
    return best;
}

UnitAction Agent::next_action(int u) {
    auto& w = workers_[u];
    const int pos = local_.pos_y[u] * 10 + local_.pos_x[u];
    if(returns_supplied_ && (options_.route_variant & URGENT_RETURNS)) {
        int quota[N_PRODUCTS]{};
        for(int k=0;k<routes_.count[u];++k) {
            const int j=routes_.jobs[u][k];const auto& job=plan_.jobs[j];
            if(!job.depot || owner_[j]!=u)continue;
            for(int s=cursor_[j];s<job.count;++s)quota[job.steps[s].arg]+=job.steps[s].n;
        }
        int product=-1,due=24;
        for(int k=0;k<local_.inv_nkeys[u];++k) {
            const int it=local_.inv_keys[u][k];if(it>=N_PRODUCTS || quota[it]<=0)continue;
            for(int h=0;h<due;++h)if(returns_.target[h][it]>returned_[it]){product=it;due=h;break;}
        }
        if(product>=0 && hour_+shed_distance(pos)>=due) {
            const int target=shed_cell(pos);
            if(pos%10!=target%10)return {uint8_t(pos%10<target%10?OP_EAST:OP_WEST),0,1};
            if(pos/10!=target/10)return {uint8_t(pos/10<target/10?OP_SOUTH:OP_NORTH),0,1};
            bool all=local_.inv_nkeys[u]>1;
            for(int k=0;k<local_.inv_nkeys[u];++k) {
                const int it=local_.inv_keys[u][k];
                all &= it<N_PRODUCTS && local_.inv[u][it]<=std::min(quota[it],returns_.target[23][it]-returned_[it]);
            }
            if(all)return {OP_DROP,0,1};
            return {OP_PLACE,uint8_t(product),std::min({quota[product],int(local_.inv[u][product]),returns_.target[due][product]-returned_[product]})};
        }
    }
    if (w.job >= 0 && cursor_[w.job] == plan_.jobs[w.job].count) {
        owner_[w.job] = -1;
        w.job = w.next;
        w.next = -1;
    }
    if (routes_ready_ && shed_distance(pos) == 0 && hour_ < end_hour_) {
        int consumed[N_ITEMS]{}, required[N_ITEMS]{};
        for (int n = 0; n < routes_.count[u]; ++n) {
            const int j = routes_.jobs[u][n];
            if (owner_[j] >= 0 && owner_[j] != u) continue;
            const auto& visit = plan_.jobs[j];
            if (visit.depot && cursor_[j] < visit.count) break;
            int harvest = visit.tile >= 0 ? wheat_harvest(visit, cursor_[j], local_.tiles[visit.tile / 10][visit.tile % 10], plan_.day) : 0;
            for (int k = cursor_[j]; k < plan_.jobs[j].count; ++k) {
                const auto a = plan_.jobs[j].steps[k];
                if (a.op == OP_HARVEST) consumed[WHEAT] -= harvest, harvest = 0;
                if (a.op == OP_FEED) ++consumed[WHEAT];
                if (a.op == OP_FERTILIZE) ++consumed[FERTILIZER];
                if (a.op == OP_COLLECT_FERTILIZER) --consumed[FERTILIZER];
                if (a.op == OP_PLACE && is_animal(a.arg)) ++consumed[a.arg];
                for (int it : {int(WHEAT), int(FERTILIZER), int(GOOSE), int(COW), int(SHEEP)})
                    required[it] = std::max(required[it], consumed[it]);
            }
        }
        for (int it : {int(WHEAT), int(FERTILIZER), int(GOOSE), int(COW), int(SHEEP)}) {
            int n = std::min(required[it] - int(local_.inv[u][it]), int(local_.shed[it]));
            if (n > 0) return {OP_PICKUP, uint8_t(it), n};
        }
    }
    // Delivery needs are cumulative desired sales less shed stock. Reserve
    // expected arrivals from earlier assigned workers to avoid mass returns.
    int deliver = -1, urgency = -1;
    for (int it = 0; !options_.route_delivery && it < N_PRODUCTS; ++it) if (local_.inv[u][it] > 0) {
        int stock = local_.shed[it];
        for (int v = 0; v < local_.n_units; ++v)
            if (v != u && workers_[v].returning >= 0) stock += local_.inv[v][it];
        int needed_hour = 24;
        for (int h = hour_; h <= end_hour_; ++h)
            if (plan_.sell_target[h][it] > sold_[it] + stock) { needed_hour = h; break; }
        const int slack = needed_hour - hour_ - shed_distance(pos) - 1;
        if (needed_hour < 24 && slack <= options_.delivery_lead && (deliver < 0 || slack < urgency))
            deliver = it, urgency = slack;
    }
    if (w.returning >= 0 || (deliver >= 0 && (w.job < 0 || cursor_[w.job] == 0))) {
        if (w.returning < 0) w.returning = shed_cell(pos);
        if (pos != w.returning) {
            const int target = w.returning;
            if (pos % 10 != target % 10) return {uint8_t(pos % 10 < target % 10 ? OP_EAST : OP_WEST), 0, 1};
            return {uint8_t(pos / 10 < target / 10 ? OP_SOUTH : OP_NORTH), 0, 1};
        }
        w.returning = -1;
        if (deliver >= 0 && local_.shed_total < init_.config.shed_capacity)
            return {OP_PLACE, uint8_t(deliver), local_.inv[u][deliver]};
    }
    if (w.job < 0) {
        Choice choice;
        if (routes_ready_) {
            for (int n = 0; n < routes_.count[u]; ++n) {
                const int j = routes_.jobs[u][n];
                auto& job = plan_.jobs[j];
                if (cursor_[j] == job.count || job.tile < 0 || (owner_[j] >= 0 && owner_[j] != u)) continue;
                if (!ready(j) || supply_cost(u, job) >= 100000) continue;
                if (distance(pos, job.tile) + supply_cost(u, job) + work_left(j) > end_hour_ - hour_ + 1) continue;
                choice.job = j; choice.site = job.tile; break;
            }
            if (choice.job < 0 && options_.route_repair) choice = repair(u);
            if (choice.job < 0 && options_.relocate_idle && hour_>=12) choice=relocate_crop(u);
        } else choice = choose(u);
        if (choice.job < 0) return {};
        auto& job = plan_.jobs[choice.job];
        job.tile = choice.site;
        reserved_[choice.site] = true;
        owner_[choice.job] = u;
        w.job = choice.job;
        if (choice.source >= 0) {
            owner_[choice.source] = u;
            w.next = w.job;
            w.job = choice.source;
        }
    }
    auto& job = plan_.jobs[w.job];
    auto a = job.steps[cursor_[w.job]];
    if (a.op == OP_PLANT && hour_ == end_hour_ && job.count - cursor_[w.job] > 1) return {};
    int need[N_ITEMS]{};
    for (int k = cursor_[w.job]; k < job.count; ++k) {
        const auto b = job.steps[k];
        if (b.op == OP_FEED) ++need[WHEAT];
        if (b.op == OP_FERTILIZE) ++need[FERTILIZER];
        if (b.op == OP_PLACE && is_animal(b.arg)) ++need[b.arg];
        if (b.op == OP_COLLECT_FERTILIZER) --need[FERTILIZER];
        if (b.op == OP_HARVEST) break;
    }
    int pickup = -1;
    for (int it = 0; it < N_ITEMS; ++it)
        if (need[it] > local_.inv[u][it]) { pickup = it; break; }
    int target = job.tile;
    if (pickup >= 0) {
        if (hour_ == end_hour_) return {};
        target = shed_cell(pos);
        if (target == pos) {
            int n = need[pickup] - local_.inv[u][pickup];
            if (pickup == WHEAT && !routes_ready_) {
                int carried = 0;
                for (int v = 0; v < local_.n_units; ++v) carried += local_.inv[v][WHEAT];
                n = std::max(n, std::min(options_.batch, remaining_feed_ - carried));
            }
            n = std::min(n, int(local_.shed[pickup]));
            if (n > 0) return {OP_PICKUP, uint8_t(pickup), n};
            owner_[w.job] = -1;
            if (w.next >= 0) owner_[w.next] = -1;
            w = {};
            return {};
        }
    }
    if (pos != target) {
        if (pos % 10 != target % 10) return {uint8_t(pos % 10 < target % 10 ? OP_EAST : OP_WEST), 0, 1};
        return {uint8_t(pos / 10 < target / 10 ? OP_SOUTH : OP_NORTH), 0, 1};
    }
    if ((job.service_predecessor>=0 && cursor_[job.service_predecessor]<plan_.jobs[job.service_predecessor].count) ||
        (job.predecessor >= 0 && cursor_[job.predecessor] < plan_.jobs[job.predecessor].count) ||
        (!job.depot && local_.tiles[pos / 10][pos % 10].kind == T_LOCKED) ||
        (a.op == OP_PLANT && local_.seeds[a.arg] == 0)) return {};
    if (job.new_site && cursor_[w.job] == 0 && local_.tiles[pos / 10][pos % 10].kind == T_WEED) return {OP_DIG, 0, 1};
    if (job.depot) {
        if (returns_supplied_) {
            int pending[N_PRODUCTS]{};
            for (int k = cursor_[w.job]; k < job.count; ++k)
                pending[job.steps[k].arg] += job.steps[k].n;
            bool can_drop = local_.inv_nkeys[u] > 1;
            for (int k = 0; k < local_.inv_nkeys[u]; ++k) {
                const int it = local_.inv_keys[u][k];
                can_drop &= it < N_PRODUCTS && local_.inv[u][it] <=
                    std::min(pending[it], returns_.target[23][it] - returned_[it]);
            }
            if (can_drop) return {OP_DROP, 0, 1};
            if(options_.deadline_deposits) {
                int selected=-1,due=24,quantity=0;
                for(int k=cursor_[w.job];k<job.count;++k) {
                    const auto request=job.steps[k];const int product=request.arg;
                    const int n=std::min({request.n,returns_.target[23][product]-returned_[product],int(local_.inv[u][product])});
                    if(n<=0)continue;
                    int deadline=0;
                    while(deadline<24 && returns_.target[deadline][product]<=returned_[product])++deadline;
                    if(deadline<due){selected=k;due=deadline;quantity=n;}
                }
                if(selected>=0)return {OP_PLACE,job.steps[selected].arg,quantity};
            }
            for (int k = cursor_[w.job]; k < job.count; ++k) {
                const auto request = job.steps[k];
                const int product = request.arg;
                const int needed = std::min(request.n, returns_.target[23][product] - returned_[product]);
                if (needed <= 0) { ++cursor_[w.job]; continue; }
                const int n = std::min(needed, int(local_.inv[u][product]));
                if (n > 0) return {OP_PLACE, uint8_t(product), n};
                return {};
            }
            return {};
        }
        int cargo = 0;
        for (int it = 0; it < N_ITEMS; ++it) cargo += local_.inv[u][it];
        const int room = init_.config.shed_capacity - local_.shed_total;
        if (cargo <= room) return {OP_DROP, 0, 1};
        if (room <= 0) return {};
        int product = -1;
        for (int it = 0; it < N_ITEMS; ++it) if (local_.inv[u][it]) {
            if (product < 0) product = it;
            if (it < N_PRODUCTS && plan_.sell_target[hour_][it] > sold_[it] + local_.shed[it]) { product = it; break; }
        }
        return {OP_PLACE, uint8_t(product), std::min(room, int(local_.inv[u][product]))};
    }
    return a;
}

void Agent::record_deposit(int u,int* quantities) {
    for(int it=0;it<N_PRODUCTS;++it)returned_[it]+=quantities[it];
    auto take=[&](int j) {
        if(j<0 || !plan_.jobs[j].depot || owner_[j]!=u)return;
        auto& job=plan_.jobs[j];
        for(int k=cursor_[j];k<job.count;++k) {
            auto& request=job.steps[k];const int n=std::min(request.n,quantities[request.arg]);
            request.n-=n;quantities[request.arg]-=n;
        }
        while(cursor_[j]<job.count && job.steps[cursor_[j]].n<=0)++cursor_[j];
    };
    take(workers_[u].job);
    for(int k=0;k<routes_.count[u];++k)take(routes_.jobs[u][k]);
    for(int it=0;it<N_PRODUCTS;++it)assert(quantities[it]==0);
}

bool Agent::apply(int u, UnitAction a) {
    int x = local_.pos_x[u], y = local_.pos_y[u];
    auto& t = local_.tiles[y][x];
    auto* inv = local_.inv[u];
    const int day = plan_.day;
    if (a.op == OP_PASS) return true;
    if (a.op >= OP_NORTH && a.op <= OP_WEST) {
        const int nx = x + (a.op == OP_EAST) - (a.op == OP_WEST);
        const int ny = y + (a.op == OP_SOUTH) - (a.op == OP_NORTH);
        if (nx < 0 || nx >= 10 || ny < 0 || ny >= 10) return false;
        local_.pos_x[u] = nx; local_.pos_y[u] = ny;
        return true;
    }
    if (a.op == OP_PICKUP) {
        if (shed_distance(y * 10 + x) || a.arg >= N_ITEMS) return false;
        const int n = std::min(a.n, int(local_.shed[a.arg]));
        if (n <= 0) return false;
        local_.shed[a.arg] -= n; local_.shed_total -= n; local_.inv_add(u, a.arg, n);
        return true;
    }
    if (a.op == OP_DROP) {
        if (shed_distance(y * 10 + x) || local_.inv_nkeys[u] == 0) return false;
        while (local_.inv_nkeys[u]) {
            const int it = local_.inv_keys[u][0], amount = inv[it];
            const int fit = std::min(amount, init_.config.shed_capacity - local_.shed_total);
            local_.shed[it] += fit; local_.shed_total += fit;
            local_.inv_take(u, it, amount);
        }
        return true;
    }
    if (a.op == OP_PLACE) {
        if (a.arg >= N_ITEMS || inv[a.arg] <= 0) return false;
        if (is_animal(a.arg)) {
            const int kind = ANIMALS[a.arg - GOOSE].structure == ST_COOP ? T_COOP : T_PASTURE;
            if (t.kind == kind && !t.has_animal) {
                t = {}; t.kind = TileKind(kind); t.has_animal = true; t.what = a.arg; t.planted_day = day;
                local_.inv_take(u, a.arg, 1); return true;
            }
        }
        if (shed_distance(y * 10 + x)) return false;
        const int n = std::min({a.n, int(inv[a.arg]), init_.config.shed_capacity - local_.shed_total});
        if (n <= 0) return false;
        local_.inv_take(u, a.arg, n); local_.shed[a.arg] += n; local_.shed_total += n;
        return true;
    }
    if (t.kind == T_LOCKED) return false;
    switch (a.op) {
    case OP_PLANT:
        if (a.arg >= N_CROPS || t.kind != T_EMPTY || local_.seeds[a.arg] <= 0) return false;
        --local_.seeds[a.arg]; t = {}; t.kind = T_PLANT; t.what = a.arg; t.planted_day = day;
        t.consecutive_dry = 1; t.yield_units = CROPS[a.arg].ongoing ? 0 : 1;
        t.max_lifespan_step = CROPS[a.arg].ongoing ? -1 : (day + CROPS[a.arg].max_yield_day + 1) * 24;
        return true;
    case OP_WATER:
        if (t.kind != T_PLANT || t.watered_today) return false;
        t.watered_today = true;
        if (!CROPS[t.what].ongoing && day - t.planted_day >= (CROPS[t.what].max_yield_day + 1) / 2 && day - t.planted_day <= CROPS[t.what].max_yield_day)
            t.yield_units = std::min(CROPS[t.what].max_yield, t.yield_units + (t.fertilized_until_day >= day ? 2 : 1));
        return true;
    case OP_HARVEST:
        if (t.yield_units <= 0 || (t.kind == T_PLANT && day - t.planted_day < CROPS[t.what].first_yield_day)) return false;
        if (t.kind == T_PLANT) {
            local_.inv_add(u, t.what, t.yield_units); t.yield_units = 0;
            if (!CROPS[t.what].ongoing) t = {};
        } else if (t.has_animal) {
            local_.inv_add(u, ANIMALS[t.what - GOOSE].product, t.yield_units); t.yield_units = 0;
        } else return false;
        return true;
    case OP_FERTILIZE:
        if (t.kind != T_PLANT || !local_.inv_take(u, FERTILIZER, 1)) return false;
        t.fertilized_until_day = std::max(int(t.fertilized_until_day), day + 2); --remaining_fertilize_; return true;
    case OP_DIG:
        if (t.kind == T_EMPTY || t.has_animal) return false;
        t = {}; return true;
    case OP_BUILD_COOP: case OP_BUILD_PASTURE:
        if (t.kind != T_EMPTY) return false;
        t = {}; t.kind = a.op == OP_BUILD_COOP ? T_COOP : T_PASTURE; return true;
    case OP_FEED:
        if (!t.has_animal || t.fed_today || !local_.inv_take(u, WHEAT, 1)) return false;
        t.fed_today = true; --remaining_feed_; return true;
    case OP_CARE:
        if (!t.has_animal || t.cared_today) return false;
        t.cared_today = true; return true;
    case OP_COLLECT_FERTILIZER:
        if (!t.has_animal || !t.fertilizer_available) return false;
        t.fertilizer_available = false; local_.inv_add(u, FERTILIZER, 1); return true;
    default: return false;
    }
}

void Agent::orders(const agent::AgentObservation& o, Action& action) {
    if (fixed_orders_) {
        action.n_orders = fixed_actions_[hour_].n_orders;
        std::copy_n(fixed_actions_[hour_].orders, action.n_orders, action.orders);
        return;
    }
    for (int c = 0; c < N_CROPS; ++c) previous_seeds_[c] = local_.seeds[c];
    for (int it = 0; it < N_ITEMS; ++it) {
        previous_items_[it] = local_.shed[it];
        for (int u = 0; u < local_.n_units; ++u) previous_items_[it] += local_.inv[u][it];
    }
    if (!plan_.trade) return;
    std::fill_n(previous_sold_, N_PRODUCTS, 0);
    double cash = local_.money;
    int market[N_PRODUCTS];
    std::copy_n(o.market.inventory, N_PRODUCTS, market);
    auto emit = [&](Order a) { action.orders[action.n_orders++] = a; };
    const int limit = std::min(10, init_.config.max_orders);
    int hires = local_.hires_today;
    const int hire_target = hour_ == 0 ? first_hire_wave(plan_, limit) : plan_.hires;
    auto hire = [&] {
        if (hour_ > 1 || hour_ >= end_hour_ || progress_.completed == progress_.requested) return;
        while (hires < hire_target && action.n_orders < limit) {
            const int cost = fib(hires) * init_.config.hire_mult;
            if (cash < cost) break;
            cash -= cost; ++hires; emit({M_HIRE, 0, 1});
        }
    };
    auto input_cash = [&] {
        double available = cash;
        if (options_.reserve_hire_cash && hour_ < end_hour_) for (int n = hires; n < plan_.hires; ++n) available -= fib(n) * init_.config.hire_mult;
        return std::max(0.0, available);
    };
    bool land_ordered = false;
    auto land = [&] {
        if (bought_land_ < plan_.buy_land && local_.n_quadrants < 4 && action.n_orders < limit) {
            const int cost = LAND_PRICES[local_.n_quadrants - 1];
            if (input_cash() >= cost) { emit({M_BUY_LAND, 0, 1}); cash -= cost; land_ordered = true; }
        }
    };
    if (hour_ == 0) land();
    hire();
    int input_orders = 0;
    if (options_.dawn_inputs && hour_ == 0) {
        for (int c = 0; c < N_CROPS; ++c) input_orders += plan_.buy_seeds[c] > bought_seeds_[c];
        for (int it : {int(WHEAT), int(GOOSE), int(COW), int(SHEEP)}) input_orders += plan_.buy_items[it] > bought_items_[it];
    }
    for (int it = 0; it < N_PRODUCTS && action.n_orders < limit - input_orders; ++it) {
        int retain = 0;
        if (it == WHEAT) {
            int carried = 0;
            for (int u = 0; u < local_.n_units; ++u) carried += local_.inv[u][it];
            retain = std::max(0, remaining_feed_ - carried);
        }

        if (hour_ == end_hour_) retain = 0;
        int n = std::min(plan_.sell_target[hour_][it] - sold_[it], int(local_.shed[it]) - retain);
        if (n <= 0) continue;
        emit({M_SELL, uint8_t(it), n});
        sold_[it] += n; local_.shed[it] -= n; local_.shed_total -= n;
        previous_sold_[it] = n;
        for (int k = 0; k < n; ++k) {
            int price = market_price(it, market[it]); cash += price;
            if (price > 1) ++market[it];
        }
    }
    int buying_slots = limit;
    for (int c = 0; (!options_.dawn_inputs || hour_ == 0) && c < N_CROPS && action.n_orders < limit && buying_slots > 0; ++c) {
        int n = std::min(plan_.buy_seeds[c] - bought_seeds_[c], int(input_cash()) / CROPS[c].seed);
        if (n <= 0) continue;
        emit({M_BUY_SEED, uint8_t(c), n}); pending_seeds_[c] = n;
        cash -= n * CROPS[c].seed; --buying_slots;
    }
    for (int it = 0; it < N_ITEMS && action.n_orders < limit && buying_slots > 0; ++it) {
        if (!(it == WHEAT || it == FERTILIZER || is_animal(it))) continue;
        if (options_.dawn_inputs && hour_ != 0 && is_animal(it)) continue;
        int want = std::min(plan_.buy_items[it] - bought_items_[it], init_.config.shed_capacity - local_.shed_total);
        int n = 0;
        while (n < want) {
            const int price = is_animal(it) ? ANIMALS[it - GOOSE].cost : market_price(it, market[it] - n - 1);
            // Small quote reserve for an opponent's preceding purchase in the same slot.
            if (input_cash() < price + (is_animal(it) ? 0 : 2)) break;
            cash -= price; ++n;
        }
        if (!n) continue;
        emit({uint8_t(is_animal(it) ? M_BUY_ANIMAL : M_BUY_PRODUCT), uint8_t(it), n});
        pending_items_[it] = n; local_.shed_total += n; --buying_slots;
    }
    hire();
    if (!land_ordered) land();
}

void Agent::automatic_plan(const agent::AgentObservation& o) {
    DayPlan p; p.day = o.day; p.hires = 5; p.relocate_new = true;
    int feed = 0, crops = 0;
    auto add = [&](int c) -> Job& { auto& j = p.jobs[p.count++]; j.tile = c; j.key = c; return j; };
    auto push = [](Job& j, int op, int arg = 0) { j.steps[j.count++] = {uint8_t(op), uint8_t(arg), 1}; };
    for (int c = 0; c < 100; ++c) {
        const auto& t = o.self().tiles[c / 10][c % 10];
        if (t.has_animal) {
            auto& j = add(c); push(j, OP_FEED); push(j, OP_CARE); ++feed;
            if (t.yield_units) push(j, OP_HARVEST);
            if (t.fertilizer_available) { auto& f = add(c); push(f, OP_COLLECT_FERTILIZER); }
        } else if (t.kind == T_PLANT) {
            ++crops;
            auto& j = add(c);
            const int age = o.day - t.planted_day;
            if (age != 1) push(j, OP_WATER);
            if (age >= 3) {
                push(j, OP_HARVEST);
                if (o.day < 27) { push(j, OP_PLANT, WHEAT); push(j, OP_WATER); ++p.buy_seeds[WHEAT]; }
            }
            if (!j.count) --p.count;
        }
    }
    if (o.day == 0) for (int i = 0; i < 2; ++i) {
        auto& j = add(90 + i); j.new_site = true;
        push(j, OP_BUILD_PASTURE); push(j, OP_PLACE, COW); push(j, OP_FEED); push(j, OP_CARE);
        ++p.buy_items[COW]; ++feed;
    }
    if (o.day < 25) for (int i = crops; i < 8; ++i) {
        auto& j = add(i); j.new_site = true; push(j, OP_PLANT, WHEAT); push(j, OP_WATER);
        ++p.buy_seeds[WHEAT];
    }
    p.buy_seeds[WHEAT] = std::max(0, p.buy_seeds[WHEAT] - int(o.own.seeds[WHEAT]));
    p.buy_items[WHEAT] = std::max(0, feed - int(o.own.shed[WHEAT]));
    for (int h = 0; h < 24; ++h) for (int it = 0; it < N_PRODUCTS; ++it)
        p.sell_target[h][it] = it == WHEAT ? std::max(0, int(o.own.shed[WHEAT]) - feed - 8) : 100000;
    set_plan(p);
}

void Agent::prepare_routes(const agent::AgentObservation& o, const agent::DecisionBudget& budget) {
    assert(!routes_ready_);
    plan_.buy_items[FERTILIZER] = 0;

    if (plan_.relocate_new) place_day(o,plan_);
    if (options_.route_delivery) add_deliveries(o, plan_, returns_supplied_ ? &returns_ : nullptr, options_.return_source_seed, options_.reachable_sources);
    routes_ = construct_routes(o, plan_, options_.route_rounds, options_.route_variant, options_.auto_hires, budget);
    routes_ready_ = true;
    for (int u = 0; u <= plan_.hires; ++u)
        for (int k = 0; k < routes_.count[u]; ++k) owner_[routes_.jobs[u][k]] = u;

}

void Agent::act(const agent::AgentObservation& o, const agent::DecisionBudget& budget, Action& action) {
    action.clear(); action.n_units = o.self().n_units;
    std::fill_n(action.units,action.n_units,UnitAction{}); action.finalize();
    if (budget.hard_expired() || budget.max_expansions == 0 || o.self().n_units>MAX_WORKERS) return;
    if (plan_.day != o.day) automatic_plan(o);
    load_observation(o);
    if (options_.route_rounds >= 0 && !routes_ready_) {
        if (budget.soft_expired()) return;
        prepare_routes(o,budget);
    } else if (routes_ready_ && options_.replan_after_hires && !replanned_ && o.hour > 0 && o.self().n_units == plan_.hires + 1) {
        DayPlan remaining = plan_;
        remaining.count = 0; remaining.relocate_new = false;
        int original[MAX_JOBS]{};
        for (int j = 0; j < plan_.count; ++j) if (cursor_[j] < plan_.jobs[j].count) {
            auto job = plan_.jobs[j];
            for (int k = cursor_[j]; k < job.count; ++k) job.steps[k - cursor_[j]] = job.steps[k];
            job.count -= cursor_[j]; job.new_site = false;
            original[remaining.count] = j; remaining.jobs[remaining.count++] = job;
        }
        auto updated = construct_routes(o, remaining, 0, options_.route_variant, -1, budget);
        std::fill_n(owner_, MAX_JOBS, -1);
        std::fill_n(workers_, MAX_WORKERS, Worker{});
        for (int u = 0; u <= plan_.hires; ++u) for (int k = 0; k < updated.count[u]; ++k) {
            updated.jobs[u][k] = original[updated.jobs[u][k]];
            owner_[updated.jobs[u][k]] = u;
        }
        routes_ = updated; replanned_ = true;
    } else if (!routes_ready_) assign_sites();
    if (routes_ready_ && (options_.route_variant & BIND_NEW_WORKERS) && known_units_ > 0 && local_.n_units > known_units_)
        bind_new_workers();
    known_units_ = local_.n_units;
    for (int u = 0; u < action.n_units; ++u) {
        if ((u & 3) == 0 && budget.hard_expired()) break;
        if (region_[u] < 0 && options_.region_penalty > 0) {
            int assigned[4]{};
            for (int v = 0; v < action.n_units; ++v) if (region_[v] >= 0) ++assigned[region_[v]];
            int best = -1, value = INT_MIN;
            for (int q = 0; q < 4; ++q) if (region_work_[q]) {
                const int corner = (q / 2 + 4) * 10 + q % 2 + 4;
                const int score = 1000 * region_work_[q] / (assigned[q] + 1) - 500 * distance(local_.pos_y[u] * 10 + local_.pos_x[u], corner);
                if (score > value) value = score, best = q;
            }
            region_[u] = best;
        }
        auto a = next_action(u);
        int deposit[N_PRODUCTS]{};
        if (returns_supplied_ && a.op == OP_DROP)
            std::copy_n(local_.inv[u], N_PRODUCTS, deposit);
        if (returns_supplied_ && a.op == OP_PLACE && a.arg<N_PRODUCTS)deposit[a.arg]=a.n;
        if (!apply(u, a)) {
            ++progress_.noops;
            auto& w = workers_[u];
            if (w.job >= 0) owner_[w.job] = -1;
            if (w.next >= 0) owner_[w.next] = -1;
            w = {};
            a = {};
        } else {
            if(returns_supplied_ && (a.op==OP_DROP || (a.op==OP_PLACE && a.arg<N_PRODUCTS)))record_deposit(u,deposit);
            if (a.op >= OP_NORTH && a.op <= OP_WEST) ++progress_.moves;
            else if (a.op == OP_DROP || (a.op == OP_PLACE && !is_animal(a.arg))) ++progress_.deposits;
            const int j = workers_[u].job;
            if (j >= 0 && cursor_[j] < plan_.jobs[j].count) {
                const auto expected = plan_.jobs[j].steps[cursor_[j]];
                const int pos = local_.pos_y[u] * 10 + local_.pos_x[u];
                if (a.op == OP_DIG && expected.op != OP_DIG && plan_.jobs[j].new_site) ++progress_.auxiliary;
                if (plan_.jobs[j].depot && (a.op == OP_DROP || a.op == OP_PLACE) && pos == plan_.jobs[j].tile) {
                    if (!returns_supplied_ && local_.inv_nkeys[u] == 0) cursor_[j] = plan_.jobs[j].count;
                } else if (a.op >= OP_PLACE && a.op == expected.op && a.arg == expected.arg && pos == plan_.jobs[j].tile) {
                    ++cursor_[j];
                    if (plan_.jobs[j].key == AUXILIARY_SOURCE) ++progress_.auxiliary;
                    else if (!plan_.jobs[j].depot) ++progress_.completed;
                    ++worker_done_[u];
                    if ((a.op == OP_PLANT || a.op == OP_PLACE || a.op == OP_BUILD_COOP || a.op == OP_BUILD_PASTURE) && plan_.jobs[j].key >= 0)
                        progress_.placements[plan_.jobs[j].key] = pos;
                }
            }
        }
        action.units[u] = a;
    }
    if (!budget.hard_expired()) orders(o, action);
    action.finalize();
}
}
