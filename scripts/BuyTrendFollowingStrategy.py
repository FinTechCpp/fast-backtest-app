import sys
import os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

from scripts.Strategy import Strategy
import talib
from jesse.strategies import cached
from backtesting import Strategy as BacktestingStrategy

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


    @property
    @cached
    def ema_50(self):
        return talib.EMA(self.candles['close'].values, timeperiod=50)
    
    @property
    @cached
    def ema_200(self):
        return talib.EMA(self.candles['close'].values, timeperiod=200)
    
    @property
    @cached
    def supertrend_50(self):
        return talib.SMA(self.candles['close'].values, timeperiod=50)
    
    @property
    @cached
    def stoch(self):
        return talib.STOCHF(
            self.candles['high'].values,
            self.candles['low'].values,
            self.candles['close'].values,
            fastk_period=10, slowk_period=7, fastd_period=3)
    
    def should_long(self):
        return True

    def should_short(self):
        return False
    
    def go_long(self):
        self.buy = 0.5, 20000
        self.take_profit = 0.5, 21000
        self.stop_loss = 0.5, 19000
    
    def go_short(self):
        raise NotImplementedError("La stratégie BuyTrendFollowingStrategy ne supporte pas la vente à découvert.")
    
    def ema_filter(self):
        # Vérifie si le prix est au-dessus des EMA
        return self.price > self.ema_50 and self.price > self.ema_200
        
    def supertrend_filter(self):
        # Vérifie si le prix est au-dessus du SuperTrend
        return self.price > self.supertrend_50
    
    def cross_stoch_filter(self):
        # Vérifie si le Stochastic %K croise au-dessus de %D
        k, d = self.stoch

        if len(k) < 2 or len(d) < 2:
            return False
        
        k_current = k[-1]
        d_current = d[-1]
        k_previous = k[-2]
        d_previous = d[-2]
        return k_current > d_current and k_previous < d_previous
    
    def stoch_sup_50_filter(self):
        # Vérifie si le Stochastic %D est supérieur à 50
        d, _ = self.stoch
        if len(d) < 1:
            return False
        d_current = d[-1]
        return d_current > 50
    
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
        return super().init()
    
    def next(self):
        return super().next()
    

def load_data(symbol, period='', interval='', source='parquet'):
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
        # df['EMA_200'] = talib.EMA(df['Close'].values, timeperiod=200)
        # df['EMA_50'] = talib.EMA(df['Close'].values, timeperiod=50)
        # df['ATR'] = talib.ATR(df['High'].values, df['Low'].values, df['Close'].values, timeperiod=14)
        
        # # Stochastique
        # df['stoch_k'], df['stoch_d'] = talib.STOCH(
        #     df['High'].values, df['Low'].values, df['Close'].values,
        #     fastk_period=10, slowk_period=7, slowd_period=3
        # )
        
        # st_result = utils.calculate_supertrend(df, atr_period=50, multiplier=3)
        # df['st_50_3'] = st_result['SuperTrend']
        chrono_load_data = time.time() - chrono_load_data
        print(f"Données chargées et prétraitées en {chrono_load_data:.2f} secondes")
        return df
    else:
        raise NotImplementedError("Source de données non supportée. Utilisez 'parquet'.")
    

print(glob.glob("./database/*.parquet"))
print(load_data(symbol='NDX', period='1y', interval='20secs', source='parquet').head())