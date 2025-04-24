import pandas as pd
import numpy as np
import datetime
import time
import logging
import matplotlib.pyplot as plt
import seaborn as sns
from tabulate import tabulate

from igtrader.Strategies.CrossEMA import CrossEMA, CrossEMAConfig
from igtrader.Strategies.BuyTrendFollowing import BuyTrendFollowing, BuyTrendConfig
from igtrader.Strategies.SellTrendFollowing import SellTrendFollowing, SellTrendConfig
from igtrader.Strategies.BuyHeikinGreen import BuyHeikinGreen, BuyHeikinGreenConfig
from igtrader.Strategies.Strategy import StrategyBaseConfig

def generate_candles(num_candles=1000, start_date=None, freq='1min'):
    """
    Génère un ensemble de bougies pour le benchmark.
    
    :param num_candles: Nombre de bougies à générer
    :param start_date: Date de début (par défaut: maintenant - num_candles*minutes)
    :param freq: Fréquence des bougies (par défaut: 1 minute)
    :return: DataFrame avec les bougies
    """
    if start_date is None:
        start_date = datetime.datetime.now() - datetime.timedelta(minutes=num_candles)
    
    # Générer les dates
    dates = pd.date_range(start=start_date, periods=num_candles, freq=freq)
    
    # Générer des prix aléatoires mais réalistes
    base_price = 15000  # Prix de base (comme pour le NASDAQ)
    volatility = 50     # Volatilité (écart standard des mouvements)
    
    # Générer les mouvements de prix avec une marche aléatoire
    price_changes = np.random.normal(0, volatility, num_candles)
    cumulative_changes = np.cumsum(price_changes)
    
    # Générer les prix OHLC
    prices = pd.DataFrame()
    prices['date'] = dates
    prices['Close'] = base_price + cumulative_changes
    
    # Générer Open, High, Low à partir de Close avec une certaine volatilité intraday
    intraday_vol = volatility * 0.3
    prices['Open'] = prices['Close'].shift(1, fill_value=base_price) + np.random.normal(0, intraday_vol, num_candles)
    prices['High'] = np.maximum(prices[['Open', 'Close']].max(axis=1) + abs(np.random.normal(0, intraday_vol, num_candles)), 
                               prices[['Open', 'Close']].max(axis=1))
    prices['Low'] = np.minimum(prices[['Open', 'Close']].min(axis=1) - abs(np.random.normal(0, intraday_vol, num_candles)),
                              prices[['Open', 'Close']].min(axis=1))
    
    # S'assurer que High est toujours le plus élevé et Low le plus bas
    prices['High'] = np.maximum(prices['High'], prices[['Open', 'Close']].max(axis=1))
    prices['Low'] = np.minimum(prices['Low'], prices[['Open', 'Close']].min(axis=1))
    
    return prices

