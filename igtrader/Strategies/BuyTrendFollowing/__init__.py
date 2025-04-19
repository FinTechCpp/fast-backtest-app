from ..Helpers import calculate_supertrend
from ..Strategy import Strategy
from igtrader.backtestingpy.backtesting.backtesting import Strategy as BacktestingStrategy
import logging
import numpy as np
from datetime import time
import talib


class BuyTrendFollowing(Strategy):
    """
    Stratégie de suivi de tendance à l'achat.
    """
    def __init__(self, **kwargs):
        super().__init__(**kwargs)

        self.name = "BuyTrendFollowingStrategy"
        self.symbol = None
        self.exchange = None
        self.timeframe = None

        self.k_previous = None
        self.d_previous = None

        self.tp_distance = kwargs.get('take_profit_distance', 30)
        self.sl_distance = kwargs.get('stop_loss_distance', 20)
        
        # Noms des indicateurs (dynamiques ou par défaut)
        self.ema_short_name = kwargs.get('ema_short_name', 'EMA_50')
        self.ema_long_name = kwargs.get('ema_long_name', 'EMA_200')
        self.supertrend_name = kwargs.get('supertrend_name', 'SUPERTREND_50_3')
        self.stoch_k_name = kwargs.get('stoch_k_name', 'STOCH_K_10_7_3')
        self.stoch_d_name = kwargs.get('stoch_d_name', 'STOCH_D_10_7_3')
    
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
        self.take_profit = 0.5, self.tp_distance
        self.stop_loss = 0.5, self.sl_distance
    
    def go_short(self):
        raise NotImplementedError("La stratégie BuyTrendFollowingStrategy ne supporte pas la vente à découvert.")

    def ema_filter(self):
        # Vérifie si le prix est au-dessus des EMA
        return self.price > self.candles[self.ema_short_name] and self.price > self.candles[self.ema_long_name]
        
    def supertrend_filter(self):
        # Vérifie si le prix est au-dessus du SuperTrend
        return self.price > self.candles[self.supertrend_name]
    
    def stoch_sup_50_filter(self):
        # Vérifie si le Stochastic %K est inférieur à 50
        return self.candles[self.stoch_k_name] < 50
    
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
        """
        # On travaille sur une copie du buffer pour être sûr que seules les colonnes nécessaires soient présentes.
        df = self.buffer.copy()
        if df.empty:
            # Pas suffisamment de données : on affecte par défaut la valeur du Close de la bougie actuelle.
            if self.ema_short_name not in candle:
                candle[self.ema_short_name] = candle["Close"]
            if self.ema_long_name not in candle:
                candle[self.ema_long_name] = candle["Close"]
            if self.stoch_k_name not in candle:
                candle[self.stoch_k_name] = 0.0
            if self.stoch_d_name not in candle:
                candle[self.stoch_d_name] = 0.0
            if self.supertrend_name not in candle:
                candle[self.supertrend_name] = candle["Close"]
            return candle

        # Garder uniquement les colonnes nécessaires
        df = df[['Open', 'High', 'Low', 'Close']]
        
        # Extraire les paramètres des noms d'indicateurs
        # EMA
        if self.ema_long_name not in candle:
            # Extraire la période de l'EMA du nom (ex: EMA_200 -> 200)
            period = int(self.ema_long_name.split('_')[1])
            if len(df) >= period:
                ema_val = talib.EMA(df['Close'].values, timeperiod=period)
                candle[self.ema_long_name] = float(ema_val[-1])
            else:
                candle[self.ema_long_name] = candle["Close"]
                
        if self.ema_short_name not in candle:
            period = int(self.ema_short_name.split('_')[1])
            if len(df) >= period:
                ema_val = talib.EMA(df['Close'].values, timeperiod=period)
                candle[self.ema_short_name] = float(ema_val[-1])
            else:
                candle[self.ema_short_name] = candle["Close"]

        # Stochastique
        if self.stoch_k_name not in candle or self.stoch_d_name not in candle:
            # Extraire les paramètres du nom (ex: STOCH_K_10_7_3 -> 10,7,3)
            parts = self.stoch_k_name.split('_')
            if len(parts) >= 5:  # STOCH_K_10_7_3
                fastk_period = int(parts[2])
                slowk_period = int(parts[3])
                slowd_period = int(parts[4])
                
                if len(df) >= fastk_period:
                    k, d = talib.STOCH(
                        df['High'].values, df['Low'].values, df['Close'].values,
                        fastk_period=fastk_period, slowk_period=slowk_period, slowd_period=slowd_period)
                    candle[self.stoch_k_name] = float(k[-1])
                    candle[self.stoch_d_name] = float(d[-1])
                else:
                    candle[self.stoch_k_name] = 0.0
                    candle[self.stoch_d_name] = 0.0
            else:
                candle[self.stoch_k_name] = 0.0
                candle[self.stoch_d_name] = 0.0

        # SuperTrend
        if self.supertrend_name not in candle:
            # Extraire les paramètres (ex: SUPERTREND_50_3 -> 50,3)
            parts = self.supertrend_name.split('_')
            if len(parts) >= 3:  # SUPERTREND_50_3
                atr_period = int(parts[1])
                multiplier = int(parts[2])
                
                if len(df) >= 1:
                    df_st = df[['High', 'Low', 'Close']].copy()
                    st_df = calculate_supertrend(df_st, atr_period=atr_period, multiplier=multiplier)
                    candle[self.supertrend_name] = float(st_df['SuperTrend'].iloc[-1])
                else:
                    candle[self.supertrend_name] = candle["Close"]
            else:
                candle[self.supertrend_name] = candle["Close"]
                    
        return candle



class BuyTrendFollowingBA(BacktestingStrategy):
    """
    Adapter pour la stratégie de backtesting.
    """
    def init(self, **kwargs):
        # Vérifier si des colonnes SuperTrend sont disponibles
        supertrend_cols = [col for col in self.data.df.columns if 'supertrend_' in col.lower()]
        if supertrend_cols and not kwargs.get('supertrend_name'):
            # Utiliser le premier SuperTrend disponible
            kwargs['supertrend_name'] = supertrend_cols[0].upper()
            logging.info(f"Indicateur SuperTrend détecté automatiquement: {kwargs['supertrend_name']}")

        # Obtenir ou définir les noms des indicateurs
        self.ema_short_name = kwargs.get('ema_short_name', 'EMA_50')
        self.ema_long_name = kwargs.get('ema_long_name', 'EMA_200')
        self.supertrend_name = kwargs.get('supertrend_name', 'SUPERTREND_50_3')
        self.stoch_k_name = kwargs.get('stoch_k_name', 'STOCH_K_10_7_3')
        self.stoch_d_name = kwargs.get('stoch_d_name', 'STOCH_D_10_7_3')
        
        # Vérifier que les indicateurs requis sont présents dans les données
        required_indicators = [
            self.ema_short_name, 
            self.ema_long_name, 
            self.supertrend_name, 
            self.stoch_k_name, 
            self.stoch_d_name
        ]
        
        for indicator in required_indicators:
            if indicator.lower() not in [col.lower() for col in self.data.df.columns]:
                logging.warning(f"L'indicateur {indicator} n'est pas présent dans les données. "
                            f"Colonnes disponibles: {list(self.data.df.columns)}")
            
            # Passer ces noms à la stratégie
            kwargs['ema_short_name'] = self.ema_short_name
            kwargs['ema_long_name'] = self.ema_long_name
            kwargs['supertrend_name'] = self.supertrend_name
            kwargs['stoch_k_name'] = self.stoch_k_name
            kwargs['stoch_d_name'] = self.stoch_d_name
            
            self.my_strategy = BuyTrendFollowing(**kwargs)

            def stochastic():
                k = self.data.df[self.stoch_k_name]
                d = self.data.df[self.stoch_d_name]
                level_80 = np.full(len(k), 80)
                level_20 = np.full(len(k), 20)
                return k, d, level_80, level_20

                                    
            # Ajouter des attributs pour accéder aux données
            setattr(self.data, 'ema_short', self.data.df[self.ema_short_name])
            setattr(self.data, 'ema_long', self.data.df[self.ema_long_name])
            setattr(self.data, 'supertrend', self.data.df[self.supertrend_name])
            setattr(self.data, 'stoch_k', self.data.df[self.stoch_k_name])
            setattr(self.data, 'stoch_d', self.data.df[self.stoch_d_name])
            
    
    def next(self):
        """
        Méthode appelée à chaque bougie pendant le backtest.
        """
        candle = {
            'date': self.data.index[-1],
            'Open': self.data.Open[-1],
            'High': self.data.High[-1],
            'Low': self.data.Low[-1],
            'Close': self.data.Close[-1]
        }
        
        # Corriger l'accès aux indicateurs - en utilisant self.data.df
        candle[self.ema_short_name] = self.data.df[self.ema_short_name].iloc[-1]
        candle[self.ema_long_name] = self.data.df[self.ema_long_name].iloc[-1] 
        candle[self.supertrend_name] = self.data.df[self.supertrend_name].iloc[-1]
        candle[self.stoch_k_name] = self.data.df[self.stoch_k_name].iloc[-1]
        candle[self.stoch_d_name] = self.data.df[self.stoch_d_name].iloc[-1]
        
        # Ajouter l'ATR si disponible
        atr_col = 'ATR_14'
        if atr_col in self.data.df.columns:
            candle[atr_col] = self.data.df[atr_col].iloc[-1]
        
        # TRÈS IMPORTANT: Mettre à jour la bougie AVANT d'appeler les filtres
        # car cela initialise self.candles dans la stratégie
        signal = self.my_strategy.update_candle(candle)
        
        # Maintenant que self.candles a été mis à jour, on peut déboguer
        if self.data.index[-1].day % 2 == 0 and self.data.index[-1].hour == 10:  # Limiter la quantité de logs
            logging.debug(f"Candle: {candle['date']}")
            logging.debug(f"EMA Short ({self.ema_short_name}): {candle[self.ema_short_name]}")
            logging.debug(f"EMA Long ({self.ema_long_name}): {candle[self.ema_long_name]}")
            logging.debug(f"SuperTrend ({self.supertrend_name}): {candle[self.supertrend_name]}")
            logging.debug(f"Stoch K ({self.stoch_k_name}): {candle[self.stoch_k_name]}")
            logging.debug(f"Stoch D ({self.stoch_d_name}): {candle[self.stoch_d_name]}")
            
            # Vérifier les filtres
            ema_filter = self.my_strategy.ema_filter()
            supertrend_filter = self.my_strategy.supertrend_filter()
            stoch_filter = self.my_strategy.stoch_sup_50_filter()
            should_long = self.my_strategy.should_long()
            
            logging.debug(f"Filtres - EMA: {ema_filter}, SuperTrend: {supertrend_filter}, Stoch<50: {stoch_filter}")
            logging.debug(f"Should Long: {should_long}")
    
        # Vérifier si un signal d'achat ou de vente est généré
        if signal is None:
            return
        
        if signal['action'] == 'LIQUIDATE':
            self.position.close()
        elif signal['action'] == 'BUY':
            self.buy(sl=candle['Close'] - signal['stop_loss'], 
                    tp=candle['Close'] + signal['take_profit'], 
                    size=signal['quantity'])
        elif signal['action'] == 'SELL':
            self.sell(sl=candle['Close'] + signal['stop_loss'], 
                    tp=candle['Close'] - signal['take_profit'], 
                    size=signal['quantity'])