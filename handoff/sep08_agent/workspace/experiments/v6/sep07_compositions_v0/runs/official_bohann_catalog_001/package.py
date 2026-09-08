from pathlib import Path
import hashlib
import json
import os
import re


RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
SNAP = EXP / 'runs/bohann_opening_validation_001/frozen/source_snapshot'
PACKAGE = ROOT / 'agents/external/bohann_opening_v1'
PREFIX = Path('experiments/v6/sep07_compositions_v0')


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    assert not PACKAGE.exists(), PACKAGE
    metadata = json.loads((RUN / 'DEPENDENCIES.json').read_text())
    pinned = json.loads((SNAP.parent / 'FROZEN.json').read_text())['files_sha256']
    deps = [Path(p) for p in metadata['dependencies']]
    local = [p for p in deps if p.is_relative_to(PREFIX)]
    mapping = {SNAP / p: PACKAGE / 'source/deps' / p.relative_to(PREFIX) for p in local}
    shared = {}
    copied = {}
    includes = re.compile(r'(#\s*include\s*")([^"]+)(")')
    for p in deps:
        assert digest(SNAP / p) == pinned[str(p)], p
        if p not in local:
            assert str(p).startswith(('agents/common/', 'fast_game_engine/')), p
            assert digest(ROOT / p) == pinned[str(p)], p
            shared[str(p)] = pinned[str(p)]
    for source, dest in mapping.items():
        def rewrite(match):
            name = match[2]
            candidates = [source.parent / name, SNAP / name, SNAP / PREFIX / 'include' / name]
            resolved = next((p.resolve() for p in candidates if p.is_file()), None)
            assert resolved is not None, (source, name)
            if resolved in mapping:
                target = os.path.relpath(mapping[resolved], dest.parent)
            else:
                target = str(resolved.relative_to(SNAP))
                assert target in shared, target
            return match[1] + target + match[3]
        text = includes.sub(rewrite, source.read_text())
        text = re.sub(r'\bcompositions\b', 'kag::agents::bohann_opening_v1::detail', text)
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_text(text)
        copied[str(dest.relative_to(PACKAGE))] = {
            'source': str(source.relative_to(SNAP)),
            'source_sha256': digest(source), 'packaged_sha256': digest(dest),
        }
    top = 'runs/fresh_courses_1642/proposals/bohann_opening_v1/source/agent.hpp'
    (PACKAGE / 'source/agent.hpp').write_text(
        '#pragma once\n#include "deps/' + top + '"\n'
        'namespace kag::agents::bohann_opening_v1 {\n'
        'using Agent = detail::bohann_opening_v1::Agent;\n}\n')
    (PACKAGE / 'source/agent.cpp').write_text('#include "agent.hpp"\n')
    manifest = {
        'format_version': 1, 'name': 'bohann_opening_v1', 'header': 'source/agent.hpp',
        'type': 'kag::agents::bohann_opening_v1::Agent',
        'sources': ['source/agent.cpp'] + [
            str(mapping[SNAP / Path(p)].relative_to(PACKAGE))
            for p in metadata['sources'][1:]],
    }
    (PACKAGE / 'agent.json').write_text(json.dumps(manifest, indent=2) + '\n')
    (PACKAGE / 'SOURCE_MANIFEST.json').write_text(json.dumps({
        'source_freeze_utc': '2026-09-07T17:11:34.568488+00:00',
        'transform': 'Rewrite includes to package-local or shared repository paths; isolate all compositions namespaces under kag::agents::bohann_opening_v1::detail; no policy or table changes.',
        'files': copied, 'repository_dependencies_sha256': shared,
    }, indent=2) + '\n')
    for name in ['evaluation.hpp', 'profile.hpp']:
        dest = PACKAGE / 'tests/support' / name
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_text((SNAP / PREFIX / 'include' / name).read_text().replace('namespace compositions {', 'namespace bohann_catalog_tests {'))
    print(json.dumps({'package': str(PACKAGE.relative_to(ROOT)), 'copied_files': len(copied),
                      'copied_bytes': sum(p.stat().st_size for p in mapping.values()), 'shared_files': len(shared)}))


if __name__ == '__main__':
    main()
