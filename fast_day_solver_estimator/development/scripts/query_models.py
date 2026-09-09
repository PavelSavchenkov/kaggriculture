"""Predict a bounded compiler call's success and CPU, not physical feasibility."""
import numpy as np
from sklearn.ensemble import HistGradientBoostingClassifier, HistGradientBoostingRegressor
from sklearn.linear_model import LogisticRegression


def design(x, workers, lower, names):
    # Features are C++ floats. CSV's decimal spelling is only transport, and
    # tiny spelling differences must not change boosted-tree branch decisions.
    x = np.asarray(x, dtype=np.float32).astype(np.float64)
    ix = {name: i for i, name in enumerate(names)}
    k = np.asarray(workers, dtype=float)
    capacity = 24 + 23 * (k - 1)
    columns = [x, k[:, None], (k - lower)[:, None]]
    for name in ["tasks", "task_motion_bound", "task_distance", "rooted_mst", "input_actions", "output_actions",
                 "route_pack_open", "route_pack_return", "deadline_pressure_max", "required_pickup_types"]:
        values = x[:, ix[name]]
        columns.extend([(values / k)[:, None], (values / capacity)[:, None]])
    return np.column_stack(columns)


class SuccessModel:
    def __init__(self, kind):
        self.kind = kind

    def fit(self, x, y):
        self.center = np.zeros(x.shape[1]); self.scale = np.ones(x.shape[1])
        if self.kind == "logistic":
            self.center, self.scale = x.mean(axis=0), np.maximum(1, x.std(axis=0))
            self.model = LogisticRegression(C=.1, max_iter=500, random_state=909)
        elif self.kind == "boost":
            self.model = HistGradientBoostingClassifier(max_iter=160, max_leaf_nodes=15, min_samples_leaf=25,
                learning_rate=.06, l2_regularization=5, early_stopping=False, random_state=909)
        else:
            raise ValueError(self.kind)
        self.model.fit((x - self.center) / self.scale, y)
        return self

    def predict(self, x):
        return self.model.predict_proba((x - self.center) / self.scale)[:, 1]


class CpuModel:
    def fit(self, x, y):
        self.model = HistGradientBoostingRegressor(max_iter=120, max_leaf_nodes=15, min_samples_leaf=25,
            learning_rate=.06, l2_regularization=5, early_stopping=False, random_state=909)
        self.model.fit(x, np.log(np.maximum(.0001, y)))
        return self

    def predict(self, x):
        return np.maximum(.0001, np.exp(self.model.predict(x)))
