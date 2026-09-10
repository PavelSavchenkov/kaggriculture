#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "fast_game_engine/sim.hpp"
#include "corpus/episode.hpp"

namespace py = pybind11;

namespace {

constexpr int TILE_FIELDS = 13;
constexpr int PUBLIC_FARM_FIELDS = 4;

template <typename T>
py::array_t<T> array(std::initializer_list<py::ssize_t> shape) {
    return py::array_t<T>(std::vector<py::ssize_t>(shape));
}

py::dict observe_sims(const std::vector<kag::Sim>& sims, int seat) {
    if (seat < 0 || seat > 1) throw std::invalid_argument("seat must be 0 or 1");
    const py::ssize_t batch = static_cast<py::ssize_t>(sims.size());
    auto tiles = array<int32_t>({batch, 2, kag::BOARD, kag::BOARD, TILE_FIELDS});
    auto farm = array<float>({batch, 2, PUBLIC_FARM_FIELDS});
    auto positions = array<int16_t>({batch, 2, kag::MAX_UNITS, 2});
    auto active_units = array<uint8_t>({batch, 2, kag::MAX_UNITS});
    auto own_shed = array<int32_t>({batch, kag::N_ITEMS});
    auto own_seeds = array<int32_t>({batch, kag::N_CROPS});
    auto own_inventory = array<int32_t>({batch, kag::MAX_UNITS, kag::N_ITEMS});
    auto own_inventory_order = array<uint8_t>({batch, kag::MAX_UNITS, kag::N_ITEMS});
    auto market = array<int32_t>({batch, kag::N_PRODUCTS, 2});
    auto shops = array<int32_t>({batch, kag::N_SHOPS});
    auto clock = array<int32_t>({batch, 4});

    std::fill(positions.mutable_data(), positions.mutable_data() + positions.size(), int16_t{-1});
    std::fill(active_units.mutable_data(), active_units.mutable_data() + active_units.size(), uint8_t{0});
    std::fill(own_inventory_order.mutable_data(),
              own_inventory_order.mutable_data() + own_inventory_order.size(), uint8_t{0});
    std::fill(shops.mutable_data(), shops.mutable_data() + shops.size(), int32_t{0});

    auto ti = tiles.mutable_unchecked<5>();
    auto fa = farm.mutable_unchecked<3>();
    auto po = positions.mutable_unchecked<4>();
    auto au = active_units.mutable_unchecked<3>();
    auto sh = own_shed.mutable_unchecked<2>();
    auto se = own_seeds.mutable_unchecked<2>();
    auto iv = own_inventory.mutable_unchecked<3>();
    auto io = own_inventory_order.mutable_unchecked<3>();
    auto ma = market.mutable_unchecked<3>();
    auto so = shops.mutable_unchecked<2>();
    auto cl = clock.mutable_unchecked<2>();

    for (py::ssize_t b = 0; b < batch; ++b) {
        const kag::State& state = sims[static_cast<size_t>(b)].st;
        for (int relative = 0; relative < 2; ++relative) {
            const int player = relative == 0 ? seat : 1 - seat;
            const kag::Farm& f = state.farms[player];
            fa(b, relative, 0) = static_cast<float>(f.money);
            fa(b, relative, 1) = static_cast<float>(f.n_units);
            fa(b, relative, 2) = static_cast<float>(f.n_quadrants);
            fa(b, relative, 3) = static_cast<float>(f.hires_today);
            for (int u = 0; u < f.n_units; ++u) {
                po(b, relative, u, 0) = f.pos_x[u];
                po(b, relative, u, 1) = f.pos_y[u];
                au(b, relative, u) = 1;
            }
            for (int y = 0; y < kag::BOARD; ++y) {
                for (int x = 0; x < kag::BOARD; ++x) {
                    const kag::Tile& t = f.tiles[y][x];
                    const int32_t fields[TILE_FIELDS] = {
                        t.kind, t.what, t.has_animal, t.watered_today,
                        t.fed_today, t.cared_today, t.fertilizer_available,
                        t.consecutive_dry, t.yield_units, t.pending_care_bonus,
                        t.planted_day, t.max_lifespan_step,
                        t.fertilized_until_day};
                    for (int k = 0; k < TILE_FIELDS; ++k)
                        ti(b, relative, y, x, k) = fields[k];
                }
            }
        }

        const kag::Farm& own = state.farms[seat];
        for (int item = 0; item < kag::N_ITEMS; ++item) {
            sh(b, item) = own.shed[item];
            for (int u = 0; u < kag::MAX_UNITS; ++u)
                iv(b, u, item) = own.inv[u][item];
        }
        // 1-based insertion rank by item, 0 when absent. DROP iterates Python
        // inventory dictionaries in this order, which changes what survives
        // when the shed capacity binds.
        for (int u = 0; u < own.n_units; ++u)
            for (int rank = 0; rank < own.inv_nkeys[u]; ++rank)
                io(b, u, own.inv_keys[u][rank]) = static_cast<uint8_t>(rank + 1);
        for (int crop = 0; crop < kag::N_CROPS; ++crop)
            se(b, crop) = own.seeds[crop];
        for (int product = 0; product < kag::N_PRODUCTS; ++product) {
            ma(b, product, 0) = state.market.inventory[product];
            ma(b, product, 1) = state.market.prices[product];
        }
        for (int i = 0; i < state.n_shops; ++i) ++so(b, state.shops[i]);
        cl(b, 0) = state.step;
        cl(b, 1) = state.day;
        cl(b, 2) = state.hour;
        cl(b, 3) = seat;
    }

    py::dict result;
    result["tiles"] = std::move(tiles);
    result["farm"] = std::move(farm);
    result["positions"] = std::move(positions);
    result["active_units"] = std::move(active_units);
    result["own_shed"] = std::move(own_shed);
    result["own_seeds"] = std::move(own_seeds);
    result["own_inventory"] = std::move(own_inventory);
    result["own_inventory_order"] = std::move(own_inventory_order);
    result["market"] = std::move(market);
    result["shops"] = std::move(shops);
    result["clock"] = std::move(clock);
    return result;
}

py::dict action_to_dict(const kag::Action& action) {
    auto unit_op = array<uint8_t>({kag::MAX_UNITS});
    auto unit_arg = array<uint8_t>({kag::MAX_UNITS});
    auto unit_n = array<int32_t>({kag::MAX_UNITS});
    auto order_op = array<uint8_t>({16});
    auto order_item = array<uint8_t>({16});
    auto order_n = array<int32_t>({16});
    std::fill(unit_op.mutable_data(), unit_op.mutable_data() + unit_op.size(), kag::OP_PASS);
    std::fill(unit_arg.mutable_data(), unit_arg.mutable_data() + unit_arg.size(), uint8_t{0});
    std::fill(unit_n.mutable_data(), unit_n.mutable_data() + unit_n.size(), int32_t{1});
    std::fill(order_op.mutable_data(), order_op.mutable_data() + order_op.size(), kag::M_NONE);
    std::fill(order_item.mutable_data(), order_item.mutable_data() + order_item.size(), uint8_t{0});
    std::fill(order_n.mutable_data(), order_n.mutable_data() + order_n.size(), int32_t{0});
    for (int i = 0; i < action.n_units; ++i) {
        unit_op.mutable_at(i) = action.units[i].op;
        unit_arg.mutable_at(i) = action.units[i].arg;
        unit_n.mutable_at(i) = action.units[i].n;
    }
    for (int i = 0; i < action.n_orders; ++i) {
        order_op.mutable_at(i) = action.orders[i].op;
        order_item.mutable_at(i) = action.orders[i].item;
        order_n.mutable_at(i) = action.orders[i].n;
    }
    py::dict result;
    result["unit_op"] = std::move(unit_op);
    result["unit_arg"] = std::move(unit_arg);
    result["unit_n"] = std::move(unit_n);
    result["n_units"] = action.n_units;
    result["order_op"] = std::move(order_op);
    result["order_item"] = std::move(order_item);
    result["order_n"] = std::move(order_n);
    result["n_orders"] = action.n_orders;
    return result;
}

class VectorEnv {
public:
    explicit VectorEnv(const std::vector<uint64_t>& seeds, int episode_steps = 720)
        : episode_steps_(episode_steps) {
        if (episode_steps_ < 2 || episode_steps_ > 720)
            throw std::invalid_argument("episode_steps must be in [2, 720]");
        reset(seeds);
    }

