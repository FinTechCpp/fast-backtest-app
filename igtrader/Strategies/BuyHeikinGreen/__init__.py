from ..Strategy import Strategy, StrategyBaseConfig, BaseCandle
from igtrader.backtestingpy.backtesting.backtesting import Strategy as BacktestingStrategy
import logging
import talib
import numpy as np
from dataclasses import dataclass
from typing import Optional, Dict, Any, List


# from cpp_strategies import StrategyBaseConfig, BuyHeikinGreenConfig, BuyHeikinGreen, Candle


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
    stoch_threshold: int = 50

    # Activation/désactivation des filtres
    use_ema_short_filter: bool = True
    use_ema_long_filter: bool = True
    use_stoch_filter: bool = True
    use_previous_ha_candle_red_filter: bool = True

@dataclass
class BuyHeikinGreenCandle(BaseCandle):
    """Dataclass spécifique pour la stratégie BuyHeikinGreen avec les indicateurs nécessaires"""
    # EMA
    ema_short: Optional[float] = None
    ema_long: Optional[float] = None
    
    # Stochastique
    stoch_k: Optional[float] = None
    stoch_d: Optional[float] = None
    
    # ATR
    atr: Optional[float] = None
    
    def get_indicator(self, name: str) -> Optional[float]:
        """
        Méthode d'accès unifiée aux indicateurs par nom pour compatibilité
        """
        # Mapping des noms d'indicateurs aux attributs
        indicator_mapping = {
            # Les clés sont les noms variables des indicateurs
            # Les valeurs sont les attributs de cette classe
            "ema_short": self.ema_short,
            "ema_long": self.ema_long,
            "stoch_k": self.stoch_k,
            "stoch_d": self.stoch_d,
            "atr": self.atr
        }
        
        # Retourner la valeur si trouvée, sinon None
        return indicator_mapping.get(name)



