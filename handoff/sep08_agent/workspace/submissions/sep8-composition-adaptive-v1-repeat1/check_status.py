"""Read and retain Kaggle status; this script cannot upload anything."""
import csv
import io
import json
import subprocess
from datetime import datetime, timezone
from pathlib import Path

OUT = Path(__file__).resolve().parent


def main():
    record = json.loads((OUT / 'SUBMISSION.json').read_text())
    result = subprocess.run(['conda', 'run', '-n', 'kaggriculture', 'kaggle',
                             'competitions', 'submissions', 'kaggriculture', '--csv'],
                            check=True, capture_output=True, text=True)
    (OUT / 'kaggle_submissions_after.csv').write_text(result.stdout)
    rows = list(csv.DictReader(io.StringIO(result.stdout)))
    matches = [row for row in rows if row['description'] == record['command'][-1]]
    assert len(matches) == 1, matches
    row = matches[0]
    assert record.get('submission_id', int(row['ref'])) == int(row['ref'])
    now = datetime.now(timezone.utc).isoformat()
    record.update(submission_id=int(row['ref']), server_date=row['date'],
                  status=row['status'], last_observed_utc=now,
                  submission_url='https://www.kaggle.com/competitions/kaggriculture/submissions?dialog=episodes-submission-' + row['ref'])
    if row['status'] == 'SubmissionStatus.COMPLETE':
        record.setdefault('terminal_observed_utc', now)
        if not record.get('public_score_at_validation'):
            record['public_score_at_validation'] = row['publicScore']
    (OUT / 'SUBMISSION.json').write_text(json.dumps(record, indent=2) + '\n')
    print(json.dumps(row, indent=2))


if __name__ == '__main__':
    main()
