"""Build isolated C++ policies from frozen predecessor dependency copies."""
import argparse
import hashlib
import json
import re
import shutil
import subprocess
from pathlib import Path

from prepare import NAMES

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
SOURCE = RUN.parent / 'atakan_portfolio_001'


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--debug-pair', nargs=2)
    parser.add_argument('--masked', action='store_true')
    args = parser.parse_args()
    kind = 'debug' if args.debug_pair else 'masked' if args.masked else 'generic'
    build = RUN / 'build' / kind
    build.mkdir(parents=True, exist_ok=True)
    snapshot = build / 'source_snapshot'
    hashes, sources = {}, set()
    for version in ['generic', 'reference']:
        old = SOURCE / 'build' / version
        manifest = json.loads((old / 'BUILD.json').read_text())
        for relative, digest in manifest['source_sha256'].items():
            if relative in hashes:
                assert hashes[relative] == digest
                continue
            path = old / 'source_snapshot' / relative
            assert hashlib.sha256(path.read_bytes()).hexdigest() == digest
            target = snapshot / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(path, target)
            hashes[relative] = digest
        for path in manifest['binaries'][0]['command']:
            if path.endswith('/agent.cpp'):
                sources.add(str(Path(path).relative_to(old / 'source_snapshot')))
    # Only new policy/driver files are copied from the live run. Existing policies
    # and helpers resolve exclusively to the frozen dependency snapshots above.
    for path in list((RUN / 'source').glob('*')) + list((RUN / 'proposals').glob('*/source/*')):
        relative = str(path.relative_to(ROOT));target = snapshot / relative;target.parent.mkdir(parents=True, exist_ok=True);shutil.copyfile(path,target)
        hashes[relative] = hashlib.sha256(path.read_bytes()).hexdigest()
        if path.name == 'agent.cpp':
            sources.add(relative)
    old_registry = (SOURCE / 'build/generic/registry.hpp').read_text()
    includes = '\n'.join(line for line in old_registry.splitlines() if line.startswith('#include'))+'\n'
    includes += f'#include "{(EXP / "candidates/investment_context_guarded_001_best/source/agent.hpp").relative_to(ROOT)}"\n'
    for name in NAMES:
        includes += f'#include "{(RUN / "proposals" / name / "source/agent.hpp").relative_to(ROOT)}"\n'
    code = '#pragma once\n'+includes
    if args.debug_pair:
        code += f'using PairA=compositions::{args.debug_pair[0]}::Agent;\nusing PairB=compositions::{args.debug_pair[1]}::Agent;\n'
    else:
        code += '''struct AnyAgent {virtual ~AnyAgent()=default;virtual void reset(const kag::agent::AgentInit&)=0;virtual void act(const kag::agent::AgentObservation&,const kag::agent::DecisionBudget&,kag::Action&)=0;};
template<class T>struct Model:AnyAgent {T value;void reset(const kag::agent::AgentInit&i)override{value.reset(i);}void act(const kag::agent::AgentObservation&o,const kag::agent::DecisionBudget&b,kag::Action&a)override{value.act(o,b,a);}};
struct Box {std::unique_ptr<AnyAgent> value;void reset(const kag::agent::AgentInit&i){value->reset(i);}void act(const kag::agent::AgentObservation&o,const kag::agent::DecisionBudget&b,kag::Action&a){value->act(o,b,a);}};
inline Box make_agent(const std::string& name){
'''
        code += '\n'.join(re.findall(r'if\(name==.*', old_registry))+'\n'
        for name in NAMES + ['investment_context_guarded_001_best']:
            code += f'if(name=="{name}")return {{std::make_unique<Model<compositions::{name}::Agent>>()}};\n'
        code += 'std::abort();}\n'
    registry = snapshot / build.relative_to(ROOT) / 'registry.hpp'
    registry.parent.mkdir(parents=True, exist_ok=True);registry.write_text(code)
    hashes[str(registry.relative_to(snapshot))] = hashlib.sha256(registry.read_bytes()).hexdigest()
    flags = ['-std=c++20', '-march=native', '-mtune=native', '-fno-exceptions', '-fno-rtti', '-pthread']
    flags += ['-O1', '-g', '-DKAG_VERIFY_MASKS'] if args.debug_pair or args.masked else ['-O3', '-DNDEBUG', '-flto']
    if args.debug_pair:
        flags += ['-DCOMPOSITIONS_PAIR']
    includes = ['-I', str(snapshot), '-I', str(snapshot / (EXP / 'include').relative_to(ROOT)), '-I', str(registry.parent)]
    mains = [EXP / 'src/arena.cpp'] + ([] if args.debug_pair or args.masked else [RUN / 'source/diagnostics.cpp'])
    commands = []
    for main in mains:
        target = build / ('arena' if main.name == 'arena.cpp' else 'diagnostics')
        command = ['conda', 'run', '-n', 'kaggriculture', 'g++', *flags, *includes,
                   str(snapshot / main.relative_to(ROOT)), *[str(snapshot / p) for p in sorted(sources)], '-o', str(target)]
        with (build / f'{target.name}.log').open('w') as log:
            subprocess.run(command, check=True, stdout=log, stderr=subprocess.STDOUT)
        commands.append({'command': command, 'binary_sha256': hashlib.sha256(target.read_bytes()).hexdigest()})
    (build / 'BUILD.json').write_text(json.dumps({'source_sha256': hashes, 'binaries': commands}, indent=2)+'\n')
    print(build)


if __name__ == '__main__':
    main()
