"""Make the one explicitly authorized upload after all concrete validation gates pass."""
import hashlib
import json
import subprocess
from datetime import datetime, timezone
from pathlib import Path

OUT = Path(__file__).resolve().parent


def main():
    record_path = OUT / 'SUBMISSION.json'
    assert not record_path.exists(), 'An upload attempt is already recorded. Inspect Kaggle; do not retry.'
    selection = json.loads((OUT / 'SELECTION.json').read_text())
    assert selection['validated_for_submission']
    assert selection['agent'] == 'animal_repair_q24_premium_m2'
    artifacts = json.loads((OUT / 'ARTIFACTS.json').read_text())
    for name, expected in artifacts.items():
        data = (OUT / name).read_bytes()
        assert len(data) == expected['bytes'] and hashlib.sha256(data).hexdigest() == expected['sha256']
    frozen = json.loads((OUT / 'FROZEN.json').read_text())
    for name, digest in frozen['files'].items():
        assert hashlib.sha256((OUT / 'source_tree' / name).read_bytes()).hexdigest() == digest, name
    packed = json.loads((OUT / 'packed_v1_validation.json').read_text())
    assert packed['matched_actions'] == 94908 and packed['archive_sha256'] == artifacts['submission.tar.gz']['sha256']
    assert packed['teammate']['wtl'][0] > 32 and packed['prior']['wtl'][0] > 32
    native = json.loads((OUT / 'FROZEN_CPP_VALIDATION.json').read_text())
    for name in ('teammate4096', 'prior4096'):
        record = next(r for r in native if r['name'] == name)
        assert record['games'] == 4096 and record['wtl'][0] > 2048
    operational = json.loads((OUT / 'file_runner_validation.json').read_text())
    assert sum(r['packed_native_cash_equal_games'] for r in operational['comparisons']) == 128
    assert operational['file_runner']['statuses'] == ['DONE', 'DONE']
    assert json.loads((OUT / 'branch_validation.json').read_text())['matched_actions'] >= 719 * 3
    assert json.loads((OUT / 'REBUILD.json').read_text())['artifacts'] == artifacts
    for filename, arguments in [('kaggle_submissions_before.csv', ['submissions', 'kaggriculture', '--csv']),
                                ('kaggle_limits_before.txt', ['submission-limits', 'kaggriculture'])]:
        result = subprocess.run(['conda', 'run', '-n', 'kaggriculture', 'kaggle', 'competitions', *arguments],
                                check=True, capture_output=True, text=True)
        (OUT / filename).write_text(result.stdout)
    description = 'sep8 composition adaptive v1 repeat1: byte-identical resubmission of 56101451, requested by user; archive SHA256 d65b8150db33becdedb4f752e8d773fe1c1b24306cb4459974104564d6bde374'
    command = ['conda', 'run', '-n', 'kaggriculture', 'kaggle', 'competitions', 'submit', 'kaggriculture',
               '-f', str(OUT / 'submission.tar.gz'), '-m', description]
    record = {'requested_utc': datetime.now(timezone.utc).isoformat(), 'agent': selection['agent'], 'command': command,
              'archive_sha256': artifacts['submission.tar.gz']['sha256'], 'attempts': 1, 'status': 'UPLOAD_STARTED'}
    with record_path.open('x') as stream:
        json.dump(record, stream, indent=2)
        stream.write('\n')
    result = subprocess.run(command, capture_output=True, text=True)
    record.update(returncode=result.returncode, stdout=result.stdout, stderr=result.stderr,
                  status='UPLOAD_ACCEPTED' if result.returncode == 0 else 'UPLOAD_RESULT_REQUIRES_INSPECTION')
    record_path.write_text(json.dumps(record, indent=2) + '\n')
    print(json.dumps(record, indent=2))
    result.check_returncode()


if __name__ == '__main__':
    main()
