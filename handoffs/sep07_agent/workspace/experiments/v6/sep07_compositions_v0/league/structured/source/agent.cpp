#include "agent.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <limits>

namespace league::test::structured_economic_policy {
namespace {

constexpr int TOTAL_DAYS = 30;
constexpr int LIQUIDATION_TURNS = 22;
constexpr int FEED_STOCK_DAYS = 3;
constexpr int CORE_HERD = 4;
constexpr int MID_HERD = 11;
constexpr int TARGET_HERD = 15;
constexpr int HERD_EXPANSION_DAY = 7;
constexpr int HERD_FINAL_DAY = 11;
constexpr int ANIMAL_PURCHASE_LAST_DAY = 18;
constexpr int MELON_TILES_MIN = 8;
constexpr int MELON_TILES_BASE = 10;
constexpr int MELON_TILES_MAX = 12;
constexpr int MAX_EXTRA_LAND = 2;
constexpr int SCHEDULE_WHEAT_CAP = 100;
constexpr double TRAVEL_COST = 8.0;
constexpr std::array<int, kag::N_CROPS> LAST_PLANT{24, 25, 19, 18, 18};
constexpr std::array<int, kag::N_CROPS> RIPE_DAY{4, 3, 8, 10, 10};
constexpr std::array<int, 4> ANIMAL_SLOTS{4, 7, 4, 0};
constexpr std::array<int, 2> LAND_OPEN_DAYS{5, 9};
constexpr std::array<int, 3> LAND_PRICES{1000, 2000, 4000};
constexpr std::array<double, kag::N_PRODUCTS> RESERVE_FRACTION{
    0.68, 0.55, 0.50, 0.48, 0.58, 0.65, 0.42, 0.40, 0.18,
};
constexpr std::array<int, kag::N_ITEMS> LEXICAL_ITEM_RANK{
    7, 0, 6, 5, 3, 1, 4, 8, 2, 10, 9, 11,
};
constexpr std::array<double, 7> PRIORITY_BONUS{
    120000.0, 100000.0, 1500.0, 750.0, 250.0, 0.0, -100.0,
};
constexpr std::array<int, 4> CORE_HERD_SEQUENCE{
    kag::COW, kag::COW, kag::COW, kag::SHEEP,
};

struct Pos {
    int x = 0;
    int y = 0;
};

bool same(Pos left, Pos right) {
    return left.x == right.x && left.y == right.y;
}

int distance(Pos left, Pos right) {
    return std::abs(left.x - right.x) + std::abs(left.y - right.y);
}

int quadrant(Pos position, int board_size) {
    const int half = board_size / 2;
    return (position.y < half ? 0 : 2) + (position.x < half ? 0 : 1);
}

struct PosList {
    std::array<Pos, kag::BOARD * kag::BOARD> values{};
    int size = 0;

    void push(Pos value) { values[size++] = value; }
};

PosList shed_tiles(const kag::agent::PublicFarm& farm, int board_size) {
    const int half = board_size / 2;
    const std::array<Pos, 4> candidates{{
        {half - 1, half - 1}, {half, half - 1},
        {half - 1, half}, {half, half},
    }};
    PosList result;
    for (const Pos position : candidates)
        if (farm.tiles[position.y][position.x].kind != kag::T_LOCKED)
            result.push(position);
    if (result.size == 0) result.push(candidates[0]);
    return result;
}

Pos nearest_shed(const kag::agent::PublicFarm& farm, Pos source,
                 int board_size) {
    const PosList sheds = shed_tiles(farm, board_size);
    Pos best = sheds.values[0];
    for (int index = 1; index < sheds.size; ++index) {
        const Pos candidate = sheds.values[index];
        const int candidate_distance = distance(source, candidate);
        const int best_distance = distance(source, best);
        if (candidate_distance < best_distance ||
            (candidate_distance == best_distance &&
             (candidate.y < best.y ||
              (candidate.y == best.y && candidate.x < best.x))))
            best = candidate;
    }
    return best;
}

kag::UnitAction bfs_first_step(const kag::agent::PublicFarm& farm,
                               Pos source, Pos target, int board_size) {
    if (same(source, target)) return {};
    constexpr std::array<int, 4> DX{0, -1, 0, 1};
    constexpr std::array<int, 4> DY{-1, 0, 1, 0};
    constexpr std::array<uint8_t, 4> OP{
        kag::OP_NORTH, kag::OP_WEST, kag::OP_SOUTH, kag::OP_EAST,
    };
    std::array<int, kag::BOARD * kag::BOARD> queue{};
    std::array<int, kag::BOARD * kag::BOARD> parent{};
    std::array<uint8_t, kag::BOARD * kag::BOARD> parent_move{};
    parent.fill(-2);
    const int start = source.y * kag::BOARD + source.x;
    const int finish = target.y * kag::BOARD + target.x;
    int head = 0;
    int tail = 0;
    queue[tail++] = start;
    parent[start] = -1;
    while (head < tail) {
        const int current = queue[head++];
        if (current == finish) break;
        const int x = current % kag::BOARD;
        const int y = current / kag::BOARD;
        for (int move = 0; move < 4; ++move) {
            const int nx = x + DX[move];
            const int ny = y + DY[move];
            if (nx < 0 || nx >= board_size || ny < 0 || ny >= board_size)
                continue;
            const int next = ny * kag::BOARD + nx;
            if (parent[next] != -2 || farm.tiles[ny][nx].kind == kag::T_LOCKED)
                continue;
            parent[next] = current;
            parent_move[next] = OP[move];
            queue[tail++] = next;
        }
    }
    if (parent[finish] == -2) return {};
    int current = finish;
    while (parent[current] != start) {
        current = parent[current];
        if (current < 0) return {};
    }
    return {parent_move[current], 0, 1};
}

int town_demand(const kag::agent::AgentObservation& observation, int item) {
    int demand = item == kag::FERTILIZER ? 0 : 1;
    for (int index = 0; index < observation.n_shops; ++index) {
        const int shop = observation.shops[index];
        if ((kag::SHOP_MASK[shop] & (1u << item)) != 0)
            demand += 6 * kag::SHOP_MULT[shop];
    }
    return demand;
}

int private_total(const kag::agent::PrivateFarm& own, int item,
                  int n_units) {
    int result = own.shed[item];
    for (int unit = 0; unit < n_units; ++unit) result += own.inv[unit][item];
    return result;
}

std::array<int, kag::N_ANIMALS> farm_animal_counts(
    const kag::agent::PublicFarm& farm, int board_size) {
    std::array<int, kag::N_ANIMALS> counts{};
    for (int y = 0; y < board_size; ++y)
        for (int x = 0; x < board_size; ++x) {
            const kag::Tile& tile = farm.tiles[y][x];
            if (tile.has_animal && kag::is_animal(tile.what))
                ++counts[tile.what - kag::GOOSE];
        }
    return counts;
}

std::array<int, kag::N_ANIMALS> opponent_animal_counts(
    const kag::agent::AgentObservation& observation, int board_size) {
    return farm_animal_counts(observation.opponent(), board_size);
}

int opponent_visible_supply(const kag::agent::AgentObservation& observation,
                            int item, int horizon, int board_size) {
    int total = 0;
    const kag::agent::PublicFarm& farm = observation.opponent();
    for (int y = 0; y < board_size; ++y)
        for (int x = 0; x < board_size; ++x) {
            const kag::Tile& tile = farm.tiles[y][x];
            if (item < kag::N_CROPS && tile.kind == kag::T_PLANT &&
                tile.what == item) {
                const int age = observation.day - tile.planted_day;
                const int held = tile.yield_units;
                if (held > 0 && age >= kag::CROPS[item].first_yield_day)
                    total += held;
                else if (age + horizon >= RIPE_DAY[item])
                    total += std::max(1, std::min(
                        kag::CROPS[item].max_yield, held + horizon));
            } else if (item >= kag::EGG && item <= kag::WOOL &&
                       tile.has_animal) {
                const int animal = item == kag::EGG ? kag::GOOSE :
                    item == kag::MILK ? kag::COW : kag::SHEEP;
                if (tile.what == animal) {
                    total += tile.yield_units;
                    if (horizon > 0)
                        total += std::min(2 * horizon,
                            kag::ANIMALS[animal - kag::GOOSE].max_held);
                }
            }
        }
    return total;
}

int melon_target(const kag::agent::AgentObservation& observation,
                 int board_size) {
    const int price = observation.market.prices[kag::MELON];
    int opponent_tiles = 0;
    const kag::agent::PublicFarm& farm = observation.opponent();
    for (int y = 0; y < board_size; ++y)
        for (int x = 0; x < board_size; ++x) {
            const kag::Tile& tile = farm.tiles[y][x];
            opponent_tiles += tile.kind == kag::T_PLANT &&
                tile.what == kag::MELON;
        }
    if (price >= 300 && opponent_tiles <= 5) return MELON_TILES_MAX;
    if (price <= 170 || opponent_tiles >= 12) return MELON_TILES_MIN;
    if (opponent_tiles >= 9) return MELON_TILES_BASE - 1;
    return MELON_TILES_BASE;
}

double livestock_score(const kag::agent::AgentObservation& observation,
                       int animal, int own_count, int opponent_count) {
    const kag::AnimalDef& rule = kag::ANIMALS[animal - kag::GOOSE];
    const int product = rule.product;
    const double normalized = observation.market.prices[product] /
        kag::MARKET[product].base;
    const double support = 1.0 + 0.012 * town_demand(observation, product);
    const double crowding = 1.0 + 0.18 * opponent_count + 0.08 * own_count;
    return normalized * support / crowding;
}

enum RoleKind : uint8_t { ROLE_CROP, ROLE_ANIMAL };

struct Role {
    Pos position{};
    RoleKind kind = ROLE_CROP;
    int item = 0;
};

struct RoleList {
    std::array<Role, kag::BOARD * kag::BOARD> values{};
    int size = 0;

