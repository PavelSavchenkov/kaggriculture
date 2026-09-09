"""Freeze isolated diagnostic dependencies, rerun biology, and hash artifacts."""
import hashlib
import json
import shutil
import subprocess
from datetime import datetime, timezone
from pathlib import Path

RUN = Path(__file__).resolve().parent
ROOT = RUN.parents[4]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    frozen = RUN / 'build/biology_source_snapshot'
    headers = ['agents/common/api/agent_api.hpp', 'agents/common/api/observation.hpp',
               'fast_game_engine/sim.hpp', 'fast_game_engine/pyrandom.hpp']
    for name in headers:
        target = frozen / name;target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(ROOT / name, target)
    command = ['conda', 'run', '-n', 'kaggriculture', 'g++', '-std=c++20', '-O3',
               '-DKAG_DISABLE_ENTITY_MASKS', '-DKAG_DISABLE_DECAY_MASK', '-I', str(frozen),
               str(RUN / 'source/biology_check.cpp'), '-o', str(RUN / 'build/biology_check_frozen')]
    subprocess.run(command, check=True)
    run = ['conda', 'run', '-n', 'kaggriculture', str(RUN / 'build/biology_check_frozen'),
           str(RUN / 'donor_inputs.txt'), str(RUN / 'BIOLOGY_CHECK_FROZEN.json')]
    subprocess.run(run, check=True)
    assert (RUN / 'BIOLOGY_CHECK_FROZEN.json').read_bytes() == (RUN / 'BIOLOGY_CHECK.json').read_bytes()
    report = {'commands': [command, run], 'root_header_sha256': {p: sha(frozen / p) for p in headers},
              'exact_rebuild_biology_output': True, 'scope': 'Explicit three-worker funded crop service diagnostic; no routes or economics certified.'}
    (RUN / 'BIOLOGY_LINEAGE.json').write_text(json.dumps(report, indent=2) + '\n')
    hashes = {str(p.relative_to(RUN)): sha(p) for p in RUN.rglob('*')
              if p.is_file() and '__pycache__' not in str(p) and p.name != 'FINAL_AUDIT.json'
              and p.suffix != '.log'}
    (RUN / 'FINAL_AUDIT.json').write_text(json.dumps({'artifact_count': len(hashes), 'sha256': hashes,
        'result': 'Research helper retained; no agent package, promotion, catalog edit or submission.',
        'frozen_at_utc': datetime.now(timezone.utc).isoformat()}, indent=2) + '\n')
    print('Frozen', len(hashes), 'artifacts.')


if __name__ == '__main__':
    main()
