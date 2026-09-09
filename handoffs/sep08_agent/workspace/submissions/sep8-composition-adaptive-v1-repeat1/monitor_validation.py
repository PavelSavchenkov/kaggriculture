"""Wait for the recorded submission, download validation evidence and audit it."""
import json
import subprocess
import time
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

import check_status

OUT = Path(__file__).resolve().parent


def kaggle(arguments):
    return subprocess.run(['conda', 'run', '-n', 'kaggriculture', 'kaggle',
                           'competitions', *arguments], check=True,
                          capture_output=True, text=True).stdout


def main():
    for attempt in range(20):
        check_status.main()
        record = json.loads((OUT / 'SUBMISSION.json').read_text())
        assert record['status'] in ['SubmissionStatus.PENDING', 'SubmissionStatus.COMPLETE'], record['status']
        if record['status'] == 'SubmissionStatus.COMPLETE':
            raw = kaggle(['episodes', str(record['submission_id']), '--format', 'json'])
            (OUT / 'kaggle_episodes.json.txt').write_text(raw)
            if raw.lstrip().startswith('['):
                episodes = json.JSONDecoder().raw_decode(raw.lstrip())[0]
                matches = [episode for episode in episodes
                           if episode['type'] == 'EpisodeType.EPISODE_TYPE_VALIDATION'
                           and episode['state'] == 'EpisodeState.COMPLETED']
                if matches:
                    assert len(matches) == 1, matches
                    break
        print('Waiting for completed validation, check', attempt + 1, flush=True)
        time.sleep(30)
    else:
        raise RuntimeError('Validation is still pending; inspect recorded status. Do not upload again.')
    episode = str(matches[0]['id'])
    destination = OUT / 'kaggle_validation'
    destination.mkdir(exist_ok=True)
    commands = [['replay', episode, '-p', str(destination), '-q']]
    commands += [['logs', episode, str(seat), '-p', str(destination), '-q'] for seat in range(2)]
    with ThreadPoolExecutor(max_workers=3) as pool:
        for output in pool.map(kaggle, commands):
            if output.strip():
                print(output, flush=True)
    subprocess.run(['conda', 'run', '-n', 'kaggriculture', 'python',
                    str(OUT / 'audit_kaggle_validation.py'), episode], check=True)
    audit = json.loads((destination / 'audit.json').read_text())
    record.update(validation_episode_id=int(episode), validation_audit=audit,
                  task_status='COMPLETE', uploads=1)
    (OUT / 'SUBMISSION.json').write_text(json.dumps(record, indent=2) + '\n')
    print('Submission and server replay audit complete:', record['submission_id'], flush=True)


if __name__ == '__main__':
    main()
