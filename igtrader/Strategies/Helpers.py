import pandas as pd
import numpy as np
import talib
import time
import os
import glob
import re 
import logging
from cpp_strategies import CppStrategyBaseConfig, CppTime, CppDateTime



def load_data(symbol='NDX', interval='10secs', period='1m', end_date=None,
              trading_from=None, trading_to=None, trading_days=None):
    """
    Charge les données historiques pour le backtesting à partir d'un fichier Parquet
    et filtre les données en fonction de la période spécifiée et des horaires de trading.
    
    Args:
        symbol (str): Symbole du marché à charger (ex: 'NDX')
        interval (str): Intervalle des données (ex: '20secs')
        period (str): Période de données à charger (ex: '1m', '6m', '1y')
        end_date (str|datetime): Date de fin au format 'DD/MM/YYYY' ou objet datetime
        trading_from (QTime|str): Heure de début du trading
        trading_to (QTime|str): Heure de fin du trading
        trading_days (list): Liste des jours de trading (0=Lundi, 6=Dimanche)

    Returns:
        pd.DataFrame: DataFrame contenant les données OHLC filtrées indexées par date toujours entre 15h30 et 22h (bourse ouverte)
    """
    start_time = time.time()
    logging.info(f"Chargement des données pour {symbol}, intervalle {interval}, période {period}...")
    
    # Recherche du dossier 'marketData' dans plusieurs emplacements
    current_directory = os.getcwd()
    possible_paths = [
        os.path.join(current_directory, 'ig-trading-bot/marketData'),
        os.path.join(os.path.dirname(current_directory), 'marketData'),
        os.path.join(os.getenv('BOT_REPO_PATH', ''), 'marketData'),
    ]
    market_data_path = None
    for p in possible_paths:
        if p and os.path.exists(p):
            market_data_path = p
            break
    if market_data_path is None:
        raise ValueError(f"Le dossier 'marketData' n'a pas été trouvé dans : {possible_paths}")
    
    # Rechercher le fichier correspondant au symbole
    pattern = os.path.join(market_data_path, f"{symbol}_{interval}_*.parquet")
    matching_files = glob.glob(pattern)
    
    if not matching_files:
        raise ValueError(f"Aucun fichier trouvé correspondant à {pattern}")
    
    # Utiliser le fichier le plus récent si plusieurs fichiers correspondent
    save_path = max(matching_files, key=os.path.getctime)
    
    logging.debug(f"Fichier trouvé: {save_path}")
    
    # Charger tout le fichier Parquet pour pouvoir identifier les limites de dates disponibles
    try:
        df = pd.read_parquet(save_path)
        df_original_len = len(df)
        logging.debug(f"Données chargées depuis {save_path}: {df_original_len} barres de prix")
    except FileNotFoundError:
        raise ValueError(f"Le fichier {save_path} est introuvable.")
    except Exception as e:
        raise ValueError(f"Erreur lors du chargement des données : {e}")
    
    # Convertir la colonne 'date' en datetime et la définir comme index
    if 'date' in df.columns:
        # Convertir en datetime si nécessaire
        if not pd.api.types.is_datetime64_any_dtype(df['date']):
            df['date'] = pd.to_datetime(df['date'])
        # Définir comme index
        df = df.set_index('date')
    
    # Standardiser l'index en heure française (NY + 6h)
    if pd.api.types.is_datetime64_any_dtype(df.index):
        # Convertir vers le fuseau horaire New York si nécessaire
        if df.index.tz is None:
            df.index = df.index.tz_localize('UTC').tz_convert('America/New_York')
        else:
            df.index = df.index.tz_convert('America/New_York')
        
        # Ajouter 6 heures pour transformer les heures de marché US (9h30-16h) 
        # en heures françaises standardisées (15h30-22h)
        df.index = df.index + pd.Timedelta(hours=6)
        
        # Supprimer l'information de fuseau horaire
        df.index = df.index.tz_localize(None)

    # Définir la date de fin si elle n'est pas spécifiée
    if end_date is None:
        # Utiliser la dernière date disponible dans le dataframe
        end_date = df.index.max()
        logging.debug(f"Date de fin automatique: {end_date}")
    else:
        # Convertir end_date si c'est une chaîne
        if isinstance(end_date, str):
            try:
                # Essayer d'abord le format DD/MM/YYYY
                end_date = pd.to_datetime(end_date, format="%d/%m/%Y")
            except ValueError:
                # Si ça échoue, laisser pandas détecter le format
                end_date = pd.to_datetime(end_date)
        
        # Standardiser end_date dans le même format que l'index (sans timezone)
        if hasattr(end_date, 'tzinfo') and end_date.tzinfo is not None:
            # Si end_date a une timezone, on la convertit en NY+6h puis on supprime la timezone
            end_date = pd.Timestamp(end_date).tz_convert('America/New_York') + pd.Timedelta(hours=6)
            end_date = end_date.tz_localize(None)
    
    # Calculer la date de début à partir de end_date et period
    if period.endswith('y'):  # Années
        start_date = end_date - pd.DateOffset(years=int(period[:-1]))
    elif period.endswith('m'):  # Mois
        start_date = end_date - pd.DateOffset(months=int(period[:-1]))
    elif period.endswith('d'):  # Jours
        start_date = end_date - pd.DateOffset(days=int(period[:-1]))
    else:
        raise ValueError(f"Période non reconnue : {period}. Utilisez '1y', '6m', '30d', etc.")
    
    logging.debug(f"Filtrage des données pour la période {start_date.strftime('%d/%m/%Y %H:%M')} à {end_date.strftime('%d/%m/%Y %H:%M')}...")
    
    # Filtrer les données en fonction de la période
    df = df[(df.index >= start_date) & (df.index <= end_date)]
    logging.debug(f"Données filtrées: {len(df)} barres de prix sur {df_original_len} disponibles")
    
    # Vérifier les colonnes nécessaires
    required_columns = {'open', 'high', 'low', 'close'}
    if not required_columns.issubset(df.columns):
        raise ValueError(f"Les colonnes requises sont manquantes dans les données : {required_columns - set(df.columns)}")
        
    # Renommer les colonnes pour correspondre au format attendu
    df = df.rename(columns={
        'open': 'Open',
        'high': 'High',
        'low': 'Low',
        'close': 'Close'
    })
    
    # Garder uniquement les colonnes nécessaires
    df = df[['Open', 'High', 'Low', 'Close']]
        
    # Afficher le temps total de chargement et prétraitement
    end_time = time.time()
    logging.info(f"Données chargées et prétraitées en {end_time - start_time:.2f} secondes")
        
    # Filtrage par jours et heures de trading
    if trading_days is not None and len(trading_days) > 0:
        logging.debug(f"Filtrage par jours de trading: {trading_days}")
        # Extraire le jour de la semaine (0=lundi, 6=dimanche)
        df['day_of_week'] = df.index.dayofweek
        # Ne garder que les jours de trading spécifiés
        df = df[df['day_of_week'].isin(trading_days)]
        # Supprimer la colonne temporaire
        df = df.drop('day_of_week', axis=1)
        logging.debug(f"Après filtrage par jours: {len(df)} barres de prix")

    if trading_from is not None and trading_to is not None:
        # Convertir QTime en heures et minutes si nécessaire
        if hasattr(trading_from, 'hour') and hasattr(trading_from, 'minute'):
            from_hour, from_minute = trading_from.hour(), trading_from.minute()
        else:
            # Si c'est une chaîne au format "HH:MM"
            from_hour, from_minute = map(int, str(trading_from).split(':'))
        
        if hasattr(trading_to, 'hour') and hasattr(trading_to, 'minute'):
            to_hour, to_minute = trading_to.hour(), trading_to.minute()
        else:
            # Si c'est une chaîne au format "HH:MM"
            to_hour, to_minute = map(int, str(trading_to).split(':'))
        
        logging.debug(f"Filtrage par heures de trading: {from_hour}:{from_minute} à {to_hour}:{to_minute}")
        
        # Extraire l'heure et la minute
        df['hour'] = df.index.hour
        df['minute'] = df.index.minute
        
        # Création d'un masque pour filtrer par heure
        if to_hour > from_hour or (to_hour == from_hour and to_minute >= from_minute):
            # Cas standard: période dans la même journée
            mask = ((df['hour'] > from_hour) | 
                  ((df['hour'] == from_hour) & (df['minute'] >= from_minute)))
            mask &= ((df['hour'] < to_hour) | 
                    ((df['hour'] == to_hour) & (df['minute'] <= to_minute)))
        else:
            # Cas où la période traverse minuit
            mask = ((df['hour'] > from_hour) | 
                  ((df['hour'] == from_hour) & (df['minute'] >= from_minute)) |
                  (df['hour'] < to_hour) | 
                  ((df['hour'] == to_hour) & (df['minute'] <= to_minute)))
        
        # Application du filtre
        df = df[mask]
        
        # Suppression des colonnes temporaires
        df = df.drop(['hour', 'minute'], axis=1)
        logging.debug(f"Après filtrage par heures: {len(df)} barres de prix")
    
    return df.dropna()

