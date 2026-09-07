import sys
import subprocess
subprocess.run([sys.executable, '-m', 'pip', 'install', '-q', '-U', 'kaggle-environments>=1.32.7'])
for mod in list(sys.modules.keys()):
    if mod == 'kaggle_environments' or mod.startswith('kaggle_environments.'):
        del sys.modules[mod]
import importlib
import time
from kaggle_environments import make
import main
importlib.reload(main)
main._SESSIONS.clear()
print('Starting 720 turn simulation against random agent...')
start_time = time.time()
env = make('kaggriculture', configuration={'episodeSteps': 720})
env.run(['main.py', 'random'])
duration = time.time() - start_time
score_agent = env.steps[-1][0]['reward']
score_random = env.steps[-1][1]['reward']
print(f'Match completed in {duration:.2f} seconds ({720 / duration:.1f} turns per second)')
print(f'V5 Router Score: {score_agent:,.0f} coins')
print(f'Random Baseline Score: {score_random:,.0f} coins')
assert score_agent > 100000, f'Score {score_agent} is lower than expected threshold'
print('Validation passed successfully. Agent is clean and ready for submission.')