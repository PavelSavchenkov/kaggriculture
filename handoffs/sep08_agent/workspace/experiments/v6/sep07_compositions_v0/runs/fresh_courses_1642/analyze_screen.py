"""Summarize the complete recent-course discovery matrix and retain all rows."""
import json
from pathlib import Path

RUN = Path(__file__).resolve().parent
OPPONENTS = ['crop_mix_t2_wheat', 'wheat_one_fert', 'teammate_shoprouter', 'public_router', 'king_rc4', 'public_router_v5']


def main():
    programs = json.loads((RUN / 'library/IMPORT.json').read_text())['programs']
    rows = []
    baseline = {opponent: json.loads((RUN / f'discovery/current_vs_{opponent}.json').read_text()) for opponent in OPPONENTS}
    for program in programs:
        index = program['program']
        row = {'program': index, 'donors': program['donors'], 'per_opponent': {}}
        for opponent in OPPONENTS:
            result = json.loads((RUN / f'discovery/fresh_course_{index}_vs_{opponent}.json').read_text())
            assert len(result['games']) == 64 and all(g['turns'] == 719 for g in result['games'])
            assert [(g['seed'], g['seat']) for g in result['games']] == [(g['seed'], g['seat']) for g in baseline[opponent]['games']]
            values = {key: result[key] for key in ['win_utility', 'mean_cash', 'mean_margin', 'margin_cvar10']}
            values['paired_margin_gain_vs_current'] = result['mean_margin'] - baseline[opponent]['mean_margin']
            values['paired_utility_gain_vs_current'] = result['win_utility'] - baseline[opponent]['win_utility']
            values['mean_produced'] = [sum(g['produced'][i] for g in result['games']) / 64 for i in range(9)]
            values['mean_spend'] = sum(g['spend'] for g in result['games']) / 64
            values['mean_faults'] = sum(g['unit_faults'] for g in result['games']) / 64
            values['mean_discards'] = sum(sum(g['discarded']) for g in result['games']) / 64
            row['per_opponent'][opponent] = values
        row['mean_utility'] = sum(v['win_utility'] for v in row['per_opponent'].values()) / len(OPPONENTS)
        row['mean_margin_gain_vs_current'] = sum(v['paired_margin_gain_vs_current'] for v in row['per_opponent'].values()) / len(OPPONENTS)
        rows.append(row)
    rows.sort(key=lambda r: (r['mean_utility'], r['mean_margin_gain_vs_current']), reverse=True)
    report = {'scope': 'Discovery only: 69 courses, 32 seeds, both seats, six opponents; no promotion.',
              'programs': len(programs), 'candidate_games': len(programs) * 64 * len(OPPONENTS),
              'baseline_games': 64 * len(OPPONENTS), 'results': rows}
    (RUN / 'SCREEN.json').write_text(json.dumps(report, indent=2) + '\n')
    for row in rows[:15]:
        direct = row['per_opponent']['crop_mix_t2_wheat']
        print(row['program'], row['donors'][0]['team'], 'utility', round(row['mean_utility'], 4),
              'paired margin gain', round(row['mean_margin_gain_vs_current'], 1),
              'direct', direct['win_utility'], direct['mean_margin'])


if __name__ == '__main__':
    main()
