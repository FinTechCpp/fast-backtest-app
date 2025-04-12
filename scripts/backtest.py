from igtrader.Strategies.BuyTrendFollowing import BuyTrendFollowingBA
from igtrader.Strategies.SellTrendFollowing import SellTrendFollowingBA
from igtrader.Strategies.Helpers import load_data

from igtrader.backtestingpy.backtesting.backtesting import Backtest


data = load_data(symbol='NDX', period='1m', interval='1min')

print(data.head())
print(data.tail(100))


strategy = BuyTrendFollowingBA
bt = Backtest(data, strategy, cash=100000, commission=.00, exclusive_orders=True)
stats = bt.run()
print(stats)
bt.plot(plot_volume=False, resample=False)