    void push(Role role) { values[size++] = role; }
    int find(Pos position) const {
        for (int index = 0; index < size; ++index)
            if (same(values[index].position, position)) return index;
        return -1;
    }
};

struct Zone {
    PosList crops{};
};

void reserved_slots(const kag::agent::PublicFarm& farm, int board_size,
                    PosList& animals, std::array<Zone, 4>& zones) {
    const PosList sheds = shed_tiles(farm, board_size);
    for (int quad = 0; quad < 4; ++quad) {
        if (quad >= farm.n_quadrants) continue;
        struct Cell { int distance; int y; int x; };
        std::array<Cell, kag::BOARD * kag::BOARD> cells{};
        int n_cells = 0;
        for (int y = 0; y < board_size; ++y)
            for (int x = 0; x < board_size; ++x) {
                if (farm.tiles[y][x].kind == kag::T_LOCKED ||
                    quadrant({x, y}, board_size) != quad)
                    continue;
                int nearest = std::numeric_limits<int>::max();
                for (int index = 0; index < sheds.size; ++index)
                    nearest = std::min(nearest, distance({x, y}, sheds.values[index]));
                cells[n_cells++] = {nearest, y, x};
            }
        std::sort(cells.begin(), cells.begin() + n_cells,
                  [](const Cell& left, const Cell& right) {
            if (left.distance != right.distance)
                return left.distance < right.distance;
            if (left.y != right.y) return left.y < right.y;
            return left.x < right.x;
        });
        const int reserved = std::min(ANIMAL_SLOTS[quad], n_cells);
        for (int index = 0; index < reserved; ++index)
            animals.push({cells[index].x, cells[index].y});
        for (int index = reserved; index < n_cells; ++index)
            zones[quad].crops.push({cells[index].x, cells[index].y});
    }
}

std::array<int, 2> herd_targets(
    const kag::agent::AgentObservation& observation,
    const kag::agent::PublicFarm& farm, int capacity, int board_size) {
    const auto placed = farm_animal_counts(farm, board_size);
    std::array<int, 2> owned{
        placed[kag::COW - kag::GOOSE] +
            private_total(observation.own, kag::COW, farm.n_units),
        placed[kag::SHEEP - kag::GOOSE] +
            private_total(observation.own, kag::SHEEP, farm.n_units),
    };
    int stage = observation.day < HERD_EXPANSION_DAY ? CORE_HERD :
        observation.day < HERD_FINAL_DAY ? MID_HERD : TARGET_HERD;
    if (observation.day > ANIMAL_PURCHASE_LAST_DAY ||
        TOTAL_DAYS - observation.day < 8)
        stage = owned[0] + owned[1];
    const int target_total = std::min(capacity,
        std::max(owned[0] + owned[1], stage));
    std::array<int, 2> targets{
        std::max(target_total >= CORE_HERD ? 3 : 0, owned[0]),
        std::max(target_total >= CORE_HERD ? 1 : 0, owned[1]),
    };
    const auto opponents = opponent_animal_counts(observation, board_size);
    while (targets[0] + targets[1] < target_total) {
        const double cow = livestock_score(observation, kag::COW, targets[0],
            opponents[kag::COW - kag::GOOSE]);
        const double sheep = livestock_score(observation, kag::SHEEP, targets[1],
            opponents[kag::SHEEP - kag::GOOSE]);
        const bool choose_cow = cow > sheep ||
            (cow == sheep && (-targets[0] > -targets[1] ||
             (-targets[0] == -targets[1])));
        ++targets[choose_cow ? 0 : 1];
    }
    return targets;
}

RoleList role_plan(const kag::agent::AgentObservation& observation,
                   int board_size) {
    const kag::agent::PublicFarm& farm = observation.self();
    PosList animal_slots;
    std::array<Zone, 4> zones{};
    reserved_slots(farm, board_size, animal_slots, zones);
    const auto targets = herd_targets(observation, farm, animal_slots.size,
                                      board_size);
    const int desired = std::min(animal_slots.size, targets[0] + targets[1]);
    PosList active;
    for (int index = 0; index < desired; ++index)
        active.push(animal_slots.values[index]);
    for (int index = 0; index < animal_slots.size; ++index) {
        const Pos position = animal_slots.values[index];
        if (!farm.tiles[position.y][position.x].has_animal) continue;
        bool present = false;
        for (int active_index = 0; active_index < active.size; ++active_index)
            present = present || same(active.values[active_index], position);
        if (!present) active.push(position);
    }

    RoleList roles;
    std::array<int, 2> assigned{};
    for (int index = 0; index < active.size; ++index) {
        const Pos position = active.values[index];
        const kag::Tile& tile = farm.tiles[position.y][position.x];
        int animal = -1;
        if (tile.has_animal && (tile.what == kag::COW || tile.what == kag::SHEEP))
            animal = tile.what;
        else if (index < CORE_HERD &&
                 assigned[CORE_HERD_SEQUENCE[index] == kag::COW ? 0 : 1] <
                 targets[CORE_HERD_SEQUENCE[index] == kag::COW ? 0 : 1])
            animal = CORE_HERD_SEQUENCE[index];
        else {
            const int cow_remaining = targets[0] - assigned[0];
            const int sheep_remaining = targets[1] - assigned[1];
            const int cow_stock = private_total(
                observation.own, kag::COW, farm.n_units);
            const int sheep_stock = private_total(
                observation.own, kag::SHEEP, farm.n_units);
            const bool cow = cow_remaining > sheep_remaining ||
                (cow_remaining == sheep_remaining &&
                 (cow_stock > sheep_stock || cow_stock == sheep_stock));
            animal = cow ? kag::COW : kag::SHEEP;
        }
        roles.push({position, ROLE_ANIMAL, animal});
        ++assigned[animal == kag::COW ? 0 : 1];
    }

    const int melon = melon_target(observation, board_size);
    constexpr std::array<std::array<int, 3>, 4> FIXED{{
        {{4, 2, 10}}, {{4, 1, 0}}, {{4, 1, 0}}, {{5, 2, 0}},
    }};
    for (int quad = 0; quad < 4; ++quad) {
        const PosList& cells = zones[quad].crops;
        int melons = quad == 0 ? std::min(melon, cells.size) : FIXED[quad][2];
        const int wheat = FIXED[quad][0];
        const int carrot = FIXED[quad][1];
        const int strawberries = std::max(0, cells.size - melons - wheat - carrot);
        int cell = 0;
        const auto add = [&](int item, int count, RoleList& output,
                             int& cell_index) {
            for (int n = 0; n < count && cell_index < cells.size; ++n)
                output.push({cells.values[cell_index++], ROLE_CROP, item});
        };
        if (quad == 0) add(kag::MELON, melons, roles, cell);
        add(kag::STRAWBERRY, strawberries, roles, cell);
        add(kag::WHEAT, wheat, roles, cell);
        add(kag::CARROT, carrot, roles, cell);
        if (quad != 0) add(kag::MELON, melons, roles, cell);
    }
    return roles;
}

struct Summary {
    int animals = 0;
    int unfed = 0;
    int at_risk_animals = 0;
    int at_risk_crops = 0;
    int open_structures = 0;
    int structures_todo = 0;
    int plants = 0;
    int plantable = 0;
    int weeds = 0;
    int wheat_stock = 0;
    std::array<int, kag::N_ANIMALS> animal_stock{};
    int shed_load = 0;
    int carried_load = 0;
};

int inventory_total(const kag::Count* inventory, int excluded = -1) {
    int total = 0;
    for (int item = 0; item < kag::N_ITEMS; ++item)
        if (item != excluded) total += std::max<int>(0, inventory[item]);
    return total;
}

Summary survey(const kag::agent::AgentObservation& observation,
               const RoleList& roles, int board_size) {
    const auto& farm = observation.self();
    const auto& own = observation.own;
    Summary result;
    for (int y = 0; y < board_size; ++y)
        for (int x = 0; x < board_size; ++x) {
            const kag::Tile& tile = farm.tiles[y][x];
            if (tile.has_animal) {
                ++result.animals;
                result.unfed += !tile.fed_today;
                result.at_risk_animals += tile.consecutive_dry >= 1;
            } else if (tile.kind == kag::T_COOP || tile.kind == kag::T_PASTURE) {
                ++result.open_structures;
            } else if (tile.kind == kag::T_PLANT) {
                ++result.plants;
                result.at_risk_crops += tile.consecutive_dry >= 1;
            } else if (tile.kind == kag::T_WEED) {
                ++result.weeds;
            }
        }
    for (int index = 0; index < roles.size; ++index) {
        const Role& role = roles.values[index];
        const kag::Tile& tile = farm.tiles[role.position.y][role.position.x];
        if (role.kind == ROLE_ANIMAL && tile.kind == kag::T_EMPTY)
            ++result.structures_todo;
        else if (role.kind == ROLE_CROP && tile.kind == kag::T_EMPTY &&
                 observation.day <= LAST_PLANT[role.item])
            ++result.plantable;
    }
    result.wheat_stock = own.shed[kag::WHEAT];
    for (int animal = 0; animal < kag::N_ANIMALS; ++animal)
        result.animal_stock[animal] = own.shed[kag::GOOSE + animal];
    for (int item = 0; item < kag::N_ITEMS; ++item)
        result.shed_load += std::max<int>(0, own.shed[item]);
    for (int unit = 0; unit < farm.n_units; ++unit) {
        result.wheat_stock += own.inv[unit][kag::WHEAT];
        for (int animal = 0; animal < kag::N_ANIMALS; ++animal)
            result.animal_stock[animal] += own.inv[unit][kag::GOOSE + animal];
        result.carried_load += inventory_total(own.inv[unit]);
    }
    return result;
}

enum MissionKind : uint8_t { MISSION_FIELD, MISSION_PICKUP, MISSION_DROP };

struct Mission {
    MissionKind kind = MISSION_FIELD;
    int priority = 0;
    double value = 0;
    Pos target{};
    kag::UnitAction action{};
    int need = -1;
    int latest_hour = 23;
    int item = -1;
    int amount = 0;
    int eligible = -1;
};

struct MissionList {
    std::array<Mission, 256> values{};
    int size = 0;

