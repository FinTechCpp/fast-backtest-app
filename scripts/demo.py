from igtrader.backtestingpy.backtesting import Strategy, Backtest
import numpy as np
from collections import deque
import pandas as pd
import os

class SmaCrossStrategy(Strategy):
    """
    Stratégie de croisement de moyennes mobiles similaire à l'implémentation C++
    """
    fast_period = 10
    slow_period = 30
    
    def init(self):
        # Initialisation des tampons circulaires pour les calculs
        self._fast_values = deque(maxlen=self.fast_period)
        self._slow_values = deque(maxlen=self.slow_period)
        
        # Variables pour stocker les valeurs SMA actuelles et précédentes
        self._last_fast_sma = np.nan
        self._current_fast_sma = np.nan
        self._last_slow_sma = np.nan
        self._current_slow_sma = np.nan
        
        # Compteur de barres traitées
        self._bars_processed = 0
        
        print(f"Stratégie initialisée avec FastSMA({self.fast_period}) et SlowSMA({self.slow_period})")
    
    def next(self):
        current_price = self.data.Close[-1]
        
        # Mettre à jour les tampons circulaires
        self._fast_values.append(current_price)
        self._slow_values.append(current_price)
        
        self._bars_processed += 1
        
        # Calculer les SMAs si nous avons assez de données
        self._last_fast_sma = self._current_fast_sma  # Sauvegarder la valeur précédente
        self._last_slow_sma = self._current_slow_sma  # Sauvegarder la valeur précédente
        
        if self._bars_processed >= self.fast_period:
            self._current_fast_sma = sum(self._fast_values) / self.fast_period
        
        if self._bars_processed >= self.slow_period:
            self._current_slow_sma = sum(self._slow_values) / self.slow_period
        
        # Attendre que les deux moyennes mobiles soient disponibles
        if (np.isnan(self._current_fast_sma) or np.isnan(self._current_slow_sma) or 
            np.isnan(self._last_fast_sma) or np.isnan(self._last_slow_sma)):
            return
        
        # Vérifier les croisements
        crossover = self._last_fast_sma <= self._last_slow_sma and self._current_fast_sma > self._current_slow_sma
        crossunder = self._last_fast_sma >= self._last_slow_sma and self._current_fast_sma < self._current_slow_sma
        
        # Afficher des informations de débogage sur chaque barre (suppression de la condition i % 25 == 0)
        i = len(self.data) - 1
        print(f"Bar {i}: Price={current_price:.4f}, FastSMA={self._current_fast_sma:.4f}, SlowSMA={self._current_slow_sma:.4f}")
        
        # Logique de trading - déjà avec le bon format de logs
        if crossover:
            print(f"Crossover détecté à la barre {i}, tentative d'achat...")
            # Acheter si croisement vers le haut et pas de position longue
            if not self.position:
                print(f"Pas de position, exécution de l'achat...")
                # SL à 1 point, TP à 3 points
                self.buy(size=1.0, sl_points=1, tp_points=3, tag="Crossover")
                print(f"Signal d'achat à la barre {i}, prix: {current_price:.4f}")
            elif self.position.size < 0:
                print(f"Fermeture de la position courte...")
                self.position.close()
                print(f"Ouverture d'une position longue...")
                self.buy(size=1.0, sl_points=1, tp_points=3, tag="Crossover")
                print(f"Signal d'achat à la barre {i}, prix: {current_price:.4f}")

        elif crossunder:
            print(f"Crossunder détecté à la barre {i}, tentative de vente...")
            # Vendre si croisement vers le bas et pas de position courte
            if not self.position:
                print(f"Pas de position, exécution de la vente...")
                self.sell(size=1.0, sl_points=1, tp_points=3, tag="Crossunder")
                print(f"Signal de vente à la barre {i}, prix: {current_price:.4f}")
            elif self.position.size > 0:
                print(f"Fermeture de la position longue...")
                self.position.close()
                print(f"Ouverture d'une position courte...")
                self.sell(size=1.0, sl_points=1, tp_points=3, tag="Crossunder")
                print(f"Signal de vente à la barre {i}, prix: {current_price:.4f}")



def format_stats(stats):
    """Formatter les statistiques de manière similaire à la sortie C++"""
    print("\n============== BACKTEST RESULTS ==============")
    
    # Statistiques de performance principales
    print(f"Equity Final [$]: {stats['Equity Final [$]']:.2f}")
    print(f"Equity Peak [$]: {stats['Equity Peak [$]']:.2f}")
    print(f"Return [%]: {stats['Return [%]']:.2f}")
    print(f"Buy & Hold Return [%]: {stats['Buy & Hold Return [%]']:.2f}")
    
    # Statistiques de risque
    print("\n-------------- RISK METRICS --------------")
    print(f"Max. Drawdown [%]: {stats['Max. Drawdown [%]']:.2f}")
    print(f"Sharpe Ratio: {stats['Sharpe Ratio']:.2f}")
    print(f"Sortino Ratio: {stats['Sortino Ratio']:.2f}")
    print(f"Calmar Ratio: {stats['Calmar Ratio']:.2f}")
    
    # Statistiques des trades
    print("\n-------------- TRADE STATISTICS --------------")
    print(f"# Trades: {stats['# Trades']:.2f}")
    print(f"Win Rate [%]: {stats['Win Rate [%]']:.2f}")
    print(f"Best Trade [%]: {stats['Best Trade [%]']:.2f}")
    print(f"Worst Trade [%]: {stats['Worst Trade [%]']:.2f}")
    print(f"Avg. Trade [%]: {stats['Avg. Trade [%]']:.2f}")
    print(f"Profit Factor: {stats['Profit Factor']:.2f}")
    print(f"SQN: {stats['SQN']:.2f}")

    # Stat sur les trades details
    print("\n-------------- TRADE DETAILS --------------")
    print(stats['_trades'])

    print("==============================================")

def main():
    # Vérifier si le fichier de données existe, sinon le générer
    data_file = "data/synthetic_data.csv"
    if not os.path.exists(data_file):
        print("Fichier de données non trouvé, génération de données synthétiques...")
        import generate_synthetic_data
        generate_synthetic_data.main()
    
    # Charger les données
    print("Chargement des données...")
    data = pd.read_csv(data_file)
    data['Date'] = pd.to_datetime(data['Date'])
    data.set_index('Date', inplace=True)
    print(f"Données chargées avec {len(data)} barres")
    
    # Configurer et exécuter le backtest
    print("Création et exécution du backtest...")
    bt = Backtest(
        data=data,
        strategy=SmaCrossStrategy,
        cash=10000.0,
        commission=0.001,
        margin=1.0,
        trade_on_close=False,
        hedging=False,
        exclusive_orders=False
    )
    
    # Exécuter le backtest
    stats = bt.run()
    
    # Afficher les résultats
    format_stats(stats)
    
    # Facultatif: générer un graphique
    bt.plot(filename="python_backtest_result.html", open_browser=True)
    print("Graphique de backtest sauvegardé dans python_backtest_result.html")

if __name__ == "__main__":
    main()