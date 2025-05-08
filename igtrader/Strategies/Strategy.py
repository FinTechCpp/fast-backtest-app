from abc import ABC, abstractmethod
import numpy as np
import pandas as pd
from datetime import time
import logging
from dataclasses import dataclass, field, asdict
from typing import Optional, Dict, Any, List
from dataclasses import field
from pandas import Timestamp
import time as dt
import statistics


@dataclass
class StrategyBaseConfig:
    trading_from: time = time(14, 30)
    trading_to: time = time(21, 0)
    trading_days: list = field(default_factory=lambda: [0, 1, 2, 3, 4])  # Monday to Friday
    
    # Valeurs fixes comme fallback
    take_profit_distance: float = 30.0
    stop_loss_distance: float = 20.0
    
    # Nouveaux paramètres pour l'ATR
    use_atr_for_sl_tp: bool = False
    atr_period: int = 14
    stop_loss_atr_multiplier: float = 2.0
    take_profit_atr_multiplier: float = 3.0
    
    # Valeurs minimales pour éviter des SL/TP trop serrés
    min_stop_loss_distance: float = 5.0
    min_take_profit_distance: float = 5.0
    
    use_risk_based_sizing: bool = False
    risk_percentage: float = 1.0  # Pourcentage par défaut (1% du capital)
    cash: float = 100000.0  # Capital pour calculer le risque
    max_position_percentage: float = 100.0  # Pourcentage du capital à investir par trade
    leverage_limit: float = 20.0  # Limite de levier pour le calcul du risque

    # Paramètres de break-even
    use_break_even: bool = False
    break_even_threshold: float = 0.7  # 70% du take profit par défaut

@dataclass
class BaseCandle:
    """Classe de base pour les données de bougie communes à toutes les stratégies"""
    date: Timestamp
    Open: float
    High: float
    Low: float
    Close: float

    # Informations supplémentaires
    in_position: bool = False
    entry_price: Optional[float] = None
    position_size: Optional[float] = None
    position_pl_pct: Optional[float] = None


