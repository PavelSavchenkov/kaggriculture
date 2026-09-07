"""Format all verified replay lifecycles for the isolated C++ compiler version."""
import csv
import hashlib
import json
import re
from collections import defaultdict
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]
TARGET = EXP / "candidates/composition_greedy_v1"
OPERATIONS = ["FERTILIZE", "WATER", "FEED", "CARE", "COLLECT_FERTILIZER", "HARVEST"]


def main():
    metadata = json.loads((EXP / "research/replay_library_sources_v4.json").read_text())
    proposals = {}
    support = defaultdict(lambda: {"hands": [0] * 30, "land": [0] * 30})
    masks = defaultdict(lambda: [0] * 6)
    inputs = []
    for suffix in ["", "refresh_0314", "refresh_0504", "refresh_0604"]:
        directory = EXP / "research" / suffix
        data = json.loads((directory / "compositions.json").read_text())
        for proposal in data["proposals"]:
            key = (proposal["id"], proposal["leaderboard_snapshot"])
            assert key not in proposals
            proposals[key] = proposal
        with (directory / "unit_events.csv").open() as stream:
            for row in csv.DictReader(stream):
                if row["operation"] not in OPERATIONS or not row["origin_day"]:
                    continue
                key = (suffix, int(row["episode_id"]), row["team"], int(row["x"]), int(row["y"]), row["source"], int(row["origin_day"]))
                masks[key][OPERATIONS.index(row["operation"])] |= 1 << int(row["day"])
        with (directory / "transactions.csv").open() as stream:
            for row in csv.DictReader(stream):
                entry = support[(suffix, int(row["episode_id"]), row["team"])]
                if row["operation"] == "HIRE":
                    entry["hands"][int(row["day"])] += int(row["actual"])
                if row["operation"] == "BUY_LAND":
                    entry["land"][int(row["day"])] += int(row["actual"])
        for proposal in data["proposals"]:
            proposal["research_suffix"] = suffix
        for name in ["compositions.json", "unit_events.csv", "transactions.csv"]:
            file = directory / name
            inputs.append({"path": str(file.relative_to(EXP)), "sha256": hashlib.sha256(file.read_bytes()).hexdigest()})
    values, hands, land, offsets = [], [], [], [0]
    for index, entry in enumerate(metadata):
        assert entry["program"] == index
        proposal = proposals[(entry["id"], entry["leaderboard_snapshot"])]
        suffix = proposal["research_suffix"]
        for instance in sorted(proposal["instances"], key=lambda v: (v["start_state"], v["y"], v["x"])):
            key = (suffix, proposal["episode"], proposal["team"], instance["x"], instance["y"], data["items"][instance["item"]], instance["start_day"])
            values.append([instance["item"], max(0, instance["start_state"] - 1), instance["end_state"], instance["x"], instance["y"], *masks[key]])
        offsets.append(len(values))
        observed = support[(suffix, proposal["episode"], proposal["team"])]
        hands.append(observed["hands"])
        quadrants, row = 1, []
        for purchases in observed["land"]:
            quadrants += purchases
            row.append(quadrants)
        land.append(row)
    old_file = EXP / "candidates/composition_greedy_v0/source/programs.inc"
    old = old_file.read_text()

    def old_array(name):
        match = re.search(rf"\b{name}(?:\[[^]]*\])*\s*=\s*(\{{.*?\}});", old, re.S)
        assert match, name
        return json.loads(match[1].replace("{", "[").replace("}", "]"))

    old_offsets = old_array("program_offsets")
    assert offsets[:len(old_offsets)] == old_offsets
    assert values[:old_offsets[-1]] == old_array("recorded_intents")
    assert hands[:72] == old_array("recorded_hands")
    assert land[:72] == old_array("recorded_quadrants")
    code = "// Verified replay facts; strategic compilation is C++.\n"
    code += "inline constexpr Intent recorded_intents[] = {\n" + ",\n".join("{" + ",".join(map(str, value)) + "}" for value in values) + "\n};\n"
    code += "inline constexpr int program_offsets[] = {" + ",".join(map(str, offsets)) + "};\n"
    for name, rows in [("recorded_hands", hands), ("recorded_quadrants", land)]:
        code += f"inline constexpr int {name}[{len(metadata)}][30] = {{\n" + ",\n".join("{" + ",".join(map(str, row)) + "}" for row in rows) + "\n};\n"
    (TARGET / "source/programs.inc").write_text(code)
    (TARGET / "PROGRAM_SOURCES.json").write_text(json.dumps(metadata, indent=2) + "\n")
    report = {"programs": len(metadata), "lives": len(values), "old_72_all_facts_identical": True,
              "original_sha256": hashlib.sha256(old_file.read_bytes()).hexdigest(), "inputs": inputs}
    (TARGET / "PROGRAM_DATA_IMPORT.json").write_text(json.dumps(report, indent=2) + "\n")
    print({key: value for key, value in report.items() if key != "inputs"})


if __name__ == "__main__":
    main()
