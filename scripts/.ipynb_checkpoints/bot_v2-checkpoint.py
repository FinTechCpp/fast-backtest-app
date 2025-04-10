import time
import pandas as pd
import numpy as np
import sys
import os
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
import scripts.utils as utils
import scripts.markets as markets
import argparse
from scripts.simple_backtesting import BacktestingEngine
import cProfile
import pstats
import threading
'''

1. Stochastic(10,7,3) : k croise d à la baisse
2. Stochastic[d] > 50 
3. prix de clôture < SuperTrend[50,100]
4. prix de clôture < EMA(50)
5. prix de clôture < EMA(200)
'''
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
    def __init__(self, direction=None):
        """
        Initialise la stratégie avec une direction spécifique.
        :param direction: 'buy', 'sell' ou None (pour les deux directions)
        """
        self.direction = direction  
              
    def should_open_position(self, direction, values, last_open_price, min_price_diff):
        """Version optimisée pour réduire les accès aux données"""
        if direction == 'BUY':
            # Exemples de conditions symétriques inversées par rapport à la logique de vente
            if values['k_current'] <= values['d_current'] or values['k_previous'] >= values['d_previous']:
                return False
            if values['d_current'] >= 50:
                return False
            if values['last_price'] <= values['st_50_3']:
                return False
            if values['last_price'] <= values['ema_50']:
                return False
            if values['last_price'] <= values['ema_200']:
                return False
            
            return (last_open_price is None or 
                    abs(values['last_price'] - last_open_price) > min_price_diff)
                
        elif direction == 'SELL':
            # Utiliser une formule booléenne simplifiée (éviter l'évaluation inutile)
            # Vérifier d'abord les conditions qui échouent le plus souvent
            if values['k_current'] >= values['d_current'] or values['k_previous'] <= values['d_previous']:
                return False
            if values['d_current'] <= 50:
                return False
            if values['last_price'] >= values['st_50_3']:
                return False
            if values['last_price'] >= values['ema_50']:
                return False
            if values['last_price'] >= values['ema_200']:
                return False

            # Dernière vérification sur last_open_price
            return (last_open_price is None or 
                    abs(values['last_price'] - last_open_price) > min_price_diff)
        
        return False

    def generate_signal(self, data, index, last_open_price, min_price_diff):
        if not hasattr(self, '_arrays') or index >= len(self._arrays.get('Close', [])):
            # Précharger tous les arrays en une seule fois
            self._arrays = {
                'Close': data.Close,  
                'EMA_200': data.EMA_200,
                'EMA_50': data.EMA_50,
                'st_50_3': data.st_50_3,
                'ATR': data.ATR,
                'stoch_k': data.stoch_k, 
                'stoch_d': data.stoch_d
            }
            
        # Mettre à jour les valeurs en cache pour l'index actuel
        self._cached_values = {
            'last_price': self._arrays['Close'][index],
            'ema_200': self._arrays['EMA_200'][index],
            'ema_50': self._arrays['EMA_50'][index],
            'st_50_3': self._arrays['st_50_3'][index],
            'atr': self._arrays['ATR'][index],
            'k_current': self._arrays['stoch_k'][index],
            'd_current': self._arrays['stoch_d'][index],
            'k_previous': self._arrays['stoch_k'][index-1] if index > 0 else None,
            'd_previous': self._arrays['stoch_d'][index-1] if index > 0 else None
        }
        
        # Utiliser les valeurs en cache
        values = self._cached_values
        min_price_diff = values['atr'] * 2
        
        # Code de détection de signal
        if len(data.stoch_k) < 2:  # Utilisez data.stoch_k au lieu de data['stoch_k']
            return 'HOLD'
            
        if ((self.direction is None or self.direction == 'buy') and 
            self.should_open_position('BUY', values, last_open_price, min_price_diff)):
            return 'BUY'
        elif ((self.direction is None or self.direction == 'sell') and
              self.should_open_position('SELL', values, last_open_price, min_price_diff)):
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
            from trading_ig.rest import IGService
            from trading_ig.config import config
            self.ig_service = utils.initialize_service()
            self.epic, _ = utils.select_from_dict(markets.epics_dict)
        else:
            self.ig_service = None
            self.epic = None
            
        self.trade_size = 1
        self.sl = 0.002
        self.tp = 0.001
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

    def run_backtest(self, symbol="NDX", period="1y", interval="1_min", cash=10000, 
                    commission=0.0, leverage=10, spread=0.0002, optimize=False, **kwargs):
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
        engine = BacktestingEngine(cash=cash * leverage, commission=commission, spread=spread)
        
        # Chargement des données
        data = engine.load_data(symbol, period=period, interval=interval)
        
        if data is not None:           
            # Exécution du backtest
            bt, stats = engine.run_backtest(data, plot=True, optimize=optimize, **kwargs)
            
            # Sauvegarde des résultats
            engine.save_results(stats, filename=f"backtest_{symbol.replace('=', '_')}_{period}_{interval}.csv")
            
            return bt, stats
        
        return None, None
    
def parse_arguments():
    """Parse les arguments de ligne de commande"""
    parser = argparse.ArgumentParser(description="Bot de trading IG Markets")
    parser.add_argument("--backtest", action="store_true", help="Exécuter en mode backtesting")
    parser.add_argument("--symbol", type=str, default="NDX", help="Symbole à trader/tester")
    parser.add_argument("--period", type=str, default="1y", help="Période pour le backtest")
    parser.add_argument("--interval", type=str, default="1_min", help="Intervalle des barres de prix")
    parser.add_argument("--optimize", action="store_true", help="Optimiser les paramètres")
    parser.add_argument("--direction", type=str, choices=["buy", "sell"], help="Direction de la position")
    return parser.parse_args()
    
if __name__ == "__main__":
    args = parse_arguments()
    strategy = TrendFollowingStrategy(direction=args.direction)
    if args.backtest:
        bot = TradingBot(strategy, backtest_mode=True)
        profiler = cProfile.Profile()
        profiler.enable()
        bt, stats = bot.run_backtest(
            symbol=args.symbol,
            period=args.period,
            interval=args.interval,
            optimize=args.optimize,
            direction=args.direction,
        )
        profiler.disable()
        #stats = pstats.Stats(profiler).sort_stats('cumtime')
        #stats.print_stats(20)
        if stats is not None:
            print("🔄 Backtest terminé avec succès!")
            print(f"Max. Drawdown: {stats['Max. Drawdown [%]']:.2f}%")
            print(f"Trades gagnants: {int(stats['# Trades'])} ({stats['Win Rate [%]']:.1f}%)")
        else:
            bot = TradingBot(strategy)
            bot.run()
