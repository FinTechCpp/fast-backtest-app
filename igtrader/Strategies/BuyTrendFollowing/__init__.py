from ..Strategy import Strategy, StrategyBaseConfig
from igtrader.backtestingpy.backtesting.backtesting import Strategy as BacktestingStrategy
import logging
import numpy as np
from datetime import time
import talib
from dataclasses import dataclass

@dataclass
class BuyTrendConfig:
    """
    Paramètres de la stratégie de suivi de tendance à l'achat.
    """
    ema_short_period: int = 50
    ema_long_period: int = 200
    stoch_fastk: int = 10
    stoch_slowk: int = 7
    stoch_slowd: int = 3



class BuyTrendFollowing(Strategy):
    """
    Stratégie de suivi de tendance à l'achat.
    """
    def __init__(self, base_config: StrategyBaseConfig, buy_trend_config: BuyTrendConfig):
        super().__init__(base_config)

        self.name = "BuyTrendFollowingStrategy"
        self.symbol = None
        self.exchange = None
        self.timeframe = None

        self.k_previous = None
        self.d_previous = None

        self.config = buy_trend_config
        
        # TODO : Arriver a rendre cela fixe, a ne pas redefinir a chaque fois
        # Noms des indicateurs
        self.ema_short_name = f'EMA_{self.config.ema_short_period}'
        self.ema_long_name = f'EMA_{self.config.ema_long_period}'
        self.stoch_k_name = f'STOCH_K_{self.config.stoch_fastk}_{self.config.stoch_slowk}_{self.config.stoch_slowd}'
        self.stoch_d_name = f'STOCH_D_{self.config.stoch_fastk}_{self.config.stoch_slowk}_{self.config.stoch_slowd}'

    
    def should_long(self):
        k_current, d_current = self.candles[self.stoch_k_name], self.candles[self.stoch_d_name]

        if self.k_previous is None or self.d_previous is None:
            self.k_previous = k_current
            self.d_previous = d_current
            return False
        
        should_go_long = k_current > d_current and self.k_previous < self.d_previous

        # Mettre à jour les valeurs précédentes
        self.k_previous = k_current
        self.d_previous = d_current

        return should_go_long

    def should_short(self):
        return False
    
    def go_long(self):
        self.buy = 0.5, self.price
        self.take_profit = 0.5, self.base_config.take_profit_distance
        self.stop_loss = 0.5, self.base_config.stop_loss_distance
    
    def go_short(self):
        raise NotImplementedError("La stratégie BuyTrendFollowingStrategy ne supporte pas la vente à découvert.")

    def ema_filter(self):
        # Vérifie si le prix est au-dessus des EMA
        return self.price > self.candles[self.ema_short_name] and self.price > self.candles[self.ema_long_name]
    
    def stoch_sup_50_filter(self):
        # Vérifie si le Stochastic %K est inférieur à 50
        return self.candles[self.stoch_k_name] < 50
    
    def filters(self):
        return [
            self.ema_filter,
            self.stoch_sup_50_filter
        ]
    
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
            if self.stoch_k_name not in candle:
                candle[self.stoch_k_name] = np.nan
            if self.stoch_d_name not in candle:
                candle[self.stoch_d_name] = np.nan
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

        # Stochastique
        if self.stoch_k_name not in candle or self.stoch_d_name not in candle:
            k, d = talib.STOCH(
                df['High'].values, df['Low'].values, df['Close'].values,
                fastk_period=self.config.stoch_fastk, slowk_period=self.config.stoch_slowk, slowd_period=self.config.stoch_slowd)
            candle[self.stoch_k_name] = float(k[-1])
            candle[self.stoch_d_name] = float(d[-1])
                    
        return candle



class BuyTrendFollowingBA(BacktestingStrategy):
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
        )
        # 2) extraire les clés spécifiques
        buy_trend_config = BuyTrendConfig(
            ema_short_period     = kwargs.pop('ema_short_period'),
            ema_long_period      = kwargs.pop('ema_long_period'),
            stoch_fastk          = kwargs.pop('stoch_fastk'),
            stoch_slowk          = kwargs.pop('stoch_slowk'),
            stoch_slowd          = kwargs.pop('stoch_slowd'),
        )

        # 3) stocker et instancier la stratégie “métier”
        self.base_config = base_config
        self.config      = buy_trend_config
        self.my_strategy = BuyTrendFollowing(base_config, buy_trend_config)


    def next(self):
        """
        Méthode appelée à chaque bougie pendant le backtest.
        """
        # Noms des indicateurs
        self.ema_short_name = f'EMA_{self.config.ema_short_period}'
        self.ema_long_name = f'EMA_{self.config.ema_long_period}'
        self.stoch_k_name = f'STOCH_K_{self.config.stoch_fastk}_{self.config.stoch_slowk}_{self.config.stoch_slowd}'
        self.stoch_d_name = f'STOCH_D_{self.config.stoch_fastk}_{self.config.stoch_slowk}_{self.config.stoch_slowd}'

        candle = {
            'date': self.data.index[-1],
            'Open': self.data.Open[-1],
            'High': self.data.High[-1],
            'Low': self.data.Low[-1],
            'Close': self.data.Close[-1],

            self.ema_short_name: self.data.df[self.ema_short_name].iloc[-1],
            self.ema_long_name: self.data.df[self.ema_long_name].iloc[-1],
            self.stoch_k_name: self.data.df[self.stoch_k_name].iloc[-1],
            self.stoch_d_name: self.data.df[self.stoch_d_name].iloc[-1],
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