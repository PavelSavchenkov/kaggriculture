"""Current leaderboard -> leaderboard.csv (rank, team, score)."""
import csv, io, zipfile
from pathlib import Path
from kaggle.api.kaggle_api_extended import KaggleApi
api = KaggleApi(); api.authenticate()
d = Path('lb'); d.mkdir(exist_ok=True)
api.competition_leaderboard_download('kaggriculture', str(d))
for z in d.glob('*.zip'):
    zipfile.ZipFile(z).extractall(d)
rows = list(csv.DictReader(open(next(d.glob('*.csv')), encoding='utf-8-sig')))
for r in rows[:25]: print(r['Rank'], r['TeamName'], r['Score'])
print(len(rows))
