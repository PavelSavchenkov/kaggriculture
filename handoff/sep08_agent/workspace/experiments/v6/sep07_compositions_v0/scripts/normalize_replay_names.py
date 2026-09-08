"""Resolve a renamed team from a previously validated identical submission."""
import argparse
import csv
import hashlib
import json
from pathlib import Path

EXP=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--research-dir',type=Path,required=True)
    parser.add_argument('--previous-dir',type=Path,required=True)
    args=parser.parse_args()
    current=args.research_dir.resolve();previous=args.previous_dir.resolve()
    assert current.is_relative_to(EXP) and previous.is_relative_to(EXP)
    assert (previous/'REFRESH.json').is_file()
    old=list(csv.DictReader((previous/'top_replay_manifest.csv').open()))
    programs=json.loads((previous/'PROGRAM_SOURCES.json').read_text())
    verified={(p['episode'],p['team'],p['submission']) for p in programs}
    aliases={}
    for row in old:
        assert (int(row['episode_id']),row['team'],int(row['submission_id'])) in verified
        aliases.setdefault((row['team_id'],row['submission_id']),set()).add(row.get('replay_team',row['team']))
    manifest=current/'top_replay_manifest.csv'
    original=manifest.read_bytes()
    rows=list(csv.DictReader(original.decode().splitlines()))
    changed=[]
    for row in rows:
        replay=EXP/'replays'/f"episode-{row['episode_id']}-replay.json"
        names=json.loads(replay.read_bytes())['info']['TeamNames']
        if names.count(row['team'])==1:
            replay_team=row['team']
        else:
            matches=set(names)&aliases.get((row['team_id'],row['submission_id']),set())
            if len(matches)!=1:
                raise RuntimeError(f"cannot resolve exact prior submission identity: {row['team']} {row['episode_id']} {names}")
            replay_team=matches.pop()
            if names.count(replay_team)!=1:raise RuntimeError('ambiguous replay seat')
            changed.append({'team_id':row['team_id'],'submission_id':row['submission_id'],'episode_id':row['episode_id'],
                'current_team':row['team'],'replay_team':replay_team,'seat':names.index(replay_team)})
        row['replay_team']=replay_team
    backup=current/'top_replay_manifest.before_names.csv'
    if not backup.exists():backup.write_bytes(original)
    with manifest.open('w',newline='') as f:
        writer=csv.DictWriter(f,fieldnames=rows[0]);writer.writeheader();writer.writerows(rows)
    report={'previous_validated_cohort':str(previous.relative_to(EXP)),'renamed_rows':changed,
        'previous_manifest_sha256':hashlib.sha256((previous/'top_replay_manifest.csv').read_bytes()).hexdigest(),
        'previous_program_sources_sha256':hashlib.sha256((previous/'PROGRAM_SOURCES.json').read_bytes()).hexdigest(),
        'original_manifest_sha256':hashlib.sha256(backup.read_bytes()).hexdigest(),
        'normalized_manifest_sha256':hashlib.sha256(manifest.read_bytes()).hexdigest(),
        'rule':'Current name if uniquely present; otherwise exactly one historical replay name from the same team ID AND submission ID in the validated previous cohort. Fail if absent or ambiguous. Replay files are unchanged.'}
    (current/'REPLAY_NAME_ALIASES.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Resolved',len(changed),'renamed replay rows; all',len(rows),'seats unique.')


if __name__=='__main__':main()