    void reset(const std::vector<uint64_t>& seeds) {
        sims_.clear();
        sims_.reserve(seeds.size());
        for (uint64_t seed : seeds) {
            kag::Config config;
            config.seed = seed;
            config.episode_steps = episode_steps_;
            sims_.emplace_back(config);
        }
    }

    py::dict observe(int seat) const { return observe_sims(sims_, seat); }

    py::dict step(
        py::array_t<uint8_t, py::array::c_style | py::array::forcecast> unit_op,
        py::array_t<uint8_t, py::array::c_style | py::array::forcecast> unit_arg,
        py::array_t<int32_t, py::array::c_style | py::array::forcecast> unit_n,
        py::array_t<int32_t, py::array::c_style | py::array::forcecast> n_units,
        py::array_t<uint8_t, py::array::c_style | py::array::forcecast> order_op,
        py::array_t<uint8_t, py::array::c_style | py::array::forcecast> order_item,
        py::array_t<int32_t, py::array::c_style | py::array::forcecast> order_n,
        py::array_t<int32_t, py::array::c_style | py::array::forcecast> n_orders) {
        const py::ssize_t batch = static_cast<py::ssize_t>(sims_.size());
        auto require = [](const py::buffer_info& info,
                          const std::vector<py::ssize_t>& shape,
                          const char* name) {
            if (info.ndim != static_cast<py::ssize_t>(shape.size()))
                throw std::invalid_argument(std::string(name) + " has wrong rank");
            for (py::ssize_t i = 0; i < info.ndim; ++i)
                if (info.shape[static_cast<size_t>(i)] != shape[static_cast<size_t>(i)])
                    throw std::invalid_argument(std::string(name) + " has wrong shape");
        };
        require(unit_op.request(), {batch, 2, kag::MAX_UNITS}, "unit_op");
        require(unit_arg.request(), {batch, 2, kag::MAX_UNITS}, "unit_arg");
        require(unit_n.request(), {batch, 2, kag::MAX_UNITS}, "unit_n");
        require(n_units.request(), {batch, 2}, "n_units");
        require(order_op.request(), {batch, 2, 10}, "order_op");
        require(order_item.request(), {batch, 2, 10}, "order_item");
        require(order_n.request(), {batch, 2, 10}, "order_n");
        require(n_orders.request(), {batch, 2}, "n_orders");

        auto uo = unit_op.unchecked<3>();
        auto ua = unit_arg.unchecked<3>();
        auto un = unit_n.unchecked<3>();
        auto nu = n_units.unchecked<2>();
        auto oo = order_op.unchecked<3>();
        auto oi = order_item.unchecked<3>();
        auto on = order_n.unchecked<3>();
        auto no = n_orders.unchecked<2>();
        std::vector<std::array<kag::Action, 2>> actions(static_cast<size_t>(batch));
        for (py::ssize_t b = 0; b < batch; ++b) {
            for (int seat = 0; seat < 2; ++seat) {
                kag::Action& action = actions[static_cast<size_t>(b)][seat];
                action.clear();
                action.n_units = std::clamp(nu(b, seat), 1, kag::MAX_UNITS);
                for (int unit = 0; unit < action.n_units; ++unit)
                    action.units[unit] = {uo(b, seat, unit), ua(b, seat, unit),
                                          un(b, seat, unit)};
                action.n_orders = std::clamp(no(b, seat), 0, 10);
                for (int order = 0; order < action.n_orders; ++order)
                    action.orders[order] = {oo(b, seat, order), oi(b, seat, order),
                                             on(b, seat, order)};
                action.finalize();
            }
        }
        {
            py::gil_scoped_release release;
            for (size_t b = 0; b < sims_.size(); ++b)
                sims_[b].step(actions[b][0], actions[b][1]);
        }
        return status();
    }

