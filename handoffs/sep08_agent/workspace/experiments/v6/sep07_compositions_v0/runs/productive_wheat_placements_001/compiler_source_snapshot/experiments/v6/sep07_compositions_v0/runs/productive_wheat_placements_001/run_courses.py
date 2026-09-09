"""Compile both complete continuations for seven new semantic placements."""
import hashlib
import json
import shutil
import subprocess
from concurrent.futures import ThreadPoolExecutor,as_completed
from datetime import datetime,timezone
from pathlib import Path

RUN=Path(__file__).resolve().parent
ROOT=RUN.parents[4]


def main():
    files={RUN/'CMakeLists.txt',Path(__file__).resolve(),RUN/'PROPOSALS.json'}
    for target in ['compile_wheat','compile_wheat_berry','inventory']:
        for dependency in (RUN/f'build/CMakeFiles/{target}.dir').rglob('*.o.d'):
            for name in dependency.read_text().replace('\\\n',' ').split()[1:]:
                path=Path(name).resolve()
                if path.is_relative_to(ROOT)and path.is_file():files.add(path)
    hashes={}
    for path in sorted(files):
        relative=path.relative_to(ROOT);target=RUN/'compiler_source_snapshot'/relative
        target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(path,target)
        hashes[str(relative)]=hashlib.sha256(path.read_bytes()).hexdigest()
    (RUN/'COMPILER_INPUTS.json').write_text(json.dumps({'source_sha256':hashes,'binaries':{
        name:hashlib.sha256((RUN/'build'/name).read_bytes()).hexdigest()for name in ['compile_wheat','compile_wheat_berry','inventory']}},indent=2)+'\n')
    jobs=[]
    for p in json.loads((RUN/'PROPOSALS.json').read_text()):
        if p['frozen_control']:continue
        for berry in [False,True]:
            name=p['name']+('_berry'if berry else'');assert not(RUN/name).exists()
            jobs.append((name,p['cells'],berry))
    records=[]
    def run(job):
        name,cells,berry=job
        command=['conda','run','--no-capture-output','-n','kaggriculture',str(RUN/'build'/('compile_wheat_berry'if berry else'compile_wheat')),str(RUN/name),'1000',','.join(map(str,cells)),'2']
        started=datetime.now(timezone.utc).isoformat()
        with(RUN/f'{name}.log').open('w')as log:result=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT)
        return{'name':name,'command':command,'returncode':result.returncode,'started_utc':started,'finished_utc':datetime.now(timezone.utc).isoformat()}
    with ThreadPoolExecutor(max_workers=3)as pool:
        for future in as_completed([pool.submit(run,job)for job in jobs]):
            record=future.result();records.append(record)
            (RUN/'COMPILE_COMMANDS.json').write_text(json.dumps(records,indent=2)+'\n')
            print(record['name'],record['returncode'],record['finished_utc'],flush=True)


if __name__=='__main__':main()
