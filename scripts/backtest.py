from igtrader.Strategies.BuyTrendFollowingStrategy.BacktestAdapter import BacktestingAdapter as BuyTrendFollowingBTA
from igtrader.Strategies.SellTrendFollowingStrategy.BacktestAdapter import BacktestingAdapter as SellTrendFollowingBTA
from igtrader.Strategies.Helpers import load_data

from igtrader.backtestingpy.backtesting.backtesting import Backtest


data = load_data(symbol='NDX', period='1m', interval='1min')

print(data.head())
print(data.tail(100))


strategy = BuyTrendFollowingBTA
bt = Backtest(data, strategy, cash=100000, commission=.00, exclusive_orders=True)
stats = bt.run()
print(stats)
bt.plot(plot_volume=False, resample=False)