def resample_ohlc(data, timeframe):
    """
    Resample an OHLC DataFrame to a new timeframe, aligning to standard calendar intervals.
    The function first trims the DataFrame to start at a timestamp that aligns with the requested interval.
    
    Args:
        data (pd.DataFrame): Must have a DatetimeIndex (or a 'date' column) 
                            and columns ['Open','High','Low','Close'] or ['open','high','low','close'].
        timeframe (str|int): pandas offset alias (e.g. '20secs','1T') or integer seconds.
    Returns:
        pd.DataFrame: resampled OHLC DataFrame aligned to standard intervals.
    """
    df = data.copy()
    
    # Convert date column to index if it exists
    if not isinstance(df.index, pd.DatetimeIndex):
        if 'date' in df.columns:
            df = df.set_index('date')
        else:
            df.index = pd.to_datetime(df.index)
    
    # Ensure DatetimeIndex has timezone if original data had it
    if df.index.tz is None and hasattr(data.index, 'tz') and data.index.tz is not None:
        df.index = df.index.tz_localize(data.index.tz)
    
    # Parse the timeframe into seconds for alignment calculation
    if isinstance(timeframe, int):
        seconds = timeframe
        rule = f'{timeframe}s'
    else:
        rule = str(timeframe).strip()
        # Extract the numeric value and unit
        match = re.match(r'(\d+)([a-zA-Z]+)', rule)
        if match:
            value = int(match.group(1))
            unit = match.group(2).lower()
            
            # Convert to seconds based on the unit
            if re.match(r'secs?', unit):
                seconds = value
            elif re.match(r'mins?', unit):
                seconds = value * 60
            elif re.match(r'hrs?|hours?', unit):
                seconds = value * 3600
            else:
                seconds = value  # Default if we can't determine
        else:
            seconds = 1  # Default if we can't parse
        
        # Map seconds, minutes and hours to pandas frequencies
        rule = re.sub(r'secs?$', 's', rule, flags=re.IGNORECASE)
        rule = re.sub(r'mins?$', 'T', rule, flags=re.IGNORECASE)
        rule = re.sub(r'hrs?$|hours?$', 'H', rule, flags=re.IGNORECASE)
    
    # Find the first timestamp that aligns with the interval
    first_ts = df.index[0]
    epoch_seconds = int(first_ts.timestamp())
    remainder = epoch_seconds % seconds
    
    if remainder != 0:
        # Calculate the next aligned timestamp
        aligned_epoch = epoch_seconds - remainder + seconds
        aligned_ts = pd.Timestamp(aligned_epoch, unit='s', tz=first_ts.tz)
        
        # Trim the DataFrame to start from this aligned timestamp
        df = df[df.index >= aligned_ts]
    
    # Detect column name format (uppercase or lowercase)
    has_uppercase = all(col in df.columns for col in ['Open', 'High', 'Low', 'Close'])
    has_lowercase = all(col in df.columns for col in ['open', 'high', 'low', 'close'])
    
    # Build aggregation dictionary for all columns
    if has_uppercase:
        ohlc_dict = {'Open': 'first', 'High': 'max', 'Low': 'min', 'Close': 'last'}
        # Add other columns to the aggregation dictionary with 'last' as default
        for col in df.columns:
            if col not in ['Open', 'High', 'Low', 'Close']:
                ohlc_dict[col] = 'last'
    elif has_lowercase:
        ohlc_dict = {'open': 'first', 'high': 'max', 'low': 'min', 'close': 'last'}
        # Add other columns to the aggregation dictionary with 'last' as default
        for col in df.columns:
            if col not in ['open', 'high', 'low', 'close']:
                ohlc_dict[col] = 'last'
    else:
        raise ValueError("DataFrame must have OHLC columns (either uppercase or lowercase)")
    
    # Resample the aligned DataFrame
    resampled = df.resample(rule).agg(ohlc_dict)
    
    # Drop any NaN rows
    resampled = resampled.dropna()
    
    return resampled

