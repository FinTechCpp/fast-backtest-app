import os
import sys
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

from Helpers import load_data
from backtesting import Backtest
from BacktestAdapter import BacktestingAdapter

data = load_data(symbol='NDX', period='2y', interval='1min')

bt = Backtest(data, BacktestingAdapter, cash=100000, commission=.00, exclusive_orders=True)
stats = bt.run()
print(stats)
bt.plot()
