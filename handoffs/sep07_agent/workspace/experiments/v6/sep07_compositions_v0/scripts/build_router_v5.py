"""Build an isolated arena from a frozen dependency copy, without catalog edits."""
import argparse
import hashlib
import json
import shlex
import shutil
import subprocess
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]
ROOT = EXP.parents[2]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--debug', action='store_true')
    args = parser.parse_args()
    base = EXP / 'build/refresh_1112'
    build = base / ('debug_pair' if args.debug else 'generic')
    build.mkdir(parents=True, exist_ok=True)
    catalog = json.loads((EXP / 'configs/league.json').read_text())
    names = ['public_sixday', 'teammate_shoprouter', 'king_rc4', 'binghua_116', 'public_terminal_router', 'shop_herd_guarded_001_best']
    if args.debug:
        names = ['public_sixday']
    packages = {name: ROOT / catalog[name] for name in names}
    packages['public_router_v5'] = EXP / 'league/public_router_v5'
    code = '#pragma once\n#include <memory>\n#include <string>\n#include "evaluation.hpp"\n'
    types, sources = {'pass': 'compositions::Pass'}, []
    for name, package in packages.items():
        manifest = json.loads((package / 'agent.json').read_text())
        types[name] = manifest['type']
        code += f'#include "{package.relative_to(ROOT) / manifest["header"]}"\n'
        sources += [(package / p).resolve() for p in manifest['sources']]
    if args.debug:
        code += 'using PairA=compositions::public_router_v5::Agent;\nusing PairB=compositions::public_sixday::Agent;\n'
    else:
        code += '''struct AnyAgent {virtual ~AnyAgent()=default;virtual void reset(const kag::agent::AgentInit&)=0;virtual void act(const kag::agent::AgentObservation&,const kag::agent::DecisionBudget&,kag::Action&)=0;};
template<class T>struct Model:AnyAgent {T value;void reset(const kag::agent::AgentInit&i)override{value.reset(i);}void act(const kag::agent::AgentObservation&o,const kag::agent::DecisionBudget&b,kag::Action&a)override{value.act(o,b,a);}};
struct Box {std::unique_ptr<AnyAgent> value;void reset(const kag::agent::AgentInit&i){value->reset(i);}void act(const kag::agent::AgentObservation&o,const kag::agent::DecisionBudget&b,kag::Action&a){value->act(o,b,a);}};
inline Box make_agent(const std::string& name){
'''
        for name, type_name in types.items():
            code += f'if(name=="{name}")return {{std::make_unique<Model<{type_name}>>()}};\n'
        code += 'std::abort();}\n'
    registry = build / 'registry.hpp'
    registry.write_text(code)
    sources = [EXP / 'src/arena.cpp', *sorted(set(sources))]
    include_flags = ['-I', str(ROOT), '-I', str(EXP / 'include'), '-I', str(build)]
    flags = ['-std=c++20', '-march=native', '-mtune=native', '-fno-exceptions', '-fno-rtti', '-fno-math-errno', '-fno-semantic-interposition', '-fno-plt', '-pthread']
    flags += ['-O1', '-g', '-DKAG_VERIFY_MASKS', '-DCOMPOSITIONS_PAIR'] if args.debug else ['-O3', '-DNDEBUG', '-flto']
    dependencies = subprocess.check_output(['conda', 'run', '-n', 'kaggriculture', 'g++', *flags, *include_flags, '-MM', *map(str, sources)], text=True)
    paths = set(sources)
    for line in dependencies.replace('\\\n', ' ').splitlines():
        if line.strip():
            paths.update(Path(p).resolve() for p in shlex.split(line.split(':', 1)[1]))
    snapshot = build / 'source_snapshot'
    hashes = {}
    for path in sorted(paths):
        relative = path.relative_to(ROOT)
        copied = snapshot / relative
        copied.parent.mkdir(parents=True, exist_ok=True)
        data = path.read_bytes()
        copied.write_bytes(data)
        hashes[str(relative)] = hashlib.sha256(data).hexdigest()
    frozen_include = ['-I', str(snapshot), '-I', str(snapshot / (EXP / 'include').relative_to(ROOT)), '-I', str(snapshot / build.relative_to(ROOT))]
    command = ['conda', 'run', '-n', 'kaggriculture', 'g++', *flags, *frozen_include, *[str(snapshot / p.relative_to(ROOT)) for p in sources], '-o', str(build / 'arena')]
    (build / 'build.json').write_text(json.dumps({'command': command, 'source_sha256': hashes, 'packages': {k: str(v.relative_to(ROOT)) for k, v in packages.items()}, 'compiler': subprocess.check_output(['conda', 'run', '-n', 'kaggriculture', 'g++', '--version'], text=True)}, indent=2) + '\n')
    subprocess.run(command, check=True)
    (build / 'binary.sha256').write_text(hashlib.sha256((build / 'arena').read_bytes()).hexdigest() + '\n')
    print(build / 'arena')


if __name__ == '__main__':
    main()