def create_base_config(kwargs) -> CppStrategyBaseConfig:
    """
    Crée et configure un objet CppStrategyBaseConfig à partir des kwargs.
    Extrait également les paramètres utilisés du dictionnaire kwargs.
    
    Args:
        kwargs (dict): Dictionnaire de paramètres
        
    Returns:
        CppStrategyBaseConfig: Objet de configuration de base configuré
    """
    # Créer l'objet de configuration
    config = CppStrategyBaseConfig()
    
    # Convertir les objets datetime.time en objets CppTime
    trading_from = kwargs.pop('trading_from')
    trading_to = kwargs.pop('trading_to')
    
    # Créer des objets CppTime
    from_time = CppTime()
    to_time = CppTime()
    
    if hasattr(trading_from, 'hour') and callable(trading_from.hour):
        # QTime objects
        from_time.hour = trading_from.hour()
        from_time.minute = trading_from.minute()
        from_time.second = 0
        
        to_time.hour = trading_to.hour()
        to_time.minute = trading_to.minute()
        to_time.second = 0
    else:
        # datetime.time objects
        from_time.hour = trading_from.hour
        from_time.minute = trading_from.minute
        from_time.second = 0
        
        to_time.hour = trading_to.hour
        to_time.minute = trading_to.minute
        to_time.second = 0
    
    # Affecter les objets CppTime
    config.trading_from = from_time
    config.trading_to = to_time
    
    # Configurer les jours de trading
    config.trading_days = kwargs.pop('trading_days')
    
    # Configurer les paramètres de distance
    config.take_profit_distance = float(kwargs.pop('take_profit_distance'))
    config.stop_loss_distance = float(kwargs.pop('stop_loss_distance'))
    
    # Paramètres ATR
    config.use_atr_for_sl_tp = bool(kwargs.pop('use_atr_for_sl_tp', False))
    config.atr_period = int(kwargs.pop('atr_period', 14))
    config.stop_loss_atr_multiplier = float(kwargs.pop('stop_loss_atr_multiplier', 2.0))
    config.take_profit_atr_multiplier = float(kwargs.pop('take_profit_atr_multiplier', 3.0))
    config.min_stop_loss_distance = float(kwargs.pop('min_stop_loss_distance', 5.0))
    config.min_take_profit_distance = float(kwargs.pop('min_take_profit_distance', 5.0))
    
    # Paramètres de gestion du risque
    config.use_risk_based_sizing = bool(kwargs.pop('use_risk_based_sizing', False))
    config.risk_percentage = float(kwargs.pop('risk_percentage', 1.0))
    config.cash = float(kwargs.pop('cash', 100000.0))
    config.max_position_percentage = float(kwargs.pop('max_position_percentage', 100.0))
    config.leverage_limit = float(kwargs.pop('leverage_limit', 20.0))
    
    # Paramètres break-even
    config.use_break_even = bool(kwargs.pop('use_break_even', True))
    config.break_even_threshold = float(kwargs.pop('break_even_threshold', 0.7))
    
    # Paramètres de perte maximale journalière
    config.use_daily_max_loss = bool(kwargs.pop('use_daily_max_loss', False))
    config.daily_max_loss_percentage = float(kwargs.pop('daily_max_loss_percentage', 2.0))
    
    logging.debug("Configuration de base créée")
    return config



