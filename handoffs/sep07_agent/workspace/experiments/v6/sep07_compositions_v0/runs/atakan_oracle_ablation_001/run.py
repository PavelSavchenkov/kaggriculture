"""Build and execute offline estimator diagnostics against frozen dependencies."""
import hashlib
import json
import subprocess
from pathlib import Path

RUN = Path(__file__).resolve().parent
SOURCE = RUN.parent / 'atakan_portfolio_001'


def main():
    build = RUN / 'build'
    build.mkdir(parents=True, exist_ok=True)
    frozen = SOURCE / 'build/generic/BUILD.json'
    manifest = json.loads(frozen.read_text())
    command = manifest['binaries'][1]['command'].copy()
    source = RUN / 'source/driver.cpp'
    binary = build / 'oracle_diagnostic'
    command = [str(source) if x.endswith('/source/diagnostics.cpp') else x for x in command]
    command[-1] = str(binary)
    with (build / 'compile.log').open('w') as log:
        subprocess.run(command, check=True, stdout=log, stderr=subprocess.STDOUT)
    calls = [command]
    command = ['conda', 'run', '-n', 'kaggriculture', str(binary), str(RUN / 'inputs/oracles.txt'), str(RUN / 'forecasts.json')]
    subprocess.run(command, check=True)
    calls.append(command)
    hashes = {str(p.relative_to(RUN)): hashlib.sha256(p.read_bytes()).hexdigest() for p in [source, binary, RUN / 'inputs/oracles.txt', RUN / 'forecasts.json']}
    (build / 'BUILD.json').write_text(json.dumps({'commands': calls, 'artifact_sha256': hashes,
        'frozen_dependencies': str(frozen), 'frozen_manifest_sha256': hashlib.sha256(frozen.read_bytes()).hexdigest(),
        'scope': 'Offline diagnostic executable only, no agent manifest or deployable oracle policy.'}, indent=2)+'\n')
    print('Evaluated ten diagnostic forecast configurations on 320 contexts × three branches.')


if __name__ == '__main__':
    main()