    py::dict pass_step() {
        kag::Action pass;
        pass.clear();
        pass.finalize();
        {
            py::gil_scoped_release release;
            for (kag::Sim& sim : sims_) sim.step(pass, pass);
        }
        return status();
    }

    py::dict status() const {
        const py::ssize_t batch = static_cast<py::ssize_t>(sims_.size());
        auto money = array<float>({batch, 2});
        auto done = array<uint8_t>({batch});
        auto parity_hash = array<uint64_t>({batch});
        for (py::ssize_t b = 0; b < batch; ++b) {
            const kag::Sim& sim = sims_[static_cast<size_t>(b)];
            money.mutable_at(b, 0) = static_cast<float>(sim.reward(0));
            money.mutable_at(b, 1) = static_cast<float>(sim.reward(1));
            done.mutable_at(b) = sim.st.done;
            parity_hash.mutable_at(b) = sim.parity_hash();
        }
        py::dict result;
        result["money"] = std::move(money);
        result["done"] = std::move(done);
        result["parity_hash"] = std::move(parity_hash);
        return result;
    }

    size_t size() const { return sims_.size(); }

private:
    std::vector<kag::Sim> sims_;
    int episode_steps_ = 720;
};

class ReplayCursor {
public:
    explicit ReplayCursor(const std::string& path) {
        if (!corpus::load(path, episode_)) throw std::runtime_error("failed to load replay");
        sim_ = std::make_unique<kag::Sim>(episode_.config);
    }