class BuyHeikinGreen(Strategy):
    """
    Stratégie de trading basée sur les bougies Heikin Ashi.
    """
    def __init__(self, base_config: StrategyBaseConfig, buy_heikin_green_config: BuyHeikinGreenConfig):
        super().__init__(base_config)

        self.k_previous = None
        self.d_previous = None

        # Initialize cash from base_config
        self.cash = base_config.cash

        self.config = buy_heikin_green_config
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

        self.active_filters = []
        if self.config.use_ema_short_filter:
            self.active_filters.append(self.ema_short_filter)
        if self.config.use_ema_long_filter:
            self.active_filters.append(self.ema_long_filter)
        if self.config.use_stoch_filter:
            self.active_filters.append(self.stoch_inf_threshold_filter)
        if self.config.use_previous_ha_candle_red_filter:
            self.active_filters.append(self.previous_ha_candle_red_filter)

    def before(self):
        """
        Calcule les valeurs Heikin Ashi de façon incrémentale.
        À chaque appel, déplace les valeurs actuelles vers précédentes et calcule uniquement la nouvelle bougie.
        """
        # On a besoin d'au moins 2 bougies pour calculer les valeurs HA
        if len(self.buffer) < 2:
            return
        
        # Récupérer la bougie actuelle (dataclass) et la précédente (du buffer)
        current_candle = self.current_candle
        prev = self.buffer.iloc[-2]
        
        # Créer un dictionnaire avec les données de la bougie actuelle pour compatibilité
        current = {
            "Open": current_candle.Open,
            "High": current_candle.High,
            "Low": current_candle.Low,
            "Close": current_candle.Close
    }
        
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
        if self.base_config.use_atr_for_sl_tp and hasattr(self.current_candle, 'atr') and self.current_candle.atr is not None:
            current_atr = self.current_candle.atr
            
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
            initial_capital = self.cash
            # Calculer le montant risqué en dollars
            risk_amount = initial_capital * self.base_config.risk_percentage / 100
            
            # Calculer la taille de position pour que le SL représente exactement risk_amount
            risk_based_position_size = risk_amount / stop_loss_distance
            
            # Utiliser le capital total disponible avec effet de levier
            leveraged_capital = initial_capital * self.base_config.leverage_limit
            
            # Limiter la taille maximale de position à un pourcentage du capital avec effet de levier
            max_position_value = leveraged_capital * self.base_config.max_position_percentage / 100
            max_position_size = max_position_value / self.price
            
            # Prendre le MINIMUM entre la taille basée sur le risque et la limite imposée par "leveraged capital
            raw_position_size = min(risk_based_position_size, max_position_size)
            
            # Si la taille est >= 1, arrondir à l'entier le plus proche
            if raw_position_size >= 1:
                position_size = int(raw_position_size)
            else:
                # Limiter à un minimum de 0.5
                position_size = max(0.5, min(raw_position_size, 0.99))
            
            # Calculer le risque réel après arrondis pour vérification
            real_risk_amount = position_size * stop_loss_distance
            real_risk_percentage = (real_risk_amount / initial_capital) * 100
            
            logging.debug(
                f"Position size calculation: Capital={initial_capital:.2f}\n "
                f"Capital avec levier={leveraged_capital:.2f}\n "
                f"Risque={self.base_config.risk_percentage:.2f}%\n "
                f"Effet de levier={self.base_config.leverage_limit:.2f}\n "
                f"Montant risqué={risk_amount:.2f}\n "
                f"Prix actuel={self.price:.2f}\n "
                f"Distance SL={stop_loss_distance:.2f}\n "
                f"Distance TP={take_profit_distance:.2f}\n "
                f"Taille basée sur le risque={risk_based_position_size:.2f}\n "
                f"Taille maximale par position={max_position_size:.2f}\n "
                f"Taille après limites={position_size} ({self.price * position_size:.2f}$)\n "
                f"Risque réel={real_risk_percentage:.2f}% ({real_risk_amount:.2f}$)\n "
                f"Profit potentiel={take_profit_distance * position_size:.2f} $\n "
            )
        else:
            # Taille fixe par défaut (entier)
            position_size = 1
        
        self.buy = position_size, self.price
        self.take_profit = position_size, take_profit_distance
        self.stop_loss = position_size, stop_loss_distance
    
    def go_short(self):
        raise NotImplementedError(f"La stratégie {self.name} ne supporte pas la vente à découvert.")

    def ema_short_filter(self):
        # Vérifie si le prix est au-dessus des EMA
        ema_short_value = getattr(self.current_candle, 'ema_short', None)
        if ema_short_value is None:
            ema_short_value = self.get_indicator_value(self.ema_short_name)
        return self.price > ema_short_value
    
    def ema_long_filter(self):
        # Vérifie si le prix est au-dessus des EMA
        ema_long_value = getattr(self.current_candle, 'ema_long', None)
        if ema_long_value is None:
            ema_long_value = self.get_indicator_value(self.ema_long_name)
        return self.price > ema_long_value
    
    def previous_ha_candle_red_filter(self):
        """
        Filtre vérifiant si la bougie Heikin Ashi précédente est rouge.
        """
        if len(self.buffer) < 3:
            return False
        
        # Vérifier si la bougie précédente est rouge
        return not self.ha_cache['previous']['is_green']

    def stoch_inf_threshold_filter(self):
        # Vérifie si le Stochastic %K présent est inférieur à threshold
        threshold = self.config.stoch_threshold
        
        stoch_k_value = getattr(self.current_candle, 'stoch_k', None)
        if stoch_k_value is None:
            stoch_k_value = self.get_indicator_value(self.stoch_k_name)
        
        # Récupérer le résultat du filtre avant de mettre à jour les valeurs précédentes
        result = stoch_k_value < threshold or (self.k_previous is not None and self.k_previous < threshold)
        
        # Mettre à jour les valeurs précédentes
        self.k_previous = stoch_k_value
        self.d_previous = getattr(self.current_candle, 'stoch_d', 
                                 self.get_indicator_value(self.stoch_d_name))
        
        return result
    
    # TODO : calculer les indicateur seulement si on en a besoin, donc au debut de chaque filtre on calcule les indicateurs
    def filters(self):
        return self.active_filters
    
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


