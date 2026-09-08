from pathlib import Path
from datetime import datetime, timezone
import json

EXP = Path(__file__).resolve().parents[2]
now = datetime.now(timezone.utc).isoformat()
metrics = json.loads((EXP / 'research/review_49_comparison.json').read_text())
assert len(metrics) == 82
text = f'''# Review 50 — {now}

Due 18:28 UTC. The previous goal turn verified the completed user-requested
package and collected the terminal round-robin process. This turn adds its actual
analysis and fresh preregistered validation. The goal remains active through
September 8, 00:47 UTC. No further Git operation or Kaggle upload is authorized.

The 19-opening round robin completed 24,320 full games. q32 buffer13 wins 17 of
18 non-self matchups and is level against q0 buffer13; mean opponent utility is
93.7934%. It beats the q81 hire-saving control 125/128 with mean margin $3,496.16.
The earlier concern about losing to that control was incorrect: it compared own
cash from different opponents. Direct paired results contradict that inference.
Preserve this correction and compare both players within the same matchup.

The original 20-opponent fresh panel and operational gates pass. Promotion is
held until the newly added opening league is checked on 512 independent-shop
seeds and 128 native seeds, both seats. This is a fixed-candidate verification,
not another search over fresh results. Most older-opponent gains remain about
$5; broad historical utility is nearly unchanged, and two one-win regressions
and the public-sixday tail loss remain explicit.

The larger composition priority remains crop-to-animal conversion. Use a later
released tile with observed shops, compare goose/cow/sheep/retain-crops, estimate
lost crop output and new feed/service requirements, and compile the remaining
whole farm. Include the last day's harvest/deposit/sale and preserve the later
berry branch. A source route guard does not by itself preserve a changed suffix.
First compile one full candidate and a source control, then expand contexts.

Original intuition was reread at 18:30: dated compositions, fast economic and
service estimates, improvable placement, exact day schedules, intraday trading,
replay borrowing, general animal selection including wait, and a growing league
remain the objective. Arbitrary multi-investment and strong independent cold
construction are still incomplete. Opening gains do not fill those gaps.

The latest global cohort is still 17:59:45 UTC (72 player-games, 58 unique
replays). TITAN Arlene v14 contains a byte-identical existing policy; no new port.
The 82 metrics below are unchanged from review 49 because neither cohort nor
promoted local policy changed. They compare different scenarios, not paired
superiority. Next review is due 18:48 UTC; next public refresh about 18:59.

| Metric | Global 72 | Bohann local 64 |
| --- | ---: | ---: |
'''
for row in metrics:
    text += f"| {row['metric']} | {row['global72']:.4f} | {row['local64']:.4f} |\n"
(EXP / 'research/review_50.md').write_text(text)
(EXP / 'research/review_50_comparison.json').write_text(json.dumps(metrics, indent=2) + '\n')
entry = f'\n{now}: Review 50 records all 82 unchanged cohort metrics. Opening round robin 24,320 games resolves incorrect across-opponent inference: q32 beats q81 control 125/128 and has no losing variant matchup. Fresh fixed-candidate league audit started. Larger crop-to-animal compiler is next; original scope remains incomplete. Next review 18:48, goal through 00:47.\n'
for name in ['PROGRESS.md', 'IDEAS_LEDGER.md']:
    with (EXP / name).open('a') as file:
        file.write(entry)
print('Review 50 recorded', now)
