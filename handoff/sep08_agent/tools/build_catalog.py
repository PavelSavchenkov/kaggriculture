"""Build a cached catalog arena or typed pair without the live experiment."""
from pathlib import Path
import argparse
import hashlib
import json
import shlex
import subprocess

HERE = Path(__file__).resolve().parents[1]
ROOT = HERE.parents[1]
ENV = ['conda', 'run', '--no-capture-output', '-n', 'kaggriculture']
SUPPORT = 'agents/external/bohann_opening_v1/tests/support'


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--agents', nargs='+')
    parser.add_argument('--pair', nargs=2)
    parser.add_argument('--debug', action='store_true')
    parser.add_argument('--registry', type=Path, help='JSON mapping agent names to absolute or repository-relative package paths.')
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--original-root', type=Path, help='Developer parity only: use frozen original packages from a prepared workspace.')
    args = parser.parse_args()
    if args.agents and args.pair:
        parser.error('Choose --agents or --pair')
    packages = {p['name']: p['path'] for p in json.loads((HERE / 'evidence/catalog.json').read_text())}
    if args.registry:
        packages.update(json.loads(args.registry.read_text()))
    names = sorted(set(args.agents or args.pair or packages) - {'pass'})
    assert set(names) <= packages.keys()
    root = args.original_root.resolve() if args.original_root else ROOT
    if args.original_root:
        records = json.loads((HERE / 'evidence/original_agents.json').read_text())
        packages = {r['name']: r['original_package'] for r in records}
    manifests = {name: json.loads((root / packages[name] / 'agent.json').read_text()) for name in names}
    sources = sorted({(root / packages[name] / path).resolve()
                      for name, manifest in manifests.items() for path in manifest['sources']})
    compiler = subprocess.check_output(ENV + ['g++', '--version'], text=True)
    signature = hashlib.sha256(json.dumps([names, args.pair, args.debug, str(root), compiler]).encode())
    for name in names:
        for path in sorted((root / packages[name]).rglob('*')):
            if path.is_file() and path.suffix in {'.cpp', '.hpp', '.inc', '.h', '.json'}:
                signature.update(path.read_bytes())
    for directory in ['fast_game_engine', 'agents/common/api', 'agents/common/runtime', SUPPORT]:
        for path in sorted((ROOT / directory).rglob('*')):
            if path.is_file() and path.suffix in {'.hpp', '.cpp', '.h'}:
                signature.update(path.read_bytes())
    for path in sources:
        signature.update(path.read_bytes())
    # Experimental manifests may refer to generated headers outside the package.
    if args.registry:
        output = subprocess.check_output(ENV + ['g++', '-std=c++20', '-I', str(ROOT), '-I', str(root),
            '-I', str(root / 'experiments/v6/sep07_compositions_v0/include'), '-MM', *map(str, sources)], text=True)
        dependencies = set()
        for line in output.replace('\\\n', ' ').splitlines():
            if ':' in line:
                dependencies.update(Path(p).resolve() for p in shlex.split(line.split(':', 1)[1]))
        for path in sorted(dependencies):
            signature.update(path.read_bytes())
    signature.update(Path(__file__).read_bytes())
    build = HERE / '_work/catalog' / signature.hexdigest()[:20]
    binary = build / 'bin/arena'
    if binary.exists():
        print(binary)
        return
    build.mkdir(parents=True, exist_ok=True)
    prefix = f'#include "{SUPPORT}/evaluation.hpp"\n#include "{SUPPORT}/adapter.hpp"\nnamespace t = bohann_catalog_tests;\n'
    declarations = []
    factories = []
    for name in names:
        manifest = manifests[name]
        header = root / packages[name] / manifest['header']
        code = prefix + f'#include "{header}"\n'
        if not args.original_root:
            code += f'static_assert(kag::agent::LocalAgent<{manifest["type"]}>);\n'
        code += f'std::unique_ptr<t::AgentInterface> make_{name}() {{ return std::make_unique<t::Adapter<{manifest["type"]}>>(); }}\n'
        code += f'int typed_{name}(const t::Options& o) {{ return t::run_batch(o, []{{return {manifest["type"]}();}}, []{{return t::Pass{{}};}}); }}\n'
        path = build / (name + '.cpp')
        path.write_text(code)
        factories.append(path)
        declarations.append(f'std::unique_ptr<t::AgentInterface> make_{name}();\n')
        declarations.append(f'int typed_{name}(const t::Options&);\n')
    main = prefix + ''.join(declarations)
    if args.pair:
        for name in set(args.pair) - {'pass'}:
            main += f'#include "{root / packages[name] / manifests[name]["header"]}"\n'
        kinds = [manifests[n]['type'] if n != 'pass' else 't::Pass' for n in args.pair]
        main += f'int main(int argc,char**argv) {{ auto o=t::options(argc,argv); return t::run_batch(o, []{{return {kinds[0]}{{}};}}, []{{return {kinds[1]}{{}};}}); }}\n'
        factories = []
    else:
        main += 'class Box { std::unique_ptr<t::AgentInterface> p; public:\nexplicit Box(const std::string& n) {\n'
        for name in names:
            main += f'if(n=="{name}") {{ p=make_{name}(); return; }}\n'
        main += 'if(n=="pass") { p=std::make_unique<t::Adapter<t::Pass>>(); return; } std::abort(); }\n'
        main += 'void reset(const kag::agent::AgentInit&i){p->reset(i);}\nvoid act(const kag::agent::AgentObservation&o,const kag::agent::DecisionBudget&b,kag::Action&a){p->act(o,b,a);}\n};\n'
        main += 'int main(int argc,char**argv){bool typed=false; for(int i=1;i<argc;++i) if(std::string(argv[i])=="--typed-pass") {typed=true; for(int j=i;j+1<argc;++j)argv[j]=argv[j+1]; --argc; --i;} auto o=t::options(argc,argv);\n'
        main += 'if(typed){ if(o.b!="pass")return 2;\n'
        for name in names:
            main += f'if(o.a=="{name}")return typed_{name}(o);\n'
        main += 'return 2;} return t::run_batch(o,[&]{return Box(o.a);},[&]{return Box(o.b);});}\n'
    (build / 'main.cpp').write_text(main)
    flags = '-O1 -g -UNDEBUG -DKAG_VERIFY_MASKS' if args.debug else '-O3 -DNDEBUG -flto'
    cmake = 'cmake_minimum_required(VERSION 3.20)\nproject(sep08_catalog LANGUAGES CXX)\n'
    cmake += 'add_executable(arena\n' + '\n'.join('"' + str(p) + '"' for p in [build / 'main.cpp', *factories, *sources]) + '\n)\n'
    cmake += f'target_include_directories(arena PRIVATE "{ROOT}" "{root}" "{root}/experiments/v6/sep07_compositions_v0/include")\n'
    cmake += 'target_compile_features(arena PRIVATE cxx_std_20)\n'
    cmake += f'target_compile_options(arena PRIVATE {flags} -march=native -mtune=native -fno-exceptions -fno-rtti -fno-math-errno -fno-semantic-interposition -fno-plt)\n'
    cmake += 'target_link_libraries(arena PRIVATE pthread)\n'
    if not args.debug:
        cmake += f'target_link_options(arena PRIVATE -flto={args.jobs})\n'
    (build / 'CMakeLists.txt').write_text(cmake)
    commands = [ENV + ['cmake', '-S', str(build), '-B', str(build / 'bin')],
                ENV + ['cmake', '--build', str(build / 'bin'), '-j', str(args.jobs)]]
    (build / 'BUILD.json').write_text(json.dumps({'commands': commands, 'compiler': compiler,
                                                'sources': [str(p) for p in sources]}, indent=2) + '\n')
    with (build / 'build.log').open('w') as log:
        for command in commands:
            subprocess.run(command, check=True, stdout=log, stderr=subprocess.STDOUT)
    print(binary)


if __name__ == '__main__':
    main()
