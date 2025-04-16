# from ..Helpers import calculate_supertrend
from ..Strategy import Strategy
from igtrader.backtestingpy.backtesting.backtesting import Strategy as BacktestingStrategy

import talib
import numpy as np
from datetime import time


class CrossEMA(Strategy):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)

        self.name = "TemplateStrategy"


        self.previous_ema_20_l = None
        self.previous_ema_50_l = None

        self.previous_ema_20_s = None
        self.previous_ema_50_s = None

    def should_long(self):
        """
        Détermine si la stratégie doit entrer en position longue.
        """
        current_ema_20 = self.candles['ema_20']
        current_ema_50 = self.candles['ema_50']

        if self.previous_ema_20_l is None or self.previous_ema_50_l is None:
            self.previous_ema_20_l = current_ema_20
            self.previous_ema_50_l = current_ema_50
            return False
        
        should_go_long = current_ema_20 > current_ema_50 and self.previous_ema_20_l < self.previous_ema_50_l
        self.previous_ema_20_l = current_ema_20
        self.previous_ema_50_l = current_ema_50


        # print(f"current emas : {current_ema_20}, {current_ema_50}")

        return should_go_long
    
    def should_short(self):
        """
        Détermine si la stratégie doit entrer en position courte.
        """
        current_ema_20 = self.candles['ema_20']
        current_ema_50 = self.candles['ema_50']

        if self.previous_ema_20_s is None or self.previous_ema_50_s is None:
            self.previous_ema_20_s = current_ema_20
            self.previous_ema_50_s = current_ema_50
            return False
        
        should_go_short = current_ema_20 < current_ema_50 and self.previous_ema_20_s > self.previous_ema_50_s
        self.previous_ema_20_s = current_ema_20
        self.previous_ema_50_s = current_ema_50

        return should_go_short
    
    def go_long(self):
        """
        Exécute une position longue.
        """
        self.buy = 1, self.price
        self.take_profit = 1, 30
        self.stop_loss = 1, 20

    def go_short(self):
        """
        Exécute une position courte.
        """
        self.sell = 1, self.price
        self.take_profit = 1, 30
        self.stop_loss = 1, 20


    def filters(self):
        return []
    
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
            if "ema_20" not in candle:
                candle["ema_20"] = candle["Close"]
            return candle

        # Garder uniquement les colonnes nécessaires
        df = df[['Open', 'High', 'Low', 'Close']]

        # Calculer EMA 20 si absent
        if "ema_20" not in candle:
            if len(df) >= 20:
                ema_20 = talib.EMA(df['Close'].values, timeperiod=20)
                candle["ema_20"] = float(ema_20[-1])
            else:
                candle["ema_20"] = candle["Close"]
        
        # Calculer EMA 50 si absent
        if "ema_50" not in candle:
            if len(df) >= 50:
                ema_50 = talib.EMA(df['Close'].values, timeperiod=50)
                candle["ema_50"] = float(ema_50[-1])
            else:
                candle["ema_50"] = candle["Close"]
                
        return candle
    


class CrossEMABA(BacktestingStrategy):
    """
    Adapter pour la stratégie de backtesting.
    """
    def init(self, **kwargs):
        self.my_strategy = CrossEMA(**kwargs)

        self.ema_20 = self.I(lambda: self.data.df['ema_20'], name='EMA 20', overlay=True, color='blue')
        self.ema_50 = self.I(lambda: self.data.df['ema_50'], name='EMA 50', overlay=True, color='red')

    
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

            'ema_20': self.data.ema_20[-1],
            'ema_50': self.data.ema_50[-1],
        }

        signal = self.my_strategy.update_candle(candle)

        if signal is None:
            return
        if signal['action'] == 'LIQUIDATE':
            self.position.close()
        elif signal['action'] == 'BUY':
            self.position.close()  # Fermer la position existante si elle existe
            self.buy(sl=candle['Close'] - signal['stop_loss'], tp=candle['Close'] + signal['take_profit'], size=signal['quantity'])
        elif signal['action'] == 'SELL':
            self.position.close()
            self.sell(sl=candle['Close'] + signal['stop_loss'], tp=candle['Close'] - signal['take_profit'], size=signal['quantity'])
