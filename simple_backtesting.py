import pandas as pd
import numpy as np
from backtesting import Backtest, Strategy
import yfinance as yf
import matplotlib.pyplot as plt

class BacktestingAdapter(Strategy):
    """
    Adaptateur pour utiliser notre stratégie existante avec backtesting.py
    """
    # Paramètres par défaut (peuvent être modifiés lors de l'optimisation)
    sl_percent = 0.002
    tp_percent = 0.004
    
    def init(self):
        """Initialisation des indicateurs pour le backtesting"""
        # Import différé pour éviter les imports circulaires
        from bot_v2 import MovingAverageStrategy
        self.strategy = MovingAverageStrategy()
        
    def next(self):
        """Exécuté à chaque barre de prix"""
        # Préparation des données pour notre stratégie
        df = pd.DataFrame({
            'Open': self.data.Open,
            'High': self.data.High,
            'Low': self.data.Low,
            'Close': self.data.Close,
            'Volume': self.data.Volume if hasattr(self.data, 'Volume') else np.zeros(len(self.data.Close))
        })
        
        # Afficher les colonnes pour débogage
        #print(f"Colonnes du DataFrame dans next : {df.columns.tolist()}")
        
        # Récupération du dernier prix
        last_price = self.data.Close[-1]
        
        # Détermination du prix d'ouverture de la dernière position
        last_open_price = None
        if self.trades:  # Vérifie si une position est ouverte
            last_open_price = self.trades[-1].entry_price # Utilise self.position.entry pour accéder au prix d'entrée
        
        # Génération du signal en utilisant notre stratégie existante
        signal = self.strategy.generate_signal(df, last_open_price, min_price_diff=None)
        
        # Traitement du signal
        if signal == 'BUY' and self.position.size == 0:  # Vérifie qu'aucune position n'est ouverte
            sl = last_price * (1 - self.sl_percent)
            tp = last_price * (1 + self.tp_percent)
            self.buy(sl=sl, tp=tp)
            
        elif signal == 'SELL' and self.position.size == 0:  # Vérifie qu'aucune position n'est ouverte
            sl = last_price * (1 + self.sl_percent)
            tp = last_price * (1 - self.tp_percent)
            self.sell(sl=sl, tp=tp)
            