    py::dict observe(int seat) const {
        std::vector<kag::Sim> one{*sim_};
        return observe_sims(one, seat);
    }

    py::dict recorded_action(int seat) const {
        validate_seat(seat);
        if (step_ + 1 >= episode_.n_steps)
            throw std::out_of_range("terminal replay state has no next action");
        return action_to_dict(episode_.actions[static_cast<size_t>(step_ + 1)][seat]);
    }

    py::dict solo_effective_action(int seat) const {
        validate_seat(seat);
        if (step_ + 1 >= episode_.n_steps)
            throw std::out_of_range("terminal replay state has no next action");
        const kag::Action& raw = episode_.actions[static_cast<size_t>(step_ + 1)][seat];
        return action_to_dict(sim_->sanitize_solo_action(seat, raw));
    }

    py::dict joint_effective_action(int seat) const {
        validate_seat(seat);
        if (step_ + 1 >= episode_.n_steps)
            throw std::out_of_range("terminal replay state has no next action");
        const auto& raw = episode_.actions[static_cast<size_t>(step_ + 1)];
        const auto effective = sim_->sanitize_joint_actions(raw[0], raw[1]);
        return action_to_dict(effective[seat]);
    }

    bool advance() {
        if (step_ + 1 >= episode_.n_steps) return false;
        const auto& actions = episode_.actions[static_cast<size_t>(step_ + 1)];
        sim_->step(actions[0], actions[1]);
        ++step_;
        return true;
    }

    int target(int seat) const {
        validate_seat(seat);
        const double delta = episode_.rewards[seat] - episode_.rewards[1 - seat];
        return (delta > 0) - (delta < 0);
    }

