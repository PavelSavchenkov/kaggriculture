"""Build the bounded notebook audit arena without changing the main catalog."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]
ROOT = EXP.parents[2]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--debug', action='store_true')
    args = parser.parse_args()
    catalog = json.loads((EXP / 'configs/league.json').read_text())
    names = ['public_router', 'public_terminal_router', 'king_rc4', 'teammate_shoprouter', 'shop_herd_s6_m3_g1', 'binghua_116']
    packages = {name: ROOT / catalog[name] for name in names}
    for name in ['titan_frontier', 'kaito_v43', 'market_impact_v4']:
        packages[name] = EXP / 'league' / name
    build = EXP / 'build/refresh_1010' / ('debug_pair' if args.debug else 'generic')
    build.mkdir(parents=True, exist_ok=True)
    code = '#pragma once\n#include <memory>\n#include <string>\n#include "evaluation.hpp"\n'
    types = {'pass': 'compositions::Pass'}
    sources = []
    for name, path in packages.items():
        manifest = json.loads((path / 'agent.json').read_text())
        types[name] = manifest['type']
        code += f'#include "{path.relative_to(ROOT) / manifest["header"]}"\n'
        sources += [str((path / source).resolve()) for source in manifest['sources']]
    if args.debug:
        code += 'using PairA=compositions::titan_frontier::Agent;\nusing PairB=compositions::market_impact_v4::Agent;\n'
    else:
        code += '''struct AnyAgent {virtual ~AnyAgent()=default;virtual void reset(const kag::agent::AgentInit&)=0;virtual void act(const kag::agent::AgentObservation&,const kag::agent::DecisionBudget&,kag::Action&)=0;};
template<class T>struct Model:AnyAgent {T value;void reset(const kag::agent::AgentInit&i)override{value.reset(i);}void act(const kag::agent::AgentObservation&o,const kag::agent::DecisionBudget&b,kag::Action&a)override{value.act(o,b,a);}};
struct Box {std::unique_ptr<AnyAgent> value;void reset(const kag::agent::AgentInit&i){value->reset(i);}void act(const kag::agent::AgentObservation&o,const kag::agent::DecisionBudget&b,kag::Action&a){value->act(o,b,a);}};
inline Box make_agent(const std::string& name){
'''
        for name, type_name in types.items():
            code += f'if(name=="{name}")return {{std::make_unique<Model<{type_name}>>()}};\n'
        code += 'std::abort();}\n'
    (build / 'registry.hpp').write_text(code)
    flags = ['-std=c++20', '-march=native', '-mtune=native', '-fno-exceptions', '-fno-rtti', '-fno-math-errno', '-fno-semantic-interposition', '-fno-plt', '-pthread']
    flags += ['-O1', '-g', '-DKAG_VERIFY_MASKS', '-DCOMPOSITIONS_PAIR'] if args.debug else ['-O3', '-DNDEBUG', '-flto']
    command = ['conda', 'run', '-n', 'kaggriculture', 'g++', *flags, '-I', str(ROOT), '-I', str(EXP / 'include'), '-I', str(build), str(EXP / 'src/arena.cpp'), *sorted(set(sources)), '-o', str(build / 'arena')]
    (build / 'build.json').write_text(json.dumps({'command': command, 'compiler': subprocess.check_output(['conda', 'run', '-n', 'kaggriculture', 'g++', '--version'], text=True)}, indent=2) + '\n')
    subprocess.run(command, check=True)
    (build / 'binary.sha256').write_text(hashlib.sha256((build / 'arena').read_bytes()).hexdigest() + '\n')
    print(build / 'arena')


if __name__ == '__main__':
    main()
