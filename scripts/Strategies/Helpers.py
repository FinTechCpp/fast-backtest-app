import pandas as pd
import numpy as np
import talib
import time
import os
import glob


def calculate_supertrend(data, atr_period=14, multiplier=3):
    """
    Calcule l'indicateur technique Supertrend.
    
    Args:
        data (pd.DataFrame): DataFrame contenant les données OHLC
        atr_period (int): Période pour le calcul de l'ATR. Par défaut 14.
        multiplier (float): Multiplicateur pour les bandes. Par défaut 3.
    
    Returns:
        pd.DataFrame: DataFrame contenant les valeurs du Supertrend
    """
    # Extraire les colonnes high, low, close selon la structure du DataFrame
    high, low, close = None, None, None
    
    # Vérifier différentes structures possibles de DataFrame
    if {'High', 'Low', 'Close'}.issubset(data.columns):
        high = data['High']
        low = data['Low']
        close = data['Close']
    elif {('bid', 'High'), ('bid', 'Low'), ('bid', 'Close')}.issubset(data.columns):
        high = data[('bid', 'High')]
        low = data[('bid', 'Low')]
        close = data[('bid', 'Close')]
    else:
        # Si on a uniquement le prix de clôture, on l'utilise comme approximation
        if 'close' in data.columns:
            close = data['close']
            high = close
            low = close
        elif ('close', '') in data.columns:
            close = data[('close', '')]
            high = close
            low = close
        else:
            raise KeyError("Les colonnes High, Low, Close ou une colonne close sont requises.")
    
    # Calcul de l'ATR
    atr = talib.ATR(high, low, close, timeperiod=atr_period)
    
    # Calcul des bandes
    hl2 = (high + low) / 2
    upper_band = hl2 + (multiplier * atr)
    lower_band = hl2 - (multiplier * atr)
    
    # Initialisation du DataFrame résultat
    st = pd.DataFrame(index=data.index)
    st['UpperBand'] = upper_band
    st['LowerBand'] = lower_band
    st['SuperTrend'] = np.nan
    st['Direction'] = np.nan
    
    # Trouver l'index de départ (premier point non-NaN)
    start_idx = 0
    for i in range(len(data)):
        if not np.isnan(atr.iloc[i]):
            start_idx = i
            break
    
    if np.isnan(atr).all():
        return pd.DataFrame(index=data.index, columns=['UpperBand','LowerBand','SuperTrend', 'Direction'], data=np.nan)
    
    # Premier calcul
    st.iloc[start_idx, 2] = lower_band.iloc[start_idx]  # Supertrend initial
    st.iloc[start_idx, 3] = 1  # Direction initiale haussière
    
    # Calcul du Supertrend pour chaque point suivant
    for i in range(start_idx + 1, len(data)):
        prev_supertrend = st.iloc[i-1, 2]
        prev_direction = st.iloc[i-1, 3]
        
        # Si la tendance précédente était haussière
        if prev_direction == 1:
            curr_lower_band = max(lower_band.iloc[i], prev_supertrend)
            
            if close.iloc[i] < curr_lower_band:
                # Changement vers tendance baissière
                st.iloc[i, 2] = upper_band.iloc[i]
                st.iloc[i, 3] = -1
            else:
                # Maintien tendance haussière
                st.iloc[i, 2] = curr_lower_band
                st.iloc[i, 3] = 1
        
        # Si la tendance précédente était baissière
        else:
            curr_upper_band = min(upper_band.iloc[i], prev_supertrend)
            
            if close.iloc[i] > curr_upper_band:
                # Changement vers tendance haussière
                st.iloc[i, 2] = lower_band.iloc[i]
                st.iloc[i, 3] = 1
            else:
                # Maintien tendance baissière
                st.iloc[i, 2] = curr_upper_band
                st.iloc[i, 3] = -1 
    return st



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
    pattern = f"./database/{symbol}_{interval.replace('_', '')}_*.parquet"
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
    df['st_50_3'] = calculate_supertrend(df, atr_period=50, multiplier=3)['SuperTrend']


    chrono_load_data = time.time() - chrono_load_data
    print(f"Données chargées et prétraitées en {chrono_load_data:.2f} secondes")
    return df