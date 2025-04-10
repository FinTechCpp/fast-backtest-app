from BuyTrendFollowingStrategy.BacktestAdapter import BacktestingAdapter as BuyTrendFollowingBTA
from SellTrendFollowingStrategy.BacktestAdapter import BacktestingAdapter as SellTrendFollowingBTA
from Helpers import load_data
from backtesting import Backtest



data = load_data(symbol='NDX', period='1m', interval='1min')

print(data.head())
print(data.tail(100))


strategy = BuyTrendFollowingBTA
bt = Backtest(data, strategy, cash=100000, commission=.00, exclusive_orders=True)
stats = bt.run()
print(stats)
bt.plot()

