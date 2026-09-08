def price_of(item, inventory, params=MARKET_PARAMS):
    """Return the installed engine's sell quote at the specified inventory.

    Pass the episode's resolved parameters when analysing a replay. Prices are
    rounded to whole dollars and cannot fall below the engine's price floor.
    """
    return market_price(item, inventory, params)
summary = pd.DataFrame([{'product': item, 'base': p['base'], 'T (one field, 24 days)': p['T'], 'price at I0+T': price_of(item, p['I0'] + p['T']), 'price at I0+2T': price_of(item, p['I0'] + 2 * p['T'])} for item, p in MARKET_PARAMS.items()])
summary['value kept at I0+T'] = (summary['price at I0+T'] / summary['base']).map('{:.0%}'.format)
summary
