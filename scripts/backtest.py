from igtrader.Strategies.BuyTrendFollowing import BuyTrendFollowingBA
from igtrader.Strategies.SellTrendFollowing import SellTrendFollowingBA
from igtrader.Strategies.Helpers import load_data

from igtrader.backtestingpy.backtesting.backtesting import Backtest

data = load_data(symbol='NDX', 
                 period='1m', 
                 interval='1min', 
                 end_date='10/03/2023',
                 timezone='Europe/Paris')


strategy = BuyTrendFollowingBA
bt = Backtest(data, strategy, cash=100000, commission=.00, spread=0.002, exclusive_orders=True)
stats = bt.run()
print(stats)
bt.plot(plot_volume=False, resample=False)

