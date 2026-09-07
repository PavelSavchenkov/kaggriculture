"""Format every endpoint-checked day contract for the C++ combination search."""
import argparse
import csv
import hashlib
import json
import os
import re
from pathlib import Path

from package_guarded_days import EXP, FIELDS, KINDS


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--namespace", default="day_library")
    parser.add_argument("--schedule-components", action="store_true")
    args = parser.parse_args()
    source, output = args.source.resolve(), args.output.resolve()
    assert source.is_relative_to(EXP) and output.is_relative_to(EXP / "runs")
    assert not output.exists()
    assert re.fullmatch(r"[a-z][a-z0-9_]*", args.namespace)
    output.mkdir()
    rows = list(csv.DictReader((source / "compiled.csv").open()))
    rows = [r for r in rows if all(r[k] == "1" for k in ("solved", "endpoint_equal", "cash_equal"))]
    code = '#pragma once\n#include "../../include/guarded_day.hpp"\n'
    records = []
    for row in rows:
        name = f'{source.name}_{row["id"]}'
        folder = source / "proposals" / name
        header = folder / ("schedule.hpp" if args.schedule_components else "source/agent.hpp")
        code += f'#include "{os.path.relpath(header, output)}"\n'
        records.append((name, folder, row, json.loads((folder / "problem.json").read_text()), header))
    code += f'namespace compositions::{args.namespace} {{\nstruct Entry {{int id;GuardedDay day;}};\ninline std::vector<Entry> entries(){{std::vector<Entry> result;\n'
    for name, folder, row, problem, _ in records:
        tiles = problem["start"]["managed_tiles"]
        assert len(tiles) == 100
        touched = {w["tile"] for w in problem["tile_work"]}
        keys, check = [], []
        for cell, tile in enumerate(tiles):
            assert (tile["x"], tile["y"]) == (cell % 10, cell // 10)
            state = tile["state"]
            keys.append([KINDS[state["kind"]], *[int(state[f]) for f in FIELDS]])
            check.append(int(cell in touched or state["kind"] in ("crop", "coop", "pasture")))
        quadrants = sum(t["state"]["kind"] != "locked" for t in tiles) // 25
        code += f'{{GuardedDay d;d.plan={{{row["day"]},{name}::schedule()}};\nd.tiles={{{{'
        code += ",".join("{{" + ",".join(map(str, key)) + "}}" for key in keys) + "}};\n"
        for field, data in (("check", check), ("shed", problem["start"]["shed"]), ("seeds", problem["start"]["seeds"])):
            code += f'd.{field}={{{",".join(map(str, data))}}};\n'
        code += f'd.quadrants={quadrants};result.push_back({{{row["id"]},std::move(d)}});}}\n'
    code += 'return result;}\ninline std::vector<GuardedDay> select(const std::vector<int>& ids){auto all=entries();std::vector<GuardedDay> result;for(int id:ids){bool found=false;for(auto& e:all)if(e.id==id){result.push_back(e.day);found=true;break;}if(!found)std::abort();}return result;}\n}\n'
    (output / "days.hpp").write_text(code)
    hashes = {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest()
              for _, folder, _, _, header in records for p in (header, folder / "problem.json", folder / "combined_schedule.txt")}
    lineage = {"source_run": str(source.relative_to(EXP)), "components": [{"id": int(r["id"]), "day": int(r["day"])} for r in rows],
               "files": hashes, "namespace": args.namespace, "formatter_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
               "scope": "Offline formatting only. All source day solver endpoint/cash checks passed; selection and gameplay are C++. Physical guards do not certify future finance."}
    (output / "LINEAGE.json").write_text(json.dumps(lineage, indent=2) + "\n")
    print(output, len(rows), "checked day contracts")


if __name__ == "__main__":
    main()
