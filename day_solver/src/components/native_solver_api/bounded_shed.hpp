// Included inside ExactModel. Storage follows worker order, cargo insertion
// order, then withdrawals and fixed purchases, matching strict replay.
    void add_bounded_shed() {
        const Count capacity = problem.start.shed_capacity;
        std::vector<int> carried_items;
        for (int item = 0; item < items; ++item)
            if (max_cargo[item]) carried_items.push_back(item);

        // Each action adds at most one cargo kind. Its insertion key is the
        // hour it first became positive; consuming the last unit erases it.
        std::vector<IntVar> order(cargo.size());
        std::vector<BoolVar> positive(cargo.size());
        for (int worker = 0; worker < workers; ++worker)
            for (int hour = releases[worker]; hour <= hours; ++hour)
                for (int item : carried_items) {
                    const int key = cargo_index(worker, hour, item);
                    order[key] = integer(0, hours, name("cargo_order", {worker, hour, item}));
                    positive[key] = boolean(name("cargo_positive", {worker, hour, item}));
                    model.AddGreaterOrEqual(cargo[key], 1).OnlyEnforceIf(positive[key]);
                    model.AddEquality(cargo[key], 0).OnlyEnforceIf(positive[key].Not());
                    model.AddEquality(order[key], 0).OnlyEnforceIf(positive[key].Not());
                    if (hour == releases[worker]) continue;
                    const int before = cargo_index(worker, hour - 1, item);
                    model.AddEquality(order[key], order[before]).OnlyEnforceIf({positive[key], positive[before]});
                    model.AddEquality(order[key], hour).OnlyEnforceIf({positive[key], positive[before].Not()});
                }

        std::array<LinearExpr, items> shed;
        for (int item = 0; item < items; ++item) shed[item] = problem.start.shed[item];
        auto shed_total = [&] {
            LinearExpr total;
            for (const auto& value : shed) total += value;
            return total;
        };
        auto transfer = [&](int worker, int hour, BoolVar enabled) {
            const auto room = capacity - shed_total();
            std::array<LinearExpr, items> deposited;
            for (int item : carried_items) {
                const int key = cargo_index(worker, hour, item);
                LinearExpr preceding;
                for (int other : carried_items) {
                    if (other == item) continue;
                    const int other_key = cargo_index(worker, hour, other);
                    const auto earlier = boolean(name("cargo_before", {worker, hour, other, item}));
                    model.AddLessThan(order[other_key], order[key]).OnlyEnforceIf(earlier);
                    model.AddGreaterOrEqual(order[other_key], order[key]).OnlyEnforceIf(earlier.Not());
                    const auto quantity = integer(0, max_cargo[other], name("earlier_cargo", {worker, hour, other, item}));
                    model.AddEquality(quantity, cargo[other_key]).OnlyEnforceIf(earlier);
                    model.AddEquality(quantity, 0).OnlyEnforceIf(earlier.Not());
                    preceding += quantity;
                }
                const auto remaining = integer(0, capacity, name("cargo_room", {worker, hour, item}));
                model.AddMaxEquality(remaining, {room - preceding, LinearExpr(0)});
                const auto fits = integer(0, std::min(max_cargo[item], capacity), name("cargo_fits", {worker, hour, item}));
                model.AddMinEquality(fits, {LinearExpr(cargo[key]), LinearExpr(remaining)});
                const auto value = integer(0, std::min(max_cargo[item], capacity), name("bounded_drop", {worker, hour, item}));
                model.AddEquality(value, fits).OnlyEnforceIf(enabled);
                model.AddEquality(value, 0).OnlyEnforceIf(enabled.Not());
                deposited[item] = value;
            }
            return deposited;
        };
        for (int hour = 0; hour < hours; ++hour) {
            for (int worker = 0; worker < workers; ++worker) {
                if (releases[worker] > hour) continue;
                const auto deposited = transfer(worker, hour, drop[worker * hours + hour]);
                for (int item = 0; item < items; ++item) {
                    const int key = inventory_index(worker, hour, item);
                    const auto value = integer(0, std::min(max_stock[item], capacity), name("bounded_shed_stage", {hour, worker, item}));
                    model.AddEquality(value, shed[item] - quantity(pickup_quantity[key]) + quantity(place_quantity[key]) + deposited[item]);
                    shed[item] = value;
                }
                model.AddLessOrEqual(shed_total(), capacity);
            }
            for (int item = 0; item < items; ++item) {
                const Count removed = problem.shed_availability[hour][item] - (hour ? problem.shed_availability[hour - 1][item] : 0);
                model.AddGreaterOrEqual(shed[item], removed);
                shed[item] -= removed;
            }
            // Fixed v3 market events contain only acquisitions. Enforce space
            // for every purchased unit; a partial purchase is not acceptable.
            for (const auto& event : problem.market_plan)
                if (event.hour == hour && (event.market_op == kag::M_BUY_PRODUCT || event.market_op == kag::M_BUY_ANIMAL)) {
                    shed[event.item] += event.quantity;
                    model.AddLessOrEqual(shed_total(), capacity);
                }
        }
        for (int worker = 0; worker < workers; ++worker) {
            const auto deposited = transfer(worker, hours, model.TrueVar());
            for (int item = 0; item < items; ++item) shed[item] += deposited[item];
        }
        for (int item = 0; item < items; ++item)
            model.AddEquality(shed[item], problem.end_shed[item]);
    }
