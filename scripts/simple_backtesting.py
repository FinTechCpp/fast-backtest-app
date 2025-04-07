import pandas as pd
import numpy as np
from backtesting import Backtest, Strategy
import matplotlib.pyplot as plt
import pyarrow.parquet as pq
import os
import glob
import scripts.utils as utils
import talib
import time

class BacktestingAdapter(Strategy):
    """
    Adaptateur pour utiliser notre stratégie existante avec backtesting.py
    """
    # Paramètres par défaut (peuvent être modifiés lors de l'optimisation)
    sl_percent = 0.003
    tp_percent = 0.004
    #sl_absolute = 10
    #tp_absolute = 10
    
    def init(self):
        """Initialisation des indicateurs pour le backtesting"""
        # Import différé pour éviter les imports circulaires
        from scripts.bot_v2 import TrendFollowingStrategy
        self.strategy = TrendFollowingStrategy(direction=self.direction)
        
    def next(self):
        """Exécuté à chaque barre de prix"""

        # Récupération du dernier prix
        last_price = self.data.Close[-1]
        
        # Détermination du prix d'ouverture de la dernière position
        last_open_price = None
        if self.trades:  # Vérifie si une position est ouverte
            last_open_price = self.trades[-1].entry_price # Utilise self.position.entry pour accéder au prix d'entrée
            
        index = len(self.data.Close) - 1
        
        # Génération du signal en utilisant notre stratégie existante
        signal = self.strategy.generate_signal(self.data, index, last_open_price, None)
        
        # Traitement du signal
        if signal == 'BUY' and self.position.size == 0:  # Vérifie qu'aucune position n'est ouverte
            
            sl = last_price * (1 - self.sl_percent)
            tp = last_price * (1 + self.tp_percent)
            
            #sl = last_price - self.sl_absolute  # SL en valeur absolue
            #tp = last_price + self.tp_absolute  # TP en valeur absolue
            self.buy(size=0.5, sl=sl, tp=tp)
            
        elif signal == 'SELL' and self.position.size == 0:  # Vérifie qu'aucune position n'est ouverte
            sl = last_price * (1 + self.sl_percent)
            tp = last_price * (1 - self.tp_percent)
            
            #sl = last_price + self.sl_absolute  # SL en valeur absolue
            #tp = last_price - self.tp_absolute  # TP en valeur absolue
            self.sell(size=0.5, sl=sl, tp=tp)
            
