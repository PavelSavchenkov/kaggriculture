"""Lists our Kaggle submissions (id, date, description, status, public score) into submissions.json."""
import json
from kaggle.api.kaggle_api_extended import KaggleApi
api = KaggleApi(); api.authenticate()
rows = []
for page in range(1, 6):
    subs = api.competition_submissions('kaggriculture', page_number=page)
    if not subs: break
    rows += [s.to_dict() for s in subs]
json.dump(rows, open('submissions.json', 'w'), indent=1, default=str)
for s in rows[:40]:
    print(s.get('ref') or s.get('id'), s.get('date'), s.get('status'), s.get('publicScore'), (s.get('description') or '')[:70])
