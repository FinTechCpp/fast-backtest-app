try:
    from Strategies.Strategy import Strategy
except ImportError:
    from Strategy import Strategy
    

class SellTrendFollowingStrategy(Strategy):
    """
    Stratégie de suivi de tendance à l'achat.
    """
    def __init__(self):
        super().__init__()
        """
        Initialise la stratégie.
        """

        self.name = "SellTrendFollowingStrategy"
        self.symbol = None
        self.exchange = None
        self.timeframe = None

        self.k_previous = None
        self.d_previous = None
    
    def should_long(self):
        return False

    def should_short(self):
        return True
    
    def go_long(self):
        raise NotImplementedError("La stratégie SellTrendFollowingStrategy ne supporte pas l'achat.")

    def go_short(self):
        self.sell = 0.5, self.price
        self.take_profit = 0.5, self.price * 0.99
        self.stop_loss = 0.5, self.price * 1.01
    
    def ema_filter(self):
        return self.price < self.candles['ema_50'] and self.price < self.candles['ema_200']
        
    def supertrend_filter(self):
        return self.price < self.candles['st_50_3']
    
    def cross_stoch_filter(self):
        # Vérifie si le Stochastic %K croise au-dessus de %D
        k_current, d_current = self.candles['stoch_k'], self.candles['stoch_d']

        if self.k_previous is None or self.d_previous is None:
            self.k_previous = k_current
            self.d_previous = d_current
            return False
        
        filt = k_current < d_current and self.k_previous > self.d_previous

        # Mettre à jour les valeurs précédentes
        self.k_previous = k_current
        self.d_previous = d_current


        return filt
    
    def stoch_sup_50_filter(self):
        return self.candles['stoch_d'] < 50
    
    def filters(self):
        return [
            self.ema_filter,
            self.supertrend_filter,
            self.cross_stoch_filter,
            self.stoch_sup_50_filter
        ]


