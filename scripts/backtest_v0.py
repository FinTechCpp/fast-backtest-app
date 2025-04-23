from igtrader.Strategies.BuyTrendFollowing import BuyTrendFollowingBA
from igtrader.Strategies.SellTrendFollowing import SellTrendFollowingBA
from igtrader.Strategies.CrossEMA import CrossEMABA
from igtrader.Strategies.Helpers import load_data
from igtrader.backtestingpy.backtesting.backtesting import Backtest

from datetime import time


data = load_data(symbol='NDX', period='1d', interval='20secs')

print(data.head())
strategy = BuyTrendFollowingBA
strategy_kwargs = {
  # base
  'trading_from': time(19, 0),
  'trading_to':   time(21, 0),
  'trading_days': [0, 1, 2, 3, 4],
  'take_profit_distance': 30,
  'stop_loss_distance': 20,
  # spécifique
  'ema_short_period': 50,
  'ema_long_period': 200,
  'stoch_fastk': 10,
  'stoch_slowk': 7,
  'stoch_slowd': 3,
}
bt = Backtest(data, strategy, cash=100000, commission=.00, exclusive_orders=True, strategy_kwargs=strategy_kwargs)
stats = bt.run()
print(stats)
# bt.plot(plot_volume=False, resample=False, indicator_height=300)

