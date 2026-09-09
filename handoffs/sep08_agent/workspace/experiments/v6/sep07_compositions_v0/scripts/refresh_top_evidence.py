"""Retrieve and analyze a fresh top-player cohort; never execute notebook code."""
import argparse
import csv
import hashlib
import json
import subprocess
import zipfile
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('name')
    parser.add_argument('--previous', required=True)
    parser.add_argument('--resume', action='store_true')
    args = parser.parse_args()
    assert args.name.replace('_', '').isalnum()
    output = EXP / 'research' / args.name
    output.mkdir(exist_ok=args.resume)
    commands = []

    def run(command, log):
        cmd = ['conda', 'run', '--no-capture-output', '-n', 'kaggriculture', *map(str, command)]
        with (output / log).open('w') as target:
            subprocess.run(cmd, stdout=target, stderr=subprocess.STDOUT, check=True)
        return {'command': cmd, 'log': log}

    with ThreadPoolExecutor(max_workers=2) as pool:
        futures = [pool.submit(run, ['kaggle', 'competitions', 'leaderboard', 'kaggriculture', '-d', '-p', output], 'leaderboard_download.log'),
                   pool.submit(run, ['kaggle', 'kernels', 'list', '--competition', 'kaggriculture', '--sort-by', 'dateRun', '--page-size', '100', '--csv'], 'notebooks_by_date.csv')]
        commands.extend(future.result() for future in futures)
    for archive in output.glob('*.zip'):
        with zipfile.ZipFile(archive) as compressed:
            for member in compressed.infolist():
                assert Path(member.filename).name == member.filename
                compressed.extract(member, output)
    previous = {r['ref']: r for r in csv.DictReader((EXP / 'research' / args.previous / 'notebooks_by_date.csv').open())}
    current = list(csv.DictReader((output / 'notebooks_by_date.csv').open()))
    changed = [row for row in current if row['ref'] not in previous or row['lastRunTime'] != previous[row['ref']]['lastRunTime']]
    (output / 'notebook_changes.json').write_text(json.dumps(changed, indent=2) + '\n')
    commands.append(run(['python', EXP / 'scripts/pull_replays.py', '--research-dir', output, '--teams', '12', '--episodes', '6', '--workers', '6'], 'replay_download.log'))
    commands.append(run(['python', EXP / 'scripts/normalize_replay_names.py', '--research-dir', output, '--previous-dir', EXP / 'research' / args.previous], 'replay_names.log'))
    commands.append(run(['python', EXP / 'scripts/analyze_replays.py', '--research-dir', output], 'replay_analysis.log'))
    commands.append(run(['python', EXP / 'scripts/review.py', '--research-dir', output], 'invariant_analysis.log'))
    commands.append(run(['python', EXP / 'scripts/refresh_metadata.py', '--research-dir', output], 'metadata.log'))
    paths = [p for p in output.rglob('*') if p.is_file()]
    rows = list(csv.DictReader((output / 'top_replay_manifest.csv').open()))
    paths += sorted({EXP / 'replays' / f"episode-{row['episode_id']}-replay.json" for row in rows})
    paths += [Path(__file__), *[EXP / 'scripts' / name for name in ['pull_replays.py', 'normalize_replay_names.py', 'analyze_replays.py', 'review.py', 'refresh_metadata.py']]]
    record = {'completed_utc': datetime.now(timezone.utc).isoformat(), 'commands': commands,
              'player_games': len(rows), 'raw_unique_replays': len({row['episode_id'] for row in rows}),
              'source_sha256': {str(path.relative_to(EXP)): hashlib.sha256(path.read_bytes()).hexdigest() for path in paths}}
    (output / 'REFRESH.json').write_text(json.dumps(record, indent=2) + '\n')
    print('Retrieved', len(rows), 'player-games; changed notebooks:', [row['ref'] for row in changed])


if __name__ == '__main__':
    main()