class Strategy(ABC):
    """
    Classe mère pour les stratégies de trading.
    """

    def __init__(self, base_config: StrategyBaseConfig):
        self.base_config = base_config

        # Ajout des informations de position
        self.in_position = False
        self.entry_price = None
        self.position_size = None
        self.position_pl_pct = None

        self.buy = None
        self.sell = None
        self.stop_loss = None
        self.take_profit = None
        self.signal = None # TODO : à changer pour un dict avec des clés bien définies

        self._is_executing = False

        self.current_candle = None

        self.buffer = pd.DataFrame({
            'date': pd.Series(dtype='str'),
            'Open': pd.Series(dtype='float'),
            'High': pd.Series(dtype='float'),
            'Low': pd.Series(dtype='float'),
            'Close': pd.Series(dtype='float'),
        })
        self.buffer.set_index('date', inplace=True)
        
        # Permettre de configurer la taille du buffer
        self.BUFFER_SIZE = 200
        # Définir un seuil pour le redimensionnement (par exemple 20% de plus que BUFFER_SIZE)
        self.RESIZE_THRESHOLD = int(self.BUFFER_SIZE * 1.2)

    # Propriété price modifiée pour utiliser current_candle
    @property
    def price(self) -> float:
        """ Retourne le prix actuel de l'actif. """
        if self.current_candle:
            return self.current_candle.Close
        return None

    # Méthode d'accès aux indicateurs
    def get_indicator_value(self, indicator_name: str) -> Optional[float]:
        """
        Récupère la valeur d'un indicateur à partir de la bougie actuelle
        """
        # Vérifier si l'indicateur est accessible via un attribut
        if hasattr(self.current_candle, indicator_name):
            return getattr(self.current_candle, indicator_name)
        
        # Pour la compatibilité avec les classes dérivées qui implémentent get_indicator
        if hasattr(self.current_candle, "get_indicator"):
            return self.current_candle.get_indicator(indicator_name)
        
        return None
    
    def _reset(self) -> None:
        """ Reset de la stratégie """
        self.buy = None
        self.sell = None
        self.stop_loss = None
        self.take_profit = None
        self.signal = None

    def before(self) -> None:
        """ Get's executed BEFORE executing the strategy's logic """
        pass

    def after(self) -> None:
        """ Get's executed AFTER executing the strategy's logic """
        pass

    @abstractmethod
    def should_long(self) -> bool:
        """ Should the strategy go long? """
        return False
    
    @abstractmethod
    def should_short(self) -> bool:
        """ Should the strategy go short? """
        return False
    
    @abstractmethod
    def go_long(self) -> None:
        """ On définit l'ordre d'achat. """
        pass

    @abstractmethod
    def go_short(self) -> None:
        """ On définit l'ordre de vente. """
        pass

    def filters(self) -> list:
        """ Liste des filtres à appliquer avant d'ouvrir une position. """
        return []
    
    def add_missing_indicators(self, candle: dict) -> dict:
        """
        Ajoute les indicateurs nécessaires à la bougie.
        :param candle: Un dictionnaire représentant la bougie avec les clefs:
                       'date', 'Open', 'High', 'Low', 'Close'
        """
        return candle
    
    def _check_break_even(self) -> Optional[dict]:
        """
        Vérifie si le stop loss doit être déplacé au point d'entrée (break-even).
        Retourne un signal si les conditions sont remplies, sinon None.
        """
        if not (self.in_position and self.base_config.use_break_even):
            return None
            
        if self.entry_price is None or self.position_pl_pct is None:
            return None
            
        # Calcul du seuil basé sur la distance du take profit
        threshold_pct = (self.base_config.take_profit_distance / self.entry_price * 100)
        
        # Vérifier si on a atteint le seuil pour activer le break-even
        if self.position_pl_pct > (self.base_config.break_even_threshold * threshold_pct):
            # Générer un signal de break-even
            return {
                "action": "MOVE_SL",
                "new_sl": self.entry_price
            }
        
        return None
    
    def _execute_long(self) -> None:
        self.go_long()

        # validation
        if self.buy is None:
            raise ValueError('You forgot to set self.buy. example (qty, price)')
        elif type(self.buy) not in [tuple, list]:
            raise ValueError(f'self.buy must be either a list or a tuple. example: (qty, price). You set: {type(self.buy)}')
        
        # Formatage de l'ordre d'achat
        self.buy = np.array(self.buy, dtype=float)

        # Take profit
        if self.take_profit is not None:
            # validation
            if type(self.take_profit) not in [tuple, list]:
                raise ValueError(f'self.take_profit must be either a list or a tuple. example: (qty, price). You set: {type(self.take_profit)}')
            
            # Formatage du take profit
            self.take_profit = np.array(self.take_profit, dtype=float)

        # Stop loss
        if self.stop_loss is not None:
            # validation
            if type(self.stop_loss) not in [tuple, list]:
                raise ValueError(f'self.stop_loss must be either a list or a tuple. example: (qty, price). You set: {type(self.stop_loss)}')
            
            # Formatage du stop loss
            self.stop_loss = np.array(self.stop_loss, dtype=float)

        # Submit the buy order
        self.signal = self._generate_buy_signal()

    def _execute_short(self) -> None:
        self.go_short()

        # validation
        if self.sell is None:
            raise ValueError('You forgot to set self.sell. example (qty, price)')
        elif type(self.sell) not in [tuple, list]:
            raise ValueError(f'self.sell must be either a list or a tuple. example: (qty, price). You set: {type(self.sell)}')
        
        # Formatage de l'ordre de vente
        self.sell = np.array(self.sell, dtype=float)

        # Take profit
        if self.take_profit is not None:
            # validation
            if type(self.take_profit) not in [tuple, list]:
                raise ValueError(f'self.take_profit must be either a list or a tuple. example: (qty, price). You set: {type(self.take_profit)}')
            
            # Formatage du take profit
            self.take_profit = np.array(self.take_profit, dtype=float)

        # Stop loss
        if self.stop_loss is not None:
            # validation
            if type(self.stop_loss) not in [tuple, list]:
                raise ValueError(f'self.stop_loss must be either a list or a tuple. example: (qty, price). You set: {type(self.stop_loss)}')
            
            # Formatage du stop loss
            self.stop_loss = np.array(self.stop_loss, dtype=float)
        
        # Submit the sell order
        self.signal = self._generate_sell_signal()
    
    def _generate_buy_signal(self) -> dict:
        """
        Prépare et retourne le signal d'achat.
        
        Le signal est un dictionnaire contenant par exemple :
        - action: 'BUY'
        - quantity: la quantité
        - price: le prix auquel l'ordre doit être passé
        - take_profit: niveau du take profit (optionnel)
        - stop_loss: niveau du stop loss (optionnel)
        """
        signal = {
            "action": "BUY",
            "quantity": float(self.buy[0]),
            "price": float(self.buy[1]),
            "take_profit": float(self.take_profit[1]) if self.take_profit is not None else None,
            "stop_loss": float(self.stop_loss[1]) if self.stop_loss is not None else None,
        }
        return signal

    def _generate_sell_signal(self) -> dict:
        """
        Prépare et retourne le signal de vente.

        Le signal est un dictionnaire contenant par exemple :
        - action: 'SELL'
        - quantity: la quantité
        - price: le prix auquel l'ordre doit être passé
        - take_profit: niveau du take profit (optionnel)
        - stop_loss: niveau du stop loss (optionnel)
        """
        signal = {
            "action": "SELL",
            "quantity": float(self.sell[0]),
            "price": float(self.sell[1]),
            "take_profit": float(self.take_profit[1]) if self.take_profit is not None else None,
            "stop_loss": float(self.stop_loss[1]) if self.stop_loss is not None else None,
        }
        return signal
    
    def _generate_liquidation_signal(self) -> dict:
        """
        Prépare et retourne le signal de liquidation.

        Le signal est un dictionnaire contenant par exemple :
        - action: 'LIQUIDATE'
        """
        signal = {
            "action": "LIQUIDATE"
        }
        return signal
    
    def _execute_filters(self) -> bool:
        """ Exécute les filtres et retourne True si tous les filtres passent. """
        for f in self.filters():
            if not f():
                return False

        return True
    
    def _check_time(self) -> bool:
        """Version optimisée du check de temps avec dataclass."""
        if not self.current_candle:
            return False

        try:
            # Utiliser des variables de classe pour éviter les calculs répétés
            current_date = self.current_candle.date
            if not hasattr(self, '_last_check_date') or current_date != self._last_check_date:
                self._last_check_date = current_date
                
                # Calculer et mettre en cache les résultats
                self._weekday_check = current_date.weekday() in self.base_config.trading_days
                if not self._weekday_check:
                    return False
                    
                current_time = current_date.time()
                self._time_check = (self.base_config.trading_from <= current_time <= self.base_config.trading_to)
                
                return self._time_check
            else:
                # Utiliser les résultats en cache
                return self._weekday_check and self._time_check
        except Exception as e:
            logging.error(f"Erreur dans _check_time: {e}")
            return False
    
    def _execute(self) -> None:
        """ Exécute la stratégie. """

        # Pour etre sûr que la stratégie ne s'exécute pas plusieurs fois en même temps
        if self._is_executing:
            return

        self._is_executing = True

        # Check rapide du temps avant d'exécuter quoi que ce soit d'autre
        if not self._check_time():
            self.signal = self._generate_liquidation_signal()
            self._is_executing = False
            return
    
        self.before()
    
        should_long = self.should_long()
        should_short = should_short = False if should_long else self.should_short()

        if not (should_long or should_short):
            self._reset()
            self._is_executing = False
            return
        
        if not self._execute_filters():
            # Si les filtres échouent, on ne fait rien
            self._reset()
            self._is_executing = False
            return
        if should_long:
            self._execute_long()
        else:
            self._execute_short()
        
        self.after()
        self._is_executing = False

    def initialize(self, candles: pd.DataFrame) -> None:
        """
        Initialise la stratégie avec les bougies historiques.
        
        :param candles: Un DataFrame contenant les bougies historiques avec les colonnes:
                        'Open', 'High', 'Low', 'Close'
        et indexé par 'date'.
        """
        self.buffer = candles.copy()
        if 'date' in self.buffer.columns:
            # Si la date est en colonne, on la convertit en datetime et on la définit comme index
            self.buffer['date'] = pd.to_datetime(self.buffer['date'])
            self.buffer.set_index('date', inplace=True)
        else:
            # Sinon, on suppose que la date est dans l'index
            # On convertit l'index en datetime si nécessaire puis on définit le nom 'date'
            if not pd.api.types.is_datetime64_any_dtype(self.buffer.index):
                self.buffer.index = pd.to_datetime(self.buffer.index)
            self.buffer.index.name = 'date'

    def update_candle(self, candle: BaseCandle) -> dict:
        """
        Ajoute la nouvelle bougie au DataFrame et déclenche l'exécution de la stratégie.
        
        :param candle: Un dictionnaire représentant la bougie avec les clefs:
                       'date', 'Open', 'High', 'Low', 'Close'
        """
        # Stocker la référence directe à la bougie
        self.current_candle = candle

        # Mettre à jour les informations de position
        self.in_position = candle.in_position
        self.entry_price = candle.entry_price
        self.position_size = candle.position_size
        self.position_pl_pct = candle.position_pl_pct

        # Ajouter les données au buffer (utile pour les calculs qui nécessitent un historique)
        date_value = candle.date
        
        # Créer un dictionnaire avec les données OHLC
        ohlc_data = {
            'Open': candle.Open,
            'High': candle.High,
            'Low': candle.Low,
            'Close': candle.Close
        }

        # Ajouter les attributs supplémentaires de la classe dérivée
        for attr_name, attr_value in vars(candle).items():
            if attr_name not in ['date', 'Open', 'High', 'Low', 'Close', 
                                'in_position', 'entry_price', 'position_size', 'position_pl_pct']:
                if not attr_name.startswith('_'):  # Ignorer les attributs privés
                    ohlc_data[attr_name] = attr_value
        
        # Mettre à jour le buffer
        if self.buffer.empty:
            self.buffer = pd.DataFrame([ohlc_data], index=[date_value])
        else:
            self.buffer.loc[date_value] = pd.Series(ohlc_data)
            
            if len(self.buffer) > self.RESIZE_THRESHOLD:
                self.buffer = self.buffer.iloc[-self.BUFFER_SIZE:]

        # Vérifier si on doit déclencher un break-even avant d'exécuter la stratégie
        break_even_signal = self._check_break_even()
        if break_even_signal:
            return break_even_signal

        # Exécution de la stratégie
        self._execute()

        return self.signal

