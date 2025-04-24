import logging
import datetime
import time
import pandas as pd
logging.basicConfig(level=logging.DEBUG, format='%(asctime)s - %(levelname)s - %(message)s')

from igtrader.Strategies.BuyTrendFollowing import BuyTrendFollowing
from igtrader.Strategies.CrossEMA import CrossEMA, CrossEMAConfig
from igtrader.WrapperIGAPI.Broker import Broker
from igtrader.Strategies.Strategy import StrategyBaseConfig

def is_candle_complete(candle, resolution_minutes=1):
    """
    Vérifie si une bougie est finalisée.
    
    :param candle: Dictionnaire contenant une clé 'date' (de type datetime ou Timestamp).
    :param resolution_minutes: Durée de la bougie en minutes.
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
    
    candle_end = date + datetime.timedelta(minutes=resolution_minutes)
    # Ici, on compare à l'heure actuelle. Selon votre utilisation, vous pourriez vouloir
    # vous baser sur l'heure serveur retournée par l'API plutôt que sur datetime.now()
    return datetime.datetime.now() >= candle_end


def main():
    # Création des instances
    broker = Broker(epic="IX.D.NASDAQ.IFE.IP", working_resolution='1Min')

    base_config = StrategyBaseConfig(
        trading_from=datetime.time(19, 0),
        trading_to=datetime.time(21, 0),
        trading_days=[0, 1, 2, 3, 4],
        take_profit_distance=30,
        stop_loss_distance=20,
    )
    crossEMA_config = CrossEMAConfig(
        ema_short_period=50,
        ema_long_period=200,
    )

    strategy = CrossEMA(base_config, crossEMA_config)

    historical_candles = broker.fetch_historical_prices(numpoints=50)
    if not is_candle_complete(historical_candles.iloc[-1]):
        historical_candles = historical_candles[:-1]
    strategy.initialize(historical_candles)

    logging.debug(historical_candles)


    while True:
        now = datetime.datetime.now()
        seconds_to_wait = 60 - now.second - now.microsecond / 1_000_000
        time.sleep(seconds_to_wait)

        candle_previous, candle_current = broker.fetch_previous_and_current_candles()

        if is_candle_complete(candle_current):
            logging.debug(f"Last complete candle: {candle_current}")
            signal = strategy.update_candle(candle_current)
        elif is_candle_complete(candle_previous):
            logging.debug(f"Last complete candle: {candle_previous}")
            signal = strategy.update_candle(candle_previous)

        logging.debug(f"Signal: {signal}")
        broker.execute_signal(signal)



if __name__ == '__main__':
    main()
