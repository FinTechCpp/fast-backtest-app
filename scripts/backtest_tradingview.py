import argparse  
import os
import logging

#----------------argument parser----------------
  
parser = argparse.ArgumentParser(description="Backtest a trading strategy.")
parser.add_argument('--symbol', type=str, default='NDX', help='Symbol to trade (default: NDX)')
parser.add_argument('--period', type=str, default='3d', help=f'Data period (default: 3d)')
parser.add_argument('--interval', type=str, default='20secs', help='Data interval (default: 20secs)')
parser.add_argument('--end_date', type=str, default='10/02/2025', help='End date for data (default: 10/02/2025)')
parser.add_argument('--timezone', type=str, default='Europe/Paris', help='Timezone for data (default: Europe/Paris)')
parser.add_argument('--spread', type=float, default=0.0002, help='Spread for backtesting (default: 0.0002)')
parser.add_argument('--cash', type=float, default=100000, help='Initial cash for backtesting (default: 100000)')
parser.add_argument('--strategy', type=str, default='BuyTrendFollowingBA', help='Strategy to use (default: BuyTrendFollowingBA)')
parser.add_argument('--indicators', type=str, default=None, help='JSON string or file path with indicators configuration')
parser.add_argument('--tp-distance', type=float, default=30, help='Take profit distance (default: 30)')
parser.add_argument('--sl-distance', type=float, default=20, help='Stop loss distance (default: 20)')
parser.add_argument('--save', default=False, action='store_true', help='Save the results to a CSV file')
parser.add_argument('--log-level', type=str, default='INFO', choices=['DEBUG', 'INFO', 'WARNING', 'ERROR', 'CRITICAL'], help='Set the logging level (default: INFO)')
args = parser.parse_args()
logging.basicConfig(level=getattr(logging, args.log_level.upper(), logging.INFO), format='%(asctime)s - %(levelname)s - %(message)s')

#------------------import modules----------------

os.environ["PYWEBVIEW_GUI"] = "qt"
import json
from lightweight_charts import Chart
import pandas as pd
import time
from igtrader.Strategies.BuyTrendFollowing import BuyTrendFollowingBA
from igtrader.Strategies.SellTrendFollowing import SellTrendFollowingBA
from igtrader.Strategies.Helpers import load_data
from igtrader.backtestingpy.backtesting.backtesting import Backtest

strategy_map = {
    'BuyTrendFollowingBA': BuyTrendFollowingBA,
    'SellTrendFollowingBA': SellTrendFollowingBA,
}

#----------------process indicators config----------------
# Default indicators configuration
default_indicators = {
    'EMA': [[10], [50], [200]],
    'ATR': [[14]],
    'STOCH': [[10, 10, 3]],
    'SUPERTREND': [[200, 50]]
}

indicators = default_indicators

# Parse indicators from command line or file
if args.indicators:
    # Check if it's a file path
    if os.path.exists(args.indicators):
        try:
            with open(args.indicators, 'r') as f:
                indicators = json.load(f)
            logging.info(f"Loaded indicators configuration from file: {args.indicators}")
        except Exception as e:
            logging.error(f"Failed to load indicators from file: {e}")
            logging.info("Using default indicators configuration")
    else:
        # Try to parse as JSON string
        try:
            indicators = json.loads(args.indicators)
            logging.info("Parsed indicators configuration from command line")
        except json.JSONDecodeError:
            logging.error("Failed to parse indicators JSON. Using default configuration.")
            logging.info("Example format: '{\"EMA\":[[20],[50],[200]],\"ATR\":[[14]],\"STOCH\":[[10,7,3]],\"SUPERTREND\":[[50,3]]}'")

# Log the indicators being used
logging.debug(f"Using indicators: {json.dumps(indicators, indent=2)}")


