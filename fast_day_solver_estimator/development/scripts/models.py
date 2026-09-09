"""Small offline-trained predictors with straightforward C++ equivalents."""
import numpy as np
from scipy.optimize import nnls
from sklearn.ensemble import ExtraTreesRegressor, HistGradientBoostingRegressor
from sklearn.linear_model import Ridge
from sklearn.neighbors import KNeighborsRegressor

from baseline_study import cost


def design(x, names, kind):
    ix = {name: i for i, name in enumerate(names)}
    def f(name):
        return x[:, ix[name]]
    one = np.ones(len(x))
    if kind == "tasks":
        return np.column_stack([one, f("tasks")])
    if kind == "geometry":
        transport = (2 * (f("input_distance") + f("output_distance")) + f("withdrawal_total")) / 12
        return np.column_stack([one, f("tasks"), f("rooted_mst"), transport, f("distance_max"), f("early_withdrawal_deficit")])
    if kind == "timing":
        return np.column_stack([one, f("tasks"), f("rooted_mst"), f("output_actions"), f("input_actions"),
                                f("required_pickup_types"), f("deadline_pressure_max"), f("route_pack_open"),
                                f("route_pack_return") - f("route_pack_open"),
                                f("late_input_pressure") / np.maximum(1, f("input_actions")),
                                f("locked_work_pressure") / np.maximum(1, f("tasks")),
                                f("deadline_required_ops_max") / np.maximum(1, f("output_actions"))])
    if kind == "patterns":
        return np.column_stack([f(k) for k in ["tasks", "active_tiles", "rooted_mst", "output_actions", "input_actions",
                                "distance_max", "route_pack_open", "route_pack_return", "deadline_pressure_max",
                                "purchase_last_hour", "early_withdrawal_deficit", "late_input_pressure",
                                "neighbor_work_1", "neighbor_work_2", "max_tile_tasks", "required_pickup_types"]])
    if kind == "routing":
        opened = np.where(f("optimized_open_workers") <= 40, f("optimized_open_workers"), f("route_pack_open"))
        returned = np.where(f("optimized_return_workers") <= 40, f("optimized_return_workers"), f("route_pack_return"))
        return np.column_stack([design(x, names, "timing"), opened, np.maximum(0, returned - opened),
                                1 / (1 + f("optimized_open_slack")), f("optimized_tour_open_distance")])
    raise ValueError(kind)


class Formula:
    def __init__(self, names, kind):
        self.names, self.kind = names, kind

    def fit(self, x, y):
        self.coefficient, _ = nnls(design(x, self.names, self.kind), y)
        return self

    def predict(self, x):
        return design(x, self.names, self.kind) @ self.coefficient


class FloorRidge:
    def fit(self, x, y):
        center = np.mean(x, axis=0)
        # Integer counts and almost-constant summary ratios must not acquire
        # unbounded influence from tiny training standard deviations.
        scale = np.maximum(1.0, np.std(x, axis=0))
        model = Ridge(alpha=20).fit((x - center) / scale, y)
        self.coefficient = model.coef_ / scale
        self.intercept = model.intercept_ - center @ self.coefficient
        return self

    def predict(self, x):
        return x @ self.coefficient + self.intercept


class ResidualModel:
    def __init__(self, names, base_kind, residual_kind):
        self.names, self.base_kind, self.residual_kind = names, base_kind, residual_kind

    def fit(self, x, y):
        self.base = Formula(self.names, self.base_kind).fit(x, y)
        if self.residual_kind == "extra":
            self.residual = ExtraTreesRegressor(n_estimators=128, max_depth=14, min_samples_leaf=4, random_state=909, n_jobs=1)
        elif self.residual_kind == "boost":
            self.residual = HistGradientBoostingRegressor(max_iter=160, max_leaf_nodes=15, learning_rate=.06,
                                                          l2_regularization=5, early_stopping=False, random_state=909)
        elif self.residual_kind == "patterns":
            self.residual = KNeighborsRegressor(n_neighbors=7, weights="distance")
            z = design(x, self.names, "patterns")
            self.center, self.scale = z.mean(axis=0), np.maximum(1, z.std(axis=0))
        else:
            raise ValueError(self.residual_kind)
        self.residual.fit(self.transform(x), y - self.base.predict(x))
        return self

    def transform(self, x):
        if self.residual_kind == "patterns":
            return (design(x, self.names, "patterns") - self.center) / self.scale
        return x

    def predict(self, x):
        return self.base.predict(x) + self.residual.predict(self.transform(x))


class MonotoneTiming:
    def __init__(self, names):
        self.names = names

    def fit(self, x, y):
        self.model = HistGradientBoostingRegressor(max_iter=160, max_leaf_nodes=15, learning_rate=.06,
            min_samples_leaf=10, l2_regularization=5, early_stopping=False, random_state=909,
            monotonic_cst=[0] + [1] * 11)
        self.model.fit(design(x, self.names, "timing"), y)
        return self

    def predict(self, x):
        return self.model.predict(design(x, self.names, "timing"))


class CostForest:
    def __init__(self, kind):
        self.kind = kind

    def fit(self, x, y):
        self.model = ExtraTreesRegressor(n_estimators=128, max_depth=14, min_samples_leaf=4, random_state=909, n_jobs=1)
        prices = cost(y)
        target = y if self.kind == "worker_leaf" else np.sqrt(prices) if self.kind == "root_cash" else prices
        self.model.fit(x, target)
        if self.kind == "worker_leaf":
            # Preserve workforce-based partitions, then estimate expected cost
            # inside each leaf. This is not cost(mean workforce).
            for tree in self.model.estimators_:
                leaf = tree.apply(x)
                total = np.bincount(leaf, weights=prices, minlength=tree.tree_.node_count)
                count = np.bincount(leaf, minlength=tree.tree_.node_count)
                tree.tree_.value[:, 0, 0] = np.divide(total, count, out=np.zeros_like(total), where=count > 0)
        return self

    def predict(self, x):
        estimate = self.model.predict(x)
        if self.kind == "root_cash":
            estimate = estimate ** 2
        # Expose a cost-equivalent workforce through the existing evaluator.
        # It is deliberately not described as mean workforce.
        return np.interp(estimate, cost(np.arange(1, 41)), np.arange(1, 41))


def candidates(names):
    result = {"fitted_tasks": Formula(names, "tasks"), "fitted_geometry": Formula(names, "geometry"),
            "fitted_timing": Formula(names, "timing"), "ridge_floor": FloorRidge(),
            "extra_trees": ExtraTreesRegressor(n_estimators=128, max_depth=14, min_samples_leaf=4, random_state=909, n_jobs=1),
            "geometry_extra": ResidualModel(names, "geometry", "extra"),
            "timing_extra": ResidualModel(names, "timing", "extra"),
            "timing_boost": ResidualModel(names, "timing", "boost"),
            "timing_patterns": ResidualModel(names, "timing", "patterns"),
            "monotone_timing": MonotoneTiming(names),
            "cost_direct_extra": CostForest("cash"), "cost_root_extra": CostForest("root_cash"),
            "cost_worker_leaf": CostForest("worker_leaf")}
    if "optimized_open_workers" in names:
        result.update(fitted_routing=Formula(names, "routing"), routing_extra=ResidualModel(names, "routing", "extra"),
                      routing_boost=ResidualModel(names, "routing", "boost"))
    return result