# TODO : pas mal de travail à faire pour enlever la logique de cette class.
# Plus de robustesse sur les indicateurs => si on les a pas et he pg il faudra que la strategie les calcule
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
            
            # Paramètres de gestion du risque 
            use_risk_based_sizing = kwargs.pop('use_risk_based_sizing', False),
            risk_percentage = kwargs.pop('risk_percentage', 1.0),
            cash = kwargs.pop('cash', 100000.0),
            leverage_limit = kwargs.pop('leverage_limit', 20.0),

            # Paramètres de gestion de la position
            use_break_even = kwargs.pop('use_break_even', True),
            break_even_threshold = kwargs.pop('break_even_threshold', 0.7)
        )
        
        # 2) extraire les clés spécifiques
        buy_trend_config = BuyHeikinGreenConfig(
            ema_short_period     = kwargs.pop('ema_short_period'),
            ema_long_period      = kwargs.pop('ema_long_period'),
            stoch_fastk          = kwargs.pop('stoch_fastk'),
            stoch_slowk          = kwargs.pop('stoch_slowk'),
            stoch_slowd          = kwargs.pop('stoch_slowd'),
            stoch_threshold      = kwargs.pop('stoch_threshold', 50),
            # Activation/désactivation des filtres
            use_ema_short_filter       = kwargs.pop('use_ema_short_filter', True),
            use_ema_long_filter  = kwargs.pop('use_ema_long_filter', True),
            use_stoch_filter     = kwargs.pop('use_stoch_filter', True),
            use_previous_ha_candle_red_filter = kwargs.pop('use_previous_ha_candle_red_filter', True),
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
        ema_short_name = f'EMA_{self.config.ema_short_period}'
        ema_long_name = f'EMA_{self.config.ema_long_period}'
        stoch_k_name = f'STOCH_K_{self.config.stoch_fastk}_{self.config.stoch_slowk}_{self.config.stoch_slowd}'
        stoch_d_name = f'STOCH_D_{self.config.stoch_fastk}_{self.config.stoch_slowk}_{self.config.stoch_slowd}'
        atr_name = f'ATR_{self.base_config.atr_period}'


        # Créer une instance de la dataclass spécifique
        candle = BuyHeikinGreenCandle(
            date=self.data.index[-1],
            Open=self.data.Open[-1],
            High=self.data.High[-1],
            Low=self.data.Low[-1],
            Close=self.data.Close[-1],
            # Informations de position
            in_position=bool(self.position),
            position_pl_pct=self.position.pl_pct if self.position else None,
            entry_price=self.trades[-1].entry_price if self.position and self.trades else None,
            position_size=self.position.size if self.position else None,
            
            # Indicateurs spécifiques
            ema_short=self.data.df[ema_short_name].iloc[-1],
            ema_long=self.data.df[ema_long_name].iloc[-1],
            stoch_k=self.data.df[stoch_k_name].iloc[-1],
            stoch_d=self.data.df[stoch_d_name].iloc[-1],
            # ATR (si disponible)
            atr=self.data.df[atr_name].iloc[-1] if atr_name in self.data.df.columns else None
        )        

        signal = self.my_strategy.update_candle(candle)
        
            
        # Vérifier si un signal d'achat ou de vente est généré
        if signal is None:
            return
        if signal['action'] == 'LIQUIDATE':
            self.position.close()
        elif signal['action'] == 'MOVE_SL':
            # Traiter le signal de break-even
            for trade in self.trades:
                trade.sl = signal['new_sl']
                logging.info(f"Moving stop loss to break-even at {signal['new_sl']}")
        elif not self.position and signal['action'] == 'BUY':
            self.buy(sl_points=signal['stop_loss'], 
                    tp_points=signal['take_profit'], 
                    size=signal['quantity'])
            logging.info(
                f"\n\nCandle: {candle.date}\n "
                f"Open ({candle.Open}), Close ({candle.Close})\n "
                f"EMA Short: {candle.ema_short}\n "
                f"EMA Long: {candle.ema_long}\n "
                f"Stoch K: {candle.stoch_k}\n "
                f"Stoch D: {candle.stoch_d}\n "
                f"Trade size: {signal['quantity']}\n")
            
        elif not self.position and signal['action'] == 'SELL':
            pass