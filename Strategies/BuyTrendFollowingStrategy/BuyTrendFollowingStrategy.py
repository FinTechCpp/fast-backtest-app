try:
    from Strategies.Strategy import Strategy
    from Strategies.Helpers import calculate_supertrend
except ImportError:
    from Strategy import Strategy
    from Helpers import calculate_supertrend
from datetime import time
import talib
import pandas as pd



class BuyTrendFollowingStrategy(Strategy):
    """
    Stratégie de suivi de tendance à l'achat.
    """
    def __init__(self, stop_loss_percent=0.99, take_profit_percent=1.01):
        super().__init__()


        self.name = "BuyTrendFollowingStrategy"
        self.symbol = None
        self.exchange = None
        self.timeframe = None

        self.k_previous = None
        self.d_previous = None

        self.trading_from = time(15, 0)
        self.trading_to = time(20, 0)

        self.take_profit_persent = take_profit_percent
        self.stop_loss_percent = stop_loss_percent
    
    def should_long(self):
        return True

    def should_short(self):
        return False
    
    def go_long(self):
        self.buy = 0.5, self.price
        self.take_profit = 0.5, self.price * self.take_profit_persent
        self.stop_loss = 0.5, self.price * self.stop_loss_percent
    
    def go_short(self):
        raise NotImplementedError("La stratégie BuyTrendFollowingStrategy ne supporte pas la vente à découvert.")

    def ema_filter(self):
        # Vérifie si le prix est au-dessus des EMA
        return self.price > self.candles['ema_50'] and self.price > self.candles['ema_200']
        
    def supertrend_filter(self):
        # Vérifie si le prix est au-dessus du SuperTrend
        return self.price > self.candles['st_50_3']
    
    def cross_stoch_filter(self):
        # Vérifie si le Stochastic %K croise au-dessus de %D
        k_current, d_current = self.candles['stoch_k'], self.candles['stoch_d']

        if self.k_previous is None or self.d_previous is None:
            self.k_previous = k_current
            self.d_previous = d_current
            return False
        
        filt = k_current > d_current and self.k_previous < self.d_previous

        # Mettre à jour les valeurs précédentes
        self.k_previous = k_current
        self.d_previous = d_current


        return filt
    
    def stoch_sup_50_filter(self):
        # Vérifie si le Stochastic %D est supérieur à 50
        return self.candles['stoch_d'] > 50
    
    def filters(self):
        return [
            self.ema_filter,
            self.supertrend_filter,
            self.cross_stoch_filter,
            self.stoch_sup_50_filter
        ]
    

    def add_missing_indicators(self, candle: dict) -> dict:
        """
        Ajoute les indicateurs manquants à la bougie 'candle'.
        Si un indicateur est présent dans 'candle', il n'est pas recalcule.
        Sinon, on le calcule à partir des données du buffer.
        On se base sur le buffer (self.buffer) qui contient uniquement les colonnes 
        'Open', 'High', 'Low', 'Close', 'Volume' et 'date'.
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
        df = df[['Open', 'High', 'Low', 'Close', 'Volume']]

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