class BacktestingEngine:
    """
    Moteur de backtesting pour notre bot de trading
    """
    def __init__(self, cash, commission, spread):
        self.cash = cash
        self.commission = commission
        self.spread = spread
        
    def load_data(self, symbol, period='', interval='', source='parquet'):
        """
        Charge les données historiques pour le backtesting à partir d'un fichier Parquet
        et filtre les données en fonction de la période spécifiée.
        """
        if source == 'parquet':
            chrono_load_data = time.time()
            print(f"Chargement des données pour {symbol} et calcul des indicateurs techniques ...")
            
            # Calculer la période de début et de fin
            end_date = pd.Timestamp.now(tz='US/Eastern')  # Ensure timezone matches the Parquet file
            if period.endswith('y'):  # Années
                start_date = end_date - pd.DateOffset(years=int(period[:-1]))
            elif period.endswith('m'):  # Mois
                start_date = end_date - pd.DateOffset(months=int(period[:-1]))
            elif period.endswith('d'):  # Jours
                start_date = end_date - pd.DateOffset(days=int(period[:-1]))
            else:
                raise ValueError(f"Période non reconnue : {period}. Utilisez '1y', '6m', '30d', etc.")
            # Rechercher le fichier correspondant au symbole et à l'intervalle
            pattern = f"../database/{symbol}_{interval.replace('_', '')}*.parquet"
            matching_files = glob.glob(pattern)
            
            if not matching_files:
                raise ValueError(f"Aucun fichier trouvé correspondant à {pattern}")
            
            # Utiliser le fichier le plus récent si plusieurs fichiers correspondent
            save_path = max(matching_files, key=os.path.getctime)
            
            print(f"Fichier trouvé: {save_path}")
            print(f"Filtrage des données pour la période {start_date.date()} à {end_date.date()}...")
            
            # Charger tout le fichier Parquet
            try:
                df = pd.read_parquet(save_path)
                print(f"Données chargées depuis {save_path}: {len(df)} barres de prix")
            except FileNotFoundError:
                raise ValueError(f"Le fichier {save_path} est introuvable.")
            except Exception as e:
                raise ValueError(f"Erreur lors du chargement des données : {e}")
            
            # Vérifier que l'index est un DateTimeIndex
            if not isinstance(df.index, pd.DatetimeIndex):
                try:
                    df['date'] = pd.to_datetime(df['date'])  # Convertir la colonne 'date' en datetime
                    df = df.set_index('date')  # Définir 'date' comme index
                except Exception as e:
                    raise ValueError(f"Erreur lors de la conversion de l'index en DateTimeIndex : {e}")
            
            # Ajouter un fuseau horaire explicite si nécessaire
            if df.index.tz is None:
                df.index = df.index.tz_localize('UTC')  # Remplacez 'UTC' par le fuseau horaire approprié si nécessaire
            
            # S'assurer que les dates de filtrage sont dans le même fuseau horaire que l'index
            start_date = start_date.tz_convert(df.index.tz)
            end_date = end_date.tz_convert(df.index.tz)
            
            # Filtrer les données en fonction de la période
            df = df[(df.index >= start_date) & (df.index <= end_date)]
            print(f"Données filtrées pour la période {start_date.date()} à {end_date.date()}: {len(df)} barres de prix")
            
            df.index = df.index.tz_convert(None)  # Supprime le fuseau horaire explicite
            
            # Vérifier les colonnes nécessaires
            required_columns = {'open', 'high', 'low', 'close', 'barCount'}
            if not required_columns.issubset(df.columns):
                raise ValueError(f"Les colonnes requises sont manquantes dans les données : {required_columns - set(df.columns)}")
            
            # Renommer les colonnes pour correspondre au format attendu
            df = df.rename(columns={
                'open': 'Open',
                'high': 'High',
                'low': 'Low',
                'close': 'Close',
                'barCount': 'Volume'  # Utiliser 'barCount' comme substitut pour 'Volume'
            })
            
            # Garder uniquement les colonnes nécessaires
            df = df[['Open', 'High', 'Low', 'Close', 'Volume']]
            
            # Calcul des indicateurs techniques
            df['EMA_200'] = talib.EMA(df['Close'].values, timeperiod=200)
            df['EMA_50'] = talib.EMA(df['Close'].values, timeperiod=50)
            df['ATR'] = talib.ATR(df['High'].values, df['Low'].values, df['Close'].values, timeperiod=14)
            
            # Stochastique
            df['stoch_k'], df['stoch_d'] = talib.STOCH(
                df['High'].values, df['Low'].values, df['Close'].values,
                fastk_period=10, slowk_period=7, slowd_period=3
            )
            
            st_result = utils.calculate_supertrend(df, atr_period=50, multiplier=3)
            df['st_50_3'] = st_result['SuperTrend']
            chrono_load_data = time.time() - chrono_load_data
            print(f"Données chargées et prétraitées en {chrono_load_data:.2f} secondes")
            return df
        else:
            raise NotImplementedError("Source de données non supportée. Utilisez 'parquet'.")
        
    def run_backtest(self, data, plot=True, optimize=True, **kwargs):
        """
        Exécute le backtest avec les données fournies
        """
        if data.empty:
            raise ValueError("Les données fournies sont vides.")
        
        # Vérifier que les colonnes nécessaires sont présentes
        required_columns = {'Open', 'High', 'Low', 'Close', 'Volume'}
        if not required_columns.issubset(data.columns):
            raise ValueError(f"Les colonnes requises sont manquantes dans les données : {required_columns - set(data.columns)}")
        
        # Vérifier que l'index est un DateTimeIndex
        if not isinstance(data.index, pd.DatetimeIndex):
            try:
                data.index = pd.to_datetime(data.index)
            except Exception as e:
                raise ValueError(f"Erreur lors de la conversion de l'index en DateTimeIndex : {e}")
            
        direction = kwargs.pop('direction', None)
        BacktestingAdapter.direction = direction
        # Exécuter le backtest
        bt = Backtest(data, BacktestingAdapter, 
                    cash=self.cash, 
                    commission=self.commission,
                    spread=self.spread,
                    )
        
        if optimize:
            # Paramètres par défaut pour l'optimisation
            optimization_params = {
                'sl_percent': list(np.arange(0.001, 0.005, 0.001)),  # Convertir en liste
                'tp_percent': list(np.arange(0.002, 0.01, 0.002))    # Convertir en liste
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
            bt.plot(superimpose=False, resample=False)
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
    data = engine.load_data(symbol="NDX", period='1y', interval='1_min', source='parquet')
    
    if data is not None:
        # Exécution du backtest
        bt, stats = engine.run_backtest(data, plot=True)
        
        # Sauvegarde des résultats
        #engine.save_results(stats)
        
        # Optimisation (décommentez pour exécuter)
        bt_opt, stats_opt = engine.run_backtest(data, optimize=True)
        
        # Comparaison de différents paramètres
        strategies_to_compare = [
            {'sl_percent': 0.001, 'tp_percent': 0.002},
            {'sl_percent': 0.002, 'tp_percent': 0.004},
            {'sl_percent': 0.003, 'tp_percent': 0.006}
        ]
        results = engine.compare_strategies(data, strategies_to_compare)