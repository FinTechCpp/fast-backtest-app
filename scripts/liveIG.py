import sys
import os

# Add the parent directory of scripts to the path
project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(project_root)

# Add the cpp_strategies directory specifically
cpp_strategies_dir = os.path.join(project_root, 'cpp_strategies')
sys.path.append(cpp_strategies_dir)

import logging
import datetime
import time
import pandas as pd
import traceback
import signal

# Create logs directory if it doesn't exist
log_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'logs')
os.makedirs(log_dir, exist_ok=True)

# Configure logging to write to a file in the logs directory
log_file = os.path.join(log_dir, 'liveTickIG.log')
logging.basicConfig(
    level=logging.DEBUG,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler(log_file),
        #logging.StreamHandler()  # Optional: also log to console
    ]
)

# Ajouter après la configuration de logging

def cpp_log_callback(message, level):
    """Fonction de callback pour les logs provenant du code C++"""
    if level == 0:  # DEBUG
        logging.debug(f"C++: {message}")
    elif level == 1:  # INFO
        logging.info(f"C++: {message}")
    elif level == 2:  # WARNING
        logging.warning(f"C++: {message}")
    elif level == 3:  # ERROR
        logging.error(f"C++: {message}")
    else:
        logging.info(f"C++: {message}")  # Fallback pour les autres niveaux

from igtrader.Strategies.BuyTrendFollowing import BuyTrendFollowing
from igtrader.Strategies.CrossEMA import CrossEMA, CrossEMAConfig
from igtrader.WrapperIGAPI.TickBroker import TickBroker, PriceSource
from igtrader.Strategies.Strategy import BaseCandle
# Importer les classes C++ à la place des classes Python
from cpp_strategies import (
    CppStrategyBaseConfig, 
    CppBuyHeikinGreenConfig, 
    CppBuyHeikinGreen, 
    CppCandle,
    CppDateTime,
    CppTime,
    CppSignal
)

CANDLE_TIME_UNIT = 20  # n-second time unit candles to trade with

# Variable globale pour arrêter proprement le programme
running = True

def signal_handler(sig, frame):
    """Gestionnaire de signal pour arrêter proprement le programme"""
    global running
    logging.info("Signal d'arrêt reçu, arrêt en cours...")
    running = False

def process_candle(candle: BaseCandle, broker: TickBroker, strategy):
    """
    Fonction de callback appelée lorsqu'une nouvelle bougie est prête.
    Traite la bougie et exécute la stratégie C++.
    
    Args:
        candle (BaseCandle): La bougie complète
        broker (TickBroker): Instance du broker pour exécuter les signaux
        strategy: Stratégie de trading C++ à utiliser
    """
    try:
        logging.info(f"Processing complete candle: {candle.date} - O:{candle.Open} H:{candle.High} L:{candle.Low} C:{candle.Close}")        
        
        
        # Convertir la bougie Python en bougie C++
        cpp_candle = CppCandle()
        
        # Configurer la date et l'heure
        if isinstance(candle.date, pd.Timestamp):
            dt = candle.date
            cpp_candle.date.year = dt.year
            cpp_candle.date.month = dt.month
            cpp_candle.date.day = dt.day
            cpp_candle.date.time.hour = dt.hour
            cpp_candle.date.time.minute = dt.minute
            cpp_candle.date.time.second = dt.second
        else:
            # Si ce n'est pas un timestamp, essayer de convertir depuis datetime
            cpp_candle.date.year = candle.date.year
            cpp_candle.date.month = candle.date.month
            cpp_candle.date.day = candle.date.day
            cpp_candle.date.time.hour = candle.date.hour
            cpp_candle.date.time.minute = candle.date.minute
            cpp_candle.date.time.second = candle.date.second
        
        # Configurer les valeurs OHLC
        cpp_candle.open = float(candle.Open)
        cpp_candle.high = float(candle.High)
        cpp_candle.low = float(candle.Low)
        cpp_candle.close = float(candle.Close)
        
        # Configurer les informations de position
        cpp_candle.in_position = bool(candle.in_position)
        cpp_candle.position_pl_pct = float(candle.position_pl_pct) if candle.position_pl_pct is not None else 0.0
        cpp_candle.entry_price = float(candle.entry_price) if candle.entry_price is not None else 0.0
        cpp_candle.position_size = float(candle.position_size) if candle.position_size is not None else 0.0
        
        # Mettre à jour la stratégie avec la nouvelle bougie et récupérer le signal
        cpp_signal = strategy.update_candle(cpp_candle)
        
        # Exécuter le signal s'il y en a un
        if cpp_signal is not None:
            # Convertir le signal C++ en dictionnaire pour le broker
            signal_dict = {
                "action": cpp_signal.action,
            }
            
            # Ajouter les détails du signal selon l'action
            if cpp_signal.action == "BUY" or cpp_signal.action == "SELL":
                signal_dict.update({
                    "quantity": cpp_signal.quantity,
                    "price": cpp_signal.price,
                    "take_profit": cpp_signal.take_profit,
                    "stop_loss": cpp_signal.stop_loss
                })
            elif cpp_signal.action == "MOVE_SL":
                signal_dict.update({
                    "new_sl": cpp_signal.new_sl
                })
            
            logging.info(f"Signal generated: {signal_dict}")
            broker.execute_signal(signal_dict)
    except Exception as e:
        logging.error(f"Error processing candle: {e}")
        logging.error(traceback.format_exc())