class IncrementalIndicator:
    """Classe de base pour tous les indicateurs calculés de manière incrémentale"""
    def __init__(self):
        self.is_initialized = False
        
    def requires_initialization(self):
        """Vérifie si l'indicateur a besoin d'être initialisé avec un historique"""
        return not self.is_initialized
    
class EMA(IncrementalIndicator):
    """
    Calcule l'Exponential Moving Average (EMA) de manière incrémentale.
    """
    def __init__(self, period):
        super().__init__()
        self.period = int(period)
        self.multiplier = 2.0 / (self.period + 1)
        self.current_ema = None
        self.price_history = []
    
    def initialize_with_history(self, price_history):
        if len(price_history) >= self.period:
            # Calculer la moyenne simple comme valeur initiale
            sma = sum(price_history[-self.period:]) / self.period
            self.current_ema = sma
            
            # Calculer l'EMA directement sans appeler update()
            if len(price_history) > self.period:
                for price in price_history[-self.period:]:
                    self.current_ema = (price - self.current_ema) * self.multiplier + self.current_ema
            
            self.is_initialized = True
            return self.current_ema
        return None
    
    def update(self, price):
        """Met à jour l'EMA avec le nouveau prix"""
        if not self.is_initialized:
            self.price_history.append(price)
            if len(self.price_history) >= self.period:
                return self.initialize_with_history(self.price_history)
            return None
        
        # Calculer la nouvelle valeur EMA
        self.current_ema = (price - self.current_ema) * self.multiplier + self.current_ema
        return self.current_ema
    
    def get_value(self):
        """Retourne la valeur actuelle de l'EMA"""
        return self.current_ema

