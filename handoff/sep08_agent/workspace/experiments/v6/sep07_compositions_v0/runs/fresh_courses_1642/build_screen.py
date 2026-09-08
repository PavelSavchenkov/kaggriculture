"""Build the new replay library with the existing six-opponent registry subset."""
import json
import subprocess
from pathlib import Path

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]


def main():
    parity = json.loads((RUN / 'PARITY.json').read_text())
    assert parity['result'] == f"matched_actions={parity['actions']}"
    build = RUN / 'build'
    assert not build.exists()
    build.mkdir()
    source_build = EXP / 'build/54b9e4116c8ac29456ce'
    metadata = json.loads((source_build / 'build.json').read_text())
    registry = (source_build / 'registry.hpp').read_text()
    registry = '\n'.join(line for line in registry.splitlines() if 'productive_wheat_placements_001' not in line and '"wp_' not in line) + '\n'
    (build / 'registry.hpp').write_text(registry)
    command = [str(build) if arg == str(source_build) else arg for arg in metadata['command'] if 'productive_wheat_placements_001/' not in arg]
    command[command.index(str(EXP / 'src/arena.cpp'))] = str(RUN / 'screen.cpp')
    assert command[-2] == '-o'
    command[-1] = str(build / 'arena')
    command.insert(-2, str(RUN / 'library/source/agent.cpp'))
    metadata['command'] = command
    metadata['scope'] = 'Same current policies, with unrelated placement candidates omitted from registry and translation units.'
    (build / 'build.json').write_text(json.dumps(metadata, indent=2) + '\n')
    with (build / 'build.log').open('w') as log:
        subprocess.run(command, check=True, stdout=log, stderr=subprocess.STDOUT)
    print('Built', build / 'arena')


if __name__ == '__main__':
    main()