#----------------load data----------------
data = load_data(symbol=args.symbol, 
                 period=args.period, 
                 interval=args.interval, 
                 end_date=args.end_date,
                 timezone=args.timezone,
                 indicators=indicators)

#------------------run backtest----------------
if args.strategy not in strategy_map:
    raise ValueError(f"Strategy {args.strategy} not found. Available strategies: {list(strategy_map.keys())}")

# Extraire les noms des indicateurs à partir de la configuration
strategy_kwargs = {}

# Ajouter les paramètres de prise de profit et stop loss si spécifiés
if args.tp_distance:
    strategy_kwargs["take_profit_distance"] = args.tp_distance
    
if args.sl_distance:
    strategy_kwargs["stop_loss_distance"] = args.sl_distance
logging.info(f"Paramètres de stratégie: {strategy_kwargs}")

# Configurer les indicateurs attendus par la stratégie
if "EMA" in indicators:
    # Trouver l'EMA court, moyen et long
    ema_periods = sorted([params[0] for params in indicators["EMA"]])
    if len(ema_periods) >= 2:
        strategy_kwargs["ema_short_name"] = f"EMA_{ema_periods[-2]}"  # Avant-dernier (moyen)
        strategy_kwargs["ema_long_name"] = f"EMA_{ema_periods[-1]}"   # Dernier (long)

if "SUPERTREND" in indicators:
    # Utiliser le premier SuperTrend de la configuration
    atr_period, multiplier = indicators["SUPERTREND"][0]
    strategy_kwargs["supertrend_name"] = f"SUPERTREND_{atr_period}_{multiplier}"

if "STOCH" in indicators:
    # Utiliser le premier Stochastic de la configuration
    fastk, slowk, slowd = indicators["STOCH"][0]
    strategy_kwargs["stoch_k_name"] = f"STOCH_K_{fastk}_{slowk}_{slowd}"
    strategy_kwargs["stoch_d_name"] = f"STOCH_D_{fastk}_{slowk}_{slowd}"

logging.info(f"Utilisant les indicateurs pour la stratégie: {strategy_kwargs}")

strategy = strategy_map[args.strategy]
chrono_backtest = time.time()
bt = Backtest(data, strategy, cash=args.cash, commission=.00, spread=args.spread,
              exclusive_orders=True, strategy_kwargs=strategy_kwargs)
stats = bt.run()
chrono_backtest = time.time() - chrono_backtest
logging.info(f"Backtest completed in {chrono_backtest:.2f} seconds.")
logging.info(stats)

if args.save:
    stats.to_frame().to_parquet(f"../backtest_results/{args.strategy}_{args.symbol}_{args.period}_{args.interval}_{args.end_date}.parquet")

logging.debug(stats['_trades'])
logging.debug(stats['_equity_curve'].head())

#--------------tradingview chart----------------
# Reset the index and prepare for lightweight_charts
data = data.reset_index()

# Rename the datetime column to 'time' which lightweight_charts expects
if 'index' in data.columns:
    data.rename(columns={'index': 'time'}, inplace=True)
elif 'date' in data.columns:
    data.rename(columns={'date': 'time'}, inplace=True)

# Ensure OHLC columns are properly named (lowercase)
column_map = {
    'Open': 'open',
    'High': 'high',
    'Low': 'low',
    'Close': 'close',
    'Volume': 'volume'
}
data.rename(columns=column_map, inplace=True)

# Now convert the time column
data['time'] = pd.to_datetime(data['time'])
if data['time'].dt.tz is None:
    data['time'] = data['time'].dt.tz_localize(args.timezone)
data['time'] = data['time'].dt.tz_convert(args.timezone)

# Sort the data
data.sort_values('time', inplace=True)

# Create main chart with volume disabled
chart = Chart(width=1900, 
              height=800, 
              title=f"Backtest Results - {args.strategy} on {args.symbol} in {args.interval}",
              inner_height=0.7,
              toolbox=True)  # Set inner_height to leave room for subcharts
