#pragma once

#include <cstdint>
#include <type_traits>

#include "fast_game_engine/sim.hpp"

namespace kag::agent {

inline constexpr uint32_t API_FORMAT_VERSION = 1;

// Public environment configuration. The episode seed is intentionally absent.
struct AgentConfig {
    int episode_steps = 720;
    int board_size = BOARD;
    int starting_money = 3000;
    int max_orders = 10;
    int turns_per_day = 24;
    int shed_capacity = 100;
    double weed_chance = 0.005;
    int shop_unlock_interval = 3;
    int shop_sell_interval = 4;
    int center_sell_interval = 24;
    int hire_mult = 1;
};

// The part of a farm visible to both players.
struct PublicFarm {
    double money = 0;
    Tile tiles[BOARD][BOARD]{};
    int8_t pos_x[MAX_UNITS]{};
    int8_t pos_y[MAX_UNITS]{};
    int n_units = 1;
    int n_quadrants = 1;
    int hires_today = 0;
};

// Private data for the observing player only.
struct PrivateFarm {
    Count shed[N_ITEMS]{};
    int shed_total = 0;
    Count seeds[N_CROPS]{};
    Count inv[MAX_UNITS][N_ITEMS]{};
    uint8_t inv_keys[MAX_UNITS][N_ITEMS]{};
    uint8_t inv_nkeys[MAX_UNITS]{};
};

struct AgentObservation {
    uint8_t player = 0;
    int step = 0;
    int day = 0;
    int hour = 0;
    PublicFarm farms[2]{};
    PrivateFarm own{};
    Market market{};
    uint8_t shops[MAX_SHOP_INSTANCES]{};
    int n_shops = 0;

    const PublicFarm& self() const { return farms[player]; }
    const PublicFarm& opponent() const { return farms[player ^ 1u]; }
    int own_hand_count() const { return self().n_units - 1; }
};

static_assert(std::is_trivially_copyable_v<AgentConfig>);
static_assert(std::is_trivially_copyable_v<PublicFarm>);
static_assert(std::is_trivially_copyable_v<PrivateFarm>);
static_assert(std::is_trivially_copyable_v<AgentObservation>);

}
