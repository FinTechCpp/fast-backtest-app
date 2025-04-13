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


def load_data(symbol, interval='20secs', period='1m', end_date=None, timezone='Europe/Paris'):
    """
    Charge les données historiques pour le backtesting à partir d'un fichier Parquet
    et filtre les données en fonction de la période spécifiée.
    
    Args:
        symbol (str): Symbole du marché à charger (ex: 'NDX')
        interval (str): Intervalle des données (ex: '20secs')
        period (str): Période de données à charger (ex: '1m', '6m', '1y')
        end_date (str|datetime): Date de fin au format 'DD/MM/YYYY' ou objet datetime
        timezone (str): Fuseau horaire pour les données (ex: 'Europe/Paris')
    """
    chrono_load_data = time.time()
    print(f"Chargement des données pour {symbol}, intervalle {interval}, période {period}...")
    
    # Vérifier si le dossier 'marketData' existe dans le répertoire actuel ou dans le parent
    current_directory = os.getcwd()
    market_data_path = os.path.join(current_directory, 'marketData')
    
    if not os.path.exists(market_data_path):
        # Si 'marketData' n'existe pas dans le répertoire courant, vérifier dans le parent
        parent_directory = os.path.dirname(current_directory)
        market_data_path = os.path.join(parent_directory, 'marketData')
        
        if not os.path.exists(market_data_path):
            raise ValueError(f"Le dossier 'marketData' n'existe pas dans le répertoire courant ni dans le parent.")
    
    # Rechercher le fichier correspondant au symbole
    pattern = os.path.join(market_data_path, f"{symbol}_10secs_*.parquet")
    matching_files = glob.glob(pattern)
    
    if not matching_files:
        raise ValueError(f"Aucun fichier trouvé correspondant à {pattern}")
    
    # Utiliser le fichier le plus récent si plusieurs fichiers correspondent
    save_path = max(matching_files, key=os.path.getctime)
    
    print(f"Fichier trouvé: {save_path}")
    
    # Charger tout le fichier Parquet pour pouvoir identifier les limites de dates disponibles
    try:
        df = pd.read_parquet(save_path, engine='pyarrow')
        df_original_len = len(df)
        print(f"Données chargées depuis {save_path}: {df_original_len} barres de prix")
    except FileNotFoundError:
        raise ValueError(f"Le fichier {save_path} est introuvable.")
    except Exception as e:
        raise ValueError(f"Erreur lors du chargement des données : {e}")
    
    # Convertir la colonne 'date' en datetime avec timezone si elle existe
    if 'date' in df.columns and not pd.api.types.is_datetime64_any_dtype(df['date']):
        df['date'] = pd.to_datetime(df['date'])
    
    # S'assurer que la colonne 'date' a un fuseau horaire
    if 'date' in df.columns and df['date'].dt.tz is None:
        df['date'] = df['date'].dt.tz_localize('UTC').dt.tz_convert(timezone)
    elif 'date' in df.columns:
        df['date'] = df['date'].dt.tz_convert(timezone)
    
    # Si date est dans l'index, le convertir aussi
    if pd.api.types.is_datetime64_any_dtype(df.index):
        if df.index.tz is None:
            df.index = df.index.tz_localize('UTC').tz_convert(timezone)
        else:
            df.index = df.index.tz_convert(timezone)
    
    # Définir la date de fin si elle n'est pas spécifiée
    if end_date is None:
        # Utiliser la dernière date disponible dans le dataframe
        if 'date' in df.columns:
            last_available_date = df['date'].max()
        else:
            last_available_date = df.index.max()
        end_date = last_available_date
        print(f"Date de fin automatique: {end_date}")
    else:
        # Convertir end_date si c'est une chaîne au format DD/MM/YYYY
        if isinstance(end_date, str):
            try:
                # Essayer d'abord le format DD/MM/YYYY
                end_date = pd.to_datetime(end_date, format="%d/%m/%Y")
            except ValueError:
                # Si ça échoue, laisser pandas détecter le format
                end_date = pd.to_datetime(end_date)
        
        # Ajouter le fuseau horaire si nécessaire
        if not hasattr(end_date, 'tzinfo') or end_date.tzinfo is None:
            end_date = pd.Timestamp(end_date).tz_localize('UTC').tz_convert(timezone)
    
    # Calculer la date de début à partir de end_date et period
    if period.endswith('y'):  # Années
        start_date = end_date - pd.DateOffset(years=int(period[:-1]))
    elif period.endswith('m'):  # Mois
        start_date = end_date - pd.DateOffset(months=int(period[:-1]))
    elif period.endswith('d'):  # Jours
        start_date = end_date - pd.DateOffset(days=int(period[:-1]))
    else:
        raise ValueError(f"Période non reconnue : {period}. Utilisez '1y', '6m', '30d', etc.")
    print(f"Date de début calculée: {start_date}")
    
    print(f"Filtrage des données pour la période {start_date.strftime('%d/%m/%Y %H:%M')} à {end_date.strftime('%d/%m/%Y %H:%M')}...")
    
    # Filtrer les données en fonction de la période
    if 'date' in df.columns:
        df = df.set_index('date')
    
    df = df[(df.index >= start_date) & (df.index <= end_date)]
    print(f"Données filtrées: {len(df)} barres de prix sur {df_original_len} disponibles")
    
    # Le reste de la fonction reste identique...
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
    
    # ----------Calcul des indicateurs techniques---------------------------
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