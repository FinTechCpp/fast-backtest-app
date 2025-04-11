import time
import sys
import os
parent_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
if parent_dir not in sys.path:
    sys.path.insert(0, parent_dir)

from Strategies.BuyTrendFollowingStrategy.BuyTrendFollowingStrategy import BuyTrendFollowingStrategy
from WrapperIGAPI.Broker import Broker


def main():
    # Création des instances
    broker = Broker(epic="IX.D.NASDAQ.IFE.IP", working_resolution='1Min')
    strategy = BuyTrendFollowingStrategy()

    # Initialisation avec les 200 dernières bougies historiques
    # historical_candles = broker.fetch_historical_prices(numpoints=10)

    # strategy.initialize(historical_candles)

    candle = {
        'date': '2025-04-11 12:15:00',
        'Open': 100.0,
        'High': 105.0,
        'Low': 95.0,
        'Close': 102.0,
    }

    signal = strategy.update_candle(candle)

    signal = {
        'action': 'BUY',
        'quantity': 0.5,
        # 'expiry': 'DFB',
        'take_profit': 15,
        'stop_loss': 20,
    }

    # Envoi de l'ordre au broker
    print(broker.place_order(signal))


    # # Boucle pour récupérer les bougies en live toutes les 10 secondes
    # while True:
    #     live_candle = broker.get_live_candle()
    #     signal = strategy.update_candle(live_candle)
        
    #     if signal is not None:
    #         # Si un signal est généré (achat, vente ou liquidation), on le transmet au broker
    #         broker.place_order(signal)
    #     else:
    #         print("Aucun signal pour cette bougie.")

    #     # Attendre 10 secondes avant de récupérer la prochaine bougie
    #     time.sleep(10)


if __name__ == '__main__':
    main()
