#include "routes.hpp"
#include "placement.hpp"
#include <algorithm>
#include <cassert>
#include <climits>
#include <numeric>

namespace kag::agents::sep22_worker {
namespace {
struct Node {
    int16_t jobs[24]{};
    int count = 0;
    int first = -1, last = -1, work = 0;
    int deadline = 23;
    int expiry = INT_MAX / 4;
    int shipments = 0;
    int worker = -1;
    int need[5]{}, required[5]{}; // Wheat, fertilizer, goose, cow, sheep.
};
struct Route {
    int16_t nodes[MAX_JOBS];
    int count = 0;
    int start = 44, capacity = 22;
    int worker = -1;
    inline static constexpr Count empty_inventory[N_ITEMS]{};
    // The observation outlives this route search; starting cargo never changes.
    const Count* inv = empty_inventory;
    Route() = default;
#ifndef DAY_POLICY_FULL_ROUTE_COPY
    Route(const Route& from) { *this=from; }
    Route& operator=(const Route& from) {
        if(this==&from)return *this;
        count=from.count;start=from.start;capacity=from.capacity;worker=from.worker;
        inv = from.inv;
        std::copy_n(from.nodes,count,nodes);
        return *this;
    }
#endif
};
struct Builder {
    const agent::AgentObservation& observation;
    DayPlan& plan;
    const agent::DecisionBudget& budget;
    uint64_t evaluations = 0;
    bool limited = false, exhausted = false;
    bool take() {
        if (exhausted) return false;
        if (limited && (evaluations >= budget.max_expansions ||
            ((evaluations & 63) == 0 && (budget.soft_expired() || budget.hard_expired())))) {
            exhausted = true; return false;
        }
        ++evaluations; return true;
    }
    Node nodes[MAX_JOBS]{};
    Route routes[MAX_WORKERS];
    Builder(const agent::AgentObservation& o, DayPlan& p, const agent::DecisionBudget& b)
        : observation(o), plan(p), budget(b) {}
    int n_nodes = 0;
    int n_workers = 0;
    int variant = 0;
    bool animal_cargo = false;
    bool bound_workers = false;

    void append(Node& node, int j) {
        assert(node.count < 24);
        const auto& job = plan.jobs[j];
        if (job.worker >= 0) {
            assert(node.worker < 0 || node.worker == job.worker);
            node.worker = job.worker; bound_workers = true;
        }
        const int old_work = node.work;
        if (node.count) node.work += distance(node.last, job.tile);
        else node.first = job.tile;
        node.last = job.tile;
        node.jobs[node.count++] = j;
        node.work += (variant & SHARED_DROP_SCORE) && job.depot && job.count ? 1 : job.count;
        if (job.new_site && job.steps[0].op != OP_DIG && observation.self().tiles[job.tile / 10][job.tile % 10].kind == T_WEED) ++node.work;
        if (node.expiry < 1000) node.expiry += node.work - old_work;
        const auto& initial_tile = observation.self().tiles[job.tile / 10][job.tile % 10];
        if (!job.new_site && initial_tile.kind == T_PLANT && initial_tile.max_lifespan_step >= 0 && initial_tile.yield_units > 0) {
            int last_crop_action = -1;
            for (int k = 0; k < job.count; ++k) {
                const int op = job.steps[k].op;
                if (op == OP_DIG || op == OP_PLANT) break;
                if (op == OP_WATER || op == OP_FERTILIZE || op == OP_HARVEST) last_crop_action = k;
            }
            const int last_alive = initial_tile.max_lifespan_step - observation.day * 24 + 2 * (initial_tile.yield_units - 1);
            if (last_crop_action >= 0 && last_alive < 24)
                node.expiry = std::min(node.expiry, last_alive + int(job.count) - 1 - last_crop_action);
        }
        node.deadline=std::min(node.deadline,int(job.deadline));
        if(job.depot)++node.shipments;
        int wheat = wheat_harvest(job, 0, observation.self().tiles[job.tile / 10][job.tile % 10], observation.day);
        for (int k = 0; k < job.count; ++k) {
            auto a = job.steps[k];
            if (a.op == OP_HARVEST) node.need[0] -= wheat, wheat = 0;
            if (a.op == OP_FEED) ++node.need[0];
            if (a.op == OP_FERTILIZE) ++node.need[1];
            if (a.op == OP_COLLECT_FERTILIZER) --node.need[1];
            if (a.op == OP_PLACE && is_animal(a.arg)) ++node.need[a.arg-GOOSE+2];
            if (job.depot && a.op == OP_PLACE && (a.arg==WHEAT || a.arg==FERTILIZER)) node.need[a.arg==WHEAT?0:1] += a.n;
            for (int it = 0; it < 5; ++it) node.required[it] = std::max(node.required[it], node.need[it]);
        }
    }

    void sites() { if(plan.relocate_new) place_day(observation,plan); }

