"""Product-level model residuals, runtime and paired fresh-seed uncertainty."""
import csv
import hashlib
import json
import random
import statistics as s
from collections import defaultdict
from pathlib import Path
from branch_inputs import FAMILIES

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ITEMS = 'WHEAT CARROT TOMATO STRAWBERRY MELON EGG MILK WOOL FERTILIZER'.split()


def main():
    cfg = FAMILIES['atakan'];source = EXP / 'runs' / cfg['source'];hashes = {}
    def load(path):
        hashes[str(path.relative_to(EXP))] = hashlib.sha256(path.read_bytes()).hexdigest()
        return json.loads(path.read_text())
    predictions = load(RUN / 'atakan_residuals.json')
    panels = {(rival, branch): load(source / f'results/{branch}_vs_{rival}.json')['games']
              for rival in cfg['rivals'] for branch in cfg['branches']}
    quantities = defaultdict(list);timings = defaultdict(list);case_rows = []
    original = load(RUN / 'atakan_predictions.json')
    for pred, old in zip(predictions, original):
        index = (pred['seed'] - 1000) * 2 + pred['seat']
        for mode in pred['modes']:
            old_values = next(v['branches'] for v in old['variants'] if v['integration'] == 64
                              and v['crop_mode'] == mode['mode'] and v['feed_net'] == 0)
            assert mode['values'] == old_values
            timings[mode['mode']].append(mode['nanoseconds'] / 1e6)
            for branch in cfg['branches']:
                game = panels[pred['rival'], branch][index]
                assert (game['seed'], game['seat']) == (pred['seed'], pred['seat'])
                actual = [[0] * 9 for _ in range(30)]
                for step, product, bought, sold in game['opponent_profile']['flows']:
                    if step >= 226:
                        actual[step // 24][product] += sold - bought
                for p in range(9):
                    forecast = [day[p] for day in mode['rival_flow']]
                    truth = [day[p] for day in actual]
                    quantities[mode['mode'], p].append({'predicted': sum(forecast), 'actual_net_sold': sum(truth),
                        'total_error': abs(sum(forecast) - sum(truth)),
                        'dated_error': sum(abs(a - b) for a, b in zip(forecast, truth))})
            if pred['seed'] == 1028 and pred['rival'] == 'public_router_v5':
                audits = [load(source / f'results/cash_{branch}_vs_public_router_v5.json')[index]
                          for branch in cfg['branches']]
                for p, item in enumerate(ITEMS):
                    values = mode['product_values']
                    row = {'seat': pred['seat'], 'mode': mode['mode'], 'item': item}
                    for who, seat_index in [('own', 0), ('rival', 1)]:
                        row[f'predicted_{who}_net_revenue_delta'] = values[2][p][who] - values[0][p][who]
                        actual = [a['players'][seat_index] for a in audits]
                        row[f'exact_{who}_net_revenue_delta'] = (actual[2]['revenue'][p] - actual[2]['spend'][p]
                                                               - actual[0]['revenue'][p] + actual[0]['spend'][p])
                    row['predicted_rival_quantity'] = sum(day[p] for day in mode['rival_flow'])
                    for branch, audit in zip(cfg['branches'], audits):
                        row[f'{branch}_actual_rival_sold'] = audit['players'][1]['sold'][p]
                        row[f'{branch}_actual_rival_bought'] = audit['players'][1]['bought'][p]
                    case_rows.append(row)
    metrics = [{'mode': mode, 'item': ITEMS[p], 'branch_contexts': len(rows),
                **{key: s.mean(r[key] for r in rows) for key in rows[0]}}
               for (mode, p), rows in quantities.items()]
    runtime = {mode: {'calls': len(v), 'median_ms': s.median(v), 'p95_ms': sorted(v)[int(.95 * len(v))],
                      'max_ms': max(v)} for mode, v in timings.items()}
    fresh = load(RUN / 'fresh_1660000/branch_cases.json');grouped = defaultdict(dict)
    for row in fresh:
        if row['objective'] == 'margin' and row['integration'] == 64 and row['feed_net'] == 0 and row['crop_mode'] in [0, 4]:
            grouped[row['family'], row['seed'], row['seat'], row['rival']][row['crop_mode']] = row
    paired = defaultdict(lambda: defaultdict(list))
    for (family, seed, seat, rival), rows in grouped.items():
        paired[family][seed].append((rows[4]['win'] - rows[0]['win'], rows[4]['margin'] - rows[0]['margin']))
    intervals = {};rng = random.Random(20260907)
    for family, seeds in paired.items():
        clusters = [(s.mean(v[0] for v in rows), s.mean(v[1] for v in rows)) for rows in seeds.values()]
        bootstrap = [tuple(s.mean(sample[i] for sample in rng.choices(clusters, k=len(clusters))) for i in [0, 1])
                     for _ in range(10000)]
        intervals[family] = {'independent_seed_clusters': len(clusters), 'resamples': 10000,
                            'win_rate_delta': s.mean(v[0] for v in clusters),
                            'mean_margin_delta': s.mean(v[1] for v in clusters),
                            'win_rate_delta_95_percentile': [sorted(v[0] for v in bootstrap)[i] for i in [250, 9750]],
                            'margin_delta_95_percentile': [sorted(v[1] for v in bootstrap)[i] for i in [250, 9750]]}
    report = {'scope': 'Current public crop/herd forecasts versus all later realized net sales. Future private expansion/replanting, feed use and sale timing are unobserved forecast errors. Quantities repeat three alternative branches per context.',
              'quantity_metrics': metrics, 'runtime': runtime, 'fresh_paired_bootstrap': intervals,
              'numeric_reproduction': '960 complete three-branch valuations equal the original diagnostic exactly.',
              'source_sha256': hashes}
    (RUN / 'RESIDUAL_REPORT.json').write_text(json.dumps(report, indent=2) + '\n')
    for name, rows in [('local_quantity_residuals.csv', metrics), ('seed1028_product_attribution.csv', case_rows)]:
        with (RUN / name).open('w') as f:
            writer = csv.DictWriter(f, fieldnames=rows[0]);writer.writeheader();writer.writerows(rows)
    print(json.dumps({'runtime': runtime, 'intervals': intervals}, indent=2))


if __name__ == '__main__':
    main()