    bool verify() const { return corpus::verify(episode_); }
    bool verify_joint_canonicalization() const {
        kag::Sim raw(episode_.config);
        kag::Sim canonical(episode_.config);
        for (int step = 0; step + 1 < episode_.n_steps; ++step) {
            const auto& actions = episode_.actions[static_cast<size_t>(step + 1)];
            const auto effective = canonical.sanitize_joint_actions(
                actions[0], actions[1]);
            raw.step(actions[0], actions[1]);
            canonical.step(effective[0], effective[1]);
            if (raw.parity_hash() != canonical.parity_hash()) return false;
        }
        return raw.reward(0) == canonical.reward(0) &&
               raw.reward(1) == canonical.reward(1);
    }
    int step() const { return step_; }
    int n_steps() const { return episode_.n_steps; }
    uint64_t seed() const { return episode_.seed; }
    int shed_capacity() const { return episode_.config.shed_capacity; }
    std::vector<std::string> teams() const { return {episode_.teams[0], episode_.teams[1]}; }
    std::vector<uint64_t> submission_ids() const { return episode_.submission_ids; }
    std::vector<double> rewards() const { return {episode_.rewards[0], episode_.rewards[1]}; }

private:
    static void validate_seat(int seat) {
        if (seat < 0 || seat > 1) throw std::invalid_argument("seat must be 0 or 1");
    }
    corpus::Episode episode_;
    std::unique_ptr<kag::Sim> sim_;
    int step_ = 0;
};

}  // namespace

PYBIND11_MODULE(_core, module) {
    module.doc() = "Exact Kaggriculture replay and vector-environment bindings";
    module.attr("ENGINE_VERSION") = kag::OFFICIAL_VERSION;
    module.attr("N_ITEMS") = static_cast<int>(kag::N_ITEMS);
    module.attr("N_PRODUCTS") = static_cast<int>(kag::N_PRODUCTS);
    module.attr("MAX_UNITS") = static_cast<int>(kag::MAX_UNITS);
    module.def("market_price", [](int item, int inventory) {
        if (item < 0 || item >= kag::N_PRODUCTS)
            throw std::invalid_argument("market item is out of range");
        return kag::market_price(item, inventory);
    }, py::arg("item"), py::arg("inventory"));

    py::class_<VectorEnv>(module, "VectorEnv")
        .def(py::init<const std::vector<uint64_t>&, int>(),
             py::arg("seeds"), py::arg("episode_steps") = 720)
        .def("reset", &VectorEnv::reset)
        .def("observe", &VectorEnv::observe, py::arg("seat"))
        .def("step", &VectorEnv::step,
             py::arg("unit_op"), py::arg("unit_arg"), py::arg("unit_n"),
             py::arg("n_units"), py::arg("order_op"), py::arg("order_item"),
             py::arg("order_n"), py::arg("n_orders"))
        .def("pass_step", &VectorEnv::pass_step)
        .def("status", &VectorEnv::status)
        .def_property_readonly("size", &VectorEnv::size);

    py::class_<ReplayCursor>(module, "ReplayCursor")
        .def(py::init<const std::string&>(), py::arg("path"))
        .def("observe", &ReplayCursor::observe, py::arg("seat"))
        .def("recorded_action", &ReplayCursor::recorded_action, py::arg("seat"))
        .def("solo_effective_action", &ReplayCursor::solo_effective_action,
             py::arg("seat"))
        .def("joint_effective_action", &ReplayCursor::joint_effective_action,
             py::arg("seat"))
        .def("advance", &ReplayCursor::advance)
        .def("target", &ReplayCursor::target, py::arg("seat"))
        .def("verify", &ReplayCursor::verify)
        .def("verify_joint_canonicalization",
             &ReplayCursor::verify_joint_canonicalization)
        .def_property_readonly("step", &ReplayCursor::step)
        .def_property_readonly("n_steps", &ReplayCursor::n_steps)
        .def_property_readonly("seed", &ReplayCursor::seed)
        .def_property_readonly("shed_capacity", &ReplayCursor::shed_capacity)
        .def_property_readonly("teams", &ReplayCursor::teams)
        .def_property_readonly("submission_ids", &ReplayCursor::submission_ids)
        .def_property_readonly("rewards", &ReplayCursor::rewards);
}
