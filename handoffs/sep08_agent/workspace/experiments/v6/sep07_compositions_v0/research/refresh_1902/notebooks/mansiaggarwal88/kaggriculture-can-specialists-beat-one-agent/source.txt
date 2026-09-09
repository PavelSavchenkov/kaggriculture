import pandas as pd
from IPython.display import display, HTML

timeline = [
    {"Architecture": "Phase 3 (Monolith)", "Main Idea": "Simple melon snake farmer dumping inventory", "Evaluation": "Live Leaderboard", "Observed Result": "Score: 600", "Status": "Success (Baseline)", "Lesson": "Simplicity is robust but fragile to market crashes."},
    {"Architecture": "Phase 4b (Hungarian)", "Main Idea": "Task assignment via linear_sum_assignment", "Evaluation": "Live Leaderboard", "Observed Result": "Score: 330.9 - 348.0", "Status": "Failure", "Lesson": "Ignoring spatial density destroys execution efficiency."},
    {"Architecture": "Phase 6 (Opp. Cost)", "Main Idea": "Hungarian scheduler aware of opportunity cost", "Evaluation": "Local & Leaderboard", "Observed Result": "Local WR: 78.0% | LB: 504.7", "Status": "Mixed", "Lesson": "Local dominance does not translate if market-crash dynamics are ignored."},
    {"Architecture": "Phase 9 (Real Options)", "Main Idea": "Newsvendor + Real Options parameter sweep", "Evaluation": "Local Test Pool", "Observed Result": "Win Rate: 0.0%", "Status": "Failure", "Lesson": "Adversarial supply registers as negative demand, halting all production."},
    {"Architecture": "Phase 12 (Portfolio)", "Main Idea": "4 independent specialists + 1 Overseer", "Evaluation": "Local Gauntlet (320 Eps)", "Observed Result": "Win Rate: 0.0%", "Status": "Failure", "Lesson": "A component can be locally optimal but globally harmful without feasibility constraints."},
    {"Architecture": "Phase 12b (Format Fix)", "Main Idea": "Corrected BUY_SEED / BUY_ANIMAL command format", "Evaluation": "Local Gauntlet (320 Eps)", "Observed Result": "Scores: -77.9, -49.2, 28.0", "Status": "Mixed", "Lesson": "Malformed commands silently aborting a full turn's market actions mimic strategic failure."}
]

df = pd.DataFrame(timeline)
display(HTML(df.to_html(index=False)))


import matplotlib.pyplot as plt
import matplotlib.patches as patches

fig, ax = plt.subplots(figsize=(10, 8))
ax.axis('off')

# Colors
c_bg = '#f8f9fa'
c_edge = '#495057'
c_specialist = '#e9ecef'
c_overseer = '#dee2e6'
c_engine = '#e3f2fd'
c_text = '#212529'

def draw_box(x, y, w, h, text, color, edge=c_edge):
    rect = patches.Rectangle((x, y), w, h, facecolor=color, edgecolor=edge, linewidth=2, zorder=2)
    ax.add_patch(rect)
    ax.text(x + w/2, y + h/2, text, ha='center', va='center', fontsize=11, fontweight='bold', color=c_text, zorder=3)

def draw_arrow(x1, y1, x2, y2, text=None):
    ax.annotate('', xy=(x2, y2), xytext=(x1, y1),
                arrowprops=dict(facecolor=c_edge, width=2, headwidth=10, shrink=0.05), zorder=1)
    if text:
        mid_x, mid_y = (x1+x2)/2, (y1+y2)/2
        ax.text(mid_x + 0.2, mid_y, text, ha='left', va='center', fontsize=9, bbox=dict(facecolor='white', edgecolor='none', alpha=0.8))

# Draw Diagram 1: Portfolio Architecture
draw_box(3, 9, 4, 1, 'Game State (Observation)', c_bg)

draw_box(1, 7, 2, 1, 'Crop Specialist\n(Agronomist)', c_specialist)
draw_box(3.5, 7, 2, 1, 'Market Specialist\n(Trader)', c_specialist)
draw_box(6, 7, 2, 1, 'Labor Specialist\n(HR)', c_specialist)
draw_box(8.5, 7, 2, 1, 'Capital Specialist\n(CFO)', c_specialist)

