"""Antisymmetric plan-difference models with no candidate-answer inputs."""
import numpy as np
from sklearn.ensemble import ExtraTreesRegressor, HistGradientBoostingRegressor
from sklearn.linear_model import Ridge

from models import design


class PairModel:
    def __init__(self, names, kind, basis="compact"):
        self.names, self.kind, self.basis = names, kind, basis

    def physical(self, x):
        if self.basis == "full":
            return x
        return np.column_stack([design(x, self.names, "timing")[:, 1:], design(x, self.names, "patterns")])

    def transform(self, a, b):
        left, right = self.physical(a), self.physical(b)
        return np.column_stack([(left + right) / 2, right - left])

    def fit(self, a, b, difference):
        z = np.concatenate([self.transform(a, b), self.transform(b, a)])
        target = np.concatenate([difference, -difference])
        self.scale = np.maximum(1, z.std(axis=0))
        if self.kind == "extra":
            self.model = ExtraTreesRegressor(n_estimators=128, max_depth=12, min_samples_leaf=4,
                                             random_state=913, n_jobs=1)
        elif self.kind == "boost":
            self.model = HistGradientBoostingRegressor(max_iter=160, max_leaf_nodes=15, min_samples_leaf=10,
                learning_rate=.06, l2_regularization=5, early_stopping=False, random_state=913)
        elif self.kind == "ridge":
            self.model = Ridge(alpha=20)
        else:
            raise ValueError(self.kind)
        self.model.fit(z / self.scale, target)
        return self

    def predict(self, a, b):
        return (self.model.predict(self.transform(a, b) / self.scale)
                - self.model.predict(self.transform(b, a) / self.scale)) / 2


def candidates(names):
    return {"pair_compact_extra": PairModel(names, "extra"), "pair_full_extra": PairModel(names, "extra", "full"),
            "pair_compact_boost": PairModel(names, "boost"), "pair_compact_ridge": PairModel(names, "ridge")}
