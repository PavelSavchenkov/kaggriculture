"""Compile isolated frozen arenas and a diagnostic driver; no catalog mutation."""
import argparse
import hashlib
import json
import shlex
import subprocess
from pathlib import Path

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
from generate import NAMES


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--debug', action='store_true')
    parser.add_argument('--masked', action='store_true')
    parser.add_argument('--reference')
    args = parser.parse_args()
    checked = args.debug or args.masked
    build = RUN / 'build' / ('debug' if args.debug else 'masked' if args.masked else 'reference' if args.reference else 'generic')
    build.mkdir(parents=True, exist_ok=True)
    catalog = json.loads((EXP / 'configs/league.json').read_text())
    rivals = [] if checked else ['investment_context_guarded_001_best', 'teammate_shoprouter', 'public_router_v5', 'king_rc4', 'public_router']
    if args.reference:
        rivals = [args.reference]
    packages = {name: ROOT / catalog[name] for name in rivals}
    for name in (['sheep_yarn2', 'sheep_value_margin_s64'] if args.debug else NAMES):
        packages[name] = RUN / 'proposals' / name
    code = '#pragma once\n#include <memory>\n#include <string>\n#include "evaluation.hpp"\n'
    code += f'#include "{(RUN / "source/audit.hpp").relative_to(ROOT)}"\n'
    types, sources = {'pass': 'compositions::Pass'}, []
    for name, package in packages.items():
        manifest = json.loads((package / 'agent.json').read_text())
        types[name] = manifest['type']
        code += f'#include "{package.relative_to(ROOT) / manifest["header"]}"\n'
        sources += [(package / p).resolve() for p in manifest['sources']]
    if args.debug:
        code += 'using PairA=compositions::sheep_yarn2::Agent;\nusing PairB=compositions::sheep_value_margin_s64::Agent;\n'
    else:
        code += '''struct Trace {int branch=-1,wheat=0;double cash=0,rival_cash=0;std::array<compositions::sheep_portfolio::Estimate,2> estimates{};std::vector<int> physical;};
struct AnyAgent {Trace trace;virtual ~AnyAgent()=default;virtual void reset(const kag::agent::AgentInit&)=0;virtual void act(const kag::agent::AgentObservation&,const kag::agent::DecisionBudget&,kag::Action&)=0;};
template<class T>struct Model:AnyAgent {T value;void reset(const kag::agent::AgentInit&i)override{value.reset(i);trace={};}void act(const kag::agent::AgentObservation&o,const kag::agent::DecisionBudget&b,kag::Action&a)override{value.act(o,b,a);if constexpr(requires{value.branch();value.estimates();})if(o.step==288){trace.branch=value.branch();trace.estimates=value.estimates();trace.cash=o.self().money;trace.rival_cash=o.opponent().money;trace.wheat=o.own.shed[kag::WHEAT];trace.physical=compositions::sheep_portfolio::physical(o);}}};
struct Box {std::unique_ptr<AnyAgent> value;void reset(const kag::agent::AgentInit&i){value->reset(i);}void act(const kag::agent::AgentObservation&o,const kag::agent::DecisionBudget&b,kag::Action&a){value->act(o,b,a);}};
inline Box make_agent(const std::string& name){
'''
        for name, type_name in types.items():
            code += f'if(name=="{name}")return {{std::make_unique<Model<{type_name}>>()}};\n'
        code += 'std::abort();}\n'
    (build / 'registry.hpp').write_text(code)
    sources = sorted(set(sources))
    main_sources = [EXP / 'src/arena.cpp'] + ([] if checked or args.reference else [RUN / 'source/diagnostics.cpp'])
    flags = ['-std=c++20', '-march=native', '-mtune=native', '-fno-exceptions', '-fno-rtti', '-pthread']
    flags += (['-O1', '-g', '-DKAG_VERIFY_MASKS'] + (['-DCOMPOSITIONS_PAIR'] if args.debug else [])) if checked else ['-O3', '-DNDEBUG', '-flto']
    includes = ['-I', str(ROOT), '-I', str(EXP / 'include'), '-I', str(build)]
    dependencies = subprocess.check_output(['conda', 'run', '-n', 'kaggriculture', 'g++', *flags, *includes, '-MM', *map(str, main_sources + sources)], text=True)
    paths = set(main_sources + sources)
    for line in dependencies.replace('\\\n', ' ').splitlines():
        if line.strip():
            paths.update(Path(p).resolve() for p in shlex.split(line.split(':', 1)[1]))
    snapshot = build / 'source_snapshot'
    hashes = {}
    for path in sorted(paths):
        relative = path.relative_to(ROOT)
        copied = snapshot / relative
        copied.parent.mkdir(parents=True, exist_ok=True)
        data = path.read_bytes();copied.write_bytes(data)
        hashes[str(relative)] = hashlib.sha256(data).hexdigest()
    includes = ['-I', str(snapshot), '-I', str(snapshot / (EXP / 'include').relative_to(ROOT)), '-I', str(snapshot / build.relative_to(ROOT))]
    commands = []
    for main in main_sources:
        target = build / ('arena' if main.name == 'arena.cpp' else 'diagnostics')
        command = ['conda', 'run', '-n', 'kaggriculture', 'g++', *flags, *includes, *[str(snapshot / p.relative_to(ROOT)) for p in [main, *sources]], '-o', str(target)]
        subprocess.run(command, check=True)
        commands.append({'command': command, 'binary_sha256': hashlib.sha256(target.read_bytes()).hexdigest()})
    (build / 'BUILD.json').write_text(json.dumps({'source_sha256': hashes, 'binaries': commands}, indent=2) + '\n')
    print(build)


if __name__ == '__main__':
    main()
