"""Export cost scoring and bounded compiler-call models to fixed C++ arrays."""
import argparse
import hashlib
import json
import sys
from pathlib import Path

import joblib
import numpy as np


EXP = Path(__file__).resolve().parents[1]


def number(value):
    assert np.isfinite(value)
    return format(float(value), ".17g")


def forest(name, estimators, boosted=False):
    nodes, roots = [], []
    for estimator in estimators:
        offset = len(nodes); roots.append(offset)
        if boosted:
            for node in estimator.nodes:
                leaf = bool(node["is_leaf"])
                assert not node["is_categorical"]
                nodes.append((int(node["feature_idx"]), -1 if leaf else int(node["left"]) + offset,
                              -1 if leaf else int(node["right"]) + offset, node["num_threshold"], node["value"]))
        else:
            tree = estimator.tree_
            for i in range(tree.node_count):
                left, right = int(tree.children_left[i]), int(tree.children_right[i])
                nodes.append((int(tree.feature[i]), -1 if left < 0 else left + offset,
                              -1 if right < 0 else right + offset, tree.threshold[i], tree.value[i, 0, 0]))
    lines = [f"inline constexpr Node {name}_nodes[] = {{"]
    lines += [f"{{{f},{l},{r},{number(t)},{number(v)}}}," for f, l, r, t, v in nodes]
    lines += ["};", f"inline constexpr int {name}_roots[] = {{{','.join(map(str, roots))}}};"]
    return lines, {"trees": len(roots), "nodes": len(nodes)}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("cost_study", type=Path)
    parser.add_argument("query_study", type=Path)
    parser.add_argument("name")
    args = parser.parse_args()
    output = EXP / "models" / args.name
    output.mkdir(exist_ok=False)
    sys.path[:0] = [str(args.cost_study.resolve()), str(args.query_study.resolve())]
    cost = joblib.load(args.cost_study / "models.joblib")
    query = joblib.load(args.query_study / "models.joblib")
    names = cost["feature_names"]
    assert query["feature_names"] == names and len(names) == 233
    logistic = query["models"]["logistic"]
    weights = logistic.model.coef_[0] / logistic.scale
    intercept = logistic.model.intercept_[0] - weights @ logistic.center
    header = ["#pragma once", '#include "features.hpp"', "namespace labor::search_model {",
              "static_assert(labor::count == 233);", "using QueryFeatures = std::array<double, 255>;",
              "inline QueryFeatures query_features(const Features& f, int workers, int lower) {",
              "QueryFeatures x{}; for(int i=0;i<count;++i) x[i]=f[i]; int n=count;",
              "x[n++]=workers; x[n++]=workers-lower; const double capacity=24+23*(workers-1);",
              "for(Feature i : {tasks,task_motion_bound,task_distance,rooted_mst,input_actions,output_actions,route_pack_open,route_pack_return,deadline_pressure_max,required_pickup_types}) {",
              "x[n++]=double(f[i])/workers; x[n++]=double(f[i])/capacity; } return x; }",
              "struct Node { int feature,left,right; double threshold,value; };",
              "template<class X, size_t N, size_t R> inline double sum_trees(const X& x,const Node (&nodes)[N],const int (&roots)[R]) {",
              "double result=0; for(int root:roots) { int n=root; while(nodes[n].left>=0) n=double(x[nodes[n].feature])<=nodes[n].threshold?nodes[n].left:nodes[n].right; result+=nodes[n].value; } return result; }",
              "inline double sigmoid(double x) { if(x>=0)return 1/(1+std::exp(-x)); const double e=std::exp(x);return e/(1+e); }",
              f"inline constexpr double logistic_weights[] = {{{','.join(number(v) for v in weights)}}};",
              f"inline double logistic(const QueryFeatures& x) {{ double y={number(intercept)}; for(int i=0;i<255;++i)y+=x[i]*logistic_weights[i];return sigmoid(y); }}"]
    metadata = {}
    lines, metadata["cost"] = forest("cost", cost["models"]["cost_direct_extra"].model.estimators_)
    header += lines + [f"inline double cost(const Features& f) {{ return sum_trees(f,cost_nodes,cost_roots)/{metadata['cost']['trees']}; }}"]
    for name in ["boost", "cpu"]:
        model = query["models"][name].model
        assert all(len(trees) == 1 for trees in model._predictors)
        lines, metadata[name] = forest(name, [trees[0] for trees in model._predictors], True)
        header += lines
        value = number(np.ravel(model._baseline_prediction)[0])
        transform = "sigmoid" if name == "boost" else "std::exp"
        header += [f"inline double {name}(const QueryFeatures& x) {{ return {transform}({value}+sum_trees(x,{name}_nodes,{name}_roots)); }}"]
    header += ["}"]
    (output / "search_model.hpp").write_text("\n".join(header) + "\n")
    report = {"status": "Development export; not accepted or frozen for an untouched wave.", "feature_names": names,
              "models": metadata, "query_input_count": 255, "active_hours": 24,
              "cpu_target": "Log CPU: output is a geometric-mean heuristic, not expected CPU.",
              "source_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in [args.cost_study / "models.joblib", args.query_study / "models.joblib", EXP / "include/features.hpp", EXP / "include/bounds.hpp"]},
              "header_sha256": hashlib.sha256((output / "search_model.hpp").read_bytes()).hexdigest(),
              "inference": "Fixed C++ arrays, physical features and explicit proposed workforce. No solver, schedule, Python, file IO or allocation in the model call."}
    (output / "MANIFEST.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(metadata))


if __name__ == "__main__":
    main()