draw_arrow(5, 9, 2, 8)
draw_arrow(5, 9, 4.5, 8)
draw_arrow(5, 9, 7, 8)
draw_arrow(5, 9, 9.5, 8)

draw_box(3, 5, 4, 1, 'Portfolio Overseer\n(Conflict Resolution)', c_overseer)

draw_arrow(2, 7, 4, 6, "Target Crops")
draw_arrow(4.5, 7, 4.5, 6, "Sell Orders")
draw_arrow(7, 7, 5.5, 6, "Payroll Req")
draw_arrow(9.5, 7, 6, 6, "Cap Req")

draw_box(3, 3, 4, 1, 'Execution Engine', c_engine)
draw_arrow(5, 5, 5, 4, "Feasible Global Plan")

draw_box(3, 1, 4, 1, 'New Game State', c_bg)
draw_arrow(5, 3, 5, 2, "Action Submitted")

plt.title("Diagram 1: Portfolio Architecture", fontsize=14, fontweight='bold', pad=20)
plt.xlim(0, 11.5)
plt.ylim(0, 10.5)
plt.show()


import matplotlib.pyplot as plt
import matplotlib.patches as patches

fig, ax = plt.subplots(figsize=(8, 6))
ax.axis('off')

# Colors
c_bg = '#f8f9fa'
c_edge = '#495057'
c_model = '#e9ecef'
c_check = '#dee2e6'
c_action = '#e3f2fd'
c_text = '#212529'

def draw_box(x, y, w, h, text, color):
    rect = patches.Rectangle((x, y), w, h, facecolor=color, edgecolor=c_edge, linewidth=2)
    ax.add_patch(rect)
    ax.text(x + w/2, y + h/2, text, ha='center', va='center', fontsize=11, fontweight='bold', color=c_text)

def draw_arrow(x1, y1, x2, y2):
    ax.annotate('', xy=(x2, y2), xytext=(x1, y1),
                arrowprops=dict(facecolor=c_edge, width=2, headwidth=10, shrink=0.05))

draw_box(2.5, 9, 3, 1, 'Observation', c_bg)
draw_arrow(4, 9, 4, 7.5)
draw_box(2.5, 6.5, 3, 1, 'Specialist Model', c_model)
draw_arrow(4, 6.5, 4, 5)
draw_box(2.5, 4, 3, 1, 'Recommendation', c_bg)
draw_arrow(4, 4, 4, 2.5)
draw_box(1.5, 1.5, 5, 1, 'Constraint Check', c_check)
draw_arrow(4, 1.5, 4, 0)
draw_box(2.5, -1, 3, 1, 'Accept/Modify/Reject', c_bg)
draw_arrow(4, -1, 4, -2.5)
draw_box(2.5, -3.5, 3, 1, 'Execution', c_action)

plt.title("Diagram 2: Specialist Decision Flow", fontsize=14, fontweight='bold', pad=20)
plt.xlim(0, 8)
plt.ylim(-4.5, 10.5)
plt.show()


import matplotlib.pyplot as plt
import matplotlib.patches as patches

fig, ax = plt.subplots(figsize=(8, 8))
ax.axis('off')

# Colors
c_edge = '#495057'
c_text = '#212529'
c_node = '#f8f9fa'

steps = [
    "Crop Target",
    "Large Seed Purchase",
    "Cash Drain",
    "Insufficient Payroll",
    "Insufficient Labor",
    "Unmanaged Tasks",
    "Care Lapses",
    "Economic Collapse"
]

y = 8
for i, step in enumerate(steps):
    rect = patches.Rectangle((2.5, y-0.5), 3, 1, facecolor=c_node, edgecolor=c_edge, linewidth=2)
    ax.add_patch(rect)
    ax.text(4, y, step, ha='center', va='center', fontsize=10, fontweight='bold', color=c_text)
    
    if i < len(steps) - 1:
        ax.annotate('', xy=(4, y-0.5), xytext=(4, y-1.5),
                    arrowprops=dict(facecolor=c_edge, width=2, headwidth=10, shrink=0.05))
    y -= 1.5

plt.title("Diagram 3: The Failure Cascade", fontsize=14, fontweight='bold', pad=20)
plt.xlim(0, 8)
plt.ylim(-3.5, 9)
plt.show()
