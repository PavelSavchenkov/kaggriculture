import matplotlib.pyplot as plt
variants = ['Fixed Base Tape', 'No Day 6 Switch', 'No Day 24 Switch', 'Full V5 Router']
win_rates = [84.57, 85.74, 91.98, 93.76]
colors = ['#94a3b8', '#38bdf8', '#0284c7', '#10b981']
plt.figure(figsize=(9, 4.5), dpi=120)
bars = plt.bar(variants, win_rates, color=colors, width=0.55, edgecolor='#1e293b')
plt.ylabel('Win Rate Percentage', fontsize=12, fontweight='bold')
plt.title('Win Rate Progression Across 44,096 Frozen Ladder Games', fontsize=13, fontweight='bold', pad=14)
plt.ylim(75, 100)
plt.grid(axis='y', linestyle=':', alpha=0.6)
for bar in bars:
    height = bar.get_height()
    plt.text(bar.get_x() + bar.get_width() / 2.0, height + 0.6, f'{height:.2f}%', ha='center', va='bottom', fontsize=10, fontweight='bold', color='#0f172a')
plt.tight_layout()
plt.show()