    void push(const Mission& mission) {
        if (size >= static_cast<int>(values.size())) std::abort();
        values[size++] = mission;
    }
};

void add_job(MissionList& jobs, int priority, double value, Pos target,
             kag::UnitAction action, int need = -1, int latest_hour = 23) {
    Mission mission;
    mission.priority = priority;
    mission.value = value;
    mission.target = target;
    mission.action = action;
    mission.need = need;
    mission.latest_hour = latest_hour;
    jobs.push(mission);
}

bool animal_produces_tonight(const kag::Tile& tile,
                             const kag::AnimalDef& rule, int day) {
    const int days_since_first = day + 1 - tile.planted_day -
        rule.first_yield_day;
    return days_since_first >= 0 && days_since_first % rule.interval == 0;
}

void crop_jobs(const kag::agent::AgentObservation& observation,
               MissionList& jobs, const kag::Tile& tile, Pos target,
               bool liquidation) {
    const int crop = tile.what;
    if (crop < 0 || crop >= kag::N_CROPS) return;
    const kag::CropDef& rule = kag::CROPS[crop];
    const double price = observation.market.prices[crop];
    const int age = observation.day - tile.planted_day;
    const int amount = tile.yield_units;
    const bool critical = tile.consecutive_dry >= 1;
    if (liquidation) {
        if (amount > 0 && age >= rule.first_yield_day)
            add_job(jobs, 0, amount * price, target,
                    {kag::OP_HARVEST, 0, 1});
        return;
    }
    if (critical && !tile.watered_today) {
        add_job(jobs, 0, std::max(static_cast<double>(amount),
                                 rule.max_yield * 0.7) * price,
                target, {kag::OP_WATER, 0, 1});
        return;
    }
    if (rule.ongoing) {
        const int days_since_first = observation.day + 1 - tile.planted_day -
            rule.first_yield_day;
        const int production_index = days_since_first >= 0 ?
            days_since_first / rule.interval + 1 : 0;
        const bool produces = days_since_first >= 0 &&
            days_since_first % rule.interval == 0 &&
            production_index <= rule.max_yield;
        if (crop == kag::STRAWBERRY && produces &&
            tile.fertilized_until_day < observation.day)
            add_job(jobs, 2, std::max(100.0, price), target,
                    {kag::OP_FERTILIZE, 0, 1}, kag::FERTILIZER);
        if (amount >= rule.max_yield - 1 ||
            (amount > 0 && observation.day >= 27))
            add_job(jobs, 2, amount * price, target,
                    {kag::OP_HARVEST, 0, 1});
        else if (!tile.watered_today && age >= rule.first_yield_day - 1)
            add_job(jobs, 3, price, target, {kag::OP_WATER, 0, 1});
        return;
    }
    const bool ripe = age >= RIPE_DAY[crop] && amount > 0;
    const bool growth = (rule.max_yield_day + 1) / 2 <= age &&
        age <= rule.max_yield_day;
    if (ripe) {
        if (growth && !tile.watered_today && amount < rule.max_yield)
            add_job(jobs, 1, price, target, {kag::OP_WATER, 0, 1});
        else
            add_job(jobs, 2, amount * price, target,
                    {kag::OP_HARVEST, 0, 1});
    } else if (growth && !tile.watered_today) {
        add_job(jobs, 3, price, target, {kag::OP_WATER, 0, 1});
    }
}

MissionList field_jobs(const kag::agent::AgentObservation& observation,
                       const RoleList& base_roles, bool liquidation,
                       int board_size) {
    const auto& farm = observation.self();
    RoleList roles = base_roles;
    for (int y = 0; y < board_size; ++y)
        for (int x = 0; x < board_size; ++x) {
            const Pos position{x, y};
            if (roles.find(position) >= 0) continue;
            const kag::Tile& tile = farm.tiles[y][x];
            if (tile.kind == kag::T_PLANT && tile.what < kag::N_CROPS)
                roles.push({position, ROLE_CROP, tile.what});
            else if (tile.has_animal && kag::is_animal(tile.what))
                roles.push({position, ROLE_ANIMAL, tile.what});
        }

    MissionList jobs;
    const int left = TOTAL_DAYS - observation.day;
    for (int index = 0; index < roles.size; ++index) {
        const Role& role = roles.values[index];
        const Pos target = role.position;
        const kag::Tile& tile = farm.tiles[target.y][target.x];
        if (tile.kind == kag::T_EMPTY) {
            if (liquidation) continue;
            if (role.kind == ROLE_ANIMAL) {
                const auto& rule = kag::ANIMALS[role.item - kag::GOOSE];
                if (left >= rule.first_yield_day + 2)
                    add_job(jobs, 3, 420, target,
                        {rule.structure == kag::ST_COOP ? kag::OP_BUILD_COOP :
                         kag::OP_BUILD_PASTURE, 0, 1}, -1, 22);
            } else if (observation.hour <= 22 &&
                       observation.day <= LAST_PLANT[role.item] &&
                       observation.own.seeds[role.item] > 0) {
                const int expected = role.item == kag::WHEAT ? 4 :
                    kag::CROPS[role.item].max_yield;
                const double value = std::max(40.0,
                    0.65 * expected * observation.market.prices[role.item] -
                    kag::CROPS[role.item].seed);
                add_job(jobs, 4, value, target,
                    {kag::OP_PLANT, static_cast<uint8_t>(role.item), 1},
                    -1, 22);
            }
            continue;
        }
        if (tile.kind == kag::T_LOCKED) continue;
        if (role.kind == ROLE_ANIMAL &&
            (tile.kind == kag::T_WEED || tile.kind == kag::T_PLANT)) {
            if (!liquidation)
                add_job(jobs, 2, 500, target, {kag::OP_DIG, 0, 1}, -1, 22);
            continue;
        }
        if (tile.kind == kag::T_WEED) {
            if (!liquidation)
                add_job(jobs, 4, left > 5 ? 120 : 10, target,
                        {kag::OP_DIG, 0, 1}, -1, 22);
            continue;
        }
        if (tile.kind == kag::T_PLANT) {
            crop_jobs(observation, jobs, tile, target, liquidation);
            continue;
        }
        if (role.kind == ROLE_ANIMAL && !tile.has_animal) {
            const auto& rule = kag::ANIMALS[role.item - kag::GOOSE];
            const kag::TileKind wanted = rule.structure == kag::ST_COOP ?
                kag::T_COOP : kag::T_PASTURE;
            if (tile.kind == wanted) {
                if (!liquidation)
                    add_job(jobs, 1, 900, target,
                        {kag::OP_PLACE, static_cast<uint8_t>(role.item), 1},
                        role.item);
            } else if (tile.kind == kag::T_COOP || tile.kind == kag::T_PASTURE) {
                if (!liquidation)
                    add_job(jobs, 3, 250, target, {kag::OP_DIG, 0, 1});
            }
            continue;
        }
        if (!tile.has_animal) continue;
        const auto& animal = kag::ANIMALS[tile.what - kag::GOOSE];
        const int product = animal.product;
        if (liquidation) {
            if (tile.yield_units > 0)
                add_job(jobs, 0,
                    tile.yield_units * observation.market.prices[product],
                    target, {kag::OP_HARVEST, 0, 1});
            if (tile.fertilizer_available)
                add_job(jobs, 0, observation.market.prices[kag::FERTILIZER],
                    target, {kag::OP_COLLECT_FERTILIZER, 0, 1});
            continue;
        }
        if (!tile.fed_today) {
            const bool risk = tile.consecutive_dry >= 1;
            add_job(jobs, risk ? 0 : 1, risk ? 900 : 260, target,
                {kag::OP_FEED, 0, 1}, kag::WHEAT);
        }
        const int held = tile.yield_units;
        const bool produces = animal_produces_tonight(
            tile, animal, observation.day);
        const int production_gain = produces ? 1 + tile.pending_care_bonus : 0;
        if (held > 0 && (held >= 3 ||
            held + production_gain >= animal.max_held || observation.day >= 27))
            add_job(jobs, 2, held * observation.market.prices[product], target,
                    {kag::OP_HARVEST, 0, 1});
        if (tile.fertilizer_available)
            add_job(jobs, 2, observation.market.prices[kag::FERTILIZER], target,
                    {kag::OP_COLLECT_FERTILIZER, 0, 1});
        if (!tile.cared_today && observation.day <= 27 &&
            held + (produces ? 0 : tile.pending_care_bonus) + 1 <
                animal.max_held && observation.market.prices[product] >= 20)
            add_job(jobs, 3, observation.market.prices[product], target,
                    {kag::OP_CARE, 0, 1});
    }
    return jobs;
}

bool terminal_feasible(const kag::agent::PublicFarm& farm, Pos position,
                       Pos target, int actions_left, int board_size) {
    const PosList sheds = shed_tiles(farm, board_size);
    int return_distance = std::numeric_limits<int>::max();
    for (int index = 0; index < sheds.size; ++index)
        return_distance = std::min(return_distance,
                                   distance(target, sheds.values[index]));
    return distance(position, target) + 1 + return_distance + 1 <= actions_left;
}

struct Pair {
    double negative_score = 0;
    int distance = 0;
    int worker = 0;
    int mission = 0;
    int target_y = 0;
    int target_x = 0;
    Pos target{};
};

bool pair_less(const Pair& left, const Pair& right) {
    if (left.negative_score != right.negative_score)
        return left.negative_score < right.negative_score;
    if (left.distance != right.distance) return left.distance < right.distance;
    if (left.worker != right.worker) return left.worker < right.worker;
    if (left.mission != right.mission) return left.mission < right.mission;
    if (left.target_y != right.target_y) return left.target_y < right.target_y;
    return left.target_x < right.target_x;
}

double priority_bonus(int priority) {
    if (priority >= -1 && priority <= 5) return PRIORITY_BONUS[priority + 1];
    return -1000.0 * priority;
}

struct FieldPlan {
    bool liquidation = false;
};

FieldPlan unit_actions(const kag::agent::AgentObservation& observation,
                       const kag::agent::AgentConfig& config,
                       const RoleList& roles, kag::Action& action) {
    const auto& farm = observation.self();
    const auto& own = observation.own;
    const int board_size = config.board_size;
    const int final_step = config.episode_steps - 2;
    const int actions_left = std::max(0, final_step - observation.step + 1);
    const bool liquidation = actions_left <= LIQUIDATION_TURNS;
    const Summary summary = survey(observation, roles, board_size);
    MissionList jobs = field_jobs(observation, roles, liquidation, board_size);
    MissionList missions = jobs;
    std::array<int, kag::N_CROPS> seed_budget{};
    for (int crop = 0; crop < kag::N_CROPS; ++crop)
        seed_budget[crop] = own.seeds[crop];

    int feed_jobs = 0;
    bool critical_feed = false;
    for (int index = 0; index < jobs.size; ++index)
        if (jobs.values[index].need == kag::WHEAT) {
            ++feed_jobs;
            critical_feed = critical_feed || jobs.values[index].priority == 0;
        }
    int carried_wheat = 0;
    for (int unit = 0; unit < farm.n_units; ++unit)
        carried_wheat += own.inv[unit][kag::WHEAT];
    const int wheat_missing = std::max(0, feed_jobs - carried_wheat);
    int wheat_remaining = std::min(wheat_missing,
        static_cast<int>(own.shed[kag::WHEAT]));
    const int wheat_pickups = std::min(farm.n_units,
        static_cast<int>(std::ceil(wheat_remaining / 6.0)));
    for (int index = 0; index < wheat_pickups; ++index) {
        Mission mission;
        mission.kind = MISSION_PICKUP;
        mission.item = kag::WHEAT;
        mission.amount = std::min(6, wheat_remaining);
        wheat_remaining -= mission.amount;
        mission.priority = critical_feed ? 0 : 1;
        mission.value = critical_feed ? 900 : 500;
        missions.push(mission);
    }

    int fertilizer_jobs = 0;
    for (int index = 0; index < jobs.size; ++index)
        fertilizer_jobs += jobs.values[index].need == kag::FERTILIZER;
    int carried_fertilizer = 0;
    for (int unit = 0; unit < farm.n_units; ++unit)
        carried_fertilizer += own.inv[unit][kag::FERTILIZER];
    int fertilizer_remaining = std::min(
        std::max(0, fertilizer_jobs - carried_fertilizer),
        static_cast<int>(own.shed[kag::FERTILIZER]));
    const int fertilizer_pickups = std::min(farm.n_units,
        static_cast<int>(std::ceil(fertilizer_remaining / 4.0)));
    for (int index = 0; index < fertilizer_pickups; ++index) {
        Mission mission;
        mission.kind = MISSION_PICKUP;
        mission.item = kag::FERTILIZER;
        mission.amount = std::min(4, fertilizer_remaining);
        fertilizer_remaining -= mission.amount;
        mission.priority = 1;
        mission.value = 700;
        missions.push(mission);
    }

    for (int animal = kag::GOOSE; animal <= kag::SHEEP; ++animal) {
        int place_jobs = 0;
        for (int index = 0; index < jobs.size; ++index)
            place_jobs += jobs.values[index].need == animal;
        int carried = 0;
        for (int unit = 0; unit < farm.n_units; ++unit)
            carried += own.inv[unit][animal];
        const int count = std::min({
            std::max(0, place_jobs - carried),
            static_cast<int>(own.shed[animal]), 2, farm.n_units,
        });
        for (int index = 0; index < count; ++index) {
            Mission mission;
            mission.kind = MISSION_PICKUP;
            mission.item = animal;
            mission.amount = 1;
            mission.priority = 1;
            mission.value = 900;
            missions.push(mission);
        }
    }

    const int pressure = summary.shed_load + summary.carried_load;
    const bool cash_needed = observation.day < 22 && farm.money < 500;
    const PosList sheds = shed_tiles(farm, board_size);
    for (int unit = 0; unit < farm.n_units; ++unit) {
        int cash_units = 0;
        double cash_value = 0;
        for (int item = 0; item < kag::N_PRODUCTS; ++item) {
            const int quantity = std::max<int>(0, own.inv[unit][item]);
            cash_units += quantity;
            cash_value += quantity * observation.market.prices[item];
        }
        if (cash_units <= 0) continue;
        const bool has_feed = own.inv[unit][kag::WHEAT] > 0 && feed_jobs > 0;
        bool on_shed = false;
        for (int index = 0; index < sheds.size; ++index)
            on_shed = on_shed || same(
                {farm.pos_x[unit], farm.pos_y[unit]}, sheds.values[index]);
        const bool should_drop = liquidation || pressure >= 80 ||
            cash_units >= 20 || cash_value >= 2500 ||
            (cash_needed && cash_value >= 400) ||
            (on_shed && !has_feed && cash_needed);
        if (!should_drop) continue;
        Mission mission;
        mission.kind = MISSION_DROP;
        mission.eligible = unit;
        mission.priority = liquidation ? -1 : 2;
        mission.value = std::max(120.0, 0.22 * cash_value);
        missions.push(mission);
    }

    std::array<Pair, kag::MAX_UNITS * 256> pairs{};
    int n_pairs = 0;
    for (int worker = 0; worker < farm.n_units; ++worker) {
        const Pos position{farm.pos_x[worker], farm.pos_y[worker]};
        for (int mission_index = 0; mission_index < missions.size; ++mission_index) {
            const Mission& mission = missions.values[mission_index];
            if (mission.kind == MISSION_DROP && mission.eligible != worker)
                continue;
            Pos target;
            int travel = 0;
            if (mission.kind == MISSION_FIELD) {
                if (mission.need >= 0 && own.inv[worker][mission.need] <= 0)
                    continue;
                target = mission.target;
                travel = distance(position, target);
                if (observation.hour + travel > mission.latest_hour) continue;
                if (liquidation && !terminal_feasible(
                    farm, position, target, actions_left, board_size))
                    continue;
            } else {
                target = nearest_shed(farm, position, board_size);
                travel = distance(position, target);
                if (mission.kind == MISSION_PICKUP &&
                    own.inv[worker][mission.item] > 0)
                    continue;
            }
            const double score = priority_bonus(mission.priority) +
                mission.value - TRAVEL_COST * travel;
            pairs[n_pairs++] = {
                -score, travel, worker, mission_index,
                target.y, target.x, target,
            };
        }
    }
    std::sort(pairs.begin(), pairs.begin() + n_pairs, pair_less);

    action.clear();
    action.n_units = farm.n_units;
    std::fill_n(action.units, action.n_units, kag::UnitAction{});
    std::array<bool, kag::MAX_UNITS> used_workers{};
    std::array<bool, 256> used_missions{};
    struct TargetKey { Pos position; int op; };
    std::array<TargetKey, 256> used_targets{};
    int n_targets = 0;
    int drop_room = std::max(0, config.shed_capacity - summary.shed_load);
    for (int pair_index = 0; pair_index < n_pairs; ++pair_index) {
        const Pair& pair = pairs[pair_index];
        if (used_workers[pair.worker] || used_missions[pair.mission]) continue;
        const Mission& mission = missions.values[pair.mission];
        int target_op = -1;
        if (mission.kind == MISSION_FIELD) {
            const int op = mission.action.op;
            if ((liquidation &&
                 (op == kag::OP_HARVEST || op == kag::OP_COLLECT_FERTILIZER)) ||
                op == kag::OP_FERTILIZE)
                target_op = op;
            bool used = false;
            for (int index = 0; index < n_targets; ++index)
                used = used || (same(used_targets[index].position, pair.target) &&
                    used_targets[index].op == target_op);
            if (used) continue;
        }

        kag::UnitAction selected{};
        int plant_crop = -1;
        if (mission.kind == MISSION_FIELD) {
            if (mission.action.op == kag::OP_PLANT) {
                plant_crop = mission.action.arg;
                if (seed_budget[plant_crop] <= 0) continue;
            }
            selected = pair.distance == 0 ? mission.action :
                bfs_first_step(farm,
                    {farm.pos_x[pair.worker], farm.pos_y[pair.worker]},
                    pair.target, board_size);
        } else if (mission.kind == MISSION_PICKUP) {
            selected = pair.distance == 0 ? kag::UnitAction{
                kag::OP_PICKUP, static_cast<uint8_t>(mission.item), mission.amount} :
                bfs_first_step(farm,
                    {farm.pos_x[pair.worker], farm.pos_y[pair.worker]},
                    pair.target, board_size);
        } else {
            if (pair.distance != 0) {
                selected = bfs_first_step(farm,
                    {farm.pos_x[pair.worker], farm.pos_y[pair.worker]},
                    pair.target, board_size);
            } else {
                int cash_units = 0;
                int noncash = 0;
                for (int item = 0; item < kag::N_ITEMS; ++item) {
                    const int quantity = std::max<int>(0, own.inv[pair.worker][item]);
                    if (item < kag::N_PRODUCTS) cash_units += quantity;
                    else noncash += quantity;
                }
                if (cash_units <= 0 || drop_room <= 0) continue;
                if (noncash <= 0 && cash_units <= drop_room) {
                    selected = {kag::OP_DROP, 0, 1};
                    drop_room -= cash_units;
                } else {
                    int best_item = -1;
                    for (int item = 0; item < kag::N_PRODUCTS; ++item) {
                        if (own.inv[pair.worker][item] <= 0) continue;
                        if (best_item < 0 ||
                            observation.market.prices[item] >
                                observation.market.prices[best_item] ||
                            (observation.market.prices[item] ==
                                 observation.market.prices[best_item] &&
                             LEXICAL_ITEM_RANK[item] >
                                 LEXICAL_ITEM_RANK[best_item]))
                            best_item = item;
                    }
                    if (best_item < 0) continue;
                    const int quantity = std::min<int>(
                        own.inv[pair.worker][best_item], drop_room);
                    if (quantity <= 0) continue;
                    selected = {kag::OP_PLACE,
                        static_cast<uint8_t>(best_item), quantity};
                    drop_room -= quantity;
                }
            }
        }
        if (selected.op == kag::OP_PASS && pair.distance > 0) continue;
        if (plant_crop >= 0) --seed_budget[plant_crop];
        if (mission.kind == MISSION_FIELD)
            used_targets[n_targets++] = {pair.target, target_op};
        action.units[pair.worker] = selected;
        used_workers[pair.worker] = true;
        used_missions[pair.mission] = true;
    }
    return {liquidation};
}

struct Storage {
    std::array<int, kag::N_ITEMS> shed{};
    std::array<std::array<int, kag::N_ITEMS>, kag::MAX_UNITS> inv{};
    std::array<std::array<uint8_t, kag::N_ITEMS>, kag::MAX_UNITS> keys{};
    std::array<int, kag::MAX_UNITS> n_keys{};
};

int storage_shed_total(const Storage& storage) {
    int total = 0;
    for (int item = 0; item < kag::N_ITEMS; ++item) total += storage.shed[item];
    return total;
}

Storage post_field_storage(const kag::agent::AgentObservation& observation,
                           const kag::Action& action, int capacity) {
    Storage storage;
    for (int item = 0; item < kag::N_ITEMS; ++item)
        storage.shed[item] = std::max<int>(0, observation.own.shed[item]);
    for (int unit = 0; unit < observation.self().n_units; ++unit) {
        for (int item = 0; item < kag::N_ITEMS; ++item)
            storage.inv[unit][item] = std::max<int>(
                0, observation.own.inv[unit][item]);
        storage.n_keys[unit] = observation.own.inv_nkeys[unit];
        for (int key = 0; key < storage.n_keys[unit]; ++key)
            storage.keys[unit][key] = observation.own.inv_keys[unit][key];
    }
    for (int unit = 0; unit < action.n_units; ++unit) {
        const kag::UnitAction& unit_action = action.units[unit];
        auto& inventory = storage.inv[unit];
        if (unit_action.op == kag::OP_DROP) {
            for (int key = 0; key < storage.n_keys[unit]; ++key) {
                const int item = storage.keys[unit][key];
                const int room = std::max(0, capacity - storage_shed_total(storage));
                const int accepted = std::min(inventory[item], room);
                if (accepted > 0) storage.shed[item] += accepted;
                inventory[item] = 0;
            }
        } else if (unit_action.op == kag::OP_PLACE &&
                   unit_action.arg < kag::N_PRODUCTS) {
            const int item = unit_action.arg;
            const int accepted = std::min({
                std::max(0, unit_action.n), inventory[item],
                std::max(0, capacity - storage_shed_total(storage)),
            });
            if (accepted > 0) {
                inventory[item] -= accepted;
                storage.shed[item] += accepted;
            }
        } else if (unit_action.op == kag::OP_PICKUP &&
                   unit_action.arg < kag::N_ITEMS) {
            const int item = unit_action.arg;
            const int picked = std::min(
                std::max(0, unit_action.n), storage.shed[item]);
            if (picked > 0) {
                storage.shed[item] -= picked;
                inventory[item] += picked;
            }
        } else if (unit_action.op == kag::OP_FEED &&
                   inventory[kag::WHEAT] > 0) {
            --inventory[kag::WHEAT];
        } else if (unit_action.op == kag::OP_FERTILIZE &&
                   inventory[kag::FERTILIZER] > 0) {
            --inventory[kag::FERTILIZER];
        }
    }
    return storage;
}

int fib(int index) {
    int a = 1;
    int b = 1;
    for (int step = 0; step < index; ++step) {
        const int next = a + b;
        a = b;
        b = next;
    }
    return a;
}

struct OrderList {
    std::array<kag::Order, 16> values{};
    int size = 0;

