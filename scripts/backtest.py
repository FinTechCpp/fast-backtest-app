import argparse
import logging
from datetime import time

import pandas as pd

from igtrader.Strategies.Helpers import load_data
from igtrader.backtestingpy.backtesting.backtesting import Backtest
from igtrader.Strategies.BuyTrendFollowing import BuyTrendFollowingBA
from igtrader.Strategies.SellTrendFollowing import SellTrendFollowingBA
from igtrader.Strategies.CrossEMA import CrossEMABA


# Map strategy names to classes
STRATEGIES = {
    'BuyTrendFollowingBA': BuyTrendFollowingBA,
    'SellTrendFollowingBA': SellTrendFollowingBA,
    'CrossEMABA': CrossEMABA,
}


def parse_time(tstr: str) -> time:
    """Parse a string HH:MM into datetime.time"""
    h, m = map(int, tstr.split(':'))
    return time(h, m)


def parse_int_list(s: str):
    return [int(item) for item in s.split(',') if item.strip()]


def main():
    parser = argparse.ArgumentParser(description="Backtest a trading strategy via CLI.")
    # Data args
    parser.add_argument('--symbol', type=str, default='NDX', help='Symbol to trade')
    parser.add_argument('--period', type=str, default='1d', help='Data period')
    parser.add_argument('--interval', type=str, default='20secs', help='Data interval')
    parser.add_argument('--end-date', type=str, default=None, help='End date for data (e.g. "2025-04-23")')
    parser.add_argument('--timezone', type=str, default='Europe/Paris', help='Timezone for data')
    parser.add_argument('--spread', type=float, default=0.0, help='Spread for backtesting')
    parser.add_argument('--cash', type=float, default=100000.0, help='Initial cash for backtesting')
    parser.add_argument('--commission', type=float, default=0.0, help='Commission rate')
    # Strategy selection
    parser.add_argument('--strategy', type=str, default='BuyTrendFollowingBA', choices=STRATEGIES.keys(), help='Strategy to use')
    # Base config
    parser.add_argument('--trading-from', type=parse_time, default='14:30', help='Trading start time HH:MM')
    parser.add_argument('--trading-to', type=parse_time, default='21:00', help='Trading end time HH:MM')
    parser.add_argument('--trading-days', type=parse_int_list, default='0,1,2,3,4', help='Comma-separated trading weekdays (0=Mon)')
    parser.add_argument('--take-profit-distance', type=float, default=30.0)
    parser.add_argument('--stop-loss-distance', type=float, default=20.0)
    # BuyTrendConfig
    parser.add_argument('--ema-short-period', type=int, default=50)
    parser.add_argument('--ema-long-period', type=int, default=200)
    parser.add_argument('--stoch-fastk', type=int, default=10)
    parser.add_argument('--stoch-slowk', type=int, default=7)
    parser.add_argument('--stoch-slowd', type=int, default=3)

    args = parser.parse_args()
    logging.basicConfig(level=logging.INFO)

    # Load data
    data = load_data(symbol=args.symbol,
                     period=args.period,
                     interval=args.interval,
                     end_date=args.end_date,
                     timezone=args.timezone)
    logging.info(f"Loaded {len(data)} rows for {args.symbol}")

    strategy_kwargs = {
        # base
        'trading_from': args.trading_from,
        'trading_to':   args.trading_to,
        'trading_days': args.trading_days,
        'take_profit_distance': args.take_profit_distance,
        'stop_loss_distance':   args.stop_loss_distance,
        # spécifique
        'ema_short_period': args.ema_short_period,
        'ema_long_period':  args.ema_long_period,
        'stoch_fastk': args.stoch_fastk,
        'stoch_slowk': args.stoch_slowk,
        'stoch_slowd': args.stoch_slowd,
    }

    # Select strategy class
    strategy_cls = STRATEGIES[args.strategy]
    # Run backtest
    bt = Backtest(
        data,
        strategy_cls,
        cash=args.cash,
        commission=args.commission,
        spread=args.spread,
        exclusive_orders=True,
        strategy_kwargs=strategy_kwargs
    )
    stats = bt.run()
    print(stats)


if __name__ == '__main__':
    main()


# python ./scripts/backtest.py --symbol NDX --period 1d --interval 20secs --trading-from 19:00 --trading-to 21:00 --trading-days 0,1,2,3,4 --take-profit-distance 30 --stop-loss-distance 20 --ema-short-period 50 --ema-long-period 200 --stoch-fastk 10 --stoch-slowk 7 --stoch-slowd 3 --strategy BuyTrendFollowingBA --cash 100000 --spread 0.0