    void build_nodes() {
        bool used[MAX_JOBS]{},split_service[MAX_JOBS]{};
        for(int j=0;j<plan.count;++j)used[j]=plan.jobs[j].count==0;
        for(int j=0;j<plan.count;++j) if(!used[j] && plan.jobs[j].depot && plan.jobs[j].worker>=0) {
            append(nodes[n_nodes++],j); used[j]=true;
        }
        for(int j=0;j<plan.count;++j)if(plan.jobs[j].service_predecessor>=0)split_service[plan.jobs[j].service_predecessor]=true;
        int paired[MAX_JOBS],shed_fertilizer[MAX_JOBS]{}; std::fill_n(paired,MAX_JOBS,-1);
        int fertilizer_jobs[100],fertilizer_count=0;
        for(int j=0;j<plan.count;++j)for(int k=0;k<plan.jobs[j].count;++k)
            if(plan.jobs[j].steps[k].op==OP_FERTILIZE)fertilizer_jobs[fertilizer_count++]=j;
        std::sort(fertilizer_jobs,fertilizer_jobs+fertilizer_count,[&](int a,int b) {
            const int da=shed_distance(plan.jobs[a].tile),db=shed_distance(plan.jobs[b].tile);
            return da!=db?da<db:a<b;
        });
        int shed_stock=observation.own.shed[FERTILIZER]+plan.buy_items[FERTILIZER];
        for(int k=0;k<fertilizer_count && shed_stock>0;++k, --shed_stock)++shed_fertilizer[fertilizer_jobs[k]];
        if (variant & 512) {
            int consumers[100], sources[100], rows=0, cols=0;
            for(int j=0;j<plan.count;++j) {
                const auto& job=plan.jobs[j];
                if(job.tile<0 || job.depot) continue;
                int need=0;for(int k=0;k<job.count;++k)need+=job.steps[k].op==OP_FERTILIZE;
                for(int k=shed_fertilizer[j];k<need;++k)consumers[rows++]=j;
                if(!job.carrier_bound && job.count==1 && job.steps[0].op==OP_COLLECT_FERTILIZER) sources[cols++]=j;
            }
            // Rectangular Hungarian assignment: each collection stays on the
            // same worker as its crop consumer, with minimum total travel.
            if(rows<=cols) {
                int u[101]{},v[101]{},p[101]{},way[101]{};
                for(int i=1;i<=rows;++i) {
                    p[0]=i; int col=0, minimum[101]; bool visited[101]{};std::fill_n(minimum,101,INT_MAX);
                    do {
                        visited[col]=true;int row=p[col],delta=INT_MAX,next=0;
                        for(int c=1;c<=cols;++c) if(!visited[c]) {
                            const int cost=distance(plan.jobs[consumers[row-1]].tile,plan.jobs[sources[c-1]].tile)-u[row]-v[c];
                            if(cost<minimum[c])minimum[c]=cost,way[c]=col;
                            if(minimum[c]<delta)delta=minimum[c],next=c;
                        }
                        for(int c=0;c<=cols;++c) if(visited[c])u[p[c]]+=delta,v[c]-=delta;else minimum[c]-=delta;
                        col=next;
                    } while(p[col]);
                    do {const int next=way[col];p[col]=p[next];col=next;}while(col);
                }
                for(int c=1;c<=cols;++c)if(p[c])paired[consumers[p[c]-1]]=sources[c-1];
            }
        }
        auto finish_delivery = [&](Node& node, int producer) {
            if (!plan.jobs[producer].carrier_bound) return;
            for (int j = 0; j < plan.count; ++j) if (plan.jobs[j].depot && plan.jobs[j].predecessor == producer) {
                if(variant & FINISH_TILE_BEFORE_RETURN) for(int child=0;child<plan.count;++child) {
                    auto& next=plan.jobs[child];
                    if(used[child] || child==producer || next.depot || next.carrier_bound || next.tile!=plan.jobs[producer].tile) continue;
                    const bool replant=next.new_site && next.predecessor==producer && next.steps[0].op==OP_PLANT;
                    bool animal_service=!next.new_site && next.predecessor<0;
                    bool simple=replant;
                    for(int k=0;k<next.count;++k) {
                        if(replant) simple &= next.steps[k].op==OP_PLANT || next.steps[k].op==OP_WATER;
                        animal_service &= next.steps[k].op==OP_FEED || next.steps[k].op==OP_CARE;
                    }
                    simple |= animal_service;
                    const int earliest=node.work+next.count+distance(next.tile,plan.jobs[j].tile)+plan.jobs[j].count+shed_distance(node.first);
                    if(!simple || earliest>plan.jobs[j].deadline) continue;
                    next.carrier_bound=true;next.predecessor=producer;append(node,child);used[child]=true;
                    plan.jobs[j].predecessor=child;
                    break;
                }
                append(node, j); used[j] = true; break;
            }
        };
        auto animal_service=[&](Node& node,int source) {
            if(!(variant & ANIMAL_SERVICE_SEED))return;
            for(int j=0;j<plan.count;++j) {
                const auto& job=plan.jobs[j];
                if(j==source || used[j] || job.carrier_bound || job.predecessor>=0 || job.new_site || job.tile!=plan.jobs[source].tile || !job.count)continue;
                bool service=true;for(int k=0;k<job.count;++k)service &= job.steps[k].op==OP_FEED || job.steps[k].op==OP_CARE;
                if(service){append(node,j);used[j]=true;}
            }
        };
        for (int j = 0; j < plan.count; ++j) {
            const auto& job = plan.jobs[j];
            if (used[j] || job.tile < 0) continue;
            int needed = 0;
            for (int k = 0; k < job.count; ++k) needed += job.steps[k].op == OP_FERTILIZE;
            if (!needed) continue;
            auto& node = nodes[n_nodes++];
            int collected = -1;
            for (int f = shed_fertilizer[j]; f < needed; ++f) {
                int source = -1, best = INT_MAX;
                for (int s = 0; s < plan.count; ++s) {
                    const auto& candidate = plan.jobs[s];
                    if(paired[j]>=0 && paired[j]!=s) continue;
                    if (used[s] || candidate.carrier_bound || candidate.tile < 0 || candidate.count != 1 || candidate.steps[0].op != OP_COLLECT_FERTILIZER) continue;
                    const auto& t = observation.self().tiles[candidate.tile / 10][candidate.tile % 10];
                    if (!t.has_animal || !t.fertilizer_available) continue;
                    int score = distance(candidate.tile, job.tile);
                    if (score < best) best = score, source = s;
                }
                if (source >= 0) {

                    // Fertilizer cannot be transferred between workers. Do not
                    // let local repair steal the collection out of this route.
                    plan.jobs[source].carrier_bound = true;
                    animal_service(node,source);
                    append(node, source); used[source] = true;
                    collected = source;
                }
            }
            int batch[4]{j}, batch_count=1;
            const int batch_cap=1+((variant>>27)&3);
            if(collected>=0 && !job.carrier_bound && job.predecessor<0 && !job.new_site) {
                for(;batch_count<batch_cap;++batch_count) {
                    int best=-1,cost=7;
                    for(int other=j+1;other<plan.count;++other) {
                        const auto& next=plan.jobs[other];const int source=paired[other];
                        if(used[other] || next.carrier_bound || next.new_site || next.predecessor>=0 || source<0 || used[source] || plan.jobs[source].carrier_bound)continue;
                        bool selected=false;for(int k=0;k<batch_count;++k)selected|=batch[k]==other;if(selected)continue;
                        const int score=distance(plan.jobs[batch[batch_count-1]].tile,next.tile)+distance(plan.jobs[collected].tile,plan.jobs[source].tile);
                        if(score<cost)cost=score,best=other;
                    }
                    if(best<0)break;
                    batch[batch_count]=best;collected=paired[best];
                    plan.jobs[collected].carrier_bound=true;animal_service(node,collected);append(node,collected);used[collected]=true;
                }
            }
            for(int k=1;k<batch_count;++k){append(node,batch[k]);used[batch[k]]=true;}
            append(node, j); used[j] = true; finish_delivery(node, j);
        }
        for (int j = 0; j < plan.count; ++j) {
            if (used[j] || plan.jobs[j].depot || plan.jobs[j].tile < 0 || plan.jobs[j].key == AUXILIARY_SOURCE) continue;
            auto& node = nodes[n_nodes++];
            if (plan.jobs[j].carrier_bound) {
                append(node, j); used[j] = true; finish_delivery(node, j);
            } else if(split_service[j] || plan.jobs[j].service_predecessor>=0) {append(node,j);used[j]=true;}
            else for (int s = j; s < plan.count; ++s)
                if (!used[s] && !split_service[s] && plan.jobs[s].service_predecessor<0 && !plan.jobs[s].carrier_bound && plan.jobs[s].key != AUXILIARY_SOURCE && plan.jobs[s].tile == plan.jobs[j].tile)
                    append(node, s), used[s] = true;
        }
    }

    void merge_deliveries() {
        const int cap = plan.delivery_batch_cap ? plan.delivery_batch_cap : 1 + variant / 4 % 4;
        if (cap == 1) return;
        for (;;) {
            int best_a = -1, best_b = -1, saving = 0;
            for (int a = 0; a < n_nodes; ++a) for (int b = 0; b < n_nodes; ++b) {
                const auto& na = nodes[a]; const auto& nb = nodes[b];
                if (na.worker >= 0 || nb.worker >= 0) continue;
                if (a == b || !na.shipments || !nb.shipments || na.shipments + nb.shipments > cap || na.count + nb.count >= 24) continue;
                const auto& da = plan.jobs[na.jobs[na.count - 1]];
                const auto& db = plan.jobs[nb.jobs[nb.count - 1]];
                if (!da.depot || !db.depot) continue;
                const int last = plan.jobs[na.jobs[na.count - 2]].tile;
                const int saved = distance(last, na.last) + 1 + distance(na.last, nb.first) - distance(last, nb.first);
                const int combined = na.work + nb.work + distance(na.last, nb.first) - saved;
                if (combined + shed_distance(na.first) > 20) continue;
                if (combined + shed_distance(na.first) > std::min(na.deadline, nb.deadline)+1) continue;
                int gain = saved;
                if (variant & 32768) {
                    const int finish = combined + shed_distance(na.first) + 1;
                    const int before = std::max(0, na.work + shed_distance(na.first) + 1 - na.deadline)
                                     + std::max(0, nb.work + shed_distance(nb.first) + 1 - nb.deadline);
                    const int after = std::max(0, finish - na.deadline) + std::max(0, finish - nb.deadline);
                    gain -= ((variant & 65536) ? 2 : 1) * std::max(0, after - before);
                }
                if (gain > saving) best_a = a, best_b = b, saving = gain;
            }
            if (best_a < 0) break;
            const auto a = nodes[best_a], b = nodes[best_b];
            auto& old_depot = plan.jobs[a.jobs[a.count - 1]];
            auto& final_depot = plan.jobs[b.jobs[b.count - 1]];
            for (int k = 0; k < b.count - 1; ++k) if (plan.jobs[b.jobs[k]].carrier_bound) {
                plan.jobs[b.jobs[k]].predecessor = old_depot.predecessor; break;
            }
            for (int k = 0; k < old_depot.count; ++k) {
                const auto step = old_depot.steps[k];
                int target = 0;
                while (target < final_depot.count && final_depot.steps[target].arg != step.arg) ++target;
                if (target == final_depot.count) { assert(target < MAX_STEPS); final_depot.steps[final_depot.count++] = step; }
                else final_depot.steps[target].n += step.n;
            }
            final_depot.deadline = std::min(final_depot.deadline, old_depot.deadline);
            old_depot.count = 0;
            Node merged;
            for (int k = 0; k < a.count - 1; ++k) append(merged, a.jobs[k]);
            for (int k = 0; k < b.count; ++k) append(merged, b.jobs[k]);
            merged.shipments = a.shipments + b.shipments;
            nodes[best_a] = merged;
            for (int k = best_b; k + 1 < n_nodes; ++k) nodes[k] = nodes[k + 1];
            --n_nodes;
        }
    }



