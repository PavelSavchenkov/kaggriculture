"""Export the development calendar-aware call models to fixed C++ arrays."""
import argparse
import hashlib
import json
import sys
from pathlib import Path

import joblib
import numpy as np

from export_search_cpp import forest, number


EXP = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("study", type=Path)
    parser.add_argument("name")
    args = parser.parse_args()
    output = EXP / "models" / args.name; output.mkdir(exist_ok=False)
    sys.path.insert(0, str(args.study.resolve()))
    artifact = joblib.load(args.study / "models.joblib")
    assert artifact["feature_schema"] == "context_query_320_v1"
    models = artifact["models"]["calendar"]
    linear = models["logistic"]
    weights = linear.model.coef_[0] / linear.scale
    intercept = linear.model.intercept_[0] - weights @ linear.center
    assert len(weights) == 320
    header = ["#pragma once", '#include "context_query_features.hpp"', "namespace labor::context_model {",
        "static_assert(context_query_count == 320);", "struct Node { int feature,left,right; double threshold,value; };",
        "template<size_t N,size_t R> inline double sum_trees(const ContextQueryFeatures& x,const Node (&nodes)[N],const int (&roots)[R]) {",
        "double y=0;for(int root:roots){int n=root;while(nodes[n].left>=0)n=x[nodes[n].feature]<=nodes[n].threshold?nodes[n].left:nodes[n].right;y+=nodes[n].value;}return y;}",
        "inline double sigmoid(double x){if(x>=0)return 1/(1+std::exp(-x));const double e=std::exp(x);return e/(1+e);}",
        "inline constexpr double logistic_weights[]={" + ",".join(map(number, weights)) + "};",
        f"inline double logistic(const ContextQueryFeatures& x){{double y={number(intercept)};for(int i=0;i<context_query_count;++i)y+=x[i]*logistic_weights[i];return sigmoid(y);}}"]
    metadata = {}
    for name in ["boost", "cpu"]:
        model = models[name].model
        assert all(len(trees) == 1 for trees in model._predictors)
        lines, metadata[name] = forest(name, [trees[0] for trees in model._predictors], True)
        header += lines
        baseline = number(np.ravel(model._baseline_prediction)[0])
        expression = f"{baseline}+sum_trees(x,{name}_nodes,{name}_roots)"
        expression = f"sigmoid({expression})" if name == "boost" else f"std::max(.0001,std::exp({expression}))"
        header.append(f"inline double {name}(const ContextQueryFeatures& x){{return {expression};}}")
    header.append("}")
    target = output / "context_model.hpp"; target.write_text("\n".join(header) + "\n")
    report = {"status": "Development export, not adopted or frozen for fresh testing.", "active_hours": [23, 24],
        "target": "Success and CPU of a proposed cold three-second compiler call. No feasibility claim and no physical cost prediction.",
        "cpu_target": "Exponentiated mean log CPU, a geometric-mean heuristic.", "query_features": 320, "models": metadata,
        "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in [args.study / "models.joblib", args.study / "PROTOCOL.json", Path(__file__), EXP / "scripts/export_search_cpp.py", EXP / "include/context_query_features.hpp"]},
        "header_sha256": hashlib.sha256(target.read_bytes()).hexdigest()}
    (output / "MANIFEST.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(metadata))


if __name__ == "__main__":
    main()
