import time
import pandas as pd
import numpy as np
import sys
sys.path.insert(0, '/home/max/ig-trading-bot')
from trading_ig.rest import IGService
from trading_ig.config import config
import scripts.utils as utils
import scripts.markets as markets
import argparse
from scripts.simple_backtesting import BacktestingEngine

class Strategy:
    """
    Classe mère pour les stratégies de trading.
    """
    def generate_signal(self, df, last_open_price, min_price_diff):
        """
        Analyse les données et retourne un signal ('BUY', 'SELL' ou None).
        """
        raise NotImplementedError("La méthode generate_signal doit être implémentée par une sous-classe.")
    
class TrendFollowingStrategy(Strategy):
    """
    Stratégie de suivi de tendance.
    """
    def should_open_position(self, direction, ema_300, stochastic, last_price, last_open_price, min_price_diff):
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
        # Récupérer les valeurs actuelles et précédentes de %K et %D
        k_current = stochastic['%K'].iloc[-1]
        d_current = stochastic['%D'].iloc[-1]
        k_previous = stochastic['%K'].iloc[-2]
        d_previous = stochastic['%D'].iloc[-2]
        
        if direction == 'BUY':
            return ema_300 < last_price and k_previous < d_previous and k_current > d_current and (last_open_price is None or abs(last_price - last_open_price) > min_price_diff)
        elif direction == 'SELL':
            return ema_300 > last_price and k_previous > d_previous and k_current < d_current and (last_open_price is None or abs(last_price - last_open_price) > min_price_diff)
        return False

    def generate_signal(self, df, last_open_price, min_price_diff):
        # Vérifier les colonnes nécessaires
        required_columns = {'High', 'Low', 'Close'}
        if not required_columns.issubset(df.columns):
            raise KeyError(f"Les colonnes requises sont manquantes dans les données : {required_columns - set(df.columns)}")
    
        # Calculer les indicateurs
        df['EMA_300'] = utils.calculate_ema(df['Close'], 300)
        stochastic = utils.calculate_stochastic(df, k_period=14, smoothing_period=3, d_period=3)
        df['ATR'] = utils.calculate_atr(df, period=14)
        last_price = df['Close'].iloc[-1]
        ema_300 = df['EMA_300'].iloc[-1]
        min_price_diff = df['ATR'].iloc[-1] * 2
        
        if len(stochastic) < 2:
            return 'HOLD'  # Pas assez de données pour détecter un croisement
    
        # Détecter les signaux
        if self.should_open_position('BUY', ema_300, stochastic, last_price, last_open_price, min_price_diff):
            return 'BUY'
        elif self.should_open_position('SELL', ema_300, stochastic, last_price, last_open_price, min_price_diff):
            return 'SELL'
        return None
    
class MovingAverageStrategy(Strategy):
    """
    Stratégie basée sur les moyennes mobiles et RSI.
    """
    def should_open_position(self, direction, ema_10, ema_30, rsi, last_price, last_open_price, min_price_diff):
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
        # Vérifier les colonnes nécessaires
        required_columns = {'High', 'Low', 'Close'}  # Changé en majuscules
        if not required_columns.issubset(df.columns):
            raise KeyError(f"Les colonnes requises sont manquantes dans les données : {required_columns - set(df.columns)}")
        
        df['EMA_10'] = utils.calculate_ema(df['Close'], 10)  # Changé de 'close' à 'Close'
        df['EMA_30'] = utils.calculate_ema(df['Close'], 30)  # Changé de 'close' à 'Close'
        df['RSI'] = utils.calculate_rsi(df['Close'])         # Changé de 'close' à 'Close'
        df['ATR'] = utils.calculate_atr(df, period=14)
        
        last_price = df['Close'].iloc[-1] # Changé de 'close' à 'Close'
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
    def __init__(self, strategy, backtest_mode=False):
        self.strategy = strategy
        self.backtest_mode = backtest_mode
        if not backtest_mode:
            self.ig_service = utils.initialize_service()
            self.epic, _ = utils.select_from_dict(markets.epics_dict)
        
        self.trade_size = 0.1
        self.sl = 0.002
        self.tp = 0.004
        self.last_open_price = None
    
    def fetch_market_data(self):
        """ Récupère les données de marché en temps réel. """
        return utils.fetch_prices(self.ig_service, self.epic)
    
    def execute_trade(self, direction, last_price):
        """ Exécute un ordre d'achat ou de vente. """
        if self.backtest_mode:
            print(f"Simulation d'un trade {direction} à {last_price}")
            return
        
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
        if self.backtest_mode:
            print("🔄 Démarrage du bot de trading en mode backtest...")
            return
        else:
            print("🔄 Démarrage du bot de trading en live...")
            
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

    def run_backtest(self, symbol="EURUSD=X", period="1y", interval="1h", cash=10000, 
                    commission=0.002, leverage=10, optimize=False, **kwargs):
        """
        Exécute un backtest de la stratégie actuelle
        
        :param symbol: Symbole du marché à tester
        :param period: Période historique à analyser
        :param interval: Intervalle des barres de prix
        :param cash: Capital initial
        :param commission: Frais de commission
        :param optimize: Activer l'optimisation des paramètres
        :param kwargs: Paramètres supplémentaires pour l'optimisation
        """
        print(f"🔄 Démarrage du backtesting pour {symbol}...")
        
        # Création du moteur de backtesting
        engine = BacktestingEngine(cash=cash * leverage, commission=commission)
        
        # Chargement des données
        data = engine.load_data(symbol, period=period, interval=interval)
        
        if data is not None:
            # Exécution du backtest
            bt, stats = engine.run_backtest(data, plot=True, optimize=optimize, **kwargs)
            
            # Sauvegarde des résultats
            #engine.save_results(stats, filename=f"backtest_{symbol.replace('=', '_')}_{period}_{interval}.csv")
            
            return bt, stats
        
        return None, None

def parse_arguments():
    """Parse les arguments de ligne de commande"""
    parser = argparse.ArgumentParser(description="Bot de trading IG Markets")
    parser.add_argument("--backtest", action="store_true", help="Exécuter en mode backtesting")
    parser.add_argument("--symbol", type=str, default="EURUSD=X", help="Symbole à trader/tester")
    parser.add_argument("--period", type=str, default="1y", help="Période pour le backtest")
    parser.add_argument("--interval", type=str, default="1h", help="Intervalle des barres de prix")
    parser.add_argument("--optimize", action="store_true", help="Optimiser les paramètres")
    return parser.parse_args()
    
if __name__ == "__main__":
    args = parse_arguments()
    strategy = MovingAverageStrategy()
    #strategy = TrendFollowingStrategy()
    if args.backtest:
        bot = TradingBot(strategy, backtest_mode=True)
        bt, stats = bot.run_backtest(
            symbol=args.symbol,
            period=args.period,
            interval=args.interval,
            optimize=args.optimize
        )
        if stats is not None:
            print("🔄 Backtest terminé avec succès!")
            print(f"Rendement: {stats['Return [%]']:.2f}%")
            print(f"Ratio de Sharpe: {stats['Sharpe Ratio']:.2f}")
            print(f"Max. Drawdown: {stats['Max. Drawdown [%]']:.2f}%")
            print(f"Trades gagnants: {int(stats['# Trades'])} ({stats['Win Rate [%]']:.1f}%)")
        else:
            bot = TradingBot(strategy)
            bot.run()
