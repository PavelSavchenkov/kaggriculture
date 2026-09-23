#include "intent.hpp"
#include <algorithm>
#include <bit>
#include <climits>
#include <cstdlib>
#include <tuple>
#include <vector>

namespace kag::day_compiler {
int DayIntent::new_crops(int product) const {
    switch (product) {
    case WHEAT: return new_wheat_count;
    case CARROT: return new_carrot_count;
    case MELON: return new_melon_count;
    case TOMATO: return new_tomato_count;
    case STRAWBERRY: return new_strawberry_count;
    default: std::abort();
    }
}
int DayIntent::animal_target(int species) const {
    switch (species) {
    case GOOSE: return target_geese_next_dawn;
    case COW: return target_cows_next_dawn;
    case SHEEP: return target_sheep_next_dawn;
    default: std::abort();
    }
}
namespace {
constexpr int64_t IMPOSSIBLE = int64_t{1} << 50;
CropState crop_state(const Observation& o, const Tile& t) {
    const auto& crop = CROPS[t.what]; const int age = o.day - t.planted_day;
    return {t.what, int16_t(age), t.yield_units, t.consecutive_dry, int16_t(t.fertilized_until_day - o.day),
            int16_t(t.max_lifespan_step < 0 ? -32768 : t.max_lifespan_step - o.step),
            int16_t(crop.ongoing && age >= crop.first_yield_day ? (age - crop.first_yield_day) % crop.interval + 1 : 0),
            t.watered_today};
}
AnimalState animal_state(const Observation& o, const Tile& t) {
    const auto& animal = ANIMALS[t.what - GOOSE]; const int age = o.day - t.planted_day;
    return {int16_t(t.what - GOOSE), int16_t(age),
            int16_t(age >= animal.first_yield_day ? (age - animal.first_yield_day) % animal.interval + 1 : 0),
            t.yield_units, t.consecutive_dry, t.pending_care_bonus, t.kind,
            t.fed_today, t.cared_today, t.fertilizer_available};
}
bool same_goal(CropGoal a, CropGoal b) {
    return a.mode == b.mode && a.harvest_age == b.harvest_age && a.min_yield == b.min_yield;
}
Calendars calendars(Biology& biology, const Tile& tile, int day, CropGoal goal) {
    return CROPS[tile.what].ongoing ? biology.ongoing(tile, day, goal.mode == CropMode::Full) :
           biology.one_shot(tile, day, goal.harvest_age, goal.min_yield);
}
bool reachable(const Observation& o, const Tile& tile, int cell, CropGoal goal) {
    if (goal.mode == CropMode::Retire || CROPS[tile.what].ongoing || tile.max_lifespan_step < 0 ||
        goal.harvest_age > o.day - tile.planted_day || tile.yield_units < goal.min_yield) return true;
    const int expiry = tile.max_lifespan_step - o.step;
    const int first_loss = expiry < 0 ? ((-expiry) & 1) : expiry;
    const int deadline = first_loss + 2 * (tile.yield_units - goal.min_yield);
    const int farmer = o.self().pos_y[0] * BOARD + o.self().pos_x[0];
    return std::min(distance(farmer, cell), 1 + shed_distance(cell)) <= deadline;
}
void add_option(CropGroup& group, const Tile& tile, const Observation& o, Biology& biology, CropGoal goal) {
    for (int option = 0; option < group.option_count; ++option)
        if (same_goal(group.options[option].goal, goal)) return;
    CropProcedure option; option.goal = goal;
    if (goal.mode != CropMode::Retire) {
        const auto plans = calendars(biology, tile, o.day, goal);
        if (!plans.count) return;
        int selected = 0;
        for (int k = 1; k < plans.count; ++k)
            if (std::pair{plans.plans[k].waters + plans.plans[k].fertilizers, plans.plans[k].fertilizers} <
                std::pair{plans.plans[selected].waters + plans.plans[selected].fertilizers, plans.plans[selected].fertilizers}) selected = k;
        option.water_actions = plans.plans[selected].waters;
        option.fertilizer_actions = plans.plans[selected].fertilizers;
    }
    for (int i = 0; i < group.count; ++i) option.maximum_count += reachable(o, tile, group.cells[i], goal);
    if (!option.maximum_count) return;
    if (group.option_count == MAX_CROP_OPTIONS) std::abort();
    group.options[group.option_count++] = option;
}
void procedures(CropGroup& group, const Observation& o, Biology& biology) {
    const int cell = group.cells[0]; const auto& tile = o.self().tiles[cell / BOARD][cell % BOARD];
    if (tile.what == WHEAT || tile.what == CARROT) {
        const int last = tile.what == WHEAT ? 4 : 3;
        for (int nominal = 2; nominal <= last; ++nominal) for (bool fertilizer : {false, true}) {
            const int target = std::max<int>(group.state.age, nominal);
            const auto plans = biology.one_shot(tile, o.day, target, 1);
            int yield = 0;
            for (int j = 0; j < plans.count; ++j)
                if (fertilizer || !plans.plans[j].fertilizers) yield = std::max(yield, int(plans.plans[j].yield));
            if (yield) add_option(group, tile, o, biology, {CropMode::Yield, uint8_t(target), uint8_t(yield)});
        }
    } else if (tile.what == MELON) {
        struct Candidate { CropGoal goal; Calendar calendar; };
        std::vector<Candidate> points;
        for (int target = std::max<int>(10, group.state.age); target <= 13; ++target)
            for (int yield = 1; yield <= 6; ++yield) {
                const auto plans = biology.one_shot(tile, o.day, target, yield);
                for (int j = 0; j < plans.count; ++j)
                    points.push_back({{CropMode::Yield, uint8_t(target), uint8_t(yield)}, plans.plans[j]});
            }
        // Preserve the default goal and every nondominated alternative, including
        // different current-day service choices with different routing costs.
        add_option(group, tile, o, biology, {CropMode::Yield, 10, 6});
        for (const auto& point : points) {
            bool dominated = false;
            const auto a = point.calendar;
            for (const auto& other : points) {
                const auto b = other.calendar;
                if ((a.water_days & 1) != (b.water_days & 1) || (a.fertilizer_days & 1) != (b.fertilizer_days & 1)) continue;
                const bool no_worse = other.goal.harvest_age <= point.goal.harvest_age && other.goal.min_yield >= point.goal.min_yield &&
                                      b.waters <= a.waters && b.fertilizers <= a.fertilizers;
                const bool better = other.goal.harvest_age < point.goal.harvest_age || other.goal.min_yield > point.goal.min_yield ||
                                    b.waters < a.waters || b.fertilizers < a.fertilizers;
                if (no_worse && better) { dominated = true; break; }
            }
            if (!dominated) add_option(group, tile, o, biology, point.goal);
        }
    } else if (o.day < 29) {
        add_option(group, tile, o, biology, {CropMode::Full, 0, 0});
        add_option(group, tile, o, biology, {CropMode::Lean, 0, 0});
    }
    add_option(group, tile, o, biology, {CropMode::Retire, 0, 0});
}
// Minimum-cost bipartite matching. Costs include hard per-entity reachability.
bool assign(int count, const int64_t (&cost)[100][100], int* assigned) {
    int64_t row_potential[101]{}, col_potential[101]{};
    int row_at[101]{}, predecessor[101]{};
    for (int row = 1; row <= count; ++row) {
        row_at[0] = row;
        int column = 0; int64_t minimum[101]; bool visited[101]{};
        std::fill_n(minimum, 101, INT64_MAX);
        do {
            visited[column] = true; const int current_row = row_at[column];
            int64_t delta = INT64_MAX; int next = 0;
            for (int j = 1; j <= count; ++j) if (!visited[j]) {
                const int64_t reduced = cost[current_row - 1][j - 1] - row_potential[current_row] - col_potential[j];
                if (reduced < minimum[j]) { minimum[j] = reduced; predecessor[j] = column; }
                if (minimum[j] < delta) { delta = minimum[j]; next = j; }
            }
            for (int j = 0; j <= count; ++j)
                if (visited[j]) { row_potential[row_at[j]] += delta; col_potential[j] -= delta; }
                else minimum[j] -= delta;
            column = next;
        } while (row_at[column]);
        do {
            const int previous = predecessor[column]; row_at[column] = row_at[previous]; column = previous;
        } while (column);
    }
    for (int j = 1; j <= count; ++j) {
        if (cost[row_at[j] - 1][j - 1] >= IMPOSSIBLE) return false;
        assigned[row_at[j] - 1] = j - 1;
    }
    return true;
}
}

IntentError IntentBinder::describe(const Observation& o, IntentSchema& schema) {
    schema = {};
    if (o.hour != 0 || o.day < 0 || o.day > 29 || o.self().n_units != 1) return IntentError::NotDawn;
    std::array<std::pair<CropState, uint8_t>, 100> crops;
    std::array<std::pair<AnimalState, uint8_t>, 100> animals;
    int nc = 0, na = 0;
    for (int cell = 0; cell < 100; ++cell) {
        const auto& tile = o.self().tiles[cell / BOARD][cell % BOARD];
        if (tile.has_animal) animals[na++] = {animal_state(o, tile), uint8_t(cell)};
        else if (tile.kind == T_PLANT) crops[nc++] = {crop_state(o, tile), uint8_t(cell)};
    }
    std::sort(crops.begin(), crops.begin() + nc); std::sort(animals.begin(), animals.begin() + na);
    for (int i = 0; i < nc; ++i) {
        if (!i || crops[i].first != crops[i - 1].first) schema.crops[schema.crop_group_count++].state = crops[i].first;
        auto& group = schema.crops[schema.crop_group_count - 1]; group.cells[group.count++] = crops[i].second;
    }
    for (int i = 0; i < na; ++i) {
        if (!i || animals[i].first != animals[i - 1].first) schema.animals[schema.animal_group_count++].state = animals[i].first;
        auto& group = schema.animals[schema.animal_group_count - 1]; group.cells[group.count++] = animals[i].second;
    }
    for (int i = 0; i < schema.crop_group_count; ++i) procedures(schema.crops[i], o, biology_);
    return IntentError::None;
}
IntentError IntentBinder::bind(const Observation& o, const DayIntent& intent, detail::BoundIntent& bound, IntentSchema* output_schema,
                             bool today_first) {
    IntentSchema local_schema; auto& schema = output_schema ? *output_schema : local_schema;
    auto error = describe(o, schema); if (error != IntentError::None) return error;
    bound = {};
    int new_count = 0;
    for (int product = 0; product < N_CROPS; ++product) {
        const int count = intent.new_crops(product);
        if (count < 0 || count > 100) return IntentError::InvalidCount;
        new_count += count; bound.new_crops[product] = count;
    }
    if (new_count > 100) return IntentError::InvalidCount;
    for (int species = GOOSE; species <= SHEEP; ++species) {
        const int count = intent.animal_target(species);
        if (count < 0 || count > 100) return IntentError::InvalidCount;
        bound.animal_target[species - GOOSE] = count;
    }
    if (intent.buy_next_land_today && o.self().n_quadrants == 4) return IntentError::NoLand;
    bound.buy_land = intent.buy_next_land_today;
    for (int i = 0; i < MAX_GROUPS; ++i) {
        if (i >= schema.crop_group_count) {
            for (int count : intent.crops[i].counts) if (count) return IntentError::UnusedSlot;
            continue;
        }
        const auto& group = schema.crops[i]; int partition = 0, slots[100], size = 0;
        for (int option = 0; option < MAX_CROP_OPTIONS; ++option) {
            const int count = intent.crops[i].counts[option];
            if (option >= group.option_count && count) return IntentError::UnusedSlot;
            if (count < 0 || count > group.count) return IntentError::InvalidCount;
            if (!count) continue;
            if (count > group.options[option].maximum_count || size + count > 100) return IntentError::UnreachableGoal;
            partition += count;
            for (int j = 0; j < count; ++j) slots[size++] = option;
        }
        if (partition != group.count) return IntentError::CropPartition;
        int64_t costs[100][100]; int assignment[100], current[MAX_CROP_OPTIONS]{}, maximum_future=0;
        if(today_first) for(int option_index=0;option_index<group.option_count;++option_index) {
            const auto& option=group.options[option_index];
            if(option.goal.mode==CropMode::Retire) continue;
            const auto& tile=o.self().tiles[group.cells[0]/BOARD][group.cells[0]%BOARD];
            const auto plans=calendars(biology_,tile,o.day,option.goal);
            int selected=0;
            auto key=[](const Calendar& c) { return std::tuple{c.fertilizers,int(c.waters+c.fertilizers),int((c.water_days&1)+(c.fertilizer_days&1))}; };
            for(int k=1;k<plans.count;++k) if(key(plans.plans[k])<key(plans.plans[selected])) selected=k;
            const auto& calendar=plans.plans[selected];
            current[option_index]=(calendar.water_days&1)+2*(calendar.fertilizer_days&1)+
                int(option.goal.mode==CropMode::Yield && option.goal.harvest_age<=group.state.age);
            maximum_future=std::max(maximum_future,1+option.water_actions+2*option.fertilizer_actions);
        }
        // Lexicographic warm start: minimize today's geometric workload first,
        // then the old remaining-calendar cost. No semantic count is changed.
        const int64_t scale=1+int64_t(group.count)*maximum_future*50;
        for (int row = 0; row < group.count; ++row) {
            const auto& option = group.options[slots[row]];
            const int work = option.goal.mode == CropMode::Retire ? 0 : 1 + option.water_actions + 2 * option.fertilizer_actions;
            for (int column = 0; column < group.count; ++column) {
                const int cell = group.cells[column]; const auto& tile = o.self().tiles[cell / BOARD][cell % BOARD];
                costs[row][column] = reachable(o, tile, cell, option.goal) ?
                                     (work+scale*current[slots[row]]) * (4 * shed_distance(cell) + distance(44, cell)) : IMPOSSIBLE;
            }
        }
        if (!assign(group.count, costs, assignment)) return IntentError::UnreachableGoal;
        for (int row = 0; row < group.count; ++row) bound.crops[group.cells[assignment[row]]] = group.options[slots[row]].goal;
    }
    int retained[3]{};
    for (int i = 0; i < MAX_GROUPS; ++i) {
        const int serve = intent.animals[i].serve_today_count, escape = intent.animals[i].allow_escape_tonight_count;
        if (i >= schema.animal_group_count) {
            if (serve || escape) return IntentError::UnusedSlot;
            continue;
        }
        const auto& group = schema.animals[i];
        if (serve < 0 || escape < 0 || serve + escape > group.count) return IntentError::AnimalPartition;
        if ((o.day == 29 && (serve || escape)) || (escape && (group.state.unfed < 1 || group.state.fed_today)))
            return IntentError::ImpossibleEscape;
        if (o.day < 29 && group.state.unfed >= 1 && !group.state.fed_today && serve + escape != group.count)
            return IntentError::AnimalPartition;
        auto cells = group.cells;
        std::sort(cells.begin(), cells.begin() + group.count, [](int a, int b) {
            return std::pair{shed_distance(a), a} < std::pair{shed_distance(b), b};
        });
        for (int j = 0; j < serve; ++j) bound.serve[cells[j]] = true;
        for (int j = group.count - escape; j < group.count; ++j) bound.escape[cells[j]] = true;
        retained[group.state.species] += group.count - escape;
    }
    for (int species = 0; species < 3; ++species)
        if (bound.animal_target[species] < retained[species]) return IntentError::AnimalTarget;
    return IntentError::None;
}
}
