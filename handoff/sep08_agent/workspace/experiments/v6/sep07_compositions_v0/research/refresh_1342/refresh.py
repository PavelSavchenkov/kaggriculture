"""Read-only network refresh and offline analysis; no gameplay policy."""
import csv
import hashlib
import json
import subprocess
import zipfile
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
from pathlib import Path

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ENV = ['conda', 'run', '--no-capture-output', '-n', 'kaggriculture']


def call(args, log):
    command = ENV + args
    result = subprocess.run(command, capture_output=True, text=True, check=True)
    (RUN / log).write_text(result.stdout + result.stderr)
    return {'command': command, 'log': log}


def main():
    with ThreadPoolExecutor(max_workers=2) as pool:
        jobs = [pool.submit(call, ['kaggle', 'competitions', 'leaderboard', 'kaggriculture', '-d', '-p', str(RUN)], 'leaderboard_download.log'),
                pool.submit(call, ['kaggle', 'kernels', 'list', '--competition', 'kaggriculture', '--sort-by', 'dateRun', '--page-size', '100', '--csv'], 'notebooks_by_date.csv')]
        commands = [j.result() for j in jobs]
    with zipfile.ZipFile(RUN / 'kaggriculture.zip') as archive:
        for name in archive.namelist():
            assert Path(name).name == name
        archive.extractall(RUN)
    old = {r['ref']: r for r in csv.DictReader((EXP / 'research/refresh_1212/notebooks_by_date.csv').open())}
    current = list(csv.DictReader((RUN / 'notebooks_by_date.csv').open()))
    changes = [r | {'previous': old.get(r['ref'])} for r in current if r['ref'] not in old or old[r['ref']]['lastRunTime'] != r['lastRunTime']]
    (RUN / 'notebook_changes.json').write_text(json.dumps(changes, indent=2) + '\n')
    print('Notebook changes:', json.dumps(changes), flush=True)
    commands.append(call(['python', str(EXP / 'scripts/pull_replays.py'), '--research-dir', str(RUN), '--teams', '12', '--episodes', '6', '--workers', '6'], 'replay_download.log'))
    commands.append(call(['python', str(EXP / 'scripts/analyze_replays.py'), '--research-dir', str(RUN)], 'replay_analysis.log'))
    hashes = {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in RUN.glob('*') if p.is_file()}
    for row in csv.DictReader((RUN / 'top_replay_manifest.csv').open()):
        p = EXP / 'replays' / f"episode-{row['episode_id']}-replay.json"
        hashes[str(p.relative_to(EXP))] = hashlib.sha256(p.read_bytes()).hexdigest()
    (RUN / 'REFRESH.json').write_text(json.dumps({'completed_utc': datetime.now(timezone.utc).isoformat(), 'commands': commands, 'source_sha256': hashes}, indent=2) + '\n')
    print('Fresh replay analysis complete.', flush=True)


if __name__ == '__main__':
    main()
