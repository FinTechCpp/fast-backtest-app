import logging
import datetime
import time
import pandas as pd
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logging.getLogger('lightstreamer.client').setLevel(logging.DEBUG)
from igtrader.Strategies.BuyTrendFollowing import BuyTrendFollowing
from igtrader.Strategies.CrossEMA import CrossEMA, CrossEMAConfig
from igtrader.WrapperIGAPI.Broker import Broker
from igtrader.Strategies.Strategy import StrategyBaseConfig
from igtrader.Strategies.BuyHeikinGreen import BuyHeikinGreen, BuyHeikinGreenConfig

def is_candle_complete(candle, resolution_seconds=10):
    """
    Vérifie si une bougie est finalisée.
    
    :param candle: Dictionnaire contenant une clé 'date' (de type datetime ou Timestamp).
    :param resolution_seconds: Durée de la bougie en secondes.
    :return: True si la bougie est finalisée, False sinon.
    """
    # Cas 1 : dict avec clé 'date'
    if isinstance(candle, dict):
        date = candle['date']
    # Cas 2 : Series avec la date comme index
    elif isinstance(candle, pd.Series):
        date = candle.name
    else:
        raise TypeError("Le format de candle n'est pas supporté.")
    
    candle_end = date + datetime.timedelta(seconds=resolution_seconds)
    # Ici, on compare à l'heure actuelle. Selon votre utilisation, vous pourriez vouloir
    # vous baser sur l'heure serveur retournée par l'API plutôt que sur datetime.now()
    return datetime.datetime.now() >= candle_end


def main():
    # Définir l'intervalle de temps pour les bougies en secondes
    candle_interval = 10  # n-second candles
    
    # Création des instances
    
    broker = Broker(epic="IX.D.NASDAQ.IFE.IP", working_resolution='SECOND', candle_interval=candle_interval, price_source='ask')
    time.sleep(1)
    
    base_config = StrategyBaseConfig(
        trading_from=datetime.time(10, 0),
        trading_to=datetime.time(22, 0),
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

    # Initialize with historical data
    #historical_candles = broker.fetch_historical_prices(numpoints=200)
    #if not is_candle_complete(historical_candles.iloc[-1], candle_interval):
     #   historical_candles = historical_candles[:-1]
    #strategy.initialize(historical_candles)

    #logging.info(f"Initialized with {len(historical_candles)} historical candles")

    while True:
        try:
            # Calculate aligned execution time
            now = datetime.datetime.now()
            seconds_in_interval = now.second % candle_interval
            next_candle_time = now + datetime.timedelta(seconds=(candle_interval - seconds_in_interval))
            
            # Log timing information
            logging.info(f"Current time: {now}, next candle at: {next_candle_time}")
            
            # Get candles from streaming data
            candle_previous, candle_current = broker.fetch_previous_and_current_candles()
            
            # Initialize signal with default value
            signal = None
            
            # Process candles
            if candle_current and is_candle_complete(candle_current, candle_interval):
                logging.info(f"Processing complete current candle: {candle_current}")
                signal = strategy.update_candle(candle_current)
            elif candle_previous and is_candle_complete(candle_previous, candle_interval):
                logging.info(f"Processing complete previous candle: {candle_previous}")
                signal = strategy.update_candle(candle_previous)
            else:
                logging.info("Waiting for complete candles...")
            
            # Execute signal if we have one
            if signal:
                logging.info(f"Signal generated: {signal}")
                broker.execute_signal(signal)
            
            # Calculate sleep time until the next candle
            sleep_time = (candle_interval - seconds_in_interval) - 1  # Wake up 1 second before next candle
            if sleep_time < 1:
                sleep_time = candle_interval - 1
            
            #logging.info(f"Sleeping for {sleep_time} seconds until next check")
            time.sleep(sleep_time)
            
        except Exception as e:
            logging.error(f"Error in main loop: {e}")
            time.sleep(5)  # Sleep on error to avoid rapid error loops

if __name__ == '__main__':
    main()
