"""Export a timing formula, stable ridge and formula-plus-trees to C++."""
import argparse
import hashlib
import json
import sys
from pathlib import Path

import joblib


EXP = Path(__file__).resolve().parents[1]


def numbers(values):
    return ",".join(format(float(v), ".17g") for v in values)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("study", type=Path)
    parser.add_argument("name")
    args = parser.parse_args()
    output = EXP / "models" / args.name
    output.mkdir(parents=True, exist_ok=False)
    sys.path.insert(0, str(args.study.resolve()))
    artifact = joblib.load(args.study / "models.joblib")
    models = artifact["models"]
    formula, ridge, residual = models["fitted_timing"], models["ridge_floor"], models["timing_extra"]
    header = ['#pragma once', '#include "features.hpp"', 'namespace labor::model {',
              f'static_assert(labor::count == {len(artifact["feature_names"])});',
              'inline std::array<double,12> basis(const Features& f) {',
              'auto g = [&](Feature i) -> double { return f[i]; };',
              'return {1,g(tasks),g(rooted_mst),g(output_actions),g(input_actions),g(required_pickup_types),',
              'g(deadline_pressure_max),g(route_pack_open),g(route_pack_return)-g(route_pack_open),',
              'g(late_input_pressure)/std::max(1.0,g(input_actions)),g(locked_work_pressure)/std::max(1.0,g(tasks)),',
              'g(deadline_required_ops_max)/std::max(1.0,g(output_actions))}; }',
              f'inline constexpr double formula_weights[] = {{{numbers(formula.coefficient)}}};',
              f'inline constexpr double base_weights[] = {{{numbers(residual.base.coefficient)}}};',
              f'inline constexpr double ridge_weights[] = {{{numbers(ridge.coefficient)}}};',
              f'inline constexpr double ridge_intercept = {float(ridge.intercept):.17g};',
              'inline double formula(const Features& f) { const auto x=basis(f); double y=0; for(int i=0;i<12;++i)y+=x[i]*formula_weights[i]; return y; }',
              'inline double ridge(const Features& f) { double y=ridge_intercept; for(int i=0;i<count;++i)y+=double(f[i])*ridge_weights[i]; return y; }',
              'struct Node { int feature,left,right; double threshold,value; };']
    nodes, roots = [], []
    for estimator in residual.residual.estimators_:
        tree = estimator.tree_
        offset = len(nodes); roots.append(offset)
        for i in range(tree.node_count):
            left, right = int(tree.children_left[i]), int(tree.children_right[i])
            nodes.append(f'{{{int(tree.feature[i])},{left + offset if left >= 0 else -1},{right + offset if right >= 0 else -1},{float(tree.threshold[i]):.17g},{float(tree.value[i, 0, 0]):.17g}}}')
    header += ['inline constexpr Node nodes[] = {', ',\n'.join(nodes), '};',
               'inline constexpr int roots[] = {' + ','.join(map(str, roots)) + '};',
               'inline double trees(const Features& f) {',
               'const auto x=basis(f); double y=0; for(int i=0;i<12;++i)y+=x[i]*base_weights[i];',
               'double residual=0; for(int root:roots) { int n=root; while(nodes[n].left>=0) n=double(f[nodes[n].feature])<=nodes[n].threshold?nodes[n].left:nodes[n].right; residual+=nodes[n].value; }',
               f'return y+residual/{len(roots)}; }}', '}']
    (output / "model.hpp").write_text("\n".join(header) + "\n")
    metadata = {"status": "candidate; not accepted", "exported_methods": ["fitted_timing", "ridge_floor", "timing_extra"],
                "feature_names": artifact["feature_names"], "tree_nodes": len(nodes), "trees": len(roots),
                "source_model_sha256": hashlib.sha256((args.study / "models.joblib").read_bytes()).hexdigest(),
                "dataset_sha256": hashlib.sha256((args.study / "DATASET.json").read_bytes()).hexdigest(),
                "features_source_sha256": hashlib.sha256((EXP / "include/features.hpp").read_bytes()).hexdigest(),
                "header_sha256": hashlib.sha256((output / "model.hpp").read_bytes()).hexdigest(),
                "inference": "Fixed C++ arrays; no Python, solver, JSON, file I/O or dynamic allocation in model evaluation."}
    (output / "MANIFEST.json").write_text(json.dumps(metadata, indent=2) + "\n")
    print(f"Exported{len(nodes)} nodes in{len(roots)} trees and two linear formulas")


if __name__ == "__main__":
    main()