    struct Cost {int time=0,late=0,expired=0,fertilizer=0;};
    template<int Count>
    Cost segmented_cost(const Route& r) const {
        constexpr int items[]={WHEAT,FERTILIZER,GOOSE,COW,SHEEP};
        Cost cost;int inventory[Count],at=r.start;
        for(int k=0;k<Count;++k)inventory[k]=r.inv[items[k]];
        for(int begin=0;begin<r.count;) {
            int end=begin,consumed[Count]{},required[Count]{};
            do {
                const auto& n=nodes[r.nodes[end]];
                for(int k=0;k<Count;++k){required[k]=std::max(required[k],consumed[k]+n.required[k]);consumed[k]+=n.need[k];}
            } while(!nodes[r.nodes[end++]].shipments && end<r.count);
            int pickups=0;
            for(int k=0;k<Count;++k)if(required[k]>inventory[k]) {
                ++pickups;
                if(observation.own.shed[items[k]]+plan.buy_items[items[k]]<required[k]-inventory[k])cost.time+=24;
                inventory[k]=required[k];
            }
            if(pickups){cost.time+=shed_distance(at)+pickups;at=shed_cell(at);}
            for(int i=begin;i<end;++i) {
                const auto& n=nodes[r.nodes[i]];
                cost.time+=distance(at,n.first)+n.work;at=n.last;
                const int elapsed=start_hour(r)+cost.time;
                if(n.deadline<23)cost.late+=std::max(0,elapsed-n.deadline-1);
                if(n.expiry<1000)cost.expired+=std::max(0,elapsed-n.expiry-1);
                for(int k=0;k<Count;++k)inventory[k]-=n.need[k];
            }
            begin=end;
        }
        return cost;
    }
    int start_hour(const Route& r) const {
        // Preserve the established dawn scoring margin for shortened days.
        // Midday starts need their actual hour, including partial-horizon tests.
        int hour=(observation.hour ? plan.hours : 24)-r.capacity;
// Route slack reserves capacity, but does not delay a worker's birth.
        if((variant & EXACT_BIRTH_TIMING) && (variant & 16))--hour;
        return hour;
    }
    int length(const Route& r) const {
        if(variant & SEGMENTED_PICKUPS)return (animal_cargo?segmented_cost<5>(r):segmented_cost<2>(r)).time;
        constexpr int items[]={WHEAT,FERTILIZER,GOOSE,COW,SHEEP};
        int total = 0, at = r.start, consumed[5]{}, required[5]{};
        for (int i = 0; i < r.count; ++i) {
            const auto& node = nodes[r.nodes[i]];
            total += distance(at, node.first) + node.work; at = node.last;
            for (int it=0;it<5;++it) {
                required[it] = std::max(required[it], consumed[it] + node.required[it]);
                consumed[it] += node.need[it];
            }
        }
        int pickups = 0;
        for (int k=0;k<5;++k) if (required[k] > r.inv[items[k]]) {
            ++pickups;
            if (observation.own.shed[items[k]] + plan.buy_items[items[k]] < required[k] - r.inv[items[k]]) total += 24;
        }
        if (pickups && r.count) total += pickups + shed_distance(r.start) + distance(shed_cell(r.start), nodes[r.nodes[0]].first) - distance(r.start, nodes[r.nodes[0]].first);
        return total;
    }

    template<int ItemCount>
    int64_t cargo_value(const Route& r) const {
        constexpr int items[] = {WHEAT, FERTILIZER, GOOSE, COW, SHEEP};
        int consumed[5]{}, required[5]{};
        int expiry_slack[MAX_JOBS], n_expiring = 0;
        int delivery_slack[MAX_JOBS], n_deliveries = 0;
        int at = r.start, time = 0, late = 0;
        for (int i = 0; i < r.count; ++i) {
            const auto& n = nodes[r.nodes[i]];
            time += distance(at, n.first) + n.work; at = n.last;
            const int elapsed = start_hour(r) + time;
            if (n.deadline < 23) late += std::max(0, elapsed - n.deadline - 1);
            if ((variant & 4096) && n.deadline < 23) delivery_slack[n_deliveries++] = n.deadline + 1 - elapsed;
            if (n.expiry < 1000) expiry_slack[n_expiring++] = n.expiry + 1 - elapsed;
            for (int k = 0; k < ItemCount; ++k) {
                required[k] = std::max(required[k], consumed[k] + n.required[k]);
                consumed[k] += n.need[k];
            }
        }
        int pickups = 0, setup = 0;
        for (int k = 0; k < ItemCount; ++k) if (required[k] > r.inv[items[k]]) {
            ++pickups;
            if (observation.own.shed[items[k]] + plan.buy_items[items[k]] < required[k] - r.inv[items[k]]) setup += 24;
        }
        if (pickups && r.count)
            setup += pickups + shed_distance(r.start) + distance(shed_cell(r.start), nodes[r.nodes[0]].first) - distance(r.start, nodes[r.nodes[0]].first);
        time += setup;
        if (variant & 4096) {
            late = 0;
            for (int i = 0; i < n_deliveries; ++i) late += std::max(0, setup - delivery_slack[i]);
        }
        int expired = 0;
        for (int i = 0; i < n_expiring; ++i) expired += std::max(0, setup - expiry_slack[i]);
        const int over = std::max(0, time - r.capacity);
        return int64_t(over) * over * 10000 + int64_t(over) * 100000 + int64_t(late) * 1000000 +
               int64_t(expired) * 10000 + time * 10 + int64_t(time) * time;
    }
    int64_t value(const Route& r) const {
        if(bound_workers) for(int k=0;k<r.count;++k)
            if(nodes[r.nodes[k]].worker>=0 && nodes[r.nodes[k]].worker!=r.worker) return INT64_MAX/4;
        if(variant & SEGMENTED_PICKUPS) {
            const auto c=animal_cargo?segmented_cost<5>(r):segmented_cost<2>(r);
            const int over=std::max(0,c.time-r.capacity);
            return int64_t(c.fertilizer)*10000000+int64_t(over)*over*10000+int64_t(over)*100000+
                int64_t(c.late)*1000000+int64_t(c.expired)*10000+c.time*10+int64_t(c.time)*c.time;
        }
        return animal_cargo ? cargo_value<5>(r) : cargo_value<2>(r);
    }

    static Route copy_route(const Route& source) {
        Route result;
        result.count = source.count; result.start = source.start; result.capacity = source.capacity;
        result.worker = source.worker;
        result.inv = source.inv;
        for (int k = 0; k < source.count; ++k) result.nodes[k] = source.nodes[k];
        return result;
    }
    static Route insert(const Route& source, int pos, int node) {
        auto r = copy_route(source);
        for (int k = r.count; k > pos; --k) r.nodes[k] = r.nodes[k - 1];
        r.nodes[pos] = node; ++r.count; return r;
    }
    static Route erase(const Route& source, int pos) {
        auto r = copy_route(source);
        for (int k = pos; k + 1 < r.count; ++k) r.nodes[k] = r.nodes[k + 1];
        --r.count; return r;
    }