def run_benchmark(strategy, candles, batch_size=1):
    """
    Exécute le benchmark en envoyant des bougies à la stratégie.
    
    :param strategy: L'instance de stratégie à tester
    :param candles: DataFrame contenant les bougies de test
    :param batch_size: Nombre de bougies à envoyer à chaque itération
    """
    # Initialiser avec un ensemble de bougies historiques
    historical_size = min(50, len(candles) // 5)  # 20% des bougies pour l'initialisation
    historical_candles = candles.iloc[:historical_size].copy()
    
    strategy.initialize(historical_candles)
    
    # Activer le benchmark
    strategy.start_benchmark()
    
    # Traiter les bougies restantes
    remaining_candles = candles.iloc[historical_size:].copy()
    
    for i in range(0, len(remaining_candles), batch_size):
        batch = remaining_candles.iloc[i:i+batch_size]
        
        for _, candle in batch.iterrows():
            # Convertir la Series en dictionnaire
            candle_dict = candle.to_dict()
            
            # Mise à jour de la stratégie avec la nouvelle bougie
            strategy.update_candle(candle_dict)
    
    # Désactiver le benchmark
    strategy.stop_benchmark()
    
    return strategy.get_benchmark_stats()

def display_results(stats):
    """Affiche les résultats du benchmark."""
    print("\n=== RÉSULTATS DU BENCHMARK ===")
    
    # Tableau de statistiques
    table = [
        ["Nombre de bougies", stats["count"]],
        ["Temps d'exécution moyen", f"{stats['mean']:.2f} ms"],
        ["Temps d'exécution médian", f"{stats['median']:.2f} ms"],
        ["Temps minimum", f"{stats['min']:.2f} ms"],
        ["Temps maximum", f"{stats['max']:.2f} ms"],
        ["95ème percentile", f"{stats['p95']:.2f} ms"],
        ["99ème percentile", f"{stats['p99']:.2f} ms"],
        ["Temps total", f"{stats['total']:.2f} ms ({stats['total']/1000:.2f} s)"]
    ]
    
    print(tabulate(table, headers=["Métrique", "Valeur"], tablefmt="grid"))
    
    # Si vous souhaitez ajouter des visualisations avec matplotlib, vous pouvez le faire ici
    if stats["count"] > 0 and "execution_times" in stats:
        plt.figure(figsize=(12, 6))
        
        # Histogramme des temps d'exécution
        plt.subplot(1, 2, 1)
        
        # Filtrer les données pour n'inclure que celles jusqu'au 99ème percentile
        filtered_times = [t for t in stats["execution_times"] if 0.2 <= t <= stats['p99']]
        sns.histplot(filtered_times, kde=True)
        plt.title("Distribution des temps d'exécution")
        plt.xlabel("Temps (ms)")
        plt.ylabel("Fréquence")
        
        # Graphique d'évolution des temps d'exécution
        plt.subplot(1, 2, 2)
        plt.plot(stats["execution_times"])
        plt.title("Évolution des temps d'exécution")
        plt.xlabel("Itération")
        plt.ylabel("Temps (ms)")
        plt.xlim(0, len(stats["execution_times"]) - 1)  # Définir l'axe x pour commencer à 0
        plt.yscale('log')  # Échelle logarithmique pour l'axe y
        
        plt.tight_layout()
        plt.savefig("benchmark_results.png")
        print("\nGraphique sauvegardé sous 'benchmark_results.png'")

def main():
    # Configuration du logging
    logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
    
    # Paramètres du benchmark
    num_candle_2_y = 589680 # bougies de 20 secondes sur 2 ans
    num_candle_6_m = num_candle_2_y // 4 # bougies de 20 secondes sur 6 mois
    num_candle_1_m = num_candle_2_y // 24 # bougies de 20 secondes sur 1 mois

    num_candles = num_candle_1_m
    
    # Générer les données de test
    logging.info(f"Génération de {num_candles} bougies pour le test...")
    candles = generate_candles(num_candles)
    
    # Créer la stratégie
    base_config = StrategyBaseConfig(
        trading_from=datetime.time(0, 0),  # Toute la journée pour le benchmark
        trading_to=datetime.time(23, 59),
        trading_days=[0, 1, 2, 3, 4, 5, 6],  # Tous les jours pour le benchmark
        take_profit_distance=30,
        stop_loss_distance=20,
    )
    crossEMA_config = CrossEMAConfig(
        ema_short_period=50,
        ema_long_period=200,
    )
    
    # strategy = CrossEMA(base_config, crossEMA_config)
    # strategy = BuyTrendFollowing(base_config, BuyTrendConfig())
    strategy = BuyHeikinGreen(base_config, BuyHeikinGreenConfig())
    
    # Exécuter le benchmark
    logging.info("Démarrage du benchmark...")
    start_time = time.time()
    stats = run_benchmark(strategy, candles)
    total_time = time.time() - start_time
    
    # Ajouter le temps d'exécution total pour la visualisation
    stats["execution_times"] = strategy.execution_times
    
    # Afficher les résultats
    logging.info(f"Benchmark terminé en {total_time:.2f} secondes")
    display_results(stats)

if __name__ == "__main__":
    main()