class BacktestingEngine:
    """
    Moteur de backtesting pour notre bot de trading
    """
    def __init__(self, cash=10000, commission=0.002):
        self.cash = cash
        self.commission = commission
        
    def load_data(self, symbol, period='1y', interval='1h', source='yahoo'):
        """
        Charge les données historiques pour le backtesting
        """
        if source == 'yahoo':
            data = yf.download(symbol, period=period, interval=interval)
            print(f"Données chargées: {len(data)} barres de prix")
            
            # Aplatir les colonnes si elles sont des tuples
            if isinstance(data.columns, pd.MultiIndex):
                data.columns = [col[0] for col in data.columns]
            
            # Ajouter une colonne Volume si elle est absente
            if 'Volume' not in data.columns:
                data['Volume'] = 0
            
            # Utiliser Adj Close si Close est absent
            if 'Close' not in data.columns and 'Adj Close' in data.columns:
                data['Close'] = data['Adj Close']
            
            # Reformater les colonnes pour correspondre à ce que backtesting.py attend
            data = data.rename(columns={
                'Open': 'Open',
                'High': 'High',
                'Low': 'Low',
                'Close': 'Close',
                'Volume': 'Volume'
            })
            
            # Réinitialiser l'index et convertir en DateTimeIndex
            data = data.reset_index()
            if 'Date' in data.columns:  # Vérifiez si une colonne Date existe après reset_index
                data['Date'] = pd.DatetimeIndex(data['Date'])  # Convertir en datetime
                data = data.set_index('Date')  # Définir la colonne Date comme index

            # Supprimer les colonnes inutiles
            data = data[['Open', 'High', 'Low', 'Close', 'Volume']]
            
            # Vérifier si des colonnes manquent
            required_columns = {'Open', 'High', 'Low', 'Close', 'Volume'}
            if not required_columns.issubset(data.columns):
                raise ValueError(f"Les colonnes requises sont manquantes dans les données : {required_columns - set(data.columns)}")
            
            return data
        else:
            raise NotImplementedError("Source de données personnalisée non implémentée")
    
    def run_backtest(self, data, plot=True, optimize=True, **kwargs):
        """
        Exécute le backtest avec les données fournies
        
        :param data: DataFrame avec les données OHLC
        :param plot: Afficher le graphique des résultats
        :param optimize: Optimiser les paramètres
        :param kwargs: Paramètres supplémentaires pour l'optimisation
        :return: Résultats du backtest
        """
        if data.empty:
            raise ValueError("Les données fournies sont vides.")
        
        # Vérifier que les colonnes nécessaires sont présentes
        required_columns = {'Open', 'High', 'Low', 'Close', 'Volume'}
        if not required_columns.issubset(data.columns):
            raise ValueError(f"Les colonnes requises sont manquantes dans les données : {required_columns - set(data.columns)}")
        
        # Vérifier que l'index est simple
        if isinstance(data.index, pd.MultiIndex):
            raise ValueError("L'index des données ne doit pas être un MultiIndex. Réinitialisez l'index.")
        
        try:
            data.index = pd.DatetimeIndex(data.index)
        except Exception as e:
            print(f"Erreur lors de la conversion de l'index en DateTimeIndex: {e}")
            print("Tentative de conversion en utilisant to_datetime...")
            data.index = pd.to_datetime(data.index)
                
        # Exécuter le backtest
        bt = Backtest(data, BacktestingAdapter, 
                      cash=self.cash, 
                      commission=self.commission)
                
        if optimize:
            # Paramètres par défaut pour l'optimisation
            optimization_params = {
                'sl_percent': np.arange(0.001, 0.005, 0.001),
                'tp_percent': np.arange(0.002, 0.01, 0.002)
            }
            
            # Ajouter les paramètres supplémentaires
            optimization_params.update(kwargs)
            
            # Exécuter l'optimisation
            stats = bt.optimize(**optimization_params, 
                                maximize='Return [%]')
            print("Paramètres optimisés:")
            print(stats._strategy)
        else:
            # Exécuter le backtest avec les paramètres par défaut
            stats = bt.run()
                
        # Afficher les résultats
        print(stats)
        if plot:
            # Afficher le graphique
            bt.plot()
                
        return bt, stats
    
    def compare_strategies(self, data, strategies_params):
        """
        Compare plusieurs ensembles de paramètres pour la stratégie
        
        :param data: DataFrame avec les données OHLC
        :param strategies_params: Liste de dictionnaires de paramètres
        :return: Liste des résultats
        """
        results = []
        
        for i, params in enumerate(strategies_params):
            print(f"Test de la stratégie {i+1} avec paramètres: {params}")
            bt = Backtest(data, BacktestingAdapter, 
                         cash=self.cash, 
                         commission=self.commission,
                         **params)
            stats = bt.run()
            results.append((bt, stats, params))
            print(f"Rendement: {stats['Return [%]']:.2f}%")
            print("-" * 40)
        
        return results
    
    def save_results(self, stats, filename="backtest_results.csv"):
        """
        Sauvegarde les résultats du backtest
        
        :param stats: Résultats du backtest
        :param filename: Nom du fichier de sauvegarde
        """
        # Convertir les résultats en DataFrame
        results_df = pd.DataFrame(stats._trades)
        results_df.to_csv(filename)
        print(f"Résultats sauvegardés dans {filename}")

if __name__ == "__main__":
    # Exemple d'utilisation
    engine = BacktestingEngine(cash=10000, commission=0.002)
    
    # Chargement des données
    data = engine.load_data("EURUSD=X", period='1y', interval='1h')
    
    if data is not None:
        # Exécution du backtest
        bt, stats = engine.run_backtest(data, plot=True)
        
        # Sauvegarde des résultats
        engine.save_results(stats)
        
        # Optimisation (décommentez pour exécuter)
        bt_opt, stats_opt = engine.run_backtest(data, optimize=True)
        
        # Comparaison de différents paramètres
        strategies_to_compare = [
            {'sl_percent': 0.001, 'tp_percent': 0.002},
            {'sl_percent': 0.002, 'tp_percent': 0.004},
            {'sl_percent': 0.003, 'tp_percent': 0.006}
        ]
        results = engine.compare_strategies(data, strategies_to_compare)