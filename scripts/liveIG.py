import logging
import datetime
import time
import pandas as pd
import os
import traceback
import signal
import sys

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

from igtrader.Strategies.BuyTrendFollowing import BuyTrendFollowing
from igtrader.Strategies.CrossEMA import CrossEMA, CrossEMAConfig
from igtrader.WrapperIGAPI.TickBroker import TickBroker, PriceSource
from igtrader.Strategies.Strategy import Strategy, StrategyBaseConfig, BaseCandle
from igtrader.Strategies.BuyHeikinGreen import BuyHeikinGreen, BuyHeikinGreenConfig

CANDLE_TIME_UNIT = 20  # n-second time unit candles to trade with

# Variable globale pour arrêter proprement le programme
running = True

def signal_handler(sig, frame):
    """Gestionnaire de signal pour arrêter proprement le programme"""
    global running
    logging.info("Signal d'arrêt reçu, arrêt en cours...")
    running = False

def process_candle(candle: BaseCandle, broker: TickBroker, strategy: Strategy):
    """
    Fonction de callback appelée lorsqu'une nouvelle bougie est prête.
    Traite la bougie et exécute la stratégie.
    
    Args:
        candle (BaseCandle): La bougie complète
        broker (TickBroker): Instance du broker pour exécuter les signaux
        strategy (Strategy): Stratégie de trading à utiliser
    """
    try:
        logging.info(f"Processing complete candle: {candle.date} - O:{candle.Open} H:{candle.High} L:{candle.Low} C:{candle.Close}")
        
        # Cette fonction contient désormais toute la logique qui était dans la boucle principale
        signal = strategy.update_candle(candle)
        
        # Execute signal if we have one
        if signal:
            logging.info(f"Signal generated: {signal}")
            broker.execute_signal(signal)
    except Exception as e:
        logging.error(f"Error processing candle: {e}")
        logging.error(traceback.format_exc())


def main():
    # Configurer le gestionnaire de signal pour Ctrl+C
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    
    base_config = StrategyBaseConfig(
        trading_from=datetime.time(7, 0),
        trading_to=datetime.time(23, 0),
        trading_days=[0, 1, 2, 3, 4],
        take_profit_distance=30,
        stop_loss_distance=20,
        use_atr_for_sl_tp=True,
        stop_loss_atr_multiplier=2.0,
        take_profit_atr_multiplier=6.0,
        atr_period=14,
        leverage_limit=20
    )
    
    buy_heikin_green_config = BuyHeikinGreenConfig(
        ema_short_period = 150,
        ema_long_period = 198,
        stoch_fastk = 10,
        stoch_slowk = 7,
        stoch_slowd = 3,
        stoch_threshold = 20,
        use_ema_short_filter = True,
        use_ema_long_filter = True,
        use_stoch_filter = True,
        use_previous_ha_candle_red_filter = True,        
    )

    strategy = BuyHeikinGreen(base_config, buy_heikin_green_config)

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