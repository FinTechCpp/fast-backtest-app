import pandas as pd
from lightweight_charts import Chart
import os

os.environ["PYWEBVIEW_GUI"] = "qt"
from igtrader.Strategies.BuyTrendFollowing import BuyTrendFollowingBA
from igtrader.Strategies.SellTrendFollowing import SellTrendFollowingBA
from igtrader.Strategies.Helpers import load_data
from igtrader.backtestingpy.backtesting.backtesting import Backtest
import argparse  

strategy_map = {
    'BuyTrendFollowingBA': BuyTrendFollowingBA,
    'SellTrendFollowingBA': SellTrendFollowingBA,
}

#----------------argument parser----------------
  
parser = argparse.ArgumentParser(description="Backtest a trading strategy.")
parser.add_argument('--symbol', type=str, default='NDX', help='Symbol to trade (default: NDX)')
parser.add_argument('--period', type=str, default='3d', help=f'Data period (default: 10d)')
parser.add_argument('--interval', type=str, default='20 secs', help='Data interval (default: 20 secs)')
parser.add_argument('--end_date', type=str, default='10/02/2025', help='End date for data (default: 10/02/2025)')
parser.add_argument('--timezone', type=str, default='Europe/Paris', help='Timezone for data (default: Europe/Paris)')
parser.add_argument('--spread', type=float, default=0.0002, help='Spread for backtesting (default: 0.0002)')
parser.add_argument('--cash', type=float, default=100000, help='Initial cash for backtesting (default: 100000)')
parser.add_argument('--strategy', type=str, default='BuyTrendFollowingBA', help='Strategy to use (default: BuyTrendFollowingBA)')
parser.add_argument('--save', default=False, action='store_true', help='Save the results to a CSV file')

args = parser.parse_args()

#----------------load data----------------

data = load_data(symbol=args.symbol, 
                 period=args.period, 
                 interval=args.interval, 
                 end_date=args.end_date,
                 timezone= args.timezone)

#------------------run backtest----------------
if args.strategy not in strategy_map:
    raise ValueError(f"Strategy {args.strategy} not found. Available strategies: {list(strategy_map.keys())}")

strategy = strategy_map[args.strategy]

bt = Backtest(data, strategy, cash=args.cash, commission=.00, spread=args.spread, exclusive_orders=True)
stats = bt.run()
print(stats)

if args.save:
    stats.to_frame().to_parquet(f"../backtest_results/{args.strategy}_{args.symbol}_{args.period}_{args.interval}_{args.end_date}.csv")

#print(stats['_trades'])
#print(stats['_equity_curve'].head())

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

# Sort the datag
data.sort_values('time', inplace=True)

# Create main chart with volume disabled
chart = Chart(width=1900, 
              height=800, 
              title=f"Backtest Results - {args.strategy} on {args.symbol} in {args.interval}",
              inner_height=0.7,
              toolbox=True)  # Set inner_height to leave room for subcharts
#chart.layout(background_color='#ffffff')
chart.set(data)

# Add EMAs as line series
if 'ema_20' in data.columns:
    ema_20_line = chart.create_line(name='EMA 20', color='blue', width=1, price_line=False)
    ema_20_df = data[['time', 'ema_20']].copy()
    ema_20_df.rename(columns={'ema_20': 'EMA 20'}, inplace=True)
    ema_20_line.set(ema_20_df)

if 'ema_50' in data.columns:
    ema_50_line = chart.create_line(name='EMA 50', color='purple', width=1, price_line=False)
    ema_50_df = data[['time', 'ema_50']].copy()
    ema_50_df.rename(columns={'ema_50': 'EMA 50'}, inplace=True)
    ema_50_line.set(ema_50_df)

if 'ema_200' in data.columns:
    ema_200_line = chart.create_line(name='EMA 200', color='red', width=1.5, price_line=False)
    ema_200_df = data[['time', 'ema_200']].copy()
    ema_200_df.rename(columns={'ema_200': 'EMA 200'}, inplace=True)
    ema_200_line.set(ema_200_df)
    
if 'st_50_3' in data.columns:
    st_line = chart.create_line(name='Supertrend', color='orange', width=1, price_line=False)
    st_df = data[['time', 'st_50_3']].copy()
    st_df.rename(columns={'st_50_3': 'Supertrend'}, inplace=True)
    st_line.set(st_df)

# Add stochastic indicator as a subchart
if 'stoch_k' in data.columns and 'stoch_d' in data.columns:
    # Create stochastic subchart
    stoch_chart = chart.create_subchart(height=0.15, width=1, position="bottom", sync=True)
    #stoch_chart.layout(background_color='#ffffff')
    stoch_chart.time_scale(visible=False)  # Hide time scale for stochastic chart
    # Add stochastic lines
    stoch_k_line = stoch_chart.create_line(name='Stoch K', color='blue', width=1, price_line=False)
    stoch_k_df = data[['time', 'stoch_k']].copy()
    stoch_k_df.rename(columns={'stoch_k': 'Stoch K'}, inplace=True)
    stoch_k_line.set(stoch_k_df)
    
    stoch_d_line = stoch_chart.create_line(name='Stoch D', color='red', width=1, price_line=False)
    stoch_d_df = data[['time', 'stoch_d']].copy()
    stoch_d_df.rename(columns={'stoch_d': 'Stoch D'}, inplace=True)
    stoch_d_line.set(stoch_d_df)
    
    # Add overbought/oversold levels
    stoch_chart.horizontal_line(price=80, color='green', width=1, style='dashed', text='Overbought')
    stoch_chart.horizontal_line(price=20, color='red', width=1, style='dashed', text='Oversold')

# Add ATR indicator as a subchart
if 'atr' in data.columns:
    # Create ATR subchart
    atr_chart = chart.create_subchart(height=0.15, width=1, position="bottom", sync=True)
    #atr_chart.layout(background_color='#f7f7f7')  # Slightly darker gray/white
    atr_chart.time_scale(visible=False)  # Hide time scale for ATR chart
    # Add ATR line
    atr_line = atr_chart.create_line(name='ATR', color='green', width=1, price_line=False)
    atr_df = data[['time', 'atr']].copy()
    atr_df.rename(columns={'atr': 'ATR'}, inplace=True)
    atr_line.set(atr_df)

# Add trade markers (keep existing code)
trades = stats['_trades']
print(trades)
for i, trade in trades.iterrows():
    # Convert entry/exit times to timestamps
    entry_time = pd.to_datetime(trade['EntryTime'])
    exit_time = pd.to_datetime(trade['ExitTime'])
    entry_price = trade['EntryPrice']
    exit_price = trade['ExitPrice']
    
    # Debug the exact times to compare with chart data
    print(f"Trade {i}: Entry time: {entry_time}, Exit time: {exit_time}, Entry price: {entry_price}, Exit price: {exit_price}")
    
    entry_color = "green" if trade['Size'] > 0 else "red"
    exit_color = "green" if trade['PnL'] > 0 else "red"
    
    # Add markers to the main chart
    chart.marker(
        time=entry_time,
        position="below",
        color=entry_color,
        text=f"Entry: {entry_price:.2f}"
    )
    
    chart.marker(
        time=exit_time,
        position="below",
        color=exit_color,
        text=f"Exit: {exit_price:.2f} (P/L: {trade['PnL']:.2f})"
    )

# Show the chart (this will display all subcharts)
chart.show(block=True)