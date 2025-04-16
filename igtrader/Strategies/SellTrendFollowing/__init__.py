from ..Helpers import calculate_supertrend
from ..Strategy import Strategy
from igtrader.backtestingpy.backtesting.backtesting import Strategy as BacktestingStrategy

import numpy as np
from datetime import time
import talib


class SellTrendFollowing(Strategy):
    """
    Stratégie de suivi de tendance à l'achat.
    """
    def __init__(self, **kwargs):
        super().__init__(**kwargs)

        self.name = "SellTrendFollowingStrategy"
        self.symbol = None
        self.exchange = None
        self.timeframe = None

        self.k_previous = None
        self.d_previous = None

        self.tp_distance = kwargs.get('take_profit_distance', 30)
        self.sl_distance = kwargs.get('stop_loss_distance', 20)
    
    def should_long(self):
        return False

    def should_short(self):
        k_current, d_current = self.candles['stoch_k'], self.candles['stoch_d']

        if self.k_previous is None or self.d_previous is None:
            self.k_previous = k_current
            self.d_previous = d_current
            return False
        
        should_go_long = k_current < d_current and self.k_previous > self.d_previous

        # Mettre à jour les valeurs précédentes
        self.k_previous = k_current
        self.d_previous = d_current

        return should_go_long
    
    def go_long(self):
        raise NotImplementedError("La stratégie SellTrendFollowingStrategy ne supporte pas l'achat.")

    def go_short(self):
        self.sell = 0.5, self.price
        self.take_profit = 0.5, self.tp_distance
        self.stop_loss = 0.5, self.sl_distance
    
    def ema_filter(self):
        return self.price < self.candles['ema_50'] and self.price < self.candles['ema_200']
        
    def supertrend_filter(self):
        return self.price < self.candles['st_50_3']
    
    def stoch_sup_50_filter(self):
        return self.candles['stoch_k'] > 50
    
    def filters(self):
        return [
            self.ema_filter,
            self.supertrend_filter,
            self.stoch_sup_50_filter
        ]


    def add_missing_indicators(self, candle: dict) -> dict:
        """
        Ajoute les indicateurs manquants à la bougie 'candle'.
        Si un indicateur est présent dans 'candle', il n'est pas recalcule.
        Sinon, on le calcule à partir des données du buffer.
        On se base sur le buffer (self.buffer) qui contient uniquement les colonnes 
        'Open', 'High', 'Low', 'Close'.
        """
        # On travaille sur une copie du buffer pour être sûr que seules les colonnes nécessaires soient présentes.
        df = self.buffer.copy()
        if df.empty:
            # Pas suffisamment de données : on affecte par défaut la valeur du Close de la bougie actuelle.
            if "ema_50" not in candle:
                candle["ema_50"] = candle["Close"]
            if "ema_200" not in candle:
                candle["ema_200"] = candle["Close"]
            if "stoch_k" not in candle:
                candle["stoch_k"] = 0.0
            if "stoch_d" not in candle:
                candle["stoch_d"] = 0.0
            if "st_50_3" not in candle:
                candle["st_50_3"] = candle["Close"]
            return candle

        # Garder uniquement les colonnes nécessaires
        df = df[['Open', 'High', 'Low', 'Close']]

        # Calculer EMA 200 si absent
        if "ema_200" not in candle:
            if len(df) >= 200:
                ema_200 = talib.EMA(df['Close'].values, timeperiod=200)
                candle["ema_200"] = float(ema_200[-1])
            else:
                candle["ema_200"] = candle["Close"]
        
        # Calculer EMA 50 si absent
        if "ema_50" not in candle:
            if len(df) >= 50:
                ema_50 = talib.EMA(df['Close'].values, timeperiod=50)
                candle["ema_50"] = float(ema_50[-1])
            else:
                candle["ema_50"] = candle["Close"]

        # Calculer les stochastiques si absents
        if ("stoch_k" not in candle) or ("stoch_d" not in candle):
            if len(df) >= 14:
                # Calcul des stochastiques
                stoch_k, stoch_d = talib.STOCH(
                    df['High'].values, df['Low'].values, df['Close'].values,
                    fastk_period=10, slowk_period=7, slowd_period=3)
                candle["stoch_k"] = float(stoch_k[-1])
                candle["stoch_d"] = float(stoch_d[-1])
            else:
                candle["stoch_k"] = 0.0
                candle["stoch_d"] = 0.0

        # Calculer le SuperTrend (st_50_3) si absent
        if "st_50_3" not in candle:
            # On vérifie que le buffer contient bien les colonnes nécessaires
            if len(df) >= 1:
                # Créer un DataFrame minimal pour le calcul : seules les colonnes High, Low et Close sont requises.
                df_st = df[['High', 'Low', 'Close']].copy()
                # Calcul du SuperTrend via votre fonction, avec une ATR calculée en interne (atr_period=14)
                st_df = calculate_supertrend(df_st, atr_period=50, multiplier=3)
                candle["st_50_3"] = float(st_df['SuperTrend'].iloc[-1])
            else:
                candle["st_50_3"] = candle["Close"]
                
        return candle
    