class STOCH(IncrementalIndicator):
    """
    Calcule l'oscillateur Stochastique de manière incrémentale.
    """
    def __init__(self, fastk_period, slowk_period, slowd_period):
        super().__init__()
        self.fastk_period = int(fastk_period)
        self.slowk_period = int(slowk_period)
        self.slowd_period = int(slowd_period)
        
        self.high_buffer = []
        self.low_buffer = []
        self.close_buffer = []
        
        self.raw_k_values = []  # Pour stocker les valeurs K brutes avant lissage
        self.k_values = []      # Pour stocker les valeurs K lissées
        self.d_values = []      # Pour stocker les valeurs D
        
        self.current_k = None
        self.current_d = None
    
    def initialize_with_history(self, high_history, low_history, close_history):
        """Initialise le stochastique avec un historique de prix"""
        if len(high_history) < self.fastk_period:
            return None, None
            
        self.high_buffer = list(high_history[-self.fastk_period:])
        self.low_buffer = list(low_history[-self.fastk_period:])
        self.close_buffer = list(close_history[-self.fastk_period:])
        
        # Calculer les valeurs K brutes initiales
        for i in range(len(close_history) - self.fastk_period + 1):
            period_high = max(high_history[i:i+self.fastk_period])
            period_low = min(low_history[i:i+self.fastk_period])
            close = close_history[i+self.fastk_period-1]
            
            raw_k = 100.0 * ((close - period_low) / (period_high - period_low) if period_high > period_low else 0)
            self.raw_k_values.append(raw_k)
        
        # Appliquer le lissage K
        if len(self.raw_k_values) >= self.slowk_period:
            for i in range(len(self.raw_k_values) - self.slowk_period + 1):
                smooth_k = sum(self.raw_k_values[i:i+self.slowk_period]) / self.slowk_period
                self.k_values.append(smooth_k)
        
        # Calculer les valeurs D (moyenne mobile des valeurs K)
        if len(self.k_values) >= self.slowd_period:
            for i in range(len(self.k_values) - self.slowd_period + 1):
                smooth_d = sum(self.k_values[i:i+self.slowd_period]) / self.slowd_period
                self.d_values.append(smooth_d)
        
        if self.k_values and self.d_values:
            self.current_k = self.k_values[-1]
            self.current_d = self.d_values[-1]
            self.is_initialized = True
            
        return self.current_k, self.current_d
    
    def update(self, high, low, close):
        """Met à jour le stochastique avec les nouveaux prix"""
        # Ajouter le nouveau prix et supprimer l'ancien si nécessaire
        self.high_buffer.append(high)
        self.low_buffer.append(low)
        self.close_buffer.append(close)
        
        if len(self.high_buffer) > self.fastk_period:
            self.high_buffer.pop(0)
            self.low_buffer.pop(0)
            self.close_buffer.pop(0)
        
        if not self.is_initialized:
            if len(self.high_buffer) == self.fastk_period:
                return self.initialize_with_history(self.high_buffer, self.low_buffer, self.close_buffer)
            return None, None
        
        # Calculer la nouvelle valeur K brute
        period_high = max(self.high_buffer)
        period_low = min(self.low_buffer)
        
        if period_high > period_low:
            raw_k = 100.0 * ((close - period_low) / (period_high - period_low))
        else:
            raw_k = 0.0
            
        self.raw_k_values.append(raw_k)
        if len(self.raw_k_values) > self.fastk_period + self.slowk_period:
            self.raw_k_values.pop(0)
        
        # Calculer la nouvelle valeur K lissée
        if len(self.raw_k_values) >= self.slowk_period:
            smooth_k = sum(self.raw_k_values[-self.slowk_period:]) / self.slowk_period
            self.k_values.append(smooth_k)
            
            if len(self.k_values) > self.fastk_period + self.slowk_period:
                self.k_values.pop(0)
        
        # Calculer la nouvelle valeur D
        if len(self.k_values) >= self.slowd_period:
            smooth_d = sum(self.k_values[-self.slowd_period:]) / self.slowd_period
            self.d_values.append(smooth_d)
            
            if len(self.d_values) > self.fastk_period + self.slowk_period:
                self.d_values.pop(0)
        
        # Mettre à jour les valeurs actuelles
        if self.k_values and self.d_values:
            self.current_k = self.k_values[-1]
            self.current_d = self.d_values[-1]
        
        return self.current_k, self.current_d
    
    def get_values(self):
        """Retourne les valeurs actuelles K et D"""
        return self.current_k, self.current_d

