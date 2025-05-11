import pandas as pd
import logging
import datetime
from igtrader.backtestingpy.backtesting.backtesting import Strategy as BacktestingStrategy
from cpp_strategies import (
    CppStrategyBaseConfig, 
    CppSellHeikinRedConfig, 
    CppSellHeikinRed, 
    CppCandle,
    CppSignal
)

class SellHeikinRedBA(BacktestingStrategy):
    """
    Adapter for the backtesting strategy using the C++ implementation of SellHeikinRed.
    This is a mirror (sell) version of the BuyHeikinGreen strategy.
    """
    def init(self, **kwargs):
        # 1) Create and configure the C++ base config object
        cpp_base_config = CppStrategyBaseConfig()
        
        # Convert datetime.time objects to hour/minute values
        trading_from = kwargs.pop('trading_from')
        trading_to = kwargs.pop('trading_to')
        if hasattr(trading_from, 'hour') and callable(trading_from.hour):
            # QTime objects
            cpp_base_config.trading_from_hour = trading_from.hour()
            cpp_base_config.trading_from_minute = trading_from.minute()
            cpp_base_config.trading_to_hour = trading_to.hour()
            cpp_base_config.trading_to_minute = trading_to.minute()
        else:
            # datetime.time objects
            cpp_base_config.trading_from_hour = trading_from.hour
            cpp_base_config.trading_from_minute = trading_from.minute
            cpp_base_config.trading_to_hour = trading_to.hour
            cpp_base_config.trading_to_minute = trading_to.minute
        
        # Set trading days
        cpp_base_config.trading_days = kwargs.pop('trading_days')
        
        # Set distance parameters
        cpp_base_config.take_profit_distance = float(kwargs.pop('take_profit_distance'))
        cpp_base_config.stop_loss_distance = float(kwargs.pop('stop_loss_distance'))
        
        # ATR parameters - ensure integers for period values
        cpp_base_config.use_atr_for_sl_tp = bool(kwargs.pop('use_atr_for_sl_tp', False))
        cpp_base_config.atr_period = int(kwargs.pop('atr_period', 14))
        cpp_base_config.stop_loss_atr_multiplier = float(kwargs.pop('stop_loss_atr_multiplier', 2.0))
        cpp_base_config.take_profit_atr_multiplier = float(kwargs.pop('take_profit_atr_multiplier', 3.0))
        cpp_base_config.min_stop_loss_distance = float(kwargs.pop('min_stop_loss_distance', 5.0))
        cpp_base_config.min_take_profit_distance = float(kwargs.pop('min_take_profit_distance', 5.0))
        
        # Risk management parameters
        cpp_base_config.use_risk_based_sizing = bool(kwargs.pop('use_risk_based_sizing', False))
        cpp_base_config.risk_percentage = float(kwargs.pop('risk_percentage', 1.0))
        cpp_base_config.cash = float(kwargs.pop('cash', 100000.0))
        cpp_base_config.max_position_percentage = float(kwargs.pop('max_position_percentage', 100.0))
        cpp_base_config.leverage_limit = float(kwargs.pop('leverage_limit', 20.0))
        
        # Break-even parameters
        cpp_base_config.use_break_even = bool(kwargs.pop('use_break_even', True))
        cpp_base_config.break_even_threshold = float(kwargs.pop('break_even_threshold', 0.7))
        
        # 2) Create and configure the C++ strategy config
        cpp_strategy_config = CppSellHeikinRedConfig()
        
        # Set period parameters
        cpp_strategy_config.ema_short_period = int(kwargs.pop('ema_short_period'))
        cpp_strategy_config.ema_long_period = int(kwargs.pop('ema_long_period'))
        cpp_strategy_config.stoch_fastk = int(kwargs.pop('stoch_fastk'))
        cpp_strategy_config.stoch_slowk = int(kwargs.pop('stoch_slowk'))
        cpp_strategy_config.stoch_slowd = int(kwargs.pop('stoch_slowd'))
        cpp_strategy_config.stoch_threshold = int(kwargs.pop('stoch_threshold', 80))  # Différent de BuyHeikinGreen
        
        # Filter settings
        cpp_strategy_config.use_ema_short_filter = bool(kwargs.pop('use_ema_short_filter', True))
        cpp_strategy_config.use_ema_long_filter = bool(kwargs.pop('use_ema_long_filter', True))
        cpp_strategy_config.use_stoch_filter = bool(kwargs.pop('use_stoch_filter', True))
        cpp_strategy_config.use_previous_ha_candle_green_filter = bool(kwargs.pop('use_previous_ha_candle_green_filter', True))
        
        # 3) Instantiate the C++ strategy with the configurations
        self.cpp_strategy = CppSellHeikinRed(cpp_base_config, cpp_strategy_config)
        
        # Store configs for reference
        self.base_config = cpp_base_config
        self.strategy_config = cpp_strategy_config
        
        logging.info("C++ SellHeikinRed strategy initialized")
    
    def next(self):
        """
        Method called for each candle during backtest.
        """
        # Create a C++ candle object with current data
        cpp_candle = CppCandle()
        
        # Format date as ISO string
        if isinstance(self.data.index[-1], pd.Timestamp):
            cpp_candle.date = self.data.index[-1].strftime('%Y-%m-%dT%H:%M:%S')
        else:
            cpp_candle.date = str(self.data.index[-1])
        
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
                
        elif not self.position and cpp_signal.action == "SELL":
            # Execute sell signal
            self.sell(
                sl_points=cpp_signal.stop_loss,
                tp_points=cpp_signal.take_profit,
                size=cpp_signal.quantity
            )
            
            # Log trade details
            logging.info(
                f"\n\nOpening SELL position:\n"
                f"Candle Date: {self.data.index[-1]}\n"
                f"Price: {cpp_signal.price}\n"
                f"Size: {cpp_signal.quantity}\n"
                f"Stop Loss: {cpp_signal.stop_loss}\n"
                f"Take Profit: {cpp_signal.take_profit}\n"
            )
            
        elif not self.position and cpp_signal.action == "BUY":
            # Not implemented for this strategy
            logging.warning("BUY signal received but not implemented in this strategy")