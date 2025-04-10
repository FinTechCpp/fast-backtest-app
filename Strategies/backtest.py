from BuyTrendFollowingStrategy.BacktestAdapter import BacktestingAdapter as BuyTrendFollowingBTA
from SellTrendFollowingStrategy.BacktestAdapter import BacktestingAdapter as SellTrendFollowingBTA
from Helpers import load_data
from backtesting import Backtest



data = load_data(symbol='NDX', period='2y', interval='1min')

print(data.head())
print(data.tail(100))


strategy = BuyTrendFollowingBTA
bt = Backtest(data, strategy, cash=100000, commission=.00, exclusive_orders=True)
stats = bt.run()
print(stats)
bt.plot()


# from BuyTrendFollowingStrategy.BuyTrendFollowingStrategy import BuyTrendFollowingStrategy

# testStrategy = BuyTrendFollowingStrategy()

# candle = {
#     'date': '2025-04-11 14:29:00',
#     'Open':  1.0,
#     'High':  1.2,
#     'Low':   0.8,
#     'Close': 1.1,
#     'Volume': 1000,

#     'ema_50': 1.05,
#     'ema_200': 1.00,
#     'st_50_3': 1.02,
#     'stoch_k': 60,
#     'stoch_d': 55,
#     'atr': 0.02,
# }

# signal = testStrategy.update_candle(candle)

# print(signal)