from abc import ABC, abstractmethod
import numpy as np
import pandas as pd
from datetime import time
import logging
from dataclasses import dataclass
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


class Strategy(ABC):
    """
    Classe mère pour les stratégies de trading.
    """

    def __init__(self, base_config: StrategyBaseConfig):
        self.name = None
        self.symbol = None
        self.exchange = None
        self.timeframe = None

        self.base_config = base_config

        self.buy = None
        self.sell = None
        self.stop_loss = None
        self.take_profit = None
        self.signal = None # TODO : à changer pour un dict avec des clés bien définies

        self._is_executing = False

        self.candles = {}
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
        
        # Configuration pour le benchmark
        self.MAX_EXECUTION_TIMES = 100000
        self.execution_times = []
        self.last_execution_time = None
        self.benchmark_active = False

    def start_benchmark(self):
        """Active la collecte des métriques de benchmark."""
        self.benchmark_active = True
        self.execution_times = []
        
    def stop_benchmark(self):
        """Désactive la collecte des métriques de benchmark."""
        self.benchmark_active = False
        
    def get_benchmark_stats(self):
        """Retourne les statistiques de performance."""
        if not self.execution_times:
            return {
                "count": 0,
                "min": 0,
                "max": 0, 
                "mean": 0,
                "median": 0,
                "p95": 0,
                "p99": 0,
                "total": 0
            }
            
        sorted_times = sorted(self.execution_times)
        p95_index = int(len(sorted_times) * 0.95)
        p99_index = int(len(sorted_times) * 0.99)
        
        return {
            "count": len(self.execution_times),
            "min": min(self.execution_times),
            "max": max(self.execution_times),
            "mean": statistics.mean(self.execution_times),
            "median": statistics.median(self.execution_times),
            "p95": sorted_times[p95_index] if p95_index < len(sorted_times) else sorted_times[-1],
            "p99": sorted_times[p99_index] if p99_index < len(sorted_times) else sorted_times[-1],
            "total": sum(self.execution_times)
        }

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
        """Version optimisée du check de temps."""
        if not self.candles or 'date' not in self.candles:
            return False

        try:
            # Utiliser des variables de classe pour éviter les calculs répétés
            if not hasattr(self, '_last_check_date') or self.candles['date'] != self._last_check_date:
                self._last_check_date = self.candles['date']
                last_candle_date = pd.to_datetime(self._last_check_date)
                
                # Calculer et mettre en cache les résultats
                self._weekday_check = last_candle_date.weekday() in self.base_config.trading_days
                if not self._weekday_check:
                    return False
                    
                current_time = last_candle_date.time()
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

    def update_candle(self, candle: dict) -> dict:
        """
        Ajoute la nouvelle bougie au DataFrame et déclenche l'exécution de la stratégie.
        
        :param candle: Un dictionnaire représentant la bougie avec les clefs:
                       'date', 'Open', 'High', 'Low', 'Close'
        """
        start_time = dt.perf_counter()

        # 1. Conversion optimisée de la date
        date_value = pd.to_datetime(candle['date'])
        
        # 2. Filtrer les données pertinentes sans créer de DataFrame intermédiaire
        candle_data = {k: v for k, v in candle.items() if k != 'date' and not pd.isna(v)}
        
        # 3. Mise à jour efficace du buffer
        if self.buffer.empty:
            # Créer un nouveau DataFrame directement si le buffer est vide
            self.buffer = pd.DataFrame([candle_data], index=[date_value])
        else:
            # Ajouter directement la ligne sans utiliser concat (qui est coûteux)
            self.buffer.loc[date_value] = pd.Series(candle_data)
            
            # 4. Redimensionnement du buffer dès qu'il dépasse le seuil
            if len(self.buffer) > self.RESIZE_THRESHOLD:
                # Ne garder que les BUFFER_SIZE dernières entrées
                self.buffer = self.buffer.iloc[-self.BUFFER_SIZE:]
        
        # 5. Éviter la copie inutile du dictionnaire candle
        self.candles = self.add_missing_indicators(candle)
        
        # 6. Exécution de la stratégie
        self._execute()
        
        duration_ms = (dt.perf_counter() - start_time) * 1000
        self.last_execution_time = duration_ms

        # Ajouter le temps d'exécution à la liste et limiter sa taille
        if self.benchmark_active:
            self.execution_times.append(duration_ms)

            if len(self.execution_times) > self.MAX_EXECUTION_TIMES:
                self.execution_times.pop(0)

        # logging.debug(f"Strategy execution time: {duration_ms:.2f} ms")

        return self.signal

    @property
    def price(self) -> float:
        """ Retourne le prix actuel de l'actif. """
        return self.candles['Close'] if self.candles is not None else None
