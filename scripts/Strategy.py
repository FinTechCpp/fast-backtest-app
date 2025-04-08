from abc import ABC, abstractmethod
import numpy as np
import pandas as pd

class Strategy(ABC):
    """
    Classe mère pour les stratégies de trading.
    """

    def __init__(self):
        self.name = None
        self.symbol = None
        self.exchange = None
        self.timeframe = None

        self.buy = None
        self.sell = None
        self.stop_loss = None
        self.take_profit = None

        self._is_executing = False

        self.candles = pd.DataFrame(columns=[
            'timestamp', 'open', 'high', 'low', 'close', 'volume'
        ])

    def _reset(self) -> None:
        """ Reset de la stratégie """
        self.buy = None
        self.sell = None
        self.stop_loss = None
        self.take_profit = None

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
        self._submit_buy_order()

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
        self._submit_sell_order()

    def _submit_buy_order(self) -> None:
        """ Soumet l'ordre d'achat. """
        pass

    def _submit_sell_order(self) -> None:
        """ Soumet l'ordre de vente. """
        pass
    
    def _execute_filters(self) -> bool:
        """ Exécute les filtres et retourne True si tous les filtres passent. """
        for f in self.filters():
            if not f():
                return False

        return True
    
    def _check(self) -> None:
        self._reset()
        
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


    def update_candle(self, candle: dict) -> None:
        """
        Ajoute la nouvelle bougie au DataFrame et déclenche l'exécution de la stratégie.
        
        :param candle: Un dictionnaire représentant la bougie avec les clefs:
                       'timestamp', 'open', 'high', 'low', 'close', 'volume'
        """
        # On créé une nouvelle DataFrame à partir du dictionnaire et on concatène
        new_row = pd.DataFrame([candle])
        self.candles = pd.concat([self.candles, new_row], ignore_index=True)

        # Optionnel : limiter la taille du DataFrame pour ne garder que les N dernières bougies
        # par exemple :
        MAX_BUFFER_SIZE = 1000
        if len(self.candles) > MAX_BUFFER_SIZE:
            self.candles = self.candles.iloc[-MAX_BUFFER_SIZE:]

        # Déclenche l'exécution de la stratégie avec la nouvelle donnée
        self._execute()


    @property
    def price(self) -> float:
        """ Retourne le prix actuel de l'actif. """
        return self.candles.iloc[-1]['close'] if not self.candles.empty else None
