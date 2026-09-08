def price_path(item):
    """Every price the engine quoted for one product, in order."""
    return [st[0]['observation']['market']['prices'][item] for st in steps]

def market_row(item):
    path, base = (price_path(item), PARAMS[item]['base'])
    return {'product': item, 'base': base, 'lowest': min(path), 'highest': max(path), 'final': path[-1], 'observations above base (%)': '{:.1%}'.format(sum((p > base for p in path)) / len(path))}
market_reality = pd.DataFrame([market_row(item) for item in PRODUCTS])
market_reality