    void push(kag::Order order) { values[size++] = order; }
};

double market_commitment_cost(
    const kag::agent::AgentObservation& observation,
    const OrderList& orders) {
    int hires = observation.self().hires_today;
    int land_index = std::max(0, observation.self().n_quadrants - 1);
    double cost = 0;
    for (int index = 0; index < orders.size; ++index) {
        const kag::Order& order = orders.values[index];
        if (order.op == kag::M_HIRE) {
            cost += fib(hires++);
        } else if (order.op == kag::M_BUY_LAND &&
                   land_index < static_cast<int>(LAND_PRICES.size())) {
            cost += LAND_PRICES[land_index++];
        } else if (order.op == kag::M_BUY_ANIMAL && kag::is_animal(order.item)) {
            cost += kag::ANIMALS[order.item - kag::GOOSE].cost * order.n;
        } else if (order.op == kag::M_BUY_SEED && kag::is_crop(order.item)) {
            cost += kag::CROPS[order.item].seed * order.n;
        } else if (order.op == kag::M_BUY_PRODUCT &&
                   kag::is_product(order.item)) {
            cost += observation.market.prices[order.item] * order.n * 1.25;
        }
    }
    return cost;
}

bool append_schedule_order(OrderList& orders, kag::Order order,
                           int max_orders) {
    if (orders.size < max_orders) {
        orders.push(order);
        return true;
    }
    for (int index = orders.size - 1; index >= 0; --index)
        if (orders.values[index].op == kag::M_HIRE) {
            orders.values[index] = order;
            return true;
        }
    return false;
}

struct PublicCounts {
    std::array<int, kag::N_ITEMS> items{};
    int pasture = 0;
};

PublicCounts public_counts(const kag::agent::PublicFarm& farm,
                           int board_size) {
    PublicCounts counts;
    for (int y = 0; y < board_size; ++y)
        for (int x = 0; x < board_size; ++x) {
            const kag::Tile& tile = farm.tiles[y][x];
            if (tile.kind == kag::T_PLANT && tile.what < kag::N_CROPS)
                ++counts.items[tile.what];
            else if (tile.has_animal && kag::is_animal(tile.what))
                ++counts.items[tile.what];
            else if (tile.kind == kag::T_PASTURE)
                ++counts.pasture;
        }
    return counts;
}

bool schedule_signature(const kag::agent::AgentObservation& observation,
                        int board_size, int& last_step, bool& active) {
    const int step = observation.step;
    if (step == 0 || step <= last_step) active = false;
    last_step = step;
    const auto& opponent = observation.opponent();
    const PublicCounts counts = public_counts(opponent, board_size);
    if (step == 24) {
        const bool hard_negative = counts.items[kag::GOOSE] ||
            counts.items[kag::CARROT] || counts.items[kag::TOMATO];
        active = !hard_negative && opponent.n_quadrants == 1 &&
            counts.items[kag::COW] == 3 && counts.items[kag::SHEEP] == 1 &&
            counts.items[kag::MELON] == 6 &&
            counts.items[kag::STRAWBERRY] == 0;
    }
    if (step == 192 && active)
        active = opponent.n_quadrants == 1 &&
            counts.items[kag::COW] >= 3 && counts.items[kag::COW] <= 5 &&
            counts.items[kag::SHEEP] == 1 &&
            counts.items[kag::MELON] >= 9 && counts.items[kag::MELON] <= 10 &&
            counts.items[kag::STRAWBERRY] >= 6 &&
            counts.items[kag::STRAWBERRY] <= 7;
    if (step == 264 && active) {
        const bool hard_negative = counts.items[kag::GOOSE] ||
            counts.items[kag::CARROT] || counts.items[kag::TOMATO];
        active = !hard_negative && opponent.n_quadrants == 2 &&
            counts.items[kag::COW] >= 3 && counts.items[kag::COW] <= 5 &&
            counts.items[kag::SHEEP] == 5 && counts.items[kag::MELON] == 6 &&
            counts.items[kag::STRAWBERRY] >= 15 &&
            counts.items[kag::STRAWBERRY] <= 16 &&
            counts.pasture >= 4 && counts.pasture <= 5;
    }
    return active;
}

void schedule_market_adjustment(
    const kag::agent::AgentObservation& observation,
    const kag::agent::AgentConfig& config, OrderList& orders,
    int& last_step, bool& signature_active, int& wheat_requested) {
    const bool active = schedule_signature(
        observation, config.board_size, last_step, signature_active);
    if (observation.step == 0 || !active) {
        wheat_requested = 0;
        return;
    }
    if (observation.day != 11) {
        wheat_requested = 0;
        return;
    }
    if (observation.hour == 2 && wheat_requested == 0) {
        const double wheat_price = observation.market.prices[kag::WHEAT];
        const double committed = market_commitment_cost(observation, orders);
        const int affordable = static_cast<int>(std::max(
            0.0, observation.self().money - committed - 750.0) /
            std::max(1.0, wheat_price + 25.0));
        const int quantity = std::min(SCHEDULE_WHEAT_CAP, affordable);
        if (quantity > 0 && append_schedule_order(orders,
            {kag::M_BUY_PRODUCT, kag::WHEAT, quantity}, config.max_orders))
            wheat_requested = quantity;
        return;
    }
    if (observation.hour == 19 && wheat_requested > 0) {
        const auto animals = farm_animal_counts(
            observation.self(), config.board_size);
        const int animal_count = animals[0] + animals[1] + animals[2];
        const int quantity = std::min(wheat_requested,
            std::max(0, static_cast<int>(observation.own.shed[kag::WHEAT]) -
                3 * animal_count));
        wheat_requested = 0;
        if (quantity > 0)
            append_schedule_order(orders,
                {kag::M_SELL, kag::WHEAT, quantity}, config.max_orders);
    }
}

int sell_quantity(const kag::agent::AgentObservation& observation,
                  int item, int have, int inventory, double shed_load) {
    const int left = TOTAL_DAYS - observation.day;
    if (left <= 1) return have;
    double reserve = kag::MARKET[item].base * RESERVE_FRACTION[item];
    if (left <= 7) reserve *= std::max(0.0, (left - 1) / 6.0);
    if (shed_load >= 0.75) reserve *= 0.55;
    const int opponent_supply = opponent_visible_supply(
        observation, item, 1, kag::BOARD);
    const int demand = town_demand(observation, item);
    if (opponent_supply > demand)
        reserve *= std::max(0.72,
            1.0 - 0.015 * (opponent_supply - demand));
    const int future_price = kag::market_price(
        item, inventory + opponent_supply - demand);
    double threshold = reserve;
    if (shed_load < 0.75 && left > 7)
        threshold = std::max(threshold, 0.88 * future_price);
    int quantity = 0;
    while (quantity < have &&
           kag::market_price(item, inventory + quantity) >= threshold)
        ++quantity;
    if (left <= 12) {
        const int forced = static_cast<int>(std::ceil(
            have / static_cast<double>(std::max(1, left - 1))));
        quantity = std::max(quantity, std::min(have, forced));
    }
    return quantity;
}

std::array<int, kag::N_CROPS> seed_needs(
    const kag::agent::AgentObservation& observation, const RoleList& roles) {
    std::array<int, kag::N_CROPS> needs{};
    for (int index = 0; index < roles.size; ++index) {
        const Role& role = roles.values[index];
        const kag::Tile& tile = observation.self().tiles[
            role.position.y][role.position.x];
        if (role.kind == ROLE_CROP &&
            (tile.kind == kag::T_EMPTY || tile.kind == kag::T_WEED) &&
            observation.day <= LAST_PLANT[role.item])
            ++needs[role.item];
    }
    for (int crop = 0; crop < kag::N_CROPS; ++crop)
        needs[crop] = std::max(0,
            needs[crop] - static_cast<int>(observation.own.seeds[crop]));
    return needs;
}

int target_hands(const kag::agent::AgentObservation& observation,
                 const RoleList& roles, int board_size) {
    const Summary summary = survey(observation, roles, board_size);
    const int due_jobs = field_jobs(
        observation, roles, false, board_size).size;
    const int active_roles = summary.plants + summary.animals +
        summary.plantable + summary.structures_todo;
    const int floor = observation.day <= 27 && active_roles > 0 ? 10 : 4;
    const int risk = 2 * (summary.at_risk_animals + summary.at_risk_crops);
    const int demand = static_cast<int>(std::ceil((due_jobs + risk) / 7.0));
    const int requested = std::max(4, std::min(13, std::max(floor, demand)));
    return std::min(requested, observation.day >= 20 ? 13 : 12);
}

enum Phase : uint8_t { BOOTSTRAP, COMPOUND, REALIZE, CRISIS, LIQUIDATE };

Phase policy_phase(const kag::agent::AgentObservation& observation,
                   const Summary& summary) {
    if (719 - observation.step <= LIQUIDATION_TURNS) return LIQUIDATE;
    if (summary.at_risk_animals + summary.at_risk_crops >
            observation.self().n_units ||
        summary.shed_load + summary.carried_load >= 95)
        return CRISIS;
    if (observation.day <= 4) return BOOTSTRAP;
    if (observation.day <= 21) return COMPOUND;
    return REALIZE;
}

OrderList market_actions(const kag::agent::AgentObservation& observation,
                         const kag::agent::AgentConfig& config,
                         const RoleList& roles, const FieldPlan& field,
                         const kag::Action& action) {
    const auto& farm = observation.self();
    const int left = TOTAL_DAYS - observation.day;
    double money = farm.money;
    Storage storage = post_field_storage(observation, action,
                                         config.shed_capacity);
    std::array<int, kag::N_PRODUCTS> market_inventory{};
    for (int item = 0; item < kag::N_PRODUCTS; ++item)
        market_inventory[item] = observation.market.inventory[item];
    const Summary summary = survey(observation, roles, config.board_size);
    const Phase phase = policy_phase(observation, summary);
    OrderList orders;
    int occupancy = storage_shed_total(storage);
    const double shed_load = occupancy /
        static_cast<double>(std::max(1, config.shed_capacity));
    int animal_stock = 0;
    for (int animal = 0; animal < kag::N_ANIMALS; ++animal)
        animal_stock += summary.animal_stock[animal];
    const int animal_pipeline = summary.animals + animal_stock;
    const int feed_floor = animal_pipeline * FEED_STOCK_DAYS;
    int total_wheat = storage.shed[kag::WHEAT];
    for (int unit = 0; unit < farm.n_units; ++unit)
        total_wheat += storage.inv[unit][kag::WHEAT];

    struct Sale { long long proceeds; int item; int quantity; };
    std::array<Sale, kag::N_PRODUCTS> sales{};
    int n_sales = 0;
    for (int item = 0; item < kag::N_PRODUCTS; ++item) {
        int have = storage.shed[item];
        if (item == kag::WHEAT && left > 2)
            have = std::min(have, std::max(0, total_wheat - feed_floor));
        if (have <= 0) continue;
        const int quantity = sell_quantity(
            observation, item, have, market_inventory[item], shed_load);
        if (quantity <= 0) continue;
        long long proceeds = 0;
        for (int offset = 0; offset < quantity; ++offset)
            proceeds += kag::market_price(item, market_inventory[item] + offset);
        sales[n_sales++] = {proceeds, item, quantity};
    }
    std::sort(sales.begin(), sales.begin() + n_sales,
              [](const Sale& left_sale, const Sale& right_sale) {
        if (left_sale.proceeds != right_sale.proceeds)
            return left_sale.proceeds > right_sale.proceeds;
        if (LEXICAL_ITEM_RANK[left_sale.item] !=
            LEXICAL_ITEM_RANK[right_sale.item])
            return LEXICAL_ITEM_RANK[left_sale.item] >
                LEXICAL_ITEM_RANK[right_sale.item];
        return left_sale.quantity > right_sale.quantity;
    });
    for (int index = 0; index < n_sales && orders.size < config.max_orders;
         ++index) {
        const Sale& sale = sales[index];
        orders.push({kag::M_SELL, static_cast<uint8_t>(sale.item), sale.quantity});
        money += 0.85 * sale.proceeds;
        occupancy = std::max(0, occupancy - sale.quantity);
        storage.shed[sale.item] = std::max(
            0, storage.shed[sale.item] - sale.quantity);
        market_inventory[sale.item] += sale.quantity;
    }

    if (field.liquidation || left <= 1) {
        if (observation.day >= 29 && observation.hour <= 1) {
            const int terminal_jobs = field_jobs(
                observation, roles, true, config.board_size).size;
            const int target = std::min(8, terminal_jobs);
            int hires = farm.hires_today;
            while (hires < target && orders.size < config.max_orders) {
                const int cost = fib(hires);
                if (money < cost + 20) break;
                orders.push({kag::M_HIRE, 0, 0});
                money -= cost;
                ++hires;
            }
        }
        return orders;
    }

    const auto placed = farm_animal_counts(farm, config.board_size);
    std::array<int, kag::N_ANIMALS> role_targets{};
    for (int index = 0; index < roles.size; ++index) {
        const Role& role = roles.values[index];
        if (role.kind == ROLE_ANIMAL && kag::is_animal(role.item))
            ++role_targets[role.item - kag::GOOSE];
    }
    std::array<int, kag::N_ANIMALS> owned{};
    for (int animal = 0; animal < kag::N_ANIMALS; ++animal)
        owned[animal] = placed[animal] + private_total(
            observation.own, kag::GOOSE + animal, farm.n_units);
    const bool animal_capital_open = phase == BOOTSTRAP || phase == COMPOUND ||
        (phase == CRISIS && summary.at_risk_animals == 0 &&
         summary.shed_load + summary.carried_load < 95 &&
         summary.open_structures > 0 &&
         animal_pipeline < summary.animals + summary.open_structures);
    if (animal_capital_open && observation.day <= ANIMAL_PURCHASE_LAST_DAY &&
        left >= 8) {
        const auto opponents = opponent_animal_counts(observation,
                                                      config.board_size);
        std::array<int, 2> purchase{kag::COW, kag::SHEEP};
        std::sort(purchase.begin(), purchase.end(), [&](int left_animal,
                                                        int right_animal) {
            const int li = left_animal - kag::GOOSE;
            const int ri = right_animal - kag::GOOSE;
            const double ls = livestock_score(observation, left_animal,
                owned[li], opponents[li]);
            const double rs = livestock_score(observation, right_animal,
                owned[ri], opponents[ri]);
            if (ls != rs) return ls > rs;
            const int lm = role_targets[li] - owned[li];
            const int rm = role_targets[ri] - owned[ri];
            if (lm != rm) return lm > rm;
            return left_animal == kag::COW;
        });
        for (const int animal : purchase) {
            if (orders.size >= config.max_orders) break;
            const int ai = animal - kag::GOOSE;
            const int missing = std::max(0, role_targets[ai] - owned[ai]);
            if (missing <= 0) continue;
            const int operating_reserve = owned[0] + owned[1] + owned[2] <
                CORE_HERD ? 80 : 220;
            const int quantity = std::min({
                missing, 2, std::max(0, config.shed_capacity - occupancy),
                std::max(0, static_cast<int>((money - operating_reserve) /
                    kag::ANIMALS[ai].cost)),
            });
            if (quantity > 0) {
                orders.push({kag::M_BUY_ANIMAL,
                    static_cast<uint8_t>(animal), quantity});
                money -= quantity * kag::ANIMALS[ai].cost;
                occupancy += quantity;
                owned[ai] += quantity;
            }
        }
    }

    total_wheat = storage.shed[kag::WHEAT];
    for (int unit = 0; unit < farm.n_units; ++unit)
        total_wheat += storage.inv[unit][kag::WHEAT];
    const int planned_herd = owned[0] + owned[1] + owned[2];
    const int desired_wheat = std::max(
        planned_herd * FEED_STOCK_DAYS, planned_herd > 0 ? 8 : 0);
    if (desired_wheat > total_wheat && orders.size < config.max_orders &&
        planned_herd > 0) {
        const int emergency_reserve = summary.at_risk_animals ? 0 : 80;
        int quantity = 0;
        int cost = 0;
        const int limit = std::min(desired_wheat - total_wheat,
            std::max(0, config.shed_capacity - occupancy));
        for (int offset = 0; offset < limit; ++offset) {
            const int unit = kag::market_price(
                kag::WHEAT, market_inventory[kag::WHEAT] - offset - 1);
            if (money - cost - unit < emergency_reserve) break;
            cost += unit;
            ++quantity;
        }
        if (quantity > 0) {
            orders.push({kag::M_BUY_PRODUCT, kag::WHEAT, quantity});
            money -= cost;
            occupancy += quantity;
        }
    }

    const int extra_land = std::max(0, farm.n_quadrants - 1);
    if ((phase == BOOTSTRAP || phase == COMPOUND) &&
        extra_land < MAX_EXTRA_LAND &&
        observation.day >= LAND_OPEN_DAYS[extra_land] && left >= 12 &&
        orders.size < config.max_orders) {
        const int cost = LAND_PRICES[extra_land];
        const int reserve = extra_land == 0 ? 300 : 500;
        if (money >= cost + reserve) {
            orders.push({kag::M_BUY_LAND, 0, 0});
            money -= cost;
        }
    }

    const auto needs = seed_needs(observation, roles);
    const int seed_reserve = observation.day <= 4 ? 80 : 150;
    constexpr std::array<int, 5> SEED_ORDER{
        kag::MELON, kag::WHEAT, kag::STRAWBERRY, kag::CARROT, kag::TOMATO,
    };
    const int seed_count = observation.day == 0 ? 1 : 5;
    for (int index = 0; index < seed_count; ++index) {
        const int crop = SEED_ORDER[index];
        if (orders.size >= config.max_orders || needs[crop] <= 0) continue;
        const int cost = kag::CROPS[crop].seed;
        const int quantity = std::min({
            needs[crop], 25,
            std::max(0, static_cast<int>((money - seed_reserve) / cost)),
        });
        if (quantity > 0) {
            orders.push({kag::M_BUY_SEED,
                static_cast<uint8_t>(crop), quantity});
            money -= quantity * cost;
        }
    }

    if (observation.hour <= 2) {
        const int target = target_hands(observation, roles, config.board_size);
        int hires = farm.hires_today;
        while (hires < target && orders.size < config.max_orders) {
            const int cost = fib(hires);
            if (money < std::max(20, 3 * cost)) break;
            orders.push({kag::M_HIRE, 0, 0});
            money -= cost;
            ++hires;
        }
    }
    return orders;
}

void decide(const kag::agent::AgentObservation& observation,
            const kag::agent::AgentConfig& config, kag::Action& action,
            int& signature_last_step, bool& signature_active,
            int& schedule_wheat_requested) {
    if (observation.player > 1 || observation.self().n_units < 1 ||
        observation.self().n_units > kag::MAX_UNITS ||
        config.board_size != kag::BOARD) {
        action.clear();
        action.n_units = std::clamp(observation.self().n_units, 1, kag::MAX_UNITS);
        std::fill_n(action.units, action.n_units, kag::UnitAction{});
        action.finalize();
        return;
    }
    const RoleList roles = role_plan(observation, config.board_size);
    const FieldPlan field = unit_actions(observation, config, roles, action);
    OrderList orders = market_actions(observation, config, roles, field, action);
    schedule_market_adjustment(observation, config, orders,
        signature_last_step, signature_active, schedule_wheat_requested);
    action.n_orders = std::min(orders.size, config.max_orders);
    std::copy_n(orders.values.begin(), action.n_orders, action.orders);
    action.finalize();
}

}

kag::agent::AgentInfo Policy::info() {
    return {"test_structured_economic_policy"};
}

void Policy::reset(const kag::agent::AgentInit& init) {
    config_ = init.config;
    signature_last_step_ = -1;
    signature_active_ = false;
    schedule_wheat_requested_ = 0;
}

void Policy::act(const kag::agent::AgentObservation& observation,
                 const kag::agent::DecisionBudget&,
                 kag::Action& action) {
    decide(observation, config_, action, signature_last_step_,
           signature_active_, schedule_wheat_requested_);
}

}