class SellTrendFollowingBA(BacktestingStrategy):
    """
    Adapter pour la stratégie de backtesting.
    """
    def init(self, **kwargs):
        self.my_strategy = SellTrendFollowing(**kwargs)

        def stochastic():
            k = self.data.df['stoch_k']
            d = self.data.df['stoch_d']
            level_80 = np.full(len(k), 80)
            level_20 = np.full(len(k), 20)
            return k, d, level_80, level_20

        self.ema_50 = self.I(lambda: self.data.df['ema_50'], name='EMA 50', overlay=True, color='red')
        self.ema_200 = self.I(lambda: self.data.df['ema_200'], name='EMA 200', overlay=True, color='blue')
        self.st_50_3 = self.I(lambda: self.data.df['st_50_3'], name='SuperTrend 50,3', overlay=True, color='purple')
        self.stoch = self.I(stochastic, 
                            name=['Stochastic %K', 'Stochastic %D', 'Surachat (80)', 'Survente (20)'],
                            overlay=False,
                            color=['green', 'orange', 'green', 'red'])
    
    def next(self):
        """
        Méthode appelée à chaque bougie pendant le backtest.
        On construit une nouvelle bougie sous forme de dictionnaire, on la transmet à 
        la stratégie custom via update_candle(), et on récupère le signal généré.
        
        Si un signal d'achat ou de vente est généré (et aucune position n'est déjà ouverte),
        on passe l'ordre correspondant.
        """
        candle = {
            'date': self.data.index[-1],
            'Open':  self.data.Open[-1],
            'High':  self.data.High[-1],
            'Low':   self.data.Low[-1],
            'Close': self.data.Close[-1],
            'Volume': self.data.Volume[-1],

            'ema_50': self.data.ema_50[-1],
            'ema_200': self.data.ema_200[-1],
            'st_50_3': self.data.st_50_3[-1],
            'stoch_k': self.data.stoch_k[-1],
            'stoch_d': self.data.stoch_d[-1],
            'atr': self.data.atr[-1],
        }

        signal = self.my_strategy.update_candle(candle)

        # Vérifier si un signal d'achat ou de vente est généré
        if signal is None:
            return
        
        if signal['action'] == 'LIQUIDATE':
            self.position.close()
        elif signal['action'] == 'BUY':
            # self.position.close()
            self.buy(sl=candle['Close'] - signal['stop_loss'], tp=candle['Close'] + signal['take_profit'], size=signal['quantity'])
        elif signal['action'] == 'SELL':
            # self.position.close()
            self.sell(sl=candle['Close'] + signal['stop_loss'], tp=candle['Close'] - signal['take_profit'], size=signal['quantity'])