    void beam_assignment(const int* order) {
        constexpr int width=4;
        struct State { Route routes[MAX_WORKERS];int64_t values[MAX_WORKERS],total=0;uint64_t hashes[MAX_WORKERS],hash=0; } storage[2][width];
        auto* beam=storage[0];auto* next=storage[1];
        auto hash_route=[&](const Route& r) {
            uint64_t h=14695981039346656037ull;
            auto add=[&](int x){h=(h^uint32_t(x))*1099511628211ull;};
            add(r.start);add(r.capacity);if(bound_workers)add(r.worker);
            for(int k=0;k<r.count;++k)add(r.nodes[k]);return h;
        };
        // Copy only live nodes into their destination, avoiding a temporary
        // whose subsequent assignment copies the entire 512-node capacity.
        auto copy=[](Route& to,const Route& from) {
            to.count=from.count;to.start=from.start;to.capacity=from.capacity;
            to.worker=from.worker;
            to.inv = from.inv;
            std::copy_n(from.nodes,from.count,to.nodes);
        };
        int count=1;
        for(int u=0;u<n_workers;++u){copy(beam[0].routes[u],routes[u]);beam[0].values[u]=value(routes[u]);
            beam[0].total+=beam[0].values[u];beam[0].hashes[u]=hash_route(routes[u]);beam[0].hash+=beam[0].hashes[u];}
        struct Choice {int parent,worker,position;int64_t score,route_value;uint64_t hash,route_hash;};
        for(int k=0;k<n_nodes;++k) {
            Choice choices[width];int kept=0;
            struct Insertion {int position=-1;int64_t value=0;uint64_t hash=0;} cache[width][MAX_WORKERS];
            for(int b=0;b<count;++b)for(int u=0;u<n_workers;++u) {
                if(nodes[order[k]].worker>=0 && nodes[order[k]].worker!=u) continue;
                const auto& route=beam[b].routes[u];
                bool equivalent=false;
                if(!bound_workers && !route.count)for(int v=0;v<u;++v) {
                    const auto& previous=beam[b].routes[v];
                    equivalent |= !previous.count && previous.start==route.start && previous.capacity==route.capacity &&
                        (previous.inv == route.inv || std::equal(previous.inv,previous.inv+N_ITEMS,route.inv));
                }
                if(equivalent){if(!take())return;continue;}
                int at=-1;int64_t best=INT64_MAX;uint64_t hash=0;
                int reused=-1;
                for(int old=0;old<b;++old) {
                    const auto& previous=beam[old].routes[u];
                    if(cache[old][u].position>=0 && beam[old].hashes[u]==beam[b].hashes[u] &&
                       previous.count==route.count && previous.start==route.start && previous.capacity==route.capacity &&
                       std::equal(previous.nodes,previous.nodes+previous.count,route.nodes)) {reused=old;break;}
                }
                if(reused>=0) {
                    // Preserve deterministic expansion accounting even when
                    // equivalent candidates need no repeated scoring.
                    if(limited){for(int pos=0;pos<=route.count;++pos)if(!take())return;}
                    else evaluations+=route.count+1;
                    const auto hit=cache[reused][u];at=hit.position;best=hit.value;hash=hit.hash;
                } else for(int pos=0;pos<=route.count;++pos) {
                    if(!take())return;
                    const auto candidate=insert(route,pos,order[k]);const auto score=value(candidate);
                    if(score<best)best=score,at=pos,hash=hash_route(candidate);
                }
                cache[b][u]={at,best,hash};
                Choice c{b,u,at,beam[b].total-beam[b].values[u]+best,best,beam[b].hash-beam[b].hashes[u]+hash,hash};
                bool duplicate=false;for(int q=0;q<kept;++q)duplicate|=choices[q].hash==c.hash;if(duplicate)continue;
                int pos=0;while(pos<kept && choices[pos].score<=c.score)++pos;
                if(pos==width)continue;
                for(int q=std::min(kept,width-1);q>pos;--q)choices[q]=choices[q-1];
                choices[pos]=c;kept=std::min(width,kept+1);
            }
            for(int b=0;b<kept;++b) {
                const auto c=choices[b];const auto& parent=beam[c.parent];auto& child=next[b];child.total=c.score;child.hash=c.hash;
                for(int u=0;u<n_workers;++u) {
                    copy(child.routes[u],parent.routes[u]);
                    if(u==c.worker) {
                        auto& r=child.routes[u];
                        for(int pos=r.count;pos>c.position;--pos)r.nodes[pos]=r.nodes[pos-1];
                        r.nodes[c.position]=order[k];++r.count;
                    }
                    child.values[u]=u==c.worker?c.route_value:parent.values[u];
                    child.hashes[u]=u==c.worker?c.route_hash:parent.hashes[u];
                }
            }
            count=kept;std::swap(beam,next);
        }
        for(int u=0;u<n_workers;++u)copy(routes[u],beam[0].routes[u]);
    }

    void regret_assignment(const int* order) {
        int64_t cost[MAX_JOBS][MAX_WORKERS],values[MAX_WORKERS]{};
        int16_t position[MAX_JOBS][MAX_WORKERS];bool used[MAX_JOBS]{};
        auto refresh=[&](int u) {
            for(int n=0;n<n_nodes;++n)if(!used[n]) {
                cost[n][u]=INT64_MAX/4;position[n][u]=0;
                for(int k=0;k<=routes[u].count;++k) {
                    if(!take())return;
                    const auto v=value(insert(routes[u],k,n))-values[u];
                    if(v<cost[n][u]){cost[n][u]=v;position[n][u]=k;}
                }
            }
        };
        for(int u=0;u<n_workers;++u)refresh(u);
        for(int added=0;added<n_nodes && !exhausted;++added) {
            int first=0;while(used[order[first]])++first;
            const auto& priority=nodes[order[first]];
            int selected=-1,worker=0;int64_t best_regret=-1,best_cost=INT64_MIN;
            for(int n=0;n<n_nodes;++n)if(!used[n]) {
                const auto& node=nodes[n];
                if(node.deadline!=priority.deadline ||
                    ((node.expiry<24 || priority.expiry<24) && node.expiry!=priority.expiry))continue;
                int u_best=0;int64_t first_cost=INT64_MAX/4,second_cost=INT64_MAX/4;
                for(int u=0;u<n_workers;++u) {
                    if(cost[n][u]<first_cost){second_cost=first_cost;first_cost=cost[n][u];u_best=u;}
                    else second_cost=std::min(second_cost,cost[n][u]);
                }
                const auto regret=second_cost-first_cost;
                if(regret>best_regret || (regret==best_regret && first_cost>best_cost)) {
                    best_regret=regret;best_cost=first_cost;selected=n;worker=u_best;
                }
            }
            assert(selected>=0);
            routes[worker]=insert(routes[worker],position[selected][worker],selected);
            used[selected]=true;values[worker]=value(routes[worker]);refresh(worker);
        }
    }

    void initialize() {
        n_workers = std::min(MAX_WORKERS, plan.hires + 1);
        int occupancy[4]{};
        const int corners[4] = {44, 45, 54, 55};
        for (int u = 0; u < n_workers; ++u) {
            auto& r = routes[u]; r.count = 0;
            r.worker = u;
            r.inv = u < observation.self().n_units ? observation.own.inv[u] : Route::empty_inventory;
            if (u < observation.self().n_units) {
                r.start = observation.self().pos_y[u] * 10 + observation.self().pos_x[u];
            } else {
                int q = int(std::min_element(occupancy, occupancy + 4) - occupancy);
                r.start = corners[q];
                if (plan.spawn_tile[u] >= 0) r.start = plan.spawn_tile[u];
            }
            for (int q = 0; q < 4; ++q) occupancy[q] += r.start == corners[q];
            int birth = observation.hour;
            if(u>=observation.self().n_units) {
                const int planned=plan.hire_hour[u]<24 ? plan.hire_hour[u]+1 : (u<=first_hire_wave(plan,10)?1:2);
                birth=std::max(observation.hour+1,planned);
            }
            r.capacity = plan.hours - birth - ((variant & 16) ? 1 : 0);
        }
        int order[MAX_JOBS]; std::iota(order, order + n_nodes, 0);
        std::sort(order, order + n_nodes, [&](int a, int b) {
            if (nodes[a].expiry != nodes[b].expiry && (nodes[a].expiry < 24 || nodes[b].expiry < 24)) return nodes[a].expiry < nodes[b].expiry;
            const int wa = nodes[a].work + (variant % 4 / 2 == 1 ? 1 : 2) * shed_distance(nodes[a].first);
            const int wb = nodes[b].work + (variant % 4 / 2 == 1 ? 1 : 2) * shed_distance(nodes[b].first);
            if (nodes[a].deadline != nodes[b].deadline) return nodes[a].deadline < nodes[b].deadline;
            return wa != wb ? wa > wb : a < b;
        });
        if((variant & REGRET_ASSIGNMENT) && n_workers>=8){regret_assignment(order);return;}
        if((variant & BEAM_ASSIGNMENT_SEED) && n_workers>=8){beam_assignment(order);return;}
        int64_t values[MAX_WORKERS];
        for (int u = 0; u < n_workers; ++u) values[u] = value(routes[u]);
        for (int k = 0; k < n_nodes; ++k) {
            int worker = 0, pos = 0; int64_t best = INT64_MAX;
            for (int u = 0; u < n_workers; ++u) {
                const auto base = values[u];
                for (int i = 0; i <= routes[u].count; ++i) {
                    if (!take()) {
                        if (best < INT64_MAX) routes[worker] = insert(routes[worker], pos, order[k]);
                        return;
                    }
                    auto candidate = insert(routes[u], i, order[k]);
                    const auto score = value(candidate) - base;
                    if (score < best) best = score, worker = u, pos = i;
                }
            }
            routes[worker] = insert(routes[worker], pos, order[k]);
            values[worker] = value(routes[worker]);
        }
    }