def main():
    # Configurer le gestionnaire de signal pour Ctrl+C
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    
    
    # Créer et configurer l'objet de configuration C++ de base
    cpp_base_config = CppStrategyBaseConfig()
    
    # Configuration des horaires de trading
    trading_from = datetime.time(7, 0)
    trading_to = datetime.time(23, 0)
    
    cpp_base_config.trading_from.hour = trading_from.hour
    cpp_base_config.trading_from.minute = trading_from.minute
    cpp_base_config.trading_from.second = 0
    
    cpp_base_config.trading_to.hour = trading_to.hour
    cpp_base_config.trading_to.minute = trading_to.minute
    cpp_base_config.trading_to.second = 0
    
    # Configurer les jours de trading
    cpp_base_config.trading_days = [0, 1, 2, 3, 4]  # Lundi à vendredi
    
    # Configurer les paramètres de distance
    cpp_base_config.take_profit_distance = 30.0
    cpp_base_config.stop_loss_distance = 20.0
    
    # Paramètres ATR
    cpp_base_config.use_atr_for_sl = True
    cpp_base_config.use_atr_for_tp = True
    cpp_base_config.atr_period = 14
    cpp_base_config.stop_loss_atr_multiplier = 3.0
    cpp_base_config.take_profit_atr_multiplier = 15.0
    cpp_base_config.min_stop_loss_distance = 5.0
    cpp_base_config.min_take_profit_distance = 5.0
    
    # Paramètres de Min/Max pour SL
    cpp_base_config.use_minmax_for_sl = False
    cpp_base_config.sl_minmax_periods = 5
    cpp_base_config.sl_minmax_delta = 5.0
    
    # Paramètres de gestion du risque
    cpp_base_config.use_risk_based_sizing = True
    cpp_base_config.risk_percentage = 1.0
    cpp_base_config.cash = 100000.0
    cpp_base_config.max_position_percentage = 2.0
    cpp_base_config.leverage_limit = 100.0
    
    # Paramètres de break-even
    cpp_base_config.use_break_even = True
    cpp_base_config.break_even_threshold = 0.3
    
    # Paramètres de perte maximale journalière
    cpp_base_config.use_daily_max_loss = True
    cpp_base_config.daily_max_loss_percentage = 2.0
    cpp_base_config.daily_max_loss_amount = 0.0 # Calculé à partir de cash et daily_max_loss_percentage
    
    # Créer et configurer la configuration de la stratégie C++
    cpp_strategy_config = CppBuyHeikinGreenConfig()
    cpp_strategy_config.ema_short_period = 20
    cpp_strategy_config.ema_long_period = 198
    cpp_strategy_config.stoch_fastk = 10
    cpp_strategy_config.stoch_slowk = 7
    cpp_strategy_config.stoch_slowd = 3
    cpp_strategy_config.stoch_threshold = 30
    cpp_strategy_config.rsi_period = 14
    cpp_strategy_config.rsi_threshold = 50
    
    cpp_strategy_config.use_ema_short_filter = True
    cpp_strategy_config.use_ema_long_filter = False
    cpp_strategy_config.use_stoch_filter = True
    cpp_strategy_config.use_rsi_filter = False
    cpp_strategy_config.use_previous_ha_candle_red_filter = True
    

    # Instancier la stratégie C++
    strategy = CppBuyHeikinGreen(cpp_base_config, cpp_strategy_config)
    from cpp_strategies import set_log_callback
    set_log_callback(cpp_log_callback)  # Configurer le callback de log C++

    # Création des instances avec le nouveau TickBroker
    broker = TickBroker(
        epic="IX.D.NASDAQ.IFE.IP", 
        candle_interval=CANDLE_TIME_UNIT, 
        price_source=PriceSource.ASK
    )

    # Configurer le callback pour traiter les bougies
    broker.set_candle_callback(lambda candle: process_candle(candle, broker, strategy))

    logging.info("Bot de trading démarré. En attente de bougies...")

    # Boucle principale simple pour maintenir le programme en vie
    # Plus besoin de polling, le callback sera appelé automatiquement
    while running:
        try:
            time.sleep(1)
        except Exception as e:
            logging.error(f"Error in main loop: {e}")
            logging.error(traceback.format_exc())
            time.sleep(5)
    
    logging.info("Bot de trading arrêté proprement.")

if __name__ == '__main__':
    main()