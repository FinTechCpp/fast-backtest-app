import sys
import os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

from scripts.Strategy import Strategy
import talib
from backtesting import Strategy as BacktestingStrategy
from backtesting import Backtest

#temporary import
import pandas as pd
import time
import os
import glob
import scripts.utils as utils



class BuyTrendFollowingStrategy(Strategy):
    """
    Stratégie de suivi de tendance à l'achat.
    """
    def __init__(self):
        super().__init__()
        """
        Initialise la stratégie.
        """

        self.name = "BuyTrendFollowingStrategy"
        self.symbol = None
        self.exchange = None
        self.timeframe = None

        self.k_previous = None
        self.d_previous = None
    
    def should_long(self):
        return True

    def should_short(self):
        return False
    
    def go_long(self):
        self.buy = 0.5, self.price
        self.take_profit = 0.5, self.price * 1.01
        self.stop_loss = 0.5, self.price * 0.99
    
    def go_short(self):
        raise NotImplementedError("La stratégie BuyTrendFollowingStrategy ne supporte pas la vente à découvert.")
    
    def ema_filter(self):
        # Vérifie si le prix est au-dessus des EMA
        return self.price > self.candles['ema_50'] and self.price > self.candles['ema_200']
        
    def supertrend_filter(self):
        # Vérifie si le prix est au-dessus du SuperTrend
        return self.price > self.candles['st_50_3']
    
    def cross_stoch_filter(self):
        # Vérifie si le Stochastic %K croise au-dessus de %D
        k_current, d_current = self.candles['stoch_k'], self.candles['stoch_d']

        if self.k_previous is None or self.d_previous is None:
            self.k_previous = k_current
            self.d_previous = d_current
            return False
        
        filt = k_current > d_current and self.k_previous < self.d_previous

        # Mettre à jour les valeurs précédentes
        self.k_previous = k_current
        self.d_previous = d_current


        return filt
    
    def stoch_sup_50_filter(self):
        # Vérifie si le Stochastic %D est supérieur à 50
        return self.candles['stoch_d'] > 50
    
    def filters(self):
        return [
            self.ema_filter,
            self.supertrend_filter,
            self.cross_stoch_filter,
            self.stoch_sup_50_filter
        ]
    



class BacktestingAdapter(BacktestingStrategy):
    """
    Adapter pour la stratégie de backtesting.
    """
    def init(self):
        self.my_strategy = BuyTrendFollowingStrategy()
    
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

    

def load_data(symbol, interval='1min', period='1m'):
    """
    Charge les données historiques pour le backtesting à partir d'un fichier Parquet
    et filtre les données en fonction de la période spécifiée.
    """
    chrono_load_data = time.time()
    print(f"Chargement des données pour {symbol}, intervalle {interval}, période {period}...")
    
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
    pattern = f"./database/{symbol}_{interval.replace('_', '')}*.parquet"
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
    
    # # Calcul des indicateurs techniques
    df['ema_200'] = talib.EMA(df['Close'].values, timeperiod=200)
    df['ema_50'] = talib.EMA(df['Close'].values, timeperiod=50)
    df['atr'] = talib.ATR(df['High'].values, df['Low'].values, df['Close'].values, timeperiod=14)
    df['stoch_k'], df['stoch_d'] = talib.STOCH(
        df['High'].values, df['Low'].values, df['Close'].values,
        fastk_period=10, slowk_period=7, slowd_period=3)
    df['st_50_3'] = utils.calculate_supertrend(df, atr_period=50, multiplier=3)['SuperTrend']


    chrono_load_data = time.time() - chrono_load_data
    print(f"Données chargées et prétraitées en {chrono_load_data:.2f} secondes")
    return df
    

data = load_data(symbol='NDX', period='1m', interval='1min')

print(data.head())
print(data.columns)
print(data.tail())

bt = Backtest(data, BacktestingAdapter, cash=100000, commission=.00, exclusive_orders=True)
stats = bt.run()
print(stats)
bt.plot()


