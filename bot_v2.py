import time
import pandas as pd
import numpy as np
from trading_ig.rest import IGService
from trading_ig.config import config
import utils
import markets

class Strategy:
    """
    Classe mère pour les stratégies de trading.
    """
    def generate_signal(self, df, last_open_price, min_price_diff):
        """
        Analyse les données et retourne un signal ('BUY', 'SELL' ou None).
        """
        raise NotImplementedError("La méthode generate_signal doit être implémentée par une sous-classe.")

class MovingAverageStrategy(Strategy):
    """
    Stratégie basée sur les moyennes mobiles et RSI.
    """
    def should_open_position(direction, ema_10, ema_30, rsi, last_price, last_open_price, min_price_diff):
        """
        Détermine si une position doit être ouverte en fonction des conditions de trading.

        :param direction: 'BUY' ou 'SELL'
        :param ema_10: Valeur actuelle de l'EMA 10
        :param ema_30: Valeur actuelle de l'EMA 30
        :param rsi: Valeur actuelle du RSI
        :param last_price: Dernier prix
        :param last_open_price: Dernier prix d'ouverture
        :param min_price_diff: Différence minimale de prix pour éviter les faux signaux
        :return: True si une position doit être ouverte, False sinon
        """
        if direction == 'BUY':
            return ema_10 > ema_30 and rsi < 70 and (last_open_price is None or abs(last_price - last_open_price) > min_price_diff)
        elif direction == 'SELL':
            return ema_10 < ema_30 and rsi > 30 and (last_open_price is None or abs(last_price - last_open_price) > min_price_diff)
        return False

    def generate_signal(self, df, last_open_price, min_price_diff):
        df['EMA_10'] = utils.calculate_ema(df['close'], 10)
        df['EMA_30'] = utils.calculate_ema(df['close'], 30)
        df['RSI'] = utils.calculate_rsi(df['close'])
        df['ATR'] = utils.calculate_atr(df, period=14)
        
        last_price = df['close'].iloc[-1]
        ema_10 = df['EMA_10'].iloc[-1]
        ema_30 = df['EMA_30'].iloc[-1]
        rsi = df['RSI'].iloc[-1]
        min_price_diff = df['ATR'].iloc[-1] * 2  # Dynamique
        
        if self.should_open_position('BUY', ema_10, ema_30, rsi, last_price, last_open_price, min_price_diff):
            return 'BUY'
        elif self.should_open_position('SELL', ema_10, ema_30, rsi, last_price, last_open_price, min_price_diff):
            return 'SELL'
        return None

class TradingBot:
    """
    Classe mère pour un bot de trading.
    """
    def __init__(self, strategy):
        self.strategy = strategy
        self.ig_service = utils.initialize_service()
        self.epic, _ = utils.select_from_dict(markets.epics_dict)
        self.trade_size = 0.5
        self.sl = 0.002
        self.tp = 0.004
        self.last_open_price = None
    
    def fetch_market_data(self):
        """ Récupère les données de marché en temps réel. """
        return utils.fetch_prices(self.ig_service, self.epic)
    
    def execute_trade(self, direction, last_price):
        """ Exécute un ordre d'achat ou de vente. """
        stop_distance = round(last_price * self.sl, 2)
        limit_distance = round(last_price * self.tp, 2)
        
        response = self.ig_service.create_open_position(
            epic=self.epic,
            direction=direction,
            size=self.trade_size,
            currency_code='EUR',
            expiry='-',
            order_type='MARKET',
            guaranteed_stop=False,
            force_open=True,
            trailing_stop=False,
            time_in_force='FILL_OR_KILL',
            stop_distance=stop_distance,
            limit_distance=limit_distance)
        
        utils.position_output(response)
        self.last_open_price = last_price

    def run(self):
        """ Boucle principale du bot. """
        print("🔄 Démarrage du bot de trading...")
        while True:
            df = self.fetch_market_data()
            signal = self.strategy.generate_signal(df, self.last_open_price, min_price_diff=5)
            last_price = df['close'].iloc[-1]
            
            if signal == 'BUY':
                print("📈 Signal d'achat détecté!")
                self.execute_trade('BUY', last_price)
            elif signal == 'SELL':
                print("📉 Signal de vente détecté!")
                self.execute_trade('SELL', last_price)
            
            time.sleep(60)

if __name__ == "__main__":
    strategy = MovingAverageStrategy()
    bot = TradingBot(strategy)
    bot.run()
