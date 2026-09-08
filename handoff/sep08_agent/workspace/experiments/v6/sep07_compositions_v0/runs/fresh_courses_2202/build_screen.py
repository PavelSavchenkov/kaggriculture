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
    source_build = Path('/home/pavel/Programming/kaggriculture/experiments/v6/sep07_compositions_v0/build/2cec17ae4fb5f9f6ad95')
    metadata = json.loads((source_build / 'build.json').read_text())
    (build / 'registry.hpp').write_text((source_build / 'registry.hpp').read_text())
    command = [str(build) if arg == str(source_build) else arg for arg in metadata['command']]
    command[command.index(str(EXP / 'src/arena.cpp'))] = str(RUN / 'screen.cpp')
    assert command[-2] == '-o'
    command[-1] = str(build / 'arena')
    command.insert(-2, str(RUN / 'library/source/agent.cpp'))
    metadata['command'] = command
    metadata['scope'] = 'Current27-opponent registry, with a new independent fresh course library; actual screen uses six diverse opponents.'
    (build / 'build.json').write_text(json.dumps(metadata, indent=2) + '\n')
    with (build / 'build.log').open('w') as log:
        subprocess.run(command, check=True, stdout=log, stderr=subprocess.STDOUT)
    print('Built', build / 'arena')


if __name__ == '__main__':
    main()
