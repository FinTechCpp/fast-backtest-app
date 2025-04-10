from BuyTrendFollowingStrategy import BuyTrendFollowingStrategy
from backtesting import Strategy as BacktestingStrategy
import numpy as np



class BacktestingAdapter(BacktestingStrategy):
    """
    Adapter pour la stratégie de backtesting.
    """
    def init(self):
        self.my_strategy = BuyTrendFollowingStrategy()

        def stochastic():
            k = self.data.df['stoch_k']
            d = self.data.df['stoch_d']
            level_80 = np.full(len(k), 80)
            level_20 = np.full(len(k), 20)
            return k, d, level_80, level_20

        self.ema_50 = self.I(lambda: self.data.df['ema_50'], name='EMA 50', overlay=True, color='red')
        self.ema_200 = self.I(lambda: self.data.df['ema_200'], name='EMA 200', overlay=True, color='blue')
        self.st_50_3 = self.I(lambda: self.data.df['st_50_3'], name='SuperTrend 50,3', overlay=True, color='purple')
        self.stoch = self.I(stochastic, 
                            name=['Stochastic %K', 'Stochastic %D', 'Surachat (80)', 'Survente (20)'],
                            overlay=False,
                            color=['green', 'orange', 'green', 'red'])
    
    def next(self):
        """
        Méthode appelée à chaque bougie pendant le backtest.
        On construit une nouvelle bougie sous forme de dictionnaire, on la transmet à 
        la stratégie custom via update_candle(), et on récupère le signal généré.
        
        Si un signal d'achat ou de vente est généré (et aucune position n'est déjà ouverte),
        on passe l'ordre correspondant.
        """
        candle = {
            'timestamp': self.data.index[-1],
            'Open':  self.data.Open[-1],
            'High':  self.data.High[-1],
            'Low':   self.data.Low[-1],
            'Close': self.data.Close[-1],
            'Volume': self.data.Volume[-1],

            'ema_50': self.data.ema_50[-1],
            'ema_200': self.data.ema_200[-1],
            'st_50_3': self.data.st_50_3[-1],
            'stoch_k': self.data.stoch_k[-1],
            'stoch_d': self.data.stoch_d[-1],
            'atr': self.data.atr[-1],
        }

        signal = self.my_strategy.update_candle(candle)

        # Vérifier si un signal d'achat ou de vente est généré
        if signal is None:
            return
        
        if signal['action'] == 'BUY':
            self.position.close()  # Fermer la position existante si elle existe
            self.buy(sl=signal['stop_loss'], tp=signal['take_profit'], size=signal['quantity'])
        elif signal['action'] == 'SELL':
            self.position.close()
            self.sell(sl=signal['stop_loss'], tp=signal['take_profit'], size=signal['quantity'])
