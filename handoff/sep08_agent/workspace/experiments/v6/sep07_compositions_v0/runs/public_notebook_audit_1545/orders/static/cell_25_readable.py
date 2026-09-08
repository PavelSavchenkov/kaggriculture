p0 = A['ledger'][A['ledger'].player == 0]
gross = p0.loc[p0.category.eq('revenue'), 'cash'].sum()
buckets = p0[~p0.category.eq('revenue')].groupby('category')['cash'].sum().abs().sort_values(ascending=True)
fig, ax = plt.subplots(figsize=(8, 3.0))
ax.barh(list(buckets.index), list(buckets.values), height=0.62, color=RED)
for name, value in buckets.items():
    ax.annotate(f'${value:,.0f}', xy=(value, name), xytext=(6, 0), textcoords='offset points', va='center', fontsize=8.5, color=INK_2)
ax.set_xlim(0, max(1, buckets.max() * 1.25) if not buckets.empty else 1)
ax.set_xlabel('total spend')
ax.xaxis.set_major_formatter(mticker.StrMethodFormatter('${x:,.0f}'))
ax.grid(axis='x')
ax.set_title(f'Spending by category (total revenue: ${gross:,.0f})')
plt.show()
