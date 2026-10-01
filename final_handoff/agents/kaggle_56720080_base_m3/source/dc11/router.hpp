#pragma once
// Joint worker routing and site placement for one day (DESIGN.md, "Router").
// Work is a set of stops (consecutive actions of one worker on one tile). Routes are evaluated
// analytically: moves are Manhattan steps, every action takes one turn, pickups happen at shed
// stops (1 turn per item type) for the needs of the following trip, deposits earn the value the
// sale DP gives a unit reaching the shed at that hour. New crops and animals choose their tile
// inside the search: a free tile (own stop) or a tile another stop frees today (same worker).
// Unserved work costs its value, so an overfull day gives up the least valuable stops.
#include "source/world.hpp"
#include <array>
#include <cstdint>
#include <vector>

namespace dc11 {
using namespace dc10;

constexpr int MAX_WORKERS = 16;  // farmer + up to 15 hires (Options::max_hires, default 13)
constexpr int MAX_STEPS = 12;
constexpr int SHED_TILES[4] = {44, 45, 54, 55};  // engine spawn order: NW, NE, SW, SE
inline bool is_shed_tile(int tile) { return tile == 44 || tile == 45 || tile == 54 || tile == 55; }

// Carried consumables tracked by the router.
enum Carry : uint8_t { CW = 0, CF, CG, CC, CS, N_CARRY };
constexpr uint8_t CARRY_ITEM[N_CARRY] = {WHEAT, FERTILIZER, GOOSE, COW, SHEEP};

struct Step { uint8_t op = OP_PASS, arg = 0; };

// Priority classes: what an overfull day gives up first (design: survival last).
enum Priority : uint8_t { P_SURVIVAL = 0, P_OUTPUT, P_NEW, P_EXTRA };
constexpr double PRIORITY_COST[4] = {100000, 10000, 3000, 0};

struct Stop {
    bool shed = false;             // shed stop: deposit carried output, pick up the next trip's needs
    int16_t tile = -1;             // tile stops
    uint8_t n = 0;                 // steps (tile stops)
    uint8_t base_n = 0;            // steps before an attached entity's steps
    Step steps[MAX_STEPS]{};
    int8_t release = 0;            // earliest hour of the first action
    int8_t priority = P_OUTPUT;
    float value = 0;               // loss if the stop is not served (on top of its priority cost)
    // Consumption of carried items: total and peak prefix within the stop (negative = gained).
    int8_t net[N_CARRY]{}, peak[N_CARRY]{};
    // Output added to the cargo: one harvest (crop or animal product) and collected fertilizer.
    int8_t product = -1, units = 0, harvest_at = 0;
    int8_t decay_from = -1;        // hour of the first yield loss (then every 2 h), -1: none
    float unit_value = 0;          // value of one harvested unit (decay losses)
    int8_t fertilizer = 0;         // COLLECT_FERTILIZER units
    int8_t plant_crop = -1;        // crop of a PLANT step (seed timing)
    int16_t group = -1;            // members of a group are interchangeable (swap tiles)
    int16_t entity = -1;           // new entity served here (own stop or attached to a freeing stop)
    bool frees = false;            // leaves the tile empty for a new entity (harvest/clear)
    bool dead = false;             // unused slot (a removed entity stop)
    bool animal = false;           // an animal's service or output stop
};

struct Entity {
    uint8_t item = 0;       // crop id or GOOSE/COW/SHEEP
    uint8_t extra = 0;      // bit 1 fertilize, 2 water, 4 feed, 8 care
    int16_t stop = -1;      // stop serving it
    float site_weight = 0;  // long-term cost per shed-distance step of the chosen tile
    int8_t priority = P_NEW;
    float value = 0;
};

struct Worker {
    int8_t tile = 44;
    int8_t hour = 0;              // first hour it acts
    int16_t carry[N_CARRY]{};     // carried consumables at the start
    int16_t cargo[N_PRODUCTS]{};  // carried products at the start (not counted as deposit gains)
};

struct Problem {
    int day = 0;
    int last_hour = 23;                    // last hour with actions
    std::vector<Stop> stops;               // tile work
    std::vector<Entity> entities;          // new crops and animals
    Tile tiles[BOARD * BOARD]{};           // dawn tiles (entity steps on free sites)
    int8_t site_release[BOARD * BOARD]{};  // earliest hour a free site can be used, -1: not a site
    int16_t member_group[BOARD * BOARD]{}; // group of the tile's crop/animal for member swaps, -1
    int dawn_stock[N_CARRY]{};             // shed stock usable by the farmer at hour 0
    int8_t buy_from[N_CARRY];              // first hour bought units can be picked up (cash)
    int dawn_seeds[N_CROPS]{}, plants[N_CROPS]{};  // seeds at dawn, plants planned today
    int8_t seed_from[N_CROPS];             // first hour bought seeds can be planted
    double gain[N_PRODUCTS][HOURS]{};      // value of one unit deposited at that hour (vs overnight)
    double wage_per_turn = 2;              // value of one worker turn (route time, site costs)
    int search_radius = 4;                 // swaps: stops at most this far apart; tail exchanges:
                                           // new links at most this many moves longer
    int search_rounds = 6;                 // local search rounds (each stops early without a gain)
    int drop_any = 0;                      // hire search: routes besides the last tried for removal (least-loaded first)
    bool night_trim = false;               // nighttrim: remove pure output stops whose cargo cannot be stored tonight
    int night_rounds = 0;                  // night-cargo repair rounds (0: 3)
    int site_candidates = 5;               // free tiles tried per new entity (best by site cost + distance)
    int max_hires = 13;
    bool final_return = true;              // routes end with a deposit trip when the cargo pays for it
    bool place_deposits = false;           // regime M: shed stops PLACE each product; collected fertilizer and supplies stay carried
    bool lazy_wheat = false;               // wheatcash: wheat not yet buyable is fetched at the first stop that needs it
    double animal_morning = 0;             // regime M: cost per hour an animal stop starts after hour 11 (morning animal block)
    double return_cost = 1;                // (x the trip's worker turns)
    int first_wave = 10;                   // new hires ordered in the first hour (the rest one hour later)
    Worker farmer;                         // start of the farmer (dawn: tile 44, hour 0)
    bool reserve_animal_sites = true;      // crops avoid the tiles nearest the shed
    // Lateness counts required work only: optional (P_EXTRA) stops past the end of the day are simply not done (their value
    // is the cost), so they neither pay the late-turn penalty nor make a crew incomplete in the hire search.
    bool optional_late = false;
    // Tonight's shed room for the workers' cargo (the shed sold down to kept inputs) and the feed
    // wheat tomorrow needs (cargo wheat counts); overflow is destroyed.
    int night_room = 1000, feed_reserve = 0;
    double unit_value[N_PRODUCTS]{};       // value of a unit lost to overflow
    Problem() {
        std::fill_n(buy_from, N_CARRY, int8_t(1));
        std::fill_n(seed_from, N_CROPS, int8_t(1));
        std::fill_n(member_group, BOARD * BOARD, int16_t(-1));
    }
};

struct RouteOut {
    int hires = 0;                         // hires today
    std::vector<std::vector<int>> routes;  // per worker: stop indices into `stops` (incl. shed stops)
    std::vector<Stop> stops;               // problem stops + entity stops + shed stops
    std::vector<Entity> entities;
    std::vector<Worker> starts;
    std::vector<int> dropped;              // tile stops not served (entity stops included)
    int moves = 0, actions = 0, waits = 0;
    long evaluations = 0;
};

// Hire-count search: from the work lower bound up to the first complete count and one more,
// then down again; keeps the cheapest plan (wages + route time - deposit gains + unserved value).
RouteOut route_day(Problem problem);

// Actions of one route from an actual start (index = hour; PASS where idle). Returns the hour
// after its last action (> last_hour + 1: the route does not fit in the day).
int route_actions(const Problem& problem, const std::vector<Stop>& stops, const Worker& start, const std::vector<int>& route,
                  UnitAction out[HOURS]);

// Engine spawn rule: the least occupied shed tile (ties NW, NE, SW, SE).
int spawn_tile(const int* positions, int count);
}
