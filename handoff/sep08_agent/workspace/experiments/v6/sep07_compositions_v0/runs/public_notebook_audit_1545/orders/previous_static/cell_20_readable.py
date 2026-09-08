CAT_ORDER = ['revenue', 'seeds', 'livestock', 'feed', 'fertilizer', 'labor', 'land']

def pnl(ledger, player=0, params=MARKET_PARAMS):
    """Summarise cash flows using the episode's resolved market parameters."""
    df = ledger[ledger.player == player]
    t = df.groupby(['category', 'item'], as_index=False).agg(units=('units', 'sum'), cash=('cash', 'sum'))
    base = t['item'].map(lambda i: params[i]['base'] if i in params else None)
    is_rev = t.category.eq('revenue')
    t['avg price'] = (t.cash / t.units).where(is_rev).round(1)
    t['base price'] = base.where(is_rev)
    t['revenue gap vs base'] = (t.units * base - t.cash).where(is_rev).round(0)
    t['_order'] = t.category.map(lambda c: CAT_ORDER.index(c) if c in CAT_ORDER else 99)
    return t.sort_values(['_order', 'cash'], ascending=[True, False]).drop(columns='_order').reset_index(drop=True)
table = pnl(A['ledger'], 0, A['params'])
start = A['start'][0]
gross = table.loc[table.category.eq('revenue'), 'cash'].sum()
costs = table.loc[~table.category.eq('revenue'), 'cash'].sum()
print(f'start ${start:,.0f}   + revenue ${gross:,.0f}   - costs ${-costs:,.0f}   = ${start + gross + costs:,.0f}\n')
table.fillna('')
