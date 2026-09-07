"""Build/cache C++ arenas; no game policy or strategy search runs in Python."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path


EXPERIMENT = Path(__file__).resolve().parents[1]
ROOT = EXPERIMENT.parents[2]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--pair", nargs=2)
    parser.add_argument("--debug", action="store_true")
    parser.add_argument("--driver",choices=["arena","probe_openings"],default="arena")
    args = parser.parse_args()
    catalog = json.loads((EXPERIMENT / "configs/league.json").read_text())
    names = sorted(set(args.pair)) if args.pair else sorted(catalog)
    names = [name for name in names if name != "pass"]
    packages = {name: ROOT / catalog[name] for name in names}
    manifests = {name: json.loads((path / "agent.json").read_text()) for name, path in packages.items()}
    flags = ["-std=c++20", "-march=native", "-mtune=native", "-fno-exceptions", "-fno-rtti",
             "-fno-math-errno", "-fno-semantic-interposition", "-fno-plt", "-pthread"]
    flags += ["-O1", "-g", "-DKAG_VERIFY_MASKS"] if args.debug else ["-O3", "-DNDEBUG", "-flto"]
    if args.pair:
        flags += ["-DCOMPOSITIONS_PAIR"]
    compiler = subprocess.check_output(["conda", "run", "-n", "kaggriculture", "g++", "--version"], text=True)
    digest = hashlib.sha256(json.dumps([names, args.pair, args.driver, flags, compiler]).encode())
    roots = [EXPERIMENT / "include", EXPERIMENT / "src", ROOT / "fast_game_engine",
             ROOT / "agents/common/api", ROOT / "agents/common/runtime", *packages.values()]
    roots += [(packages[name] / source).resolve().parent
              for name, manifest in manifests.items() for source in manifest["sources"]]
    for root in roots:
        for path in sorted(root.rglob("*")):
            if path.suffix in {".hpp", ".cpp", ".inc", ".json", ".h", ".txt"} and path.is_file():
                digest.update(str(path.relative_to(ROOT)).encode()); digest.update(path.read_bytes())
    # Candidate wrappers can include generated day plans from a different run.
    # Hash those code dependencies too, without reading large result JSON files.
    for path in sorted((EXPERIMENT / "runs").rglob("*")):
        if path.is_file() and path.suffix in {".hpp", ".cpp", ".inc", ".h"}:
            digest.update(str(path.relative_to(ROOT)).encode()); digest.update(path.read_bytes())
    digest.update(Path(__file__).read_bytes())
    build = EXPERIMENT / "build" / digest.hexdigest()[:20]
    binary = build / "arena"
    if binary.exists():
        print(binary); return
    build.mkdir(parents=True, exist_ok=True)
    code = '#pragma once\n#include <memory>\n#include <string>\n#include "evaluation.hpp"\n'
    types = {"pass": "compositions::Pass"}
    sources = []
    for name, manifest in manifests.items():
        code += f'#include "{packages[name].relative_to(ROOT) / manifest["header"]}"\n'
        types[name] = manifest["type"]
        sources.extend(str(packages[name] / source) for source in manifest["sources"])
    if args.pair:
        code += f'using PairA = {types[args.pair[0]]};\nusing PairB = {types[args.pair[1]]};\n'
    else:
        code += '''struct AnyAgent {
    virtual ~AnyAgent() = default;
    virtual void reset(const kag::agent::AgentInit&) = 0;
    virtual void act(const kag::agent::AgentObservation&, const kag::agent::DecisionBudget&, kag::Action&) = 0;
};
template<class T> struct AgentModel : AnyAgent {
    T value;
    void reset(const kag::agent::AgentInit& i) override { value.reset(i); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) override { value.act(o,b,a); }
};
struct AgentBox {
    std::unique_ptr<AnyAgent> value;
    void reset(const kag::agent::AgentInit& i) { value->reset(i); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) { value->act(o,b,a); }
};
inline AgentBox make_agent(const std::string& name) {
'''
        for name, kind in types.items():
            code += f'    if (name == "{name}") return {{std::make_unique<AgentModel<{kind}>>()}};\n'
        code += '    std::abort();\n}\n'
    (build / "registry.hpp").write_text(code)
    command = ["conda", "run", "-n", "kaggriculture", "g++", *flags, "-I", str(ROOT),
               "-I", str(EXPERIMENT / "include"), "-I", str(build),
               str(EXPERIMENT / f"src/{args.driver}.cpp"), *sorted({str(Path(source).resolve()) for source in sources}), "-o", str(binary)]
    (build / "build.json").write_text(json.dumps({"command": command, "compiler": compiler}, indent=2))
    subprocess.run(command, check=True)
    print(binary)


if __name__ == "__main__":
    main()
