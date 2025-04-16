from igtrader.Strategies.BuyTrendFollowing import BuyTrendFollowingBA
from igtrader.Strategies.SellTrendFollowing import SellTrendFollowingBA
from igtrader.Strategies.CrossEMA import CrossEMABA
from igtrader.Strategies.Helpers import load_data
from igtrader.backtestingpy.backtesting.backtesting import Backtest

from datetime import time


data = load_data(symbol='NDX', period='7d', interval='1min')


strategy = SellTrendFollowingBA
strategy_params = {
    'stop_loss_distance': 20,
    'take_profit_distance': 30,
    'trading_from': time(19, 0),
    'trading_to': time(21, 0),
    'trading_days': [0, 1, 2, 3, 4],  # Monday to Friday
}
bt = Backtest(data, strategy, cash=100000, commission=.00, exclusive_orders=True, strategy_kwargs=strategy_params)
stats = bt.run()
print(stats)
bt.plot(plot_volume=False, resample=False, indicator_height=300)

