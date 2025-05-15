from igtrader.backtestingpy.backtesting.backtesting import Strategy as BacktestingStrategy
from ..Helpers import create_base_config
import pandas as pd
import logging
import datetime
from cpp_strategies import (
    CppStrategyBaseConfig, 
    CppBuyHeikinGreenConfig, 
    CppBuyHeikinGreen, 
    CppCandle,
    CppDateTime,
    CppTime,
    CppSignal,
    set_log_callback,
    LogLevel
)

# Configuration du callback
def log_from_cpp(message, level):
    if level == LogLevel.DEBUG:
        logging.debug(f"C++: {message}")
    elif level == LogLevel.INFO:
        logging.info(f"C++: {message}")
    elif level == LogLevel.WARNING:
        logging.warning(f"C++: {message}")
    elif level == LogLevel.ERROR:
        logging.error(f"C++: {message}")

# Enregistrer le callback
set_log_callback(log_from_cpp)

class BuyHeikinGreenBA(BacktestingStrategy):
    """
    Adapter for the backtesting strategy using the C++ implementation of BuyHeikinGreen.
    """
    def init(self, **kwargs):
        # Initialisation des attributs pour suivre les trades fermés
        self._last_closed_trade_count = 0
        self._last_trade_closed = False
        self._last_trade_pnl = 0.0  

        # 1) Create and configure the C++ base config object
        cpp_base_config = create_base_config(kwargs=kwargs)
        
        # 2) Create and configure the C++ strategy config
        cpp_strategy_config = CppBuyHeikinGreenConfig()
        
        # Set period parameters
        cpp_strategy_config.ema_short_period = int(kwargs.pop('ema_short_period'))
        cpp_strategy_config.ema_long_period = int(kwargs.pop('ema_long_period'))
        cpp_strategy_config.stoch_fastk = int(kwargs.pop('stoch_fastk'))
        cpp_strategy_config.stoch_slowk = int(kwargs.pop('stoch_slowk'))
        cpp_strategy_config.stoch_slowd = int(kwargs.pop('stoch_slowd'))
        cpp_strategy_config.stoch_threshold = int(kwargs.pop('stoch_threshold'))
        cpp_strategy_config.rsi_period = int(kwargs.pop('rsi_period'))
        cpp_strategy_config.rsi_threshold = int(kwargs.pop('rsi_threshold'))
        
        # Filter settings
        cpp_strategy_config.use_ema_short_filter = bool(kwargs.pop('use_ema_short_filter', False))
        cpp_strategy_config.use_ema_long_filter = bool(kwargs.pop('use_ema_long_filter', False))
        cpp_strategy_config.use_stoch_filter = bool(kwargs.pop('use_stoch_filter', False))
        cpp_strategy_config.use_previous_ha_candle_red_filter = bool(kwargs.pop('use_previous_ha_candle_red_filter', False))
        cpp_strategy_config.use_rsi_filter = bool(kwargs.pop('use_rsi_filter', False))
        
        # 3) Instantiate the C++ strategy with the configurations
        self.cpp_strategy = CppBuyHeikinGreen(cpp_base_config, cpp_strategy_config)
        
        # Store configs for reference
        self.base_config = cpp_base_config
        self.strategy_config = cpp_strategy_config
        
        logging.info("C++ BuyHeikinGreen strategy initialized")
    
    def next(self):
        """
        Method called for each candle during backtest.
        """
        # Vérifier d'abord si une position a été fermée lors de la dernière bougie
        if self.closed_trades and len(self.closed_trades) > self._last_closed_trade_count:
            last_trade = self.closed_trades[-1]
            
            # Vérifier si le trade a été fermé à la dernière bougie
            if last_trade.exit_bar == len(self.data)-1:
                self._last_trade_closed = True
                self._last_trade_pnl = last_trade.pl  # Profit/Loss en valeur absolue
                # print(f"Trade fermé: PnL = {self._last_trade_pnl}")
                
            # Mettre à jour le compteur pour ne pas retraiter ce trade
            self._last_closed_trade_count = len(self.closed_trades)

        # Create a C++ candle object with current data
        cpp_candle = CppCandle()
        
        # Remplir la structure DateTime directement depuis le timestamp
        if isinstance(self.data.index[-1], pd.Timestamp):
            dt = self.data.index[-1]
            cpp_candle.date.year = dt.year
            cpp_candle.date.month = dt.month
            cpp_candle.date.day = dt.day
            cpp_candle.date.time.hour = dt.hour
            cpp_candle.date.time.minute = dt.minute
            cpp_candle.date.time.second = dt.second
        else:
            # Si ce n'est pas un timestamp, parser la chaîne
            try:
                date_str = str(self.data.index[-1])
                dt = datetime.datetime.strptime(date_str, "%Y-%m-%d %H:%M:%S")
                cpp_candle.date.year = dt.year
                cpp_candle.date.month = dt.month
                cpp_candle.date.day = dt.day
                cpp_candle.date.time.hour = dt.hour
                cpp_candle.date.time.minute = dt.minute
                cpp_candle.date.time.second = dt.second
            except ValueError:
                logging.warning(f"Impossible de parser la date: {date_str}")
        
        # Set OHLC values
        cpp_candle.open = float(self.data.Open[-1])
        cpp_candle.high = float(self.data.High[-1])
        cpp_candle.low = float(self.data.Low[-1])
        cpp_candle.close = float(self.data.Close[-1])
        
        # Set position information
        cpp_candle.in_position = bool(self.position)
        cpp_candle.position_pl_pct = float(self.position.pl_pct) if self.position else 0.0
        cpp_candle.entry_price = float(self.trades[-1].entry_price) if self.position and self.trades else 0.0
        cpp_candle.position_size = float(self.position.size) if self.position else 0.0
        
        # Calculer le P&L du dernier trade fermé si applicable
        cpp_candle.closed_trade_pnl = 0.0
        if self._last_trade_closed:
            cpp_candle.closed_trade_pnl = self._last_trade_pnl
            self._last_trade_closed = False
            self._last_trade_pnl = 0.0
        
        # Update strategy with new candle and get signal
        cpp_signal = self.cpp_strategy.update_candle(cpp_candle)
        
        # Process signal if one was generated
        if cpp_signal is None:
            return
            
        if cpp_signal.action == "LIQUIDATE":
            if self.position:
                self.position.close()
                logging.info("Closing position due to LIQUIDATE signal")
                
        elif cpp_signal.action == "MOVE_SL":
            # Apply break-even stop loss
            for trade in self.trades:
                trade.sl = cpp_signal.new_sl
                logging.info(f"Moving stop loss to break-even at {cpp_signal.new_sl}")
                
        elif not self.position and cpp_signal.action == "BUY":
            # Execute buy signal
            self.buy(
                sl_points=cpp_signal.stop_loss,
                tp_points=cpp_signal.take_profit,
                size=cpp_signal.quantity
            )
            
            # Log trade details
            logging.info(
                f"\n\nOpening BUY position:\n"
                f"Candle Date: {self.data.index[-1]}\n"
                f"Price: {cpp_signal.price}\n"
                f"Size: {cpp_signal.quantity}\n"
                f"Stop Loss: {cpp_signal.stop_loss}\n"
                f"Take Profit: {cpp_signal.take_profit}\n"
            )
            
        elif not self.position and cpp_signal.action == "SELL":
            # Not implemented for this strategy
            logging.warning("SELL signal received but not implemented in this strategy")