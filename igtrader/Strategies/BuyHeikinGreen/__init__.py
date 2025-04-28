from ..Strategy import Strategy, StrategyBaseConfig
from igtrader.backtestingpy.backtesting.backtesting import Strategy as BacktestingStrategy
import logging
logging.basicConfig(level=logging.DEBUG, format='%(asctime)s - %(levelname)s - %(message)s')
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
        self.current_equity = None
        # TODO : Arriver a rendre cela fixe, à ne pas redéfinir à chaque fois
        # Noms des indicateurs
        self.ema_short_name = f'EMA_{self.config.ema_short_period}'
        self.ema_long_name = f'EMA_{self.config.ema_long_period}'
        self.stoch_k_name = f'STOCH_K_{self.config.stoch_fastk}_{self.config.stoch_slowk}_{self.config.stoch_slowd}'
        self.stoch_d_name = f'STOCH_D_{self.config.stoch_fastk}_{self.config.stoch_slowk}_{self.config.stoch_slowd}'
        self.atr_name = f'ATR_{self.base_config.atr_period}'
        self.ha_cache = {
            'current': {'open': None, 'close': None, 'is_green': None},
            'previous': {'open': None, 'close': None, 'is_green': None},
        }

    def before(self):
        """
        Calcule les valeurs Heikin Ashi de façon incrémentale.
        À chaque appel, déplace les valeurs actuelles vers précédentes et calcule uniquement la nouvelle bougie.
        """
        # On a besoin d'au moins 2 bougies pour calculer les valeurs HA
        if len(self.buffer) < 2:
            return
        
        # Récupérer la bougie actuelle et la précédente
        current, prev = self.candles, self.buffer.iloc[-2]
        
        # Si c'est la première fois qu'on calcule ou après une réinitialisation
        if self.ha_cache['current']['close'] is None:
            # On doit initialiser les deux bougies
            if len(self.buffer) >= 3:
                prev2 = self.buffer.iloc[-3]
                
                # Calculer HA pour la bougie précédente
                ha_close_prev = (prev["Open"] + prev["High"] + prev["Low"] + prev["Close"]) / 4
                ha_open_prev = (prev2["Open"] + prev2["Close"]) / 2
                
                self.ha_cache['previous'] = {
                    'open': ha_open_prev,
                    'close': ha_close_prev,
                    'is_green': ha_close_prev > ha_open_prev
                }
            else:
                # Si pas assez d'historique, initialiser avec des valeurs de base
                self.ha_cache['previous'] = {
                    'open': prev["Open"],
                    'close': prev["Close"],
                    'is_green': prev["Close"] > prev["Open"]
                }
        else:
            # Déplacer les valeurs actuelles vers précédentes (réutilisation des calculs)
            self.ha_cache['previous'] = self.ha_cache['current'].copy()
        
        # Calculer HA uniquement pour la bougie actuelle
        ha_close_current = (current["Open"] + current["High"] + current["Low"] + current["Close"]) / 4
        ha_open_current = (self.ha_cache['previous']['open'] + self.ha_cache['previous']['close']) / 2
        
        # Mettre à jour le cache pour la bougie actuelle
        self.ha_cache['current'] = {
            'open': ha_open_current,
            'close': ha_close_current,
            'is_green': ha_close_current > ha_open_current
        }
    
    def should_long(self):
        """
        Vérifie si la condition d'entrée en position longue est remplie:
        - La bougie Heikin Ashi actuelle est verte
        """
        if len(self.buffer) < 3:
            return False
        
        # Vérifier seulement si la bougie actuelle est verte
        return self.ha_cache['current']['is_green']

    def should_short(self):
        return False
    
    def go_long(self):
        # Si nous utilisons l'ATR pour les SL/TP
        if self.base_config.use_atr_for_sl_tp and self.atr_name in self.candles:
            current_atr = self.candles[self.atr_name]
            
            # Vérifier que l'ATR n'est pas zéro ou négatif
            if current_atr <= 0:
                logging.warning(f"ATR invalide: {current_atr}, utilisation de la valeur minimale")
                current_atr = self.base_config.min_stop_loss_distance / self.base_config.stop_loss_atr_multiplier
            
            # Calculer le stop loss basé sur l'ATR avec un minimum
            stop_loss_distance = max(
                current_atr * self.base_config.stop_loss_atr_multiplier,
                self.base_config.min_stop_loss_distance
            )
            
            # Calculer le take profit basé sur l'ATR avec un minimum
            take_profit_distance = max(
                current_atr * self.base_config.take_profit_atr_multiplier,
                self.base_config.min_take_profit_distance
            )
        else:
            # Utiliser les valeurs fixes de fallback
            stop_loss_distance = self.base_config.stop_loss_distance
            take_profit_distance = self.base_config.take_profit_distance
        
        # Calculer la taille du trade basée sur le risque si activé
        if self.base_config.use_risk_based_sizing:
            # Utiliser l'equity actuelle si disponible, sinon utiliser la valeur de base
            capital = self.current_equity if self.current_equity is not None else self.base_config.cash
            
            # Montant que l'utilisateur est prêt à risquer par trade
            risk_amount = capital * (self.base_config.risk_percentage / 100)
            
            # Calculer la taille de position: montant risqué divisé par la distance du stop loss
            raw_position_size = risk_amount / self.price
            
            # Si la taille est >= 1, arrondir à l'entier supérieur
            # Si la taille est < 1, la laisser telle quelle (fraction d'equity)
            if raw_position_size >= 1:
                position_size = round(raw_position_size)  # Arrondir à l'entier le plus proche
            else:
                # Limiter à un minimum de 0.01 (1% d'equity)
                position_size = max(0.5, min(raw_position_size, 0.99))
            
            logging.debug(
                f"Risk-based sizing: Capital={capital}, "
                f"Risque={self.base_config.risk_percentage}%, "
                f"Montant risqué={risk_amount}, "
                f"Taille calculée={position_size} (raw={raw_position_size})"
                f"Prix actuel de l'actif={self.price}, "
            )
        else:
            # Taille fixe par défaut (entier)
            position_size = 1
        
        self.buy = position_size, self.price
        self.take_profit = position_size, take_profit_distance
        self.stop_loss = position_size, stop_loss_distance
    
    def go_short(self):
        raise NotImplementedError(f"La stratégie {self.name} ne supporte pas la vente à découvert.")

    def ema_filter(self):
        # Vérifie si le prix est au-dessus des EMA
        return self.price > self.candles[self.ema_short_name] and self.price > self.candles[self.ema_long_name]
    
    def previous_ha_candle_red_filter(self):
        """
        Filtre vérifiant si la bougie Heikin Ashi précédente est rouge.
        """
        if len(self.buffer) < 3:
            return False
        
        # Vérifier si la bougie précédente est rouge
        return not self.ha_cache['previous']['is_green']
    
    def stoch_inf_50_filter(self):
        # Vérifie si le Stochastic %K présent est inférieur à 50 ou qu'il est ete en dessous de 50 sur la bougie précédente
        return self.candles[self.stoch_k_name] < 50 and (self.k_previous is not None and self.k_previous < 50)
    
    # TODO : calculer les indicateur seulement si on en a besoin, donc au debut de chaque filtre on calcule les indicateurs
    def filters(self):
        return [
            self.previous_ha_candle_red_filter,
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
            if self.atr_name not in candle:
                candle[self.atr_name] = np.nan
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
        
        if self.atr_name not in candle and self.base_config.use_atr_for_sl_tp:
            atr_val = talib.ATR(df['High'].values, df['Low'].values, df['Close'].values, timeperiod=self.base_config.atr_period)
            candle[self.atr_name] = float(atr_val[-1]) if not np.isnan(atr_val[-1]) else 0.0
                
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
            # Nouveaux paramètres ATR
            use_atr_for_sl_tp = kwargs.pop('use_atr_for_sl_tp', False),
            atr_period = kwargs.pop('atr_period', 14),
            stop_loss_atr_multiplier = kwargs.pop('stop_loss_atr_multiplier', 2.0),
            take_profit_atr_multiplier = kwargs.pop('take_profit_atr_multiplier', 3.0),
            min_stop_loss_distance = kwargs.pop('min_stop_loss_distance', 5.0),
            min_take_profit_distance = kwargs.pop('min_take_profit_distance', 5.0),
            # Paramètres de gestion du risque - AJOUT DES PARAMÈTRES MANQUANTS
            use_risk_based_sizing = kwargs.pop('use_risk_based_sizing', False),
            risk_percentage = kwargs.pop('risk_percentage', 1.0),
            cash = kwargs.pop('cash', 100000.0),
        )
        
        # 2) extraire les clés spécifiques
        buy_trend_config = BuyHeikinGreenConfig(
            ema_short_period     = kwargs.pop('ema_short_period'),
            ema_long_period      = kwargs.pop('ema_long_period'),
            stoch_fastk          = kwargs.pop('stoch_fastk'),
            stoch_slowk          = kwargs.pop('stoch_slowk'),
            stoch_slowd          = kwargs.pop('stoch_slowd'),
        )
    
        # 3) stocker et instancier la stratégie "métier"
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
        self.atr_name = f'ATR_{self.base_config.atr_period}'
    
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
        
        # Ajouter l'ATR s'il est disponible
        if self.atr_name in self.data.df.columns:
            candle[self.atr_name] = self.data.df[self.atr_name].iloc[-1]
                
        # Mettre à jour les valeurs précédentes du Stochastique
        self.my_strategy.k_previous = candle[self.stoch_k_name]
        self.my_strategy.d_previous = candle[self.stoch_d_name]
        
        # Transmettre l'equity actuelle à la stratégie
        self.my_strategy.current_equity = self.equity
        
        # TRÈS IMPORTANT: Mettre à jour la bougie AVANT d'appeler les filtres
        # car cela initialise self.candles dans la stratégie
        signal = self.my_strategy.update_candle(candle)
        
        # Vérifier les filtres
        
        cash = self.equity
        # Vérifier les filtres
        ema_filter = self.my_strategy.ema_filter()
        stoch_filter = self.my_strategy.stoch_inf_50_filter()
        previous_ha_candle_red_filter = self.my_strategy.previous_ha_candle_red_filter()
        should_long = self.my_strategy.should_long()

        # affiche un warning sur un signal d'achat avec des filtre a false
        if signal is not None and signal['action'] == 'BUY' and (not ema_filter or not stoch_filter or not previous_ha_candle_red_filter or not should_long):
            logging.warning(
                f"Signal d'achat généré avec des filtres non respectés : "
                f"EMA: {ema_filter}, Stoch<50: {stoch_filter}, Should Long: {should_long}, Previous HA Candle Red: {previous_ha_candle_red_filter}"
            )
    
        # Vérifier si un signal d'achat ou de vente est généré
        if signal is None or ema_filter is False or stoch_filter is False or should_long is False:
            return
        if signal['action'] == 'LIQUIDATE':
            self.position.close()
        elif not self.position and signal['action'] == 'BUY' and ema_filter and stoch_filter and should_long:
            self.buy(sl=candle['Close'] - signal['stop_loss'], 
                    tp=candle['Close'] + signal['take_profit'], 
                    size=signal['quantity'])
            logging.info(
                f"\n\nCandle: {candle['date']}\n "
                f"EMA Short ({self.ema_short_name}): {candle[self.ema_short_name]}\n "
                f"EMA Long ({self.ema_long_name}): {candle[self.ema_long_name]}\n "
                f"Stoch K ({self.stoch_k_name}): {candle[self.stoch_k_name]}\n "
                f"Stoch D ({self.stoch_d_name}): {candle[self.stoch_d_name]}\n "
                f"Filtres - EMA: {ema_filter}, Stoch<50: {stoch_filter}\n "
                f"Should Long: {should_long}\n "
                f"Trade size: {signal["quantity"]}\n")
            
        elif not self.position and signal['action'] == 'SELL':
            self.sell(sl=candle['Close'] + signal['stop_loss'], 
                    tp=candle['Close'] - signal['take_profit'], 
                    size=signal['quantity'])
            logging.info(
                f"\n\nCandle: {candle['date']}\n "
                f"EMA Short ({self.ema_short_name}): {candle[self.ema_short_name]}\n "
                f"EMA Long ({self.ema_long_name}): {candle[self.ema_long_name]}\n "
                f"Stoch K ({self.stoch_k_name}): {candle[self.stoch_k_name]}\n "
                f"Stoch D ({self.stoch_d_name}): {candle[self.stoch_d_name]}\n "
                f"Filtres - EMA: {ema_filter}, Stoch<50: {stoch_filter}\n "
                f"Should Long: {should_long}\n\n"
            )