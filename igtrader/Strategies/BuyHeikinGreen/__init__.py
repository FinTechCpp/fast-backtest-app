from ..Strategy import Strategy, StrategyBaseConfig
from igtrader.backtestingpy.backtesting.backtesting import Strategy as BacktestingStrategy
import logging
import talib
import numpy as np
from dataclasses import dataclass

@dataclass
class BuyHeikinGreenConfig:
    """
    Paramètres de la stratégie de trading basée sur les bougies Heikin Ashi.
    """
    ema_short_period: int = 50
    ema_long_period: int = 200
    stoch_fastk: int = 10
    stoch_slowk: int = 7
    stoch_slowd: int = 3


class BuyHeikinGreen(Strategy):
    """
    Stratégie de trading basée sur les bougies Heikin Ashi.
    """
    def __init__(self, base_config: StrategyBaseConfig, buy_heikin_green_config: BuyHeikinGreenConfig):
        super().__init__(base_config)

        self.name = "BuyHeikinGreen"
        self.symbol = None
        self.exchange = None
        self.timeframe = None

        self.k_previous = None
        self.d_previous = None

        self.config = buy_heikin_green_config
        
        # TODO : Arriver a rendre cela fixe, a ne pas redefinir a chaque fois
        # Noms des indicateurs
        self.ema_short_name = f'EMA_{self.config.ema_short_period}'
        self.ema_long_name = f'EMA_{self.config.ema_long_period}'
        self.stoch_k_name = f'STOCH_K_{self.config.stoch_fastk}_{self.config.stoch_slowk}_{self.config.stoch_slowd}'
        self.stoch_d_name = f'STOCH_D_{self.config.stoch_fastk}_{self.config.stoch_slowk}_{self.config.stoch_slowd}'
    
    def should_long(self):
        # Vérifie si la bougie actuelle est verte (Close > Open)
        return self.candles["Close"] > self.candles["Open"]

    def should_short(self):
        return False
    
    def go_long(self):
        self.buy = 0.5, self.price
        self.take_profit = 0.5, self.base_config.take_profit_distance
        self.stop_loss = 0.5, self.base_config.stop_loss_distance
    
    def go_short(self):
        raise NotImplementedError(f"La stratégie {self.name} ne supporte pas la vente à découvert.")

    def ema_filter(self):
        # Vérifie si le prix est au-dessus des EMA
        return self.price > self.candles[self.ema_short_name] and self.price > self.candles[self.ema_long_name]
    
    def stoch_inf_50_filter(self):
        # Vérifie si le Stochastic %K présent et previous est inférieur à 50
        return self.candles[self.stoch_k_name] < 50 and (self.k_previous is not None and self.k_previous < 50)
    

    
    def filters(self):
        return [
            self.ema_filter,
            self.stoch_inf_50_filter,
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
            # Pas suffisamment de données : on affecte par défaut la valeur du Close de la bougie actuelle.
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

        if self.stoch_k_name not in candle or self.stoch_d_name not in candle:
            k, d = talib.STOCH(
                df['High'].values, df['Low'].values, df['Close'].values,
                fastk_period=self.config.stoch_fastk, slowk_period=self.config.stoch_slowk, slowd_period=self.config.stoch_slowd)
            candle[self.stoch_k_name] = float(k[-1])
            candle[self.stoch_d_name] = float(d[-1])
                    
        return candle



class BuyHeikinGreenBA(BacktestingStrategy):
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
        buy_trend_config = BuyHeikinGreenConfig(
            ema_short_period     = kwargs.pop('ema_short_period'),
            ema_long_period      = kwargs.pop('ema_long_period'),
            stoch_fastk          = kwargs.pop('stoch_fastk'),
            stoch_slowk          = kwargs.pop('stoch_slowk'),
            stoch_slowd          = kwargs.pop('stoch_slowd'),
        )

        # 3) stocker et instancier la stratégie “métier”
        self.base_config = base_config
        self.config      = buy_trend_config
        self.my_strategy = BuyHeikinGreen(base_config, buy_trend_config)

            
    
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
        
        
        # Mettre à jour les valeurs précédentes du Stochastique
        self.my_strategy.k_previous = candle[self.stoch_k_name]
        self.my_strategy.d_previous = candle[self.stoch_d_name]
        
        # TRÈS IMPORTANT: Mettre à jour la bougie AVANT d'appeler les filtres
        # car cela initialise self.candles dans la stratégie
        signal = self.my_strategy.update_candle(candle)
        
        # Vérifier les filtres
        ema_filter = self.my_strategy.ema_filter()
        stoch_filter = self.my_strategy.stoch_inf_50_filter()
        should_long = self.my_strategy.should_long()
        
        # Maintenant que self.candles a été mis à jour, on peut déboguer
        if self.data.index[-1].day % 2 == 0 and self.data.index[-1].hour == 10:  # Limiter la quantité de logs
            logging.debug(f"Candle: {candle['date']}")
            logging.debug(f"EMA Short ({self.ema_short_name}): {candle[self.ema_short_name]}")
            logging.debug(f"EMA Long ({self.ema_long_name}): {candle[self.ema_long_name]}")
            logging.debug(f"Stoch K ({self.stoch_k_name}): {candle[self.stoch_k_name]}")
            logging.debug(f"Stoch D ({self.stoch_d_name}): {candle[self.stoch_d_name]}")
            logging.debug(f"Filtres - EMA: {ema_filter}, Stoch<50: {stoch_filter}")
            logging.debug(f"Should Long: {should_long}")
    
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