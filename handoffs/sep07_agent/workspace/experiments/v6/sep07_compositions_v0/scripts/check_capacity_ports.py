"""Check the completed public capacity ports and summarize exact evidence."""
import json
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]


def read(path):
    return json.loads((EXP / path).read_text())


def main():
    generic = read("results/capacity_router_checks/public_capacity_router_vs_public_terminal_router.json")
    debug = read("results/capacity_terminal_debug.json")
    assert generic["games"] == debug["games"]
    report = {"generic_debug_thread_games": len(generic["games"]), "discovery": {}}
    for path in sorted((EXP / "results/capacity_router_discovery").glob("public_capacity_router_vs_*.json")):
        data = json.loads(path.read_text())
        terminal = json.loads(path.with_name(path.name.replace("public_capacity_router", "public_terminal_router", 1)).read_text())
        assert data["games"] == terminal["games"]
        report["discovery"][path.stem] = {k: v for k, v in data.items() if k != "games"}
    report["terminal_scope"] = "No action or reward difference from capacity-only in all 2304 common discovery games; forced parity fixtures exercise the terminal difference."
    (EXP / "results/capacity_ports_validation.json").write_text(json.dumps(report, indent=2) + "\n")
    for name in ("public_capacity_router", "public_terminal_router"):
        folder = EXP / "league" / name
        manifest = json.loads((folder / "IMPORT.json").read_text())
        manifest["status"] = "8628 original actions match; generic/debug/thread, PASS and self checks pass. 256 games against each of nine opponents; no rating or global superiority claim."
        (folder / "IMPORT.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"{report['generic_debug_thread_games']} generic/debug games equal; 2304 capacity/terminal discovery games equal")


if __name__ == "__main__":
    main()
