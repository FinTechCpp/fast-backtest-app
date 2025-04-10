try:
    from Strategies.Strategy import Strategy
except ImportError:
    from Strategy import Strategy
from datetime import time


class BuyTrendFollowingStrategy(Strategy):
    """
    Stratégie de suivi de tendance à l'achat.
    """
    def __init__(self):
        super().__init__()
        """
        Initialise la stratégie.
        """

        self.name = "BuyTrendFollowingStrategy"
        self.symbol = None
        self.exchange = None
        self.timeframe = None

        self.k_previous = None
        self.d_previous = None

        self.trading_from = time(15, 0)
        self.trading_to = time(20, 0)
    
    def should_long(self):
        return True

    def should_short(self):
        return False
    
    def go_long(self):
        self.buy = 0.5, self.price
        self.take_profit = 0.5, self.price * 1.01
        self.stop_loss = 0.5, self.price * 0.99
    
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
    