    void improve(int rounds) {
        if (rounds <= 0) return;
        int64_t values[MAX_WORKERS];
        for (int u = 0; u < n_workers; ++u) values[u] = value(routes[u]);
        int travel[MAX_WORKERS];
        auto raw_time = [&](const Route& route) {
            int time = 0, at = route.start;
            for (int k = 0; k < route.count; ++k) {
                const auto& node = nodes[route.nodes[k]];
                time += distance(at, node.first) + node.work; at = node.last;
            }
            return time;
        };
        for (int u = 0; u < n_workers; ++u) travel[u] = raw_time(routes[u]);
        auto insertion_bound = [&](int worker, int position, int job) {
            const auto& route = routes[worker]; const auto& node = nodes[job];
            const int previous = position ? nodes[route.nodes[position-1]].last : route.start;
            int time = travel[worker] + distance(previous, node.first) + node.work;
            if (position < route.count) {
                const int next = nodes[route.nodes[position]].first;
                time += distance(node.last, next) - distance(previous, next);
            }
            const int over = std::max(0, time-route.capacity);
            return int64_t(over)*over*10000 + int64_t(over)*100000 + time*10 + int64_t(time)*time;
        };
        for (int round = 0; round < rounds; ++round) {
            bool changed = false;
            int order[MAX_WORKERS]; std::iota(order, order + n_workers, 0);
            std::sort(order, order + n_workers, [&](int a, int b) {
                const auto va = values[a], vb = values[b]; return va != vb ? va > vb : a < b;
            });
            for (int index = 0; index < n_workers; ++index) {
                const int a = order[index];
                int64_t best_delta = 0; int from = -1, worker = -1, position = -1;
                const auto va = values[a];
                auto commit = [&] {
                    if (worker < 0) return;
                    routes[worker] = insert(routes[worker], position, routes[a].nodes[from]);
                    routes[a] = erase(routes[a], from); changed = true;
                    values[a] = value(routes[a]); values[worker] = value(routes[worker]);
                    travel[a] = raw_time(routes[a]); travel[worker] = raw_time(routes[worker]);
                };
                for (int i = 0; i < routes[a].count; ++i) {
                    const auto shortened = erase(routes[a], i);
                    const auto da = value(shortened) - va;
                    for (int b = 0; b < n_workers; ++b) {
                        if (a == b) continue;
                        const auto vb = values[b];
                        for (int j = 0; j <= routes[b].count; ++j) {
                            if (!take()) { commit(); return; }
                            if (da + insertion_bound(b, j, routes[a].nodes[i]) - vb >= best_delta) continue;
                            const auto delta = da + value(insert(routes[b], j, routes[a].nodes[i])) - vb;
                            if (delta < best_delta) best_delta = delta, from = i, worker = b, position = j;
                        }
                    }
                }
                commit();
                for (int i = 0; i < routes[a].count; ++i) for (int j = i + 1; j < routes[a].count; ++j) {
                    if (!take()) return;
                    auto candidate = copy_route(routes[a]);
                    std::reverse(candidate.nodes + i, candidate.nodes + j + 1);
                    const auto candidate_value = value(candidate);
                    if (candidate_value < values[a]) {
                        routes[a] = candidate; values[a] = candidate_value; changed = true;
                        travel[a] = raw_time(routes[a]);
                    }
                }
            }
            if (!changed) break;
        }
    }

    void improve_new_sites() {
        bool used[100]{};
        const int owned = std::min(4, observation.self().n_quadrants + plan.buy_land);
        for (int c = 0; c < 100; ++c) {
            const auto& t = observation.self().tiles[c / 10][c % 10];
            used[c] = !(t.kind == T_EMPTY || (t.kind == T_LOCKED && quadrant_of(c % 10, c / 10, 10) < owned));
        }
        for (int j = 0; j < plan.count; ++j) if (plan.jobs[j].tile >= 0 && !plan.jobs[j].depot) used[plan.jobs[j].tile] = true;
        for (int u = 0; u < n_workers; ++u) for (int k = 0; k < routes[u].count; ++k) {
            const int n = routes[u].nodes[k]; auto& node = nodes[n];
            for (int m = 0; m < node.count; ++m) {
                auto& job = plan.jobs[node.jobs[m]];
                if (!job.new_site || !plan.relocate_new || job.steps[0].op != OP_PLANT) continue;
                const auto original = node;
                const int old = job.tile; used[old] = false;
                int64_t best = value(routes[u]); int site = old; Node best_node = node;
                for (int c = 0; c < 100; ++c) if (!used[c]) {
                    if (!take()) { job.tile = site; node = best_node; used[site] = true; return; }
                    job.tile = c; Node candidate;
                    for (int i = 0; i < original.count; ++i) append(candidate, original.jobs[i]);
                    node = candidate;
                    const auto score = value(routes[u]);
                    if (score < best) best = score, site = c, best_node = candidate;
                }
                job.tile = site; node = best_node; used[site] = true;
            }
        }
    }

    void swap_overloaded() {
        for (int a = 0; a < n_workers; ++a) {
            if (length(routes[a]) <= routes[a].capacity) continue;
            int64_t best = 0; int other = -1, ai = -1, bi = -1;
            const auto va = value(routes[a]);
            for (int b = 0; b < n_workers; ++b) if (a != b) {
                const auto vb = value(routes[b]);
                for (int i = 0; i < routes[a].count; ++i) for (int j = 0; j < routes[b].count; ++j) {
                    if (!take()) return;
                    auto ra = copy_route(routes[a]), rb = copy_route(routes[b]);
                    std::swap(ra.nodes[i], rb.nodes[j]);
                    const auto delta = value(ra) + value(rb) - va - vb;
                    if (delta < best) best = delta, other = b, ai = i, bi = j;
                }
            }
            if (other >= 0) std::swap(routes[a].nodes[ai], routes[other].nodes[bi]);
        }
    }

    Routes finish() const {
        Routes out; out.evaluations = evaluations; out.truncated = exhausted;
        for (int u = 0; u < n_workers; ++u) {
            out.start[u] = routes[u].start;
            out.predicted_length[u] = length(routes[u]);
            out.overrun += std::max(0, out.predicted_length[u] - routes[u].capacity);
            for (int i = 0; i < routes[u].count; ++i) {
                const auto& node = nodes[routes[u].nodes[i]];
                for (int k = 0; k < node.count; ++k) out.jobs[u][out.count[u]++] = node.jobs[k];
            }
        }
        return out;
    }

