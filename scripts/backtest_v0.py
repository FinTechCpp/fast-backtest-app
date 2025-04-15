from igtrader.Strategies.BuyTrendFollowing import BuyTrendFollowingBA
from igtrader.Strategies.SellTrendFollowing import SellTrendFollowingBA
from igtrader.Strategies.CrossEMA import CrossEMABA
from igtrader.Strategies.Helpers import load_data

from igtrader.backtestingpy.backtesting.backtesting import Backtest


data = load_data(symbol='NDX', period='1d', interval='1min')


strategy = SellTrendFollowingBA
bt = Backtest(data, strategy, cash=100000, commission=.00, exclusive_orders=True, strategy_params={'stop_loss_distance': 20, 'take_profit_distance': 30})
stats = bt.run()
print(stats)
bt.plot(plot_volume=False, resample=False, indicator_height=300)

