"""Create a parameter-frozen cold corpus using the C++ local-state oracle."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path

from import_problems import physical_key


EXP = Path(__file__).resolve().parents[1]
REPO = EXP.parents[2]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("name")
    parser.add_argument("--seed", type=int, required=True)
    args = parser.parse_args()
    output = EXP / "data" / args.name
    output.mkdir(exist_ok=False)
    specifications = []
    for profile in range(17):
        for index in range(17):
            threshold = index >= 12
            local_seed = args.seed + 1009 * profile + (900 if threshold else index)
            count = [1, 2, 4, 8, 12, 16, 24, 36, 48, 64, 80, 96][index] if not threshold else 10 + (profile * 7 % 39) + index - 12
            if 13 <= profile < 16:
                count = min(count, 36 + index - 12) if threshold else min(count, 40)
            geometry = profile % 3 if threshold else (profile + index) % 3
            deadline = [-1, 10, 18][profile % 3] if threshold else (-1 if index < 4 else [8, 12, 18, 22][index % 4])
            release = [-1, 0, 8, 16][profile % 4] if threshold else (-1 if index < 4 else [-1, 0, 8, 21][index % 4])
            specifications.append({"id": f"p{profile:02}_i{index:02}", "seed": local_seed, "profile": profile, "count": count,
                "geometry": geometry, "deadline": deadline, "release": release,
                "family": f"synthetic_profile_{profile:02}", "pool": f"synthetic_{args.seed}_p{profile:02}" if threshold else None,
                "variant": ("original" if index == 12 else f"add{index - 12}") if threshold else None})
    manifest = output / "generator.txt"
    manifest.write_text("".join(f"{r['id']} {r['seed']} {r['profile']} {r['count']} {r['geometry']} {r['deadline']} {r['release']}\n" for r in specifications))
    protocol = {"scope": "Independent relative-age dawn contracts, constructed without source routes or learned predictions.",
                "profiles": "Five crop maintenance, five mature harvest/replant, three animal service, three animal construction, and a mixed profile.",
                "geometry": "Near, far and deterministic shuffled positions on an owned board; omitted tiles are traversable.",
                "timing": "No/early/late input releases and withdrawal deadlines; impossible or difficult inputs remain in the corpus.",
                "threshold_pools": "Five adjacent asset counts sharing seed, arrangement, timing and local-state prefix. Their output value differs.",
                "endpoint_oracle": "Independently simulate each tile for24 hours with a local worker and the required inputs. This defines exact field effects and night transitions; it is not a global scheduling witness.",
                "scope_limits": "Same public v3 physical contract: cash, shed capacity and rival markets remain outside the model. Dense contracts may need those constraints handled separately in a full-game caller.",
                "label_ceiling": "The generated worker_count is a workload-based query ceiling, not a verified answer and never a feature.",
                "seed": args.seed, "specifications": specifications,
                "generator_sha256": hashlib.sha256((EXP / "source/generate_synthetic.cpp").read_bytes()).hexdigest(),
                "binary_sha256": hashlib.sha256((EXP / "build/generate_synthetic").read_bytes()).hexdigest()}
    (output / "PROTOCOL.json").write_text(json.dumps(protocol, indent=2) + "\n")
    command = ["conda", "run", "--no-capture-output", "-n", "kaggriculture", str(REPO / "day_solver/with_runtime.sh"),
               str(EXP / "build/generate_synthetic"), str(manifest), str(output / "contracts")]
    subprocess.run(command, cwd=REPO, check=True)
    rows = []
    for spec in specifications:
        path = output / "contracts" / (spec["id"] + ".json")
        p = json.loads(path.read_text()); source = f"cold/{args.name}/{spec['id']}"
        rows.append({"source": source, "sources": [source], "source_family": spec["family"], "pool": spec["pool"], "variant": spec["variant"],
                     "problem": str(path.relative_to(EXP)), "source_sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
                     "physical_key": physical_key(p), "observed_workers": p["worker_count"],
                     "worker_count_provenance": "unverified workload-based query ceiling", "witnesses": [],
                     "tasks": sum(len(r["actions"]) for r in p["tile_work"]), "active_tiles": len(p["tile_work"]), "parameters": spec})
    (output / "index.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in rows))
    print(f"{len(rows)} independent cold contracts in17 profile families, including17 five-plan threshold pools")


if __name__ == "__main__":
    main()
