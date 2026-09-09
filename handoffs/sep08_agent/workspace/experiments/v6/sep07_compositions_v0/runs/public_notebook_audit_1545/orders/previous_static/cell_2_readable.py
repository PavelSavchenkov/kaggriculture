import warnings
from collections import defaultdict
from copy import deepcopy
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import pandas as pd
import kaggle_environments
from kaggle_environments import make
try:
    from kaggle_environments.envs.kaggriculture.kaggriculture import MARKET_PARAMS, CROPS, ANIMALS, PRODUCTS, LAND_PRICES, PRICE_FLOOR, market_price, _apply_unit_action
except ImportError as exc:
    raise ImportError(f'{exc} -- this notebook needs kaggle-environments == 1.32.7, and the installed version is {kaggle_environments.__version__}. Turn on internet so the install cell above can run, or upgrade the package.') from exc
warnings.filterwarnings('ignore')
pd.set_option('display.width', 120)
SURFACE, INK, INK_2, GRID = ('#fcfcfb', '#0b0b0b', '#52514e', '#dcdbd6')
BLUE, ORANGE, RED = ('#2a78d6', '#eb6834', '#e34948')
plt.rcParams.update({'figure.facecolor': SURFACE, 'axes.facecolor': SURFACE, 'savefig.facecolor': SURFACE, 'figure.dpi': 130, 'font.size': 9, 'axes.edgecolor': GRID, 'axes.labelcolor': INK_2, 'axes.titlecolor': INK, 'axes.titlesize': 11, 'axes.titleweight': 'bold', 'axes.titlelocation': 'left', 'axes.titlepad': 10, 'axes.spines.top': False, 'axes.spines.right': False, 'xtick.color': INK_2, 'ytick.color': INK_2, 'text.color': INK, 'grid.color': GRID, 'grid.linewidth': 0.6, 'legend.frameon': False})
print('kaggle_environments', kaggle_environments.__version__)
print('market parameters loaded for:', ', '.join(PRODUCTS))
