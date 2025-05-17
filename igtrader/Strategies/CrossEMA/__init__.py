from ..Strategy import Strategy, StrategyBaseConfig
from igtrader.backtestingpy.backtesting.backtesting import Strategy as BacktestingStrategy
import logging
import numpy as np
from datetime import time
from dataclasses import dataclass

@dataclass
class CrossEMAConfig:
    ema_short_period: int = 50
    ema_long_period: int = 200


class CrossEMA(Strategy):
    def __init__(self, base_config: StrategyBaseConfig, buy_trend_config: CrossEMAConfig):
        super().__init__(base_config)

        self.name = "TemplateStrategy"

        self.previous_ema_short_l = None
        self.previous_ema_long_l = None

        self.previous_ema_short_s = None
        self.previous_ema_long_s = None

        self.config = buy_trend_config
        
        # TODO : Arriver a rendre cela fixe, a ne pas redefinir a chaque fois
        # Noms des indicateurs
        self.ema_short_name = f'EMA_{self.config.ema_short_period}'
        self.ema_long_name = f'EMA_{self.config.ema_long_period}'

    
    def should_long(self):
        """
        Détermine si la stratégie doit entrer en position longue.
        """
        current_ema_short = self.candles[self.ema_short_name]
        current_ema_long = self.candles[self.ema_long_name]

        if self.previous_ema_short_l is None or self.previous_ema_long_l is None:
            self.previous_ema_short_l = current_ema_short
            self.previous_ema_long_l = current_ema_long
            return False
        
        should_go_long = current_ema_short > current_ema_long and self.previous_ema_short_l < self.previous_ema_long_l
        self.previous_ema_short_l = current_ema_short
        self.previous_ema_long_l = current_ema_long

        return should_go_long

    def should_short(self):
        """
        Détermine si la stratégie doit entrer en position courte.
        """
        current_ema_short = self.candles[self.ema_short_name]
        current_ema_long = self.candles[self.ema_long_name]

        if self.previous_ema_short_s is None or self.previous_ema_long_s is None:
            self.previous_ema_short_s = current_ema_short
            self.previous_ema_long_s = current_ema_long
            return False
        
        should_go_short = current_ema_short < current_ema_long and self.previous_ema_short_s > self.previous_ema_long_s
        self.previous_ema_short_s = current_ema_short
        self.previous_ema_long_s = current_ema_long

        return should_go_short
    
    def go_long(self):
        self.buy = 1, self.price
        self.take_profit = 1, self.base_config.take_profit_distance
        self.stop_loss = 1, self.base_config.stop_loss_distance
    
    def go_short(self):
        self.sell = 1, self.price
        self.take_profit = 1, self.base_config.take_profit_distance
        self.stop_loss = 1, self.base_config.stop_loss_distance
    
    def filters(self):
        return []
    
    def add_missing_indicators(self, candle: dict) -> dict:
        """
        Ajoute les indicateurs manquants à la bougie 'candle'.
        Si un indicateur est présent dans 'candle', il n'est pas recalcule.
        Sinon, on le calcule à partir des données du buffer.
        """
        # On travaille sur une copie du buffer pour être sûr que seules les colonnes nécessaires soient présentes.
        df = self.buffer.copy()
        if df.empty:
            # Pas suffisamment de données : on affecte np.nan
            if self.ema_short_name not in candle:
                candle[self.ema_short_name] = np.nan
            if self.ema_long_name not in candle:
                candle[self.ema_long_name] = np.nan
            return candle

        # Garder uniquement les colonnes nécessaires
        df = df[['Open', 'High', 'Low', 'Close']]
        
        # EMA
        if self.ema_long_name not in candle:
            ema_val = talib.EMA(df['Close'].values, timeperiod=self.config.ema_long_period)
            candle[self.ema_long_name] = float(ema_val[-1])
                
        if self.ema_short_name not in candle:
            ema_val = talib.EMA(df['Close'].values, timeperiod=self.config.ema_short_period)
            candle[self.ema_short_name] = float(ema_val[-1])
                    
        return candle



class CrossEMABA(BacktestingStrategy):
    """
    Adapter pour la stratégie de backtesting.
    """
    def init(self, **kwargs):

        # 1) extraire les clés génériques
        base_config = StrategyBaseConfig(
            trading_from    = kwargs.pop('trading_from'),
            trading_to      = kwargs.pop('trading_to'),
            trading_days    = kwargs.pop('trading_days'),
            take_profit_distance = kwargs.pop('take_profit_distance'),
            stop_loss_distance   = kwargs.pop('stop_loss_distance'),
            
            # Nouveaux paramètres ATR
            use_atr_for_sl_tp = kwargs.pop('use_atr_for_sl_tp', False),
            atr_period = kwargs.pop('atr_period', 14),
            stop_loss_atr_multiplier = kwargs.pop('stop_loss_atr_multiplier', 2.0),
            take_profit_atr_multiplier = kwargs.pop('take_profit_atr_multiplier', 3.0),
            min_stop_loss_distance = kwargs.pop('min_stop_loss_distance', 5.0),
            min_take_profit_distance = kwargs.pop('min_take_profit_distance', 5.0),
            
            # Paramètres de gestion du risque 
            use_risk_based_sizing = kwargs.pop('use_risk_based_sizing', False),
            risk_percentage = kwargs.pop('risk_percentage', 1.0),
            cash = kwargs.pop('cash', 100000.0),
            leverage_limit= kwargs.pop('leverage_limit', 20.0) 
        )
        
        # 2) extraire les clés spécifiques
        buy_trend_config = CrossEMAConfig(
            ema_short_period     = kwargs.pop('ema_short_period'),
            ema_long_period      = kwargs.pop('ema_long_period'),
        )

        # 3) stocker et instancier la stratégie “métier”
        self.base_config = base_config
        self.config      = buy_trend_config
        self.my_strategy = CrossEMA(base_config, buy_trend_config)


    def next(self):
        """
        Méthode appelée à chaque bougie pendant le backtest.
        """
        # Noms des indicateurs
        self.ema_short_name = f'EMA_{self.config.ema_short_period}'
        self.ema_long_name = f'EMA_{self.config.ema_long_period}'

        candle = {
            'date': self.data.index[-1],
            'Open': self.data.Open[-1],
            'High': self.data.High[-1],
            'Low': self.data.Low[-1],
            'Close': self.data.Close[-1],

            self.ema_short_name: self.data.df[self.ema_short_name].iloc[-1],
            self.ema_long_name: self.data.df[self.ema_long_name].iloc[-1],
        }

        signal = self.my_strategy.update_candle(candle)

        # Vérifier si un signal d'achat ou de vente est généré
        if signal is None:
            return

        if signal['action'] == 'LIQUIDATE':
            self.position.close()
        elif not self.position and signal['action'] == 'BUY':
            self.buy(sl=candle['Close'] - signal['stop_loss'], 
                    tp=candle['Close'] + signal['take_profit'], 
                    size=signal['quantity'])
        elif not self.position and signal['action'] == 'SELL':
            self.sell(sl=candle['Close'] + signal['stop_loss'], 
                    tp=candle['Close'] - signal['take_profit'], 
                    size=signal['quantity'])
            
