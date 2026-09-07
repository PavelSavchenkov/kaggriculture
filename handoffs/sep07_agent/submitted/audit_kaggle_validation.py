import argparse
import hashlib
import importlib.util
import json
from pathlib import Path

from kaggle_environments import make

OUT = Path(__file__).resolve().parent


def load(seat):
    spec = importlib.util.spec_from_file_location(f'validation_{seat}', OUT / 'unpacked/main.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module.agent


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('episode', type=int)
    args = parser.parse_args()
    replay_path = OUT / 'kaggle_validation' / f'episode-{args.episode}-replay.json'
    remote = json.loads(replay_path.read_text())
    assert remote['statuses'] == ['DONE', 'DONE'], remote['statuses']
    cfg = dict(remote['configuration'], seed=remote['info']['seed'])
    env = make('kaggriculture', configuration=cfg, debug=True)
    env.run([load(0), load(1)])
    assert [s.status for s in env.state] == ['DONE', 'DONE']
    rewards = [s.reward for s in env.state]
    assert rewards == remote['rewards'], (rewards, remote['rewards'])
    assert len(env.steps) == len(remote['steps']) == 720
    for step in range(1, 720):
        for seat in range(2):
            actual = env.steps[step][seat].action
            expected = remote['steps'][step][seat]['action']
            assert actual == expected, (step, seat, actual, expected)
    logs = []
    for seat in range(2):
        path = OUT / 'kaggle_validation' / f'episode-{args.episode}-agent-{seat}-logs.json'
        calls = json.loads(path.read_text())
        entries = [entry for call in calls for entry in call]
        assert len(entries) == 719
        assert all(not e['stdout'] and not e['stderr'] for e in entries)
        logs.append({'seat': seat, 'calls': len(entries),
                     'total_seconds': sum(float(e['duration']) for e in entries),
                     'max_seconds': max(float(e['duration']) for e in entries)})
    report = {'episode': args.episode, 'rewards': rewards, 'statuses': remote['statuses'],
              'submitted_actions': 1438, 'action_mismatches': 0, 'logs': logs,
              'replay_sha256': hashlib.sha256(replay_path.read_bytes()).hexdigest()}
    (OUT / 'kaggle_validation/audit.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
