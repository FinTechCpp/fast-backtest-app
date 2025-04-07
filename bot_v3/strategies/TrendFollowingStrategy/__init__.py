from jesse.strategies import Strategy, cached
import jesse.indicators as ta
from jesse import utils


class TrendFollowingStrategy(Strategy):
    @cached
    def stoch(self):
        """
        Calcule les indicateurs stochastiques.
        Nous utilisons une période de 10, un %K lissé sur 7 et un %D sur 3.
        Renvoie deux listes : stoch_k et stoch_d.
        """
        stoch_k, stoch_d = ta.stoch(self.candles, 10, 7, 3)
        return stoch_k, stoch_d
    
    @cached
    def ema_50(self):
        return ta.ema(self.candles, period=50)
    
    @cached
    def ema_200(self):
        return ta.ema(self.candles, period=200)
    
    @cached
    def atr(self):
        return ta.atr(self.candles, period=14)
    
    @cached
    def supertrend_line(self):
        # Voir la fonction dans le fichier utils.py
        return 0
    
    def should_long(self) -> bool:
        """
        Conditions pour entrer en position longue (achat).
        Conditions :
          1. %K croise au-dessus de %D : (k_current > d_current et k_previous < d_previous)
          2. %D actuel est inférieur à 50.
          3. Le prix de clôture est supérieur à la ligne supertrend.
          4. Le prix de clôture est supérieur à EMA50 et EMA200.
          5. S'il y a déjà une position ouverte, la différence avec le prix d'entrée doit être supérieure à 2*ATR.
        """

        # S'assurer que nous avons suffisamment de bougies pour calculer les indicateurs
        if len(self.candles) < 2:
            return False
        
        # Récupérer les valeurs des indicateurs
        stoch_k, stoch_d = self.stoch()
        ema50 = self.ema_50()
        ema200 = self.ema_200()
        atr = self.atr()
        supertrend = self.supertrend_line()

        # Indices : -1 pour la bougie actuelle, -2 pour la précédente
        k_current = stoch_k[-1]
        d_current = stoch_d[-1]
        k_previous = stoch_k[-2]
        d_previous = stoch_d[-2]

        last_close = self.candles[-1, 4]  # Prix de clôture de la dernière bougie
        current_atr = atr[-1]

        # Condition sur le croisement stochastique
        if not (k_current > d_current and k_previous < d_previous):
            return False
        
        # Condition sur le %D
        if d_current >= 50:
            return False
        
        # Condition sur le supertrend
        if last_close <= supertrend[-1]:
            return False
        
        # Condition sur les EMA
        if last_close <= ema50[-1] or last_close <= ema200[-1]:
            return False
        
        # Si une position est déjà ouverte, vérifier la différence par rapport au prix d'entrée
        last_open_price = self.position.entry_price if self.position.is_long else None
        if last_open_price is not None and abs(last_close - last_open_price) <= 2 * current_atr:
            return False
        
        
        return True

    def should_short(self) -> bool:
        return False

    def should_cancel_entry(self) -> bool:
        return False

    def go_long(self):
        qty = utils.size(self.capital, self.candles[-1, 4])
        self.buy = qty

    def go_short(self):
        pass

