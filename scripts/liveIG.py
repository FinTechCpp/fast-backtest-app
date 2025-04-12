import datetime
import time
import pandas as pd

from igtrader.Strategies.BuyTrendFollowingStrategy import BuyTrendFollowingStrategy
from igtrader.WrapperIGAPI.Broker import Broker


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
    strategy = BuyTrendFollowingStrategy()

    historical_candles = broker.fetch_historical_prices(numpoints=10)
    if not is_candle_complete(historical_candles.iloc[-1]):
        historical_candles = historical_candles[:-1]
    strategy.initialize(historical_candles)

    print(historical_candles)


    while True:
        now = datetime.datetime.now()
        seconds_to_wait = 60 - now.second - now.microsecond / 1_000_000
        time.sleep(seconds_to_wait)

        candle_previous, candle_current = broker.fetch_previous_and_current_candles()
        print(f"Previous candle: {candle_previous}, is finished: {is_candle_complete(candle_previous)}")
        print(f"Current candle: {candle_current}, is finished: {is_candle_complete(candle_current)}")

        if is_candle_complete(candle_current):
            signal = strategy.update_candle(candle_current)
        elif is_candle_complete(candle_previous):
            signal = strategy.update_candle(candle_previous)

        print(f"Signal: {signal}")
        broker.execute_signal(signal)



if __name__ == '__main__':
    main()