chart.set(data)

# Indicator color configurations
indicator_colors = {
    'EMA': ['blue', 'purple', 'red', 'green', 'cyan', 'magenta'],
    'SUPERTREND': ['orange', 'brown', 'gold'],
    'STOCH_K': ['blue'],
    'STOCH_D': ['red'],
    'ATR': ['green', 'teal']
}

# Ensure indicator_columns are properly retrieved
indicator_columns = data.attrs.get('indicator_columns', {})

# Process EMAs
if 'EMA' in indicator_columns:
    for i, ema_col in enumerate(indicator_columns['EMA']):
        if ema_col in data.columns:
            color_idx = i % len(indicator_colors['EMA'])
            ema_line = chart.create_line(name=ema_col, 
                                         color=indicator_colors['EMA'][color_idx], 
                                         width=1, 
                                         price_line=False)
            ema_df = data[['time', ema_col]].copy()
            ema_line.set(ema_df)
            logging.debug(f"Added EMA indicator: {ema_col}")

# Process SuperTrend
if 'SUPERTREND' in indicator_columns:
    for i, st_col in enumerate(indicator_columns['SUPERTREND']):
        if st_col in data.columns:
            color_idx = i % len(indicator_colors['SUPERTREND'])
            st_line = chart.create_line(name=st_col, 
                                        color=indicator_colors['SUPERTREND'][color_idx], 
                                        width=1, 
                                        price_line=False)
            st_df = data[['time', st_col]].copy()
            st_line.set(st_df)
            logging.debug(f"Added SuperTrend indicator: {st_col}")

# Process Stochastic as a subchart
if 'STOCH' in indicator_columns:
    stoch_chart = chart.create_subchart(height=0.15, width=1, position="bottom", sync=True)
    for i, stoch_col in enumerate(indicator_columns['STOCH']):
        if stoch_col in data.columns:
            color = indicator_colors['STOCH_K'][0] if 'K' in stoch_col else indicator_colors['STOCH_D'][0]
            stoch_line = stoch_chart.create_line(name=stoch_col, color=color, width=1, price_line=False)
            stoch_df = data[['time', stoch_col]].copy()
            stoch_line.set(stoch_df)
            logging.debug(f"Added Stochastic indicator: {stoch_col}")
    stoch_chart.horizontal_line(price=80, color='green', width=1, style='dashed', text='Overbought')
    stoch_chart.horizontal_line(price=20, color='red', width=1, style='dashed', text='Oversold')

# Process ATR as a subchart
if 'ATR' in indicator_columns:
    atr_chart = chart.create_subchart(height=0.15, width=1, position="bottom", sync=True)
    for i, atr_col in enumerate(indicator_columns['ATR']):
        if atr_col in data.columns:
            color_idx = i % len(indicator_colors['ATR'])
            atr_line = atr_chart.create_line(name=atr_col, 
                                             color=indicator_colors['ATR'][color_idx], 
                                             width=1, 
                                             price_line=False)
            atr_df = data[['time', atr_col]].copy()
            atr_line.set(atr_df)
            logging.debug(f"Added ATR indicator: {atr_col}")

# Add trade markers
trades = stats['_trades']
for i, trade in trades.iterrows():
    entry_time = pd.to_datetime(trade['EntryTime'])
    exit_time = pd.to_datetime(trade['ExitTime'])
    entry_price = trade['EntryPrice']
    exit_price = trade['ExitPrice']
    entry_color = "yellow" if trade['Size'] > 0 else "red"
    exit_color = "green" if trade['PnL'] > 0 else "red"
    chart.marker(time=entry_time, position="above", color=entry_color, text=f"Entry: {entry_price:.2f}")
    chart.marker(time=exit_time, position="below", color=exit_color, text=f"Exit: {exit_price:.2f} (P/L: {trade['PnL']:.2f})")

# Show the chart
chart.show(block=True)
