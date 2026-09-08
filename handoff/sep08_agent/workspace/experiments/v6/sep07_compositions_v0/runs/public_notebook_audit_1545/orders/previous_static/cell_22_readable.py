def cash_curve(ep, player):
    return [st[0]['observation']['farms'][player]['money'] for st in ep['steps']]
fig, ax = plt.subplots(figsize=(8, 3.4))
for player, colour, name in ((0, BLUE, 'player 0'), (1, ORANGE, 'player 1')):
    series = cash_curve(A['episode'], player)
    ax.plot([s / A['turns_per_day'] for s in range(len(series))], series, lw=2, color=colour, label=name)
    ax.annotate(f'{name}  ${series[-1]:,.0f}', xy=(A['last_day'], series[-1]), xytext=(6, 0), textcoords='offset points', va='center', color=colour, fontsize=8.5, weight='bold')
ax.axhline(A['start'][0], lw=1, color=GRID, zorder=0)
ax.set_xlim(0, max(1, A['last_day'] * 1.22))
ax.set_xlabel('day')
ax.set_ylabel('cash in bank')
ax.yaxis.set_major_formatter(mticker.StrMethodFormatter('${x:,.0f}'))
ax.grid(axis='y')
ax.set_title('Cash balance through the season')
plt.show()
