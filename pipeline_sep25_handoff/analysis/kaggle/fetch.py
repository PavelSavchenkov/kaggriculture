"""Downloads our Kaggle submissions' episodes and exports engine traces (reuses the experiment's
collect_replays.py: rate-limited API, export_replay with parity hashes).
usage: fetch.py <submission id> [...]  -> episodes_<id>.json, replays/<ep>.json.gz, traces/<ep>.txt,
meta_<id>.csv (episode, seat, our/opponent team, submission ids, rewards, created)."""
import csv, gzip, importlib.util, json, os, sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
HERE = Path(__file__).resolve().parent
E = Path(os.environ['BC_EXPERIMENT'])  # experiment folder made by setup_experiment.sh
spec = importlib.util.spec_from_file_location('cr', E / 'scripts/collect_replays.py')
cr = importlib.util.module_from_spec(spec); spec.loader.exec_module(cr)

def fetch(episode):
    d = HERE / 'replays'; d.mkdir(exist_ok=True)
    path = d / f'{episode}.json.gz'
    if not path.exists():
        cr.request(cr.api().competition_episode_replay, episode, str(d), quiet=True)
        raw = d / f'episode-{episode}-replay.json'
        data = raw.read_bytes()
        with gzip.open(path, 'wb', compresslevel=2) as s: s.write(data)
        raw.unlink()
    trace = HERE / 'traces' / f'{episode}.txt'
    with gzip.open(path, 'rb') as s: replay = json.load(s)
    if not trace.exists() and len(replay['steps']) == 720:
        trace.parent.mkdir(exist_ok=True)
        cr.export_replay(replay, trace)
    return episode, replay

for sub in map(int, sys.argv[1:]):
    listing = HERE / f'episodes_{sub}.json'
    rows = [r.to_dict() for r in cr.request(cr.api().competition_list_episodes, sub)]
    json.dump(rows, open(listing, 'w'), indent=1, default=str)
    done = [r for r in rows if 'COMPLETED' in str(r['state'])]
    print(sub, 'episodes', len(rows), 'completed', len(done), flush=True)
    out = []
    with ThreadPoolExecutor(max_workers=3) as pool:
        for ep, replay in pool.map(fetch, [int(r['id']) for r in done]):
            meta = next(r for r in done if int(r['id']) == ep)
            ours = [a for a in meta['agents'] if int(a['submissionId']) == sub]
            if len(ours) != 1: continue  # validation episode: our submission in both seats
            seat = int(ours[0].get('index', 0))
            other = next(a for a in meta['agents'] if int(a['submissionId']) != sub)
            final = replay['steps'][-1]
            out.append(dict(episode=ep, seat=seat, type=meta.get('type'), created=meta['createTime'],
                            opp_team=replay['info']['TeamNames'][1 - seat], opp_submission=other['submissionId'],
                            opp_team_id=other.get('teamId'), steps=len(replay['steps']),
                            own=final[seat]['reward'], opp=final[1 - seat]['reward'],
                            own_status=final[seat]['status'], opp_status=final[1 - seat]['status'],
                            own_score_before=next((a.get('initialScore') for a in meta['agents'] if int(a['submissionId']) == sub), None),
                            opp_score_before=other.get('initialScore'), own_score_after=next((a.get('updatedScore') for a in meta['agents'] if int(a['submissionId']) == sub), None)))
            print(ep, out[-1]['opp_team'], out[-1]['own'], out[-1]['opp'], flush=True)
    with open(HERE / f'meta_{sub}.csv', 'w', newline='') as f:
        w = csv.DictWriter(f, fieldnames=list(out[0])); w.writeheader(); w.writerows(sorted(out, key=lambda r: r['created']))