class ATR(IncrementalIndicator):
    """
    Calcule l'Average True Range (ATR) de manière incrémentale.
    """
    def __init__(self, period):
        super().__init__()
        self.period = int(period)
        self.current_atr = None
        self.previous_close = None
        self.true_range_history = []
    
    def initialize_with_history(self, high_history, low_history, close_history):
        """Initialise l'ATR avec un historique de prix"""
        if len(high_history) < self.period + 1:
            return None
            
        # Calculer les True Ranges sur toute la période
        true_ranges = []
        for i in range(1, len(high_history)):
            high = high_history[i]
            low = low_history[i]
            prev_close = close_history[i-1]
            
            tr = max(
                high - low,
                abs(high - prev_close),
                abs(low - prev_close)
            )
            true_ranges.append(tr)
        
        # Calculer l'ATR initial en tant que moyenne simple des TR
        if len(true_ranges) >= self.period:
            self.current_atr = sum(true_ranges[-self.period:]) / self.period
            self.previous_close = close_history[-1]
            self.is_initialized = True
            
        return self.current_atr
    
    def update(self, high, low, close):
        """Met à jour l'ATR avec les nouveaux prix"""
        if not self.is_initialized:
            if self.previous_close is None:
                self.previous_close = close
                return None
                
            self.true_range_history.append(max(
                high - low,
                abs(high - self.previous_close),
                abs(low - self.previous_close)
            ))
            
            self.previous_close = close
            
            if len(self.true_range_history) >= self.period:
                self.current_atr = sum(self.true_range_history) / self.period
                self.is_initialized = True
                return self.current_atr
            
            return None
        
        # Calculer le nouveau True Range
        tr = max(
            high - low,
            abs(high - self.previous_close),
            abs(low - self.previous_close)
        )
        
        # Mettre à jour l'ATR en utilisant la méthode de Wilder (lissage exponentiel)
        self.current_atr = ((self.current_atr * (self.period - 1)) + tr) / self.period
        self.previous_close = close
        
        return self.current_atr
    
    def get_value(self):
        """Retourne la valeur actuelle de l'ATR"""
        return self.current_atr


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