    Routes reorder_jobs(Routes out) {
        // Assignment is already fixed. Break the construction bundles only
        // within a worker's route, so fertilizer never changes carrier.
        for(int j=0;j<plan.count;++j) { nodes[j]=Node{};if(plan.jobs[j].tile>=0)append(nodes[j],j); }
        for(int u=0;u<n_workers;++u) {
            auto& route=routes[u];route.count=out.count[u];
            std::copy_n(out.jobs[u],route.count,route.nodes);
        }
        struct Visit {
            int16_t need[5],required[5],predecessor,service_predecessor,expiry;
            uint8_t tile,work,deadline;
            bool depot,carrier;
        } visits[MAX_JOBS];
        constexpr int items[]={WHEAT,FERTILIZER,GOOSE,COW,SHEEP};
        for(int j=0;j<plan.count;++j) {
            const auto& n=nodes[j];auto& v=visits[j];
            for(int k=0;k<5;++k){v.need[k]=n.need[k];v.required[k]=n.required[k];}
            v.service_predecessor=plan.jobs[j].service_predecessor;v.predecessor=plan.jobs[j].predecessor;v.expiry=std::min(32767,n.expiry);
            v.tile=plan.jobs[j].tile;v.work=n.work;v.deadline=n.deadline;
            v.depot=plan.jobs[j].depot;v.carrier=plan.jobs[j].carrier_bound;
        }
        auto compact_score=[&]<int Count>(const Route& r)->int64_t {
            uint64_t present[8]{},seen[8]{};
            for(int k=0;k<r.count;++k){const int j=r.nodes[k];present[j/64]|=uint64_t{1}<<(j%64);}
            auto legal=[&](int j) {
                if(plan.jobs[j].worker>=0 && plan.jobs[j].worker!=r.worker) return false;
                const auto& v=visits[j];const int p=v.predecessor;
                if(p>=0) {
                    const bool here=present[p/64]&(uint64_t{1}<<(p%64));
                    if((here && !(seen[p/64]&(uint64_t{1}<<(p%64)))) ||
                        ((variant & EXCHANGE_ROUTE_TAILS) && v.carrier && !here))return false;
                }
                const int service=v.service_predecessor;
                if(service>=0 && (present[service/64]&(uint64_t{1}<<(service%64))) &&
                    !(seen[service/64]&(uint64_t{1}<<(service%64))))return false;
                seen[j/64]|=uint64_t{1}<<(j%64);return true;
            };
            if(variant & SEGMENTED_PICKUPS) {
                int inventory[Count],stock[Count],at=r.start,time=0,late=0,expired=0;
                for(int k=0;k<Count;++k){inventory[k]=r.inv[items[k]];stock[k]=observation.own.shed[items[k]]+plan.buy_items[items[k]];}
                for(int begin=0;begin<r.count;) {
                    int end=begin,used[Count]{},needed[Count]{};
                    do {
                        const int j=r.nodes[end];const auto& v=visits[j];
                        if(!legal(j))return INT64_MAX/4;
                        for(int k=0;k<Count;++k){needed[k]=std::max(needed[k],used[k]+v.required[k]);used[k]+=v.need[k];}
                    } while(!visits[r.nodes[end++]].depot && end<r.count);
                    int pickups=0;
                    for(int k=0;k<Count;++k)if(needed[k]>inventory[k]) {
                        ++pickups;if(stock[k]<needed[k]-inventory[k])time+=24;inventory[k]=needed[k];
                    }
                    if(pickups){time+=shed_distance(at)+pickups;at=shed_cell(at);}
                    for(int i=begin;i<end;++i) {
                        const auto& v=visits[r.nodes[i]];time+=distance(at,v.tile)+v.work;at=v.tile;
                        const int elapsed=start_hour(r)+time;
                        if(v.deadline<23)late+=std::max(0,elapsed-v.deadline-1);
                        if(v.expiry<1000)expired+=std::max(0,elapsed-v.expiry-1);
                        for(int k=0;k<Count;++k)inventory[k]-=v.need[k];
                    }
                    begin=end;
                }
                const int over=std::max(0,time-r.capacity);
                return int64_t(over)*over*10000+int64_t(over)*100000+int64_t(late)*1000000+int64_t(expired)*10000+time*10+int64_t(time)*time;
            }
            int consumed[Count]{},required[Count]{},delivery_slack[MAX_JOBS],expiry_slack[MAX_JOBS],nd=0,ne=0;
            int at=r.start,time=0,late=0;
            for(int k=0;k<r.count;++k) {
                const int j=r.nodes[k];const auto& v=visits[j];
                if(!legal(j))return INT64_MAX/4;
                time+=distance(at,v.tile)+v.work;at=v.tile;
                const int elapsed=start_hour(r)+time;
                if(v.deadline<23){late+=std::max(0,elapsed-v.deadline-1);if(variant&4096)delivery_slack[nd++]=v.deadline+1-elapsed;}
                if(v.expiry<1000)expiry_slack[ne++]=v.expiry+1-elapsed;
                for(int it=0;it<Count;++it){required[it]=std::max(required[it],consumed[it]+v.required[it]);consumed[it]+=v.need[it];}
            }
            int pickups=0,setup=0;
            for(int k=0;k<Count;++k)if(required[k]>r.inv[items[k]]) {
                ++pickups;
                if(observation.own.shed[items[k]]+plan.buy_items[items[k]]<required[k]-r.inv[items[k]])setup+=24;
            }
            if(pickups && r.count)setup+=pickups+shed_distance(r.start)+distance(shed_cell(r.start),visits[r.nodes[0]].tile)-distance(r.start,visits[r.nodes[0]].tile);
            time+=setup;
            if(variant&4096){late=0;for(int k=0;k<nd;++k)late+=std::max(0,setup-delivery_slack[k]);}
            int expired=0;for(int k=0;k<ne;++k)expired+=std::max(0,setup-expiry_slack[k]);
            const int over=std::max(0,time-r.capacity);
            return int64_t(over)*over*10000+int64_t(over)*100000+int64_t(late)*1000000+int64_t(expired)*10000+time*10+int64_t(time)*time;
        };
        auto score=[&](const Route& r) {
#ifndef DAY_POLICY_REFERENCE_SCORE
            return animal_cargo?compact_score.template operator()<5>(r):compact_score.template operator()<2>(r);
#else
            if(variant & EXCHANGE_ROUTE_TAILS) {
                uint64_t present[8]{};
                for(int k=0;k<r.count;++k){const int j=r.nodes[k];present[j/64]|=uint64_t{1}<<(j%64);}
                for(int k=0;k<r.count;++k) {
                    const auto& j=plan.jobs[r.nodes[k]];const int p=j.predecessor;
                    if(j.carrier_bound && p>=0 && !(present[p/64]&(uint64_t{1}<<(p%64))))return INT64_MAX/4;
                }
            }
                bool present[MAX_JOBS]{},seen[MAX_JOBS]{};int fertilizer=r.inv[FERTILIZER];
                for(int k=0;k<r.count;++k)present[r.nodes[k]]=true;
                for(int k=0;k<r.count;++k) {
                    const int j=r.nodes[k],p=plan.jobs[j].predecessor;
                    if(p>=0 && present[p] && !seen[p])return INT64_MAX/4;
                    const int service=plan.jobs[j].service_predecessor;
                    if(service>=0 && present[service] && !seen[service])return INT64_MAX/4;
                    const auto& n=nodes[j];
                    if(n.required[1]>fertilizer)return INT64_MAX/4;
                    fertilizer-=n.need[1];seen[j]=true;
                }
                return value(r);
#endif
        };
        auto time_bound=[](int time,int capacity) {const int over=std::max(0,time-capacity);return int64_t(over)*over*10000+int64_t(over)*100000+time*10+int64_t(time)*time;};
        auto raw_time=[&](const Route& r) {int time=0,at=r.start;for(int k=0;k<r.count;++k){const auto& v=visits[r.nodes[k]];time+=distance(at,v.tile)+v.work;at=v.tile;}return time;};
        auto removed_time=[&](const Route& r,int i,int time) {
            const auto& v=visits[r.nodes[i]];const int prev=i?visits[r.nodes[i-1]].tile:r.start;
            time-=distance(prev,v.tile)+v.work;
            if(i+1<r.count){const int next=visits[r.nodes[i+1]].tile;time+=distance(prev,next)-distance(v.tile,next);}return time;
        };
        auto inserted_time=[&](const Route& r,int i,int job,int time) {
            const auto& v=visits[job];const int prev=i?visits[r.nodes[i-1]].tile:r.start;
            time+=distance(prev,v.tile)+v.work;
            if(i<r.count){const int next=visits[r.nodes[i]].tile;time+=distance(v.tile,next)-distance(prev,next);}return time;
        };
        auto replaced_time=[&](const Route& r,int i,int job,int time) {
            const auto& old=visits[r.nodes[i]];const auto& v=visits[job];const int prev=i?visits[r.nodes[i-1]].tile:r.start;
            time+=distance(prev,v.tile)+v.work-distance(prev,old.tile)-old.work;
            if(i+1<r.count){const int next=visits[r.nodes[i+1]].tile;time+=distance(v.tile,next)-distance(old.tile,next);}return time;
        };
        auto refine_route=[&](Route& route) {
            int64_t current=score(route);int elapsed=raw_time(route);
            for(int round=0;round<2;++round) {
                bool changed=false;
                for(int i=0;i<route.count;++i) {
                    const auto shortened=erase(route,i);const int short_time=removed_time(route,i,elapsed);int at=-1;int64_t best=current;
                    for(int k=0;k<route.count;++k)if(k!=i) {
                        if(!take())break;
                        if(time_bound(inserted_time(shortened,k,route.nodes[i],short_time),route.capacity)>=best)continue;
                        const auto candidate=insert(shortened,k,route.nodes[i]);const auto cost=score(candidate);
                        if(cost<best)best=cost,at=k;
                    }
                    if(at>=0){route=insert(shortened,at,route.nodes[i]);current=best;elapsed=raw_time(route);changed=true;}
                    if(exhausted)break;
                }
                if(!changed || exhausted)break;
            }
        };
        for(int u=0;u<n_workers;++u)refine_route(routes[u]);
        if((variant & REASSIGN_ROUTE_JOBS) && n_workers>=8) {
            bool protected_job[MAX_JOBS]{};
            for(int j=0;j<plan.count;++j)if(plan.jobs[j].depot)
                for(int p=j;p>=0 && !protected_job[p];p=plan.jobs[p].predecessor)protected_job[p]=true;
            int64_t values[MAX_WORKERS];int elapsed[MAX_WORKERS];for(int u=0;u<n_workers;++u){values[u]=score(routes[u]);elapsed[u]=raw_time(routes[u]);}
            for(int a=0;a<n_workers;++a) {
                if(length(routes[a])<routes[a].capacity-2)continue;
                int ai=-1,best_b=-1,bj=-1;bool swap=false;int64_t best=0;
                for(int i=0;i<routes[a].count;++i) {
                    const int job=routes[a].nodes[i];if(protected_job[job])continue;
                    const auto smaller=erase(routes[a],i);const auto removed=score(smaller);
                    for(int b=0;b<n_workers;++b)if(a!=b) {
                        for(int j=0;j<=routes[b].count;++j) {
                            if(!take())break;
                            if(removed+time_bound(inserted_time(routes[b],j,job,elapsed[b]),routes[b].capacity)-values[a]-values[b]<best) {
                                const auto inserted=insert(routes[b],j,job);
                                const auto delta=removed+score(inserted)-values[a]-values[b];
                                if(delta<best)best=delta,ai=i,best_b=b,bj=j,swap=false;
                            }
                            if(j==routes[b].count || protected_job[routes[b].nodes[j]] ||
                                nodes[job].need[1]!=nodes[routes[b].nodes[j]].need[1])continue;
                            if(time_bound(replaced_time(routes[a],i,routes[b].nodes[j],elapsed[a]),routes[a].capacity)+
                                time_bound(replaced_time(routes[b],j,job,elapsed[b]),routes[b].capacity)-values[a]-values[b]>=best)continue;
                            auto x=copy_route(routes[a]),y=copy_route(routes[b]);std::swap(x.nodes[i],y.nodes[j]);
                            const auto swapped=score(x)+score(y)-values[a]-values[b];
                            if(swapped<best)best=swapped,ai=i,best_b=b,bj=j,swap=true;
                        }
                    }
                }
                if(best_b>=0) {
                    if(swap)std::swap(routes[a].nodes[ai],routes[best_b].nodes[bj]);
                    else {routes[best_b]=insert(routes[best_b],bj,routes[a].nodes[ai]);routes[a]=erase(routes[a],ai);}
                    values[a]=score(routes[a]);values[best_b]=score(routes[best_b]);
                    elapsed[a]=raw_time(routes[a]);elapsed[best_b]=raw_time(routes[best_b]);
                }
                if(exhausted)break;
            }
        }
        if(variant & EXCHANGE_ROUTE_TAILS) {
            int64_t values[MAX_WORKERS];for(int u=0;u<n_workers;++u)values[u]=score(routes[u]);
            for(int round=0;round<3;++round) {
                bool changed=false;
                for(int a=0;a<n_workers;++a) {
                    if(length(routes[a])<routes[a].capacity-2)continue;
                    int best_b=-1;int64_t best=0;Route best_a,best_other;
                    int64_t shortened[MAX_JOBS][3];
                    for(int i=0;i<routes[a].count;++i)std::fill_n(shortened[i],3,INT64_MIN);
                    auto summarize=[&](const Route& r,bool* closed,int* fertilizer,int* elapsed) {
                        int position[MAX_JOBS];std::fill_n(position,plan.count,-1);
                        std::fill_n(closed,r.count+1,true);fertilizer[0]=r.inv[FERTILIZER];elapsed[0]=0;int at=r.start;
                        for(int k=0;k<r.count;++k){position[r.nodes[k]]=k;fertilizer[k+1]=fertilizer[k]-nodes[r.nodes[k]].need[1];
                            const auto& v=visits[r.nodes[k]];elapsed[k+1]=elapsed[k]+distance(at,v.tile)+v.work;at=v.tile;}
                        for(int k=0;k<r.count;++k) {
                            const auto& job=plan.jobs[r.nodes[k]];
                            if(!job.carrier_bound || job.predecessor<0 || position[job.predecessor]<0)continue;
                            const int p=position[job.predecessor];
                            for(int cut=std::min(p,k)+1;cut<=std::max(p,k);++cut)closed[cut]=false;
                        }
                    };
                    auto bound=[](int time,int capacity) {const int over=std::max(0,time-capacity);return int64_t(over)*over*10000+int64_t(over)*100000+time*10+int64_t(time)*time;};
                    auto end_at=[&](const Route& r,int k){return k?int(visits[r.nodes[k-1]].tile):r.start;};
                    auto suffix=[&](const Route& r,const int* elapsed,int k,int at) {
                        return k==r.count?0:elapsed[r.count]-elapsed[k]-distance(end_at(r,k),visits[r.nodes[k]].tile)+distance(at,visits[r.nodes[k]].tile);
                    };
                    bool a_closed[MAX_JOBS+1];int a_f[MAX_JOBS+1],a_time[MAX_JOBS+1];summarize(routes[a],a_closed,a_f,a_time);
                    for(int b=0;b<n_workers;++b)if(a!=b) {
                        bool b_closed[MAX_JOBS+1];int b_f[MAX_JOBS+1],b_time[MAX_JOBS+1];summarize(routes[b],b_closed,b_f,b_time);
                        const bool finite=values[a]<INT64_MAX/8 && values[b]<INT64_MAX/8;
                        for(int i=0;i<routes[a].count;++i)for(int span=2;span<=4 && i+span<=routes[a].count;++span) {
                            auto& vx=shortened[i][span-2];
                            if(vx==INT64_MIN) {
                                auto x=copy_route(routes[a]);x.count=0;
                                for(int k=0;k<routes[a].count;++k)if(k<i || k>=i+span)x.nodes[x.count++]=routes[a].nodes[k];
                                vx=score(x);
                                if(vx<INT64_MAX/4)for(int k=i;k<i+span;++k) {
                                    const auto& job=plan.jobs[routes[a].nodes[k]];
                                    if(job.carrier_bound && job.predecessor>=0 &&
                                        std::find(routes[a].nodes+i,routes[a].nodes+i+span,job.predecessor)==routes[a].nodes+i+span){vx=INT64_MAX/4;break;}
                                }
                            }
                            if(vx>=INT64_MAX/4)continue;
                            if(finite && b_f[routes[b].count]+a_f[i+span]-a_f[i]<0)continue;
                            const int block_first=visits[routes[a].nodes[i]].tile,block_last=visits[routes[a].nodes[i+span-1]].tile;
                            const int block_time=a_time[i+span]-a_time[i]-distance(end_at(routes[a],i),block_first);
                            for(int j=0;j<=routes[b].count;++j) {
                                if(!take())break;
                                const int lower=b_time[j]+distance(end_at(routes[b],j),block_first)+block_time+suffix(routes[b],b_time,j,block_last);
                                if(vx+bound(lower,routes[b].capacity)-values[a]-values[b]>=best)continue;
                                auto y=copy_route(routes[b]);y.count=0;
                                for(int k=0;k<j;++k)y.nodes[y.count++]=routes[b].nodes[k];
                                for(int k=i;k<i+span;++k)y.nodes[y.count++]=routes[a].nodes[k];
                                for(int k=j;k<routes[b].count;++k)y.nodes[y.count++]=routes[b].nodes[k];
                                const auto delta=vx+score(y)-values[a]-values[b];
                                if(delta<best){best=delta;best_b=b;best_a=copy_route(routes[a]);best_a.count=0;
                                    for(int k=0;k<routes[a].count;++k)if(k<i || k>=i+span)best_a.nodes[best_a.count++]=routes[a].nodes[k];
                                    best_other=copy_route(y);}
                            }
                        }
                        for(int i=0;i<=routes[a].count;++i)for(int j=0;j<=routes[b].count;++j) {
                            if(finite && (!a_closed[i] || !b_closed[j] ||
                                a_f[i]+b_f[routes[b].count]-b_f[j]<0 || b_f[j]+a_f[routes[a].count]-a_f[i]<0))continue;
                            if(!take())break;
                            const int x_time=a_time[i]+suffix(routes[b],b_time,j,end_at(routes[a],i));
                            const int y_time=b_time[j]+suffix(routes[a],a_time,i,end_at(routes[b],j));
                            if(bound(x_time,routes[a].capacity)+bound(y_time,routes[b].capacity)-values[a]-values[b]>=best)continue;
                            auto x=copy_route(routes[a]),y=copy_route(routes[b]);x.count=i;y.count=j;
                            for(int k=j;k<routes[b].count;++k)x.nodes[x.count++]=routes[b].nodes[k];
                            for(int k=i;k<routes[a].count;++k)y.nodes[y.count++]=routes[a].nodes[k];
                            const auto delta=score(x)+score(y)-values[a]-values[b];
                            if(delta<best){best=delta;best_b=b;best_a=copy_route(x);best_other=copy_route(y);}
                        }
                    }
                    if(best_b>=0){routes[a]=copy_route(best_a);routes[best_b]=copy_route(best_other);
                        refine_route(routes[a]);refine_route(routes[best_b]);
                        values[a]=score(routes[a]);values[best_b]=score(routes[best_b]);changed=true;}
                    if(exhausted)break;
                }
                if(!changed || exhausted)break;
            }
        }
        out.overrun=0;
        for(int u=0;u<n_workers;++u) {
            const auto& route=routes[u];out.count[u]=route.count;
            std::copy_n(route.nodes,route.count,out.jobs[u]);
            out.predicted_length[u]=length(route);
            out.overrun+=std::max(0,out.predicted_length[u]-route.capacity);
        }
        out.evaluations=evaluations;out.truncated=exhausted;return out;
    }
};
}

void add_deliveries(const agent::AgentObservation& o, DayPlan& plan, const DayReturns* returns, int source_seed, bool reachable_sources) {
    const int original = plan.count;
    int reserve[N_PRODUCTS]{};
    for (int j = 0; j < original; ++j) for (int k = 0; k < plan.jobs[j].count; ++k) {
        reserve[WHEAT] += plan.jobs[j].steps[k].op == OP_FEED;
        reserve[FERTILIZER] += plan.jobs[j].steps[k].op == OP_FERTILIZE;
    }
    for (int it = 0; it < N_PRODUCTS; ++it) {
        const int stock = std::max(0, int(o.own.shed[it]) + plan.buy_items[it] - (it == FERTILIZER ? 0 : reserve[it]));
        int required[24], available = 0;
        for (int h = 0; h < 24; ++h) required[h] = returns ? returns->target[h][it] : std::max(0, plan.sell_target[h][it] - stock);
        if (required[23] <= 0) continue;
        int carriers[MAX_WORKERS], carrier_count=0;
        for(int u=0;u<o.self().n_units;++u) if(o.own.inv[u][it]>0) carriers[carrier_count++]=u;
        std::sort(carriers,carriers+carrier_count,[&](int a,int b) {
            const int da=shed_distance(o.self().pos_y[a]*10+o.self().pos_x[a]);
            const int db=shed_distance(o.self().pos_y[b]*10+o.self().pos_x[b]);
            return da!=db ? da<db : a<b;
        });
        for(int k=0;k<carrier_count && available<required[23];++k) {
            const int u=carriers[k],quantity=std::min(int(o.own.inv[u][it]),required[23]-available);
            int due=o.hour;
            while(due<23 && required[due]<=available)++due;
            int j=original;
            while(j<plan.count && !(plan.jobs[j].depot && plan.jobs[j].worker==u))++j;
            if(j==plan.count) {
                assert(plan.count<MAX_JOBS);
                Job depot; depot.tile=shed_cell(o.self().pos_y[u]*10+o.self().pos_x[u]);
                depot.depot=depot.carrier_bound=true; depot.worker=u;
                plan.jobs[plan.count++]=depot;
            }
            auto& depot=plan.jobs[j]; assert(depot.count<MAX_STEPS);
            depot.steps[depot.count++]={OP_PLACE,uint8_t(it),quantity};
            depot.deadline=std::min(int(depot.deadline),due); available+=quantity;
        }
        int sources[MAX_JOBS], count = 0;
        for (int j = 0; j < original; ++j) {
            const auto& job = plan.jobs[j];
            if (job.tile < 0 || job.carrier_bound) continue;
            const auto& tile = o.self().tiles[job.tile / 10][job.tile % 10];
            if (harvest_output(job, 0, tile, o.day, it) > 0) sources[count++] = j;
        }
        std::sort(sources, sources + count, [&](int a, int b) {
            const int da = shed_distance(plan.jobs[a].tile), db = shed_distance(plan.jobs[b].tile);
            return da != db ? da < db : a < b;
        });
        // Explicit deposits can redistribute inputs for later pickup and use.
        // Only a sale target requires reserving fertilizer outside the shed.
        const int field_reserve=returns?0:std::max(0,reserve[FERTILIZER]-int(o.own.shed[FERTILIZER])-plan.buy_items[FERTILIZER]);
        const int return_sources = it == FERTILIZER ? std::max(0,count-field_reserve) : count;
        for (int s = 0; s < return_sources && available < required[23]; ++s) {
            int due = 23;
            for (int h = 0; h < 24; ++h) if (required[h] > available) { due = h; break; }
            if(source_seed && due>=4) {
                int best=s,best_cost=INT_MAX;
                for(int k=s;k<count;++k) {
                    const auto& job=plan.jobs[sources[k]];
                    const int d=shed_distance(job.tile),duration=2*d+job.count;
                    int cost=10*d+10*job.count+1000*std::max(0,duration-due);
                    if(source_seed==1) {
                        int detour=2*d;
                        for(int j=original;j<plan.count;++j)if(plan.jobs[j].depot && plan.jobs[j].predecessor>=0 && plan.jobs[j].deadline<=due && plan.jobs[j].deadline>=due-4) {
                            const int tile=plan.jobs[plan.jobs[j].predecessor].tile;
                            detour=std::min(detour,distance(tile,job.tile)+d-shed_distance(tile));
                            if(tile==job.tile)cost-=30;
                        }
                        cost+=20*detour;
                    } else {
                        const auto& tile=o.self().tiles[job.tile/10][job.tile%10];
                        const int yield=harvest_output(job,0,tile,o.day,it);
                        cost=cost*4/std::max(1,yield);
                        if(s && due>=8)cost+=10*distance(plan.jobs[sources[s-1]].tile,job.tile);
                    }
                    if(cost<best_cost)best_cost=cost,best=k;
                }
                std::swap(sources[s],sources[best]);
            }
            auto earliest=[&](int index) {
                const auto& job=plan.jobs[sources[index]];
                int work=0;
                do {++work;} while(work<job.count && job.steps[work-1].op!=OP_HARVEST && job.steps[work-1].op!=OP_COLLECT_FERTILIZER);
                int arrival=24;
                for(int u=0;u<o.self().n_units;++u)
                    arrival=std::min(arrival,o.hour+distance(o.self().pos_y[u]*10+o.self().pos_x[u],job.tile));
                if(plan.hires+1>o.self().n_units) {
                    const int birth=first_hire_wave(plan,10)>0?1:2;
                    arrival=std::min(arrival,std::max(o.hour+1,birth)+shed_distance(job.tile));
                }
                return arrival+work+shed_distance(job.tile); // Deposit hour.
            };
            // Change the usual source choice only when even an isolated
            // worker cannot deliver it in time. Bounds guide selection only.
            if(reachable_sources && earliest(s)>due) {
                int best=s,when=earliest(s);
                for(int k=s+1;k<count;++k) {
                    const int candidate=earliest(k);
                    if(candidate<when){when=candidate;best=k;}
                    if(when<=due)break;
                }
                std::swap(sources[s],sources[best]);
            }
            const int j = sources[s];
            auto& producer = plan.jobs[j];
            const auto& tile = o.self().tiles[producer.tile / 10][producer.tile % 10];
            int quantity = harvest_output(producer, 0, tile, o.day, it);
            quantity = std::min(quantity, required[23] - available);
            int harvest = 0;
            while (harvest < producer.count && producer.steps[harvest].op != OP_HARVEST && producer.steps[harvest].op != OP_COLLECT_FERTILIZER) ++harvest;
            assert(harvest < producer.count && plan.count + 2 <= MAX_JOBS);
            if (harvest + 1 < producer.count) {
                Job rest = producer;
                rest.count -= harvest + 1;
                for (int k = 0; k < rest.count; ++k) rest.steps[k] = producer.steps[k + harvest + 1];
                rest.predecessor = j; rest.new_site = false;
                for(int child=0;child<original;++child) if(plan.jobs[child].predecessor==j) plan.jobs[child].predecessor=plan.count;
                plan.jobs[plan.count++] = rest; producer.count = harvest + 1;
            }
            if(producer.service_predecessor>=0) {
                auto& prefix=plan.jobs[producer.service_predecessor];
                prefix.deadline=std::min(int(prefix.deadline),std::max(0,due-shed_distance(producer.tile)-1));
            }
            producer.carrier_bound = true;
            Job depot; depot.tile = shed_cell(producer.tile); depot.key = -1;
            depot.count = 1; depot.steps[0] = {OP_PLACE, uint8_t(it), quantity};
            depot.depot = depot.carrier_bound = true; depot.predecessor = j; depot.deadline = due;
            plan.jobs[plan.count++] = depot;
            available += quantity;
        }
    }
    // Only returned producers need cooperative service. Keep ordinary
    // harvest/replant visits together when no transport chain selected them.
    for(int j=0;j<original;++j) {
        auto& job=plan.jobs[j];
        if(job.service_predecessor<0 || job.carrier_bound)continue;
        auto& prefix=plan.jobs[job.service_predecessor];
        assert(job.count+prefix.count<=MAX_STEPS);
        for(int k=job.count-1;k>=0;--k)job.steps[k+prefix.count]=job.steps[k];
        std::copy_n(prefix.steps,prefix.count,job.steps);job.count+=prefix.count;
        prefix.count=0;job.service_predecessor=-1;job.prior_service=0;
    }

}

Routes construct_routes(const agent::AgentObservation& o, DayPlan& plan, int rounds, int variant, int auto_hires, const agent::DecisionBudget& budget) {
    Builder b{o, plan, budget}; b.variant = variant;
    b.limited = budget.max_expansions != UINT64_MAX || budget.soft_deadline != agent::DecisionBudget::Clock::time_point::max() ||
                budget.hard_deadline != agent::DecisionBudget::Clock::time_point::max();
    b.sites(); b.build_nodes(); b.merge_deliveries();
    for (int n = 0; n < b.n_nodes; ++n)
        for (int it=2;it<5;++it)
            b.animal_cargo |= b.nodes[n].need[it] != 0 || b.nodes[n].required[it] != 0;
    if (auto_hires >= 0) {
        const int cap = plan.hires;
        int work = 0;
        for (int n = 0; n < b.n_nodes; ++n) work += b.nodes[n].work;
        int hires = std::clamp((work + 23) / 24 - 1, 0, cap);
        for (; hires < cap; ++hires) {
            plan.hires = hires; b.initialize();
            if (b.exhausted) { plan.hires = cap; return b.finish(); }
            if (b.finish().overrun == 0) break;
        }
        plan.hires = work ? std::min(cap, hires + auto_hires) : 0;
    }
    b.initialize(); b.improve(rounds);
    if (variant & 64) b.swap_overloaded();
    if (variant & 256) b.improve_new_sites();
    auto result=b.finish();
    return variant & REORDER_ROUTE_JOBS ? b.reorder_jobs(result) : result;
}
}
