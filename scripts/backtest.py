from igtrader.Strategies.BuyTrendFollowing import BuyTrendFollowingBA
from igtrader.Strategies.SellTrendFollowing import SellTrendFollowingBA
from igtrader.Strategies.Helpers import load_data
from igtrader.backtestingpy.backtesting.backtesting import Backtest
import argparse

parser = argparse.ArgumentParser(description="Backtest a trading strategy.")
parser.add_argument('--symbol', type=str, default='NDX', help='Symbol to trade (default: NDX)')
parser.add_argument('--period', type=str, default='1m', help='Data period (default: 1m)')
parser.add_argument('--interval', type=str, default='20 secs', help='Data interval (default: 20 secs)')
parser.add_argument('--end_date', type=str, default='10/03/2023', help='End date for data (default: 10/03/2023)')
parser.add_argument('--timezone', type=str, default='Europe/Paris', help='Timezone for data (default: Europe/Paris)')
parser.add_argument('--spread', type=float, default=0.002, help='Spread for backtesting (default: 0.002)')
parser.add_argument('--cash', type=float, default=100000, help='Initial cash for backtesting (default: 100000)')
parser.add_argument('--strategy', type=str, default='BuyTrendFollowingBA', help='Strategy to use (default: BuyTrendFollowingBA)')
parser.add_argument('--save', type=bool, default=False, action='store_true', help='Save the results to a CSV file')

args = parser.parse_args()

data = load_data(symbol=args.symbol, 
                 period=args.period, 
                 interval=args.interval, 
                 end_date=args.end_date,
                 timezone= args.timezone)


strategy = args.strategy
bt = Backtest(data, strategy, cash=args.cash, commission=.00, spread=args.spread, exclusive_orders=True)

stats = bt.run()
print(stats)
if args.save:
    stats.to_frame().to_parquet(f"../backtest_results/{args.strategy}_{args.symbol}_{args.period}_{args.interval}_{args.end_date}.csv")

# need to explore optimize function
# bt.optimize()

bt.plot(plot_volume=False, resample=False)