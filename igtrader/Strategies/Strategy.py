from abc import ABC, abstractmethod
import numpy as np
import pandas as pd
from datetime import time



class Strategy(ABC):
    """
    Classe mère pour les stratégies de trading.
    """

    def __init__(self, **kwargs):
        self.name = None
        self.symbol = None
        self.exchange = None
        self.timeframe = None

        self.trading_from = kwargs.get('trading_from', time(14, 30))
        self.trading_to = kwargs.get('trading_to', time(21, 0))
        self.trading_days = kwargs.get('trading_days', [0, 1, 2, 3, 4])  # Lundi à Vendredi

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
        
        self.BUFFER_SIZE = 200

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

        # Filters
        if not self._execute_filters():
            return
        
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

        # Filters
        if not self._execute_filters():
            return
        
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
        if self.candles is None or not isinstance(self.candles, dict) or 'date' not in self.candles:
            return False

        try:
            last_candle_date = pd.to_datetime(self.candles['date'])
        except Exception as e:
            print("Erreur de conversion de la date :", e)
            return False

        # Vérifie si c'est un jour de trading
        if last_candle_date.weekday() not in self.trading_days:
            return False

        # Vérifie si c'est dans l'intervalle de temps de trading
        current_time = last_candle_date.time()
        if not (self.trading_from <= current_time <= self.trading_to):
            return False

        return True
    
    def _check(self) -> None:
        self._reset()

        # Vérifie si la stratégie est dans la période de trading
        if not self._check_time():
            self.signal = self._generate_liquidation_signal()
            return
        
        should_long = self.should_long()
        should_short = self.should_short()
        if should_long and should_short:
            raise ValueError("La stratégie ne peut pas être à la fois en position longue et courte.")
    
        if should_long:
            self._execute_long()
        elif should_short:
            self._execute_short()

    def _execute(self) -> None:
        """ Exécute la stratégie. """

        # Pour etre sûr que la stratégie ne s'exécute pas plusieurs fois en même temps
        if self._is_executing:
            return

        self._is_executing = True
        self.before()
        self._check()
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

        new_candle = pd.DataFrame([candle])
        new_candle['date'] = pd.to_datetime(new_candle['date'])
        new_candle.set_index('date', inplace=True)
    
        # On ajoute la nouvelle bougie au buffer et on la garde à la taille max +- 100
        new_candle = new_candle.dropna(axis=1, how='all')
        self.buffer = pd.concat([self.buffer, new_candle])
        if len(self.buffer) > self.BUFFER_SIZE + 100:
            self.buffer = self.buffer.iloc[-self.BUFFER_SIZE:]

        # On enrichit la bougie avec les indicateurs manquants
        self.candles = self.add_missing_indicators(candle.copy())

        # Déclenche l'exécution de la stratégie avec la nouvelle donnée
        self._execute()

        return self.signal


    @property
    def price(self) -> float:
        """ Retourne le prix actuel de l'actif. """
        return self.candles['Close'] if self.candles is not None else None
