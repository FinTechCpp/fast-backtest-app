from ..Strategy import Strategy, StrategyBaseConfig, BaseCandle
from igtrader.backtestingpy.backtesting.backtesting import Strategy as BacktestingStrategy
from ..Helpers import EMA, STOCH, ATR
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

        # Initialiser les calculateurs d'indicateurs
        self.ema_short_calculator = EMA(self.config.ema_short_period)
        self.ema_long_calculator = EMA(self.config.ema_long_period)
        self.stochastic_calculator = STOCH(
            self.config.stoch_fastk,
            self.config.stoch_slowk,
            self.config.stoch_slowd
        )
        self.atr_calculator = ATR(self.base_config.atr_period)
        
        # Variables pour stocker les valeurs calculées
        self.current_ema_short = None
        self.current_ema_long = None
        self.current_stoch_k = None
        self.current_stoch_d = None
        self.current_atr = None

        # Cache pour les bougies Heikin Ashi
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


    def initialize_indicators(self):
        """
        Initialise les indicateurs avec l'historique des prix si disponible
        """
        if len(self.buffer) < max(self.config.ema_long_period, self.config.stoch_fastk + self.config.stoch_slowk):
            return False
            
        # Extraire les données pour l'initialisation
        close_history = self.buffer['Close'].values
        high_history = self.buffer['High'].values
        low_history = self.buffer['Low'].values
        
        # Initialiser les EMA
        self.current_ema_short = self.ema_short_calculator.initialize_with_history(close_history)
        self.current_ema_long = self.ema_long_calculator.initialize_with_history(close_history)
        
        # Initialiser le stochastique
        self.current_stoch_k, self.current_stoch_d = self.stochastic_calculator.initialize_with_history(
            high_history, low_history, close_history
        )
        
        # Initialiser l'ATR
        self.current_atr = self.atr_calculator.initialize_with_history(
            high_history, low_history, close_history
        )
        
        return (self.current_ema_short is not None and
                self.current_ema_long is not None and
                self.current_stoch_k is not None and
                self.current_stoch_d is not None)
    
    def update_indicators(self):
        """
        Met à jour tous les indicateurs avec les prix actuels
        """
        if not self.current_candle:
            return False
            
        if (not self.ema_short_calculator.is_initialized or
            not self.ema_long_calculator.is_initialized or
            not self.stochastic_calculator.is_initialized or
            (self.base_config.use_atr_for_sl_tp and not self.atr_calculator.is_initialized)):
            
            # Si les indicateurs ne sont pas initialisés, essayer de les initialiser
            if not self.initialize_indicators():
                return False
        
        # Mettre à jour les EMA
        self.current_ema_short = self.ema_short_calculator.update(self.current_candle.Close)
        self.current_ema_long = self.ema_long_calculator.update(self.current_candle.Close)
        
        # Mettre à jour le stochastique
        self.current_stoch_k, self.current_stoch_d = self.stochastic_calculator.update(
            self.current_candle.High,
            self.current_candle.Low,
            self.current_candle.Close
        )
        
        # Mettre à jour l'ATR
        self.current_atr = self.atr_calculator.update(
            self.current_candle.High,
            self.current_candle.Low,
            self.current_candle.Close
        )
        
        return True

    def before(self):
        """
        Calcule les valeurs Heikin Ashi de façon incrémentale.
        À chaque appel, déplace les valeurs actuelles vers précédentes et calcule uniquement la nouvelle bougie.
        """
        # Mettre à jour tous les indicateurs techniques
        self.update_indicators()

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
        
        # Le reste de votre code reste identique
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
        if self.base_config.use_atr_for_sl_tp and self.current_atr is not None:
            current_atr = self.current_atr
            
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
        """Vérifie si le prix est au-dessus de l'EMA courte"""
        if self.current_ema_short is None:
            return False
        return self.price > self.current_ema_short
    
    def ema_long_filter(self):
        """Vérifie si le prix est au-dessus de l'EMA longue"""
        if self.current_ema_long is None:
            return False
        return self.price > self.current_ema_long
    
    def stoch_inf_threshold_filter(self):
        """Vérifie si le Stochastic %K est inférieur au seuil"""
        if self.current_stoch_k is None:
            return False
            
        threshold = self.config.stoch_threshold
        result = self.current_stoch_k < threshold or (self.k_previous is not None and self.k_previous < threshold)
        
        # Mettre à jour les valeurs précédentes
        self.k_previous = self.current_stoch_k
        self.d_previous = self.current_stoch_d
        
        return result
    
    def previous_ha_candle_red_filter(self):
        """
        Filtre vérifiant si la bougie Heikin Ashi précédente est rouge.
        """
        if len(self.buffer) < 3:
            return False
        
        # Vérifier si la bougie précédente est rouge
        return not self.ha_cache['previous']['is_green']
    
    def filters(self):
        return self.active_filters
    



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
            use_previous_ha_candle_red_filter = kwargs.pop('use_previous_ha_candle_red_filter', True)
        )

    
        # 3) stocker et instancier la stratégie "métier"
        self.base_config = base_config
        self.config      = buy_trend_config
        self.my_strategy = BuyHeikinGreen(base_config, buy_trend_config)
        
        
    
    def next(self):
        """
        Méthode appelée à chaque bougie pendant le backtest.
        """
        # Créer une instance de la dataclass spécifique
        candle = BaseCandle(
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
                f"EMA Short: {self.my_strategy.current_ema_short}\n "
                f"EMA Long: {self.my_strategy.current_ema_long}\n "
                f"Stoch K: {self.my_strategy.current_stoch_k}\n "
                f"Stoch D: {self.my_strategy.current_stoch_d}\n "
                f"Trade size: {signal['quantity']}\n")
        elif not self.position and signal['action'] == 'SELL':
            pass