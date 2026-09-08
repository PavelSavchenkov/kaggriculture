"""Build a diagnostic main against the same recorded policy registry."""
import argparse
import json
import subprocess
from pathlib import Path

RUN = Path(__file__).resolve().parent


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('source', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    source, output = args.source.resolve(), args.output.resolve()
    assert source.is_relative_to(RUN) and output.is_relative_to(RUN)
    assert source.exists() and not output.exists()
    template = json.loads((RUN / 'build/build.json').read_text())
    command = template['command']
    source_index = command.index(str(RUN / 'audit_replanting.cpp'))
    command[source_index] = str(source)
    assert command[-2] == '-o'
    command[-1] = str(output)
    output.parent.mkdir(parents=True, exist_ok=True)
    template['command'] = command
    (output.parent / 'build.json').write_text(json.dumps(template, indent=2) + '\n')
    with (output.parent / 'build.log').open('w') as log:
        subprocess.run(command, check=True, stdout=log, stderr=subprocess.STDOUT)
    print('Built', output)


if __name__ == '__main__':
    